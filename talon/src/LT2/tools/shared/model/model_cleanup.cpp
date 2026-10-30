
#include "bdefs.h"
#include "model.h"

// talon: the module opens with std::ctype<unsigned short>::id's 21-byte
// initializer (0x42f746), which only <xlocale> (via <string>) gives it.
#include <string>
#include "model_ops.h"
#include "model_cleanup.h"


typedef struct VertPolyAdjacency_t
{
	CMoArray<DWORD>	m_Tris;
} VertPolyAdjacency;



// talon: 2001's model_cleanup (0x42f746..0x42fd4d) has gn_BuildTriNormals,
// gn_BuildModelVertexNormals, gn_BuildVertexNormals, gn_IsVertUsed and
// gn_RemoveUnusedVertices; Build 69's gn_RemoveDuplicateVertices,
// gn_RemoveedUpTriangles (both stubs there), gn_GetVertRadius and
// gn_CleanupGeometry aren't in it.


static BOOL gn_BuildVertexNormals(PieceLOD *pPiece, CMoArray<LTVector> &triNormals)
{
	DWORD i, j;
	VertPolyAdjacency *pAdjacent;
	ModelTri *pCurTri;
	ModelVert *pCurVert;

	// compute the vertex normals
	pAdjacent = new VertPolyAdjacency[pPiece->m_Verts.GetSize()];
	if(!pAdjacent)
		return FALSE;

	for(i = 0; i < pPiece->m_Tris; i++)
	{
		pCurTri = &pPiece->m_Tris[i];

		pAdjacent[pCurTri->m_Indices[0]].m_Tris.Append(i);
		pAdjacent[pCurTri->m_Indices[1]].m_Tris.Append(i);
		pAdjacent[pCurTri->m_Indices[2]].m_Tris.Append(i);
	}

	for(i = 0; i < pPiece->m_Verts; i++)
	{
		pCurVert = &pPiece->m_Verts[i];
		
		pCurVert->m_Normal.Init();

		for(j = 0; j < pAdjacent[i].m_Tris; j++)
		{
			pCurVert->m_Normal += triNormals[pAdjacent[i].m_Tris[j]];
		}

		pCurVert->m_Normal /= (float)pAdjacent[i].m_Tris.GetSize();
		pCurVert->m_Normal.Norm();
	}

	delete [] pAdjacent;
	return TRUE;
}


BOOL gn_BuildTriNormals(PieceLOD *pPiece, CMoArray<LTVector> &normals)
{
	DWORD i;
	ModelTri *pCurTri;
	LTVector v1, v2;

	if(!normals.SetSize(pPiece->m_Tris.GetSize()))
		return FALSE;

	// compute the polygon normals
	for(i=0; i < pPiece->m_Tris; i++)
	{
		pCurTri = &pPiece->m_Tris[i];

		ModelVert	&p0 = pPiece->m_Verts[pCurTri->m_Indices[0]];
		ModelVert	&p1 = pPiece->m_Verts[pCurTri->m_Indices[1]];
		ModelVert	&p2 = pPiece->m_Verts[pCurTri->m_Indices[2]];

		VEC_SUB(v1, p2.m_Vec, p0.m_Vec);
		VEC_SUB(v2, p1.m_Vec, p0.m_Vec);
		normals[i] = v1.Cross(v2);
		normals[i].Norm();
	}

	return TRUE;
}


BOOL gn_BuildModelVertexNormals(Model *pModel)
{
	uint32 i, j;
	CMoArray<LTVector> normals;
	ModelPiece *pPiece;
	PieceLOD *pLOD;

	for(i=0; i < pModel->NumPieces(); i++)
	{
		pPiece = pModel->GetPiece(i);
		
		for(j=0; j < pPiece->NumLODs(); j++)
		{
			pLOD = pPiece->GetLOD(j);

			if(!gn_BuildTriNormals(pLOD, normals))
				return FALSE;

			gn_BuildVertexNormals(pLOD, normals);
		}
	}

	return TRUE;
}


// Returns TRUE if any tris in the piece reference the vertex.
BOOL gn_IsVertUsed(PieceLOD *pPiece, DWORD iTest)
{
	DWORD iTri, iTriVert;

	for(iTri=0; iTri < pPiece->m_Tris; iTri++)
	{
		for(iTriVert=0; iTriVert < 3; iTriVert++)
		{
			if(pPiece->m_Tris[iTri].m_Indices[iTriVert] == iTest)
				return TRUE;
		}
	}

	return FALSE;
}


// Removes all unused vertices.
// talon: 2001's walks each piece's own vertices (the piece is its LOD 0),
// not its LODs' (0x42fce9).
void gn_RemoveUnusedVertices(Model *pModel)
{
	uint32 iPiece, iVert;
	ModelPiece *pPiece;

	for(iPiece=0; iPiece < pModel->NumPieces(); iPiece++)
	{
		pPiece = pModel->GetPiece(iPiece);

		for(iVert=0; iVert < pPiece->NumVerts(); iVert++)
		{
			if(!gn_IsVertUsed(pPiece, iVert))
			{
				// (Doesn't matter what we replace it with, it's not referenced anywhere).
				pPiece->ReplaceVertex(iVert, 0);
				--iVert;
			}
		}
	}
}
