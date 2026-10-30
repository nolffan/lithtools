//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : EditPoly.cpp
//
//	PURPOSE	  : Defines the CEditPoly class.
//
//	CREATED	  : October 5 1996
//
//
//------------------------------------------------------------------

// talon: DEdit's 2001 one, 0x424b98..0x4257d8 (then the CMoArray
// templates it instantiates), written from the image: the texture planes,
// CEditPoly (one texture, its triangles, the LTA and .ed loaders; its
// vtable 0x4d7828), and the texture ID Build 69 moved to CTexturedPlane.
// Build 69's functions that the image has not are at the end, for the
// rest of DEdit until it is brought back to 2001.

// Includes....
#include "bdefs.h"
#include "editobjects.h"
#include "editplane.h"
#include "stringholder.h"
#include "editpoly.h"
// talon: what Build 69's adds, for its functions below
#include "geomroutines.h"
#include "editbrush.h"
#include "de_world.h"
#include "ltamgr.h"
#include "ltasaveutils.h"
#include "PolyLightMap.h"


#ifdef DIRECTEDITOR_BUILD
	#include "texture.h"
	#include "dedit.h"
#endif


// The main texture alignment planes (0x484d70, 0x40f620 -> 0x40f630) and
// right vectors (0x484db8, 0x40f6f0 -> 0x40f700): Build 69's.
CVector g_TexturePlanes[] =
{
	CVector(0.0f, 1.0f, 0.0f),		// Bottom
	CVector(0.0f, -1.0f, 0.0f),		// Top

	CVector(1.0f, 0.0f, 0.0f),		// East-facing wall
	CVector(-1.0f, 0.0f, 0.0f),		// West-facing wall
	CVector(0.0f, 0.0f, 1.0f),		// North-facing wall
	CVector(0.0f, 0.0f, -1.0f)		// South-facing wall
};

CVector g_RightVectors[] =
{
	CVector(1.0f, 0.0f, 0.0f),
	CVector(-1.0f, 0.0f, 0.0f),

	CVector(0.0f, 0.0f, 1.0f),	
	CVector(0.0f, 0.0f, -1.0f),
	CVector(-1.0f, 0.0f, 0.0f),
	CVector(1.0f, 0.0f, 0.0f),	
};


// talon: DEdit's 2001 surface flags for "spriteanimate" and "surf_xzonly"
// (0x424fd7, 0x424fbe), not Build 69's.
#define TALON_SURF_SPRITEANIMATE	(1<<14)
#define TALON_SURF_XZONLY			(1<<16)


// 0x424cc4.
CEditPoly::CEditPoly()
{
	CommonConstructor( );
}


// 0x424d06: not inlined (each constructor calls it).
inline void CEditPoly::CommonConstructor( )
{
	m_pTextureName			= "Default";
	m_pBrush				= NULL;
	m_Unknown_30			= 0;
	m_Unknown_2c			= 0;
	m_pTextureFile			= NULL;
	m_SurfaceFlags			= 0;

	P.Init( 1.0f, 0.0f, 0.0f );
	Q.Init( 0.0f, 0.0f, 1.0f );
	O.Init( 0.0f, 0.0f, 0.0f );

	m_Unknown_70			= 1;
	m_Ambient[2]			= 0;
	m_Ambient[1]			= 0;
	m_Ambient[0]			= 0;
}


// 0x424d5c.
CEditPoly::CEditPoly( CEditPoly *pCopyFrom )
{
	CommonConstructor( );
	CopyEditPoly( pCopyFrom );
}

// 0x424dae.
CEditPoly::CEditPoly( CEditBrush *pBrush )
{
	CommonConstructor( );
	m_pBrush = pBrush;
}


// 0x424df4.
CEditPoly::~CEditPoly()
{
	Term();
}


// 0x424e55.
void CEditPoly::Term()
{
	DeleteAndClearArray(m_Triangles);
	m_Indices.SetSize(0);
	m_Unknown_30 = 0;
	m_Unknown_2c = 0;
}


#define NUM_TEXTURE_PLANES	(sizeof(g_TexturePlanes) / sizeof(CVector))

// 0x424e7f: the 2001 LTA poly, by position: the base poly, its texture
// info (O, P, Q, sticktopoly, the name), its flags and its shade.
BOOL CEditPoly::LoadEditPolyLTA( CLTANode* pNode, CStringHolder *pStringHolder )
{
	BOOL retval = FALSE;
	if ( LoadBasePolyLTA(pNode) )
	{	
		CLTANode* pTextureInfo	= pNode->GetElement(4); // shallow_find_list(pNode, "textureinfo");
		CLTANode* pTemp;
		uint32 i;

		pTemp = pTextureInfo->GetElement(1);
		O.x = GetFloat( pTemp->GetElement(0) );
		O.y = GetFloat( pTemp->GetElement(1) );
		O.z = GetFloat( pTemp->GetElement(2) );
		pTemp = pTextureInfo->GetElement(2);
		P.x = GetFloat( pTemp->GetElement(0) );
		P.y = GetFloat( pTemp->GetElement(1) );
		P.z = GetFloat( pTemp->GetElement(2) );
		pTemp = pTextureInfo->GetElement(3);
		Q.x = GetFloat( pTemp->GetElement(0) );
		Q.y = GetFloat( pTemp->GetElement(1) );
		Q.z = GetFloat( pTemp->GetElement(2) );

		m_Unknown_70 = (CLTAConverter::StrToInt( PairCdrNode(pTextureInfo->GetElement(4))->GetValue() )) ? true : false;
		m_pTextureName = pStringHolder->AddString( PairCdr(pTextureInfo->GetElement(5)) );

		CLTANode* pFlags = pNode->GetElement(5);
		m_SurfaceFlags = 0;
		uint32 nFlags = pFlags->GetNumElements();
		for( i=1; i < nFlags; i++ )
		{
			const char* pFlag = pFlags->GetElement(i)->GetValue();
			if( strcmp(pFlag, "surf_xzonly") == 0 )
				m_SurfaceFlags |= TALON_SURF_XZONLY;
			else if( strcmp(pFlag, "spriteanimate") == 0 )
				m_SurfaceFlags |= TALON_SURF_SPRITEANIMATE;
		}

		CLTANode* pShade = pNode->GetElement(6);
		m_Ambient[0] = GetUint8( pShade->GetElement(1) );
		m_Ambient[1] = GetUint8( pShade->GetElement(2) );
		m_Ambient[2] = GetUint8( pShade->GetElement(3) );

		retval=TRUE;
	}
	return retval;
}


// CEditPoly::LoadEditPolyTBW (0x40fb30, Build 69's name; the .ed's poly, called
// at 0x40dbb6). The third argument is never read. A texture name over 100
// characters is dropped (0x40fc08).
BOOL CEditPoly::LoadEditPolyTBW( CAbstractIO &file, CStringHolder *pStringHolder, uint32 unused )
{
	char texName[256];

	if( !LoadBasePolyTBW(file) )
		return FALSE;

	file >> O.x;
	file >> O.y;
	file >> O.z;

	file >> P.x;
	file >> P.y;
	file >> P.z;

	file >> Q.x;
	file >> Q.y;
	file >> Q.z;

	file >> m_Unknown_70;

	if( !file.ReadString(texName, 256) )
		return FALSE;

	if( strlen(texName) > 100 )
		texName[0] = 0;

	m_pTextureName = pStringHolder->AddString(texName);

	file >> m_SurfaceFlags;
	file >> m_Ambient[0];
	file >> m_Ambient[1];
	file >> m_Ambient[2];

	return TRUE;
}


// CEditPoly::SaveEditPolyTBW (0x40fc70, Build 69's name).
void CEditPoly::SaveEditPolyTBW( CAbstractIO &file )
{
	SaveBasePolyTBW(file);
	SaveTextureInfoTBW(file);

	file << m_SurfaceFlags;
	file << m_Ambient[0];
	file << m_Ambient[1];
	file << m_Ambient[2];
}


// CEditPoly::SaveTextureInfoTBW (0x40fcf0; Build 69's CTexturedPlane name, also called at
// 0x40dcfd).
void CEditPoly::SaveTextureInfoTBW( CAbstractIO &file )
{
	file << O.x << O.y << O.z;
	file << P.x << P.y << P.z;
	file << Q.x << Q.y << Q.z;

	file << m_Unknown_70;

	if( strlen(m_pTextureName) < 256 )
		file.WriteString( m_pTextureName );
	else
		file.WriteString( "" );
}


// CEditPoly::CopyAttributes (0x40fe10).
void CEditPoly::CopyAttributes( CBasePoly *pBase, CStringHolder *pStringHolder )
{
	CEditPoly	*pOther = (CEditPoly*)pBase;

	m_Plane = pOther->m_Plane;

	O = pOther->O;
	P = pOther->P;
	Q = pOther->Q;

	if (pStringHolder)
	{
		m_pTextureName=pStringHolder->AddString(pOther->m_pTextureName);
	}
	else
	{
		m_pTextureName=pOther->m_pTextureName;
	}	

	m_Ambient[0] = pOther->m_Ambient[0];
	m_Ambient[1] = pOther->m_Ambient[1];
	m_Ambient[2] = pOther->m_Ambient[2];
	m_Unknown_70 = pOther->m_Unknown_70;
	m_SurfaceFlags = pOther->m_SurfaceFlags;
	m_pTextureFile = pOther->m_pTextureFile;
}


// 0x425338: the base poly's and its triangles'.
void CEditPoly::DecrementPoints( uint32 index )
{
	CBasePoly::DecrementPoints(index);

	for( WORD i=0; i < (WORD)m_Triangles.GetSize(); i++ )
		m_Triangles[i]->DecrementPoints(index);
}


// CEditPoly::UpdateTriangles (0x40fec0, the name ours; called at
// 0x411225): triangulates a copy of the poly into m_Triangles.
void CEditPoly::UpdateTriangles()
{
	CBasePoly	*pPoly;

	DeleteAndClearArray(m_Triangles);

	pPoly = new CBasePoly(m_pBrush, m_Indices);
	pPoly->m_Plane = m_Plane;
	pPoly->GetTriangles(m_Triangles);

	delete pPoly;
}


// 0x4253e3: through the triangles (CBasePoly::IntersectRay, 0x40fcda).
BOOL CEditPoly::IntersectRay( CEditRay &ray, CReal &t, BOOL bBackface )
{
	for( uint32 i=0; i < m_Triangles; i++ )
	{
		if( m_Triangles[i]->IntersectRay(ray, t, bBackface) )
			return TRUE;
	}

	return FALSE;
}


// 0x410000: as Build 69 has it (CopyAttributes is the 2001 CBasePoly's only
// virtual, slot 0).
BOOL CEditPoly::CopyEditPoly( CEditPoly *pPoly, BOOL bCopyIndices, CStringHolder *pStringHolder )
{
	if( bCopyIndices )
		m_Indices.CopyArray( pPoly->m_Indices );

	CopyAttributes( pPoly, pStringHolder );
	m_pBrush = pPoly->m_pBrush;

	return TRUE;
}


// 0x425454; Build 69's.
void CEditPoly::Flip()
{
	uint32		i, temp;

	for( i=0; i < m_Indices/2; i++ )
	{
		temp = m_Indices[i];
		m_Indices[i] = m_Indices[m_Indices-i-1];
		m_Indices[m_Indices-i-1] = temp;
	}

	Normal() = -Normal();
	Dist() = -Dist();
}


// 0x4254b8: Build 69's for its one texture, but a SURF_XZONLY poly takes
// the first plane.
void CEditPoly::SetupBaseTextureSpace()
{
	uint32 i, closestPlane;
	CReal dot, maxDot;


	// Find the closest texture alignment plane to this poly.
	maxDot = (CReal)-MAX_CREAL;

	closestPlane = 0;
	if( !(m_SurfaceFlags & TALON_SURF_XZONLY) )
	{
		for( i=0; i < NUM_TEXTURE_PLANES; i++ )
		{
			dot = Normal().Dot( g_TexturePlanes[i] );
			if( dot > maxDot )
			{
				maxDot = dot;
				closestPlane = i;
			}
		}
	}

	// SCHLEGZ 1/22/98: fixed problem with deleting previous mag
	// MD 3/25/98: Polies now use explicit scale variables.
	P = g_RightVectors[closestPlane];
	Q = P.Cross( g_TexturePlanes[closestPlane] );
}


// 0x42555d. P and Q made perpendicular when they are not, keeping their
// lengths.
//
// talon: a retail bug, kept: Q is made perpendicular to P by subtracting
// the dot product from each of its components (`Q -= dot`, the scalar
// operator), not P scaled by it, so Q is not perpendicular afterwards
// (0x425611..0x425627).
void CEditPoly::SetTextureSpace(LTVector newO, LTVector newP, LTVector newQ)
{
	O = newO;
	P = newP;
	Q = newQ;

	if( fabs(P.Dot(Q)) > 0.001 )
	{
		CReal fPMag = P.Mag();
		CReal fQMag = Q.Mag();

		P.Norm();
		Q.Norm();

		Q -= P.Dot(Q);
		Q.Norm();

		P *= fPMag;
		Q *= fQMag;
	}
}


// 0x42566d; Build 69's (TexturedPlane.cpp).
static BOOL IsSpriteFilename(char *pName)
{
	if(strlen(pName) > 3)
		return stricmp(&pName[strlen(pName)-3], "spr") == 0;
	else
		return FALSE;
}


// 0x4256a1: Build 69's CTexturedPlane::UpdateTextureID without the detail
// texture, setting the poly's sprite flag.
void CEditPoly::UpdateTextureID()
{
	CMoFileIO file;
	DWORD i, nFrames, frameRate, bTransparent, bTranslucent, key;
	char str[256];

	m_pTextureFile = NULL;

	if(m_pTextureName)
	{
		if(IsSpriteFilename(m_pTextureName))
		{
			// Get the first frame for the sprite.
			if(dfm_OpenFileRelative(GetFileMgr(), m_pTextureName, file))
			{
				file >> nFrames;
				file >> frameRate;
				file >> bTransparent;
				file >> bTranslucent;
				file >> key;

				if(nFrames > 0)
				{
					if(file.ReadString(str, 256))
					{
						dfm_GetFileIdentifier(GetFileMgr(), str, &m_pTextureFile);
					}
				}
				
				file.Close();
			}

			m_SurfaceFlags |= TALON_SURF_SPRITEANIMATE;
		}
		else
		{
			m_SurfaceFlags &= ~TALON_SURF_SPRITEANIMATE;
			dfm_GetFileIdentifier(GetFileMgr(), m_pTextureName, &m_pTextureFile);
		}
	}
}


// talon: Build 69's, not in the image: for the rest of DEdit until it is
// brought back to 2001.



//determines if this polygon is concave or not
bool CEditPoly::IsConcave()
{
	uint32 nNumPts = NumVerts();

	if(nNumPts <= 3)
	{
		return false;
	}

	//get the normal for this polygon
	LTVector vNormal = Normal();

	//now go through every edge
	uint32 nPrevPt = nNumPts - 1;
	for(uint32 nCurrPt = 0; nCurrPt < nNumPts; nPrevPt = nCurrPt, nCurrPt++)
	{
		//build the edge normal
		LTVector vEdge = m_pBrush->m_Points[Index(nCurrPt)] - m_pBrush->m_Points[Index(nPrevPt)];

		//find the normal
		LTVector vEdgeNorm = vNormal.Cross(vEdge);
		vEdgeNorm.Norm();

		//now run through all the other points
		for(uint32 nTestPt = 0; nTestPt < nNumPts; nTestPt++)
		{
			//ignore the points on the edge
			if((nTestPt == nCurrPt) || (nTestPt == nPrevPt))
				continue;

			//see if it is on the correct side
			if(vEdgeNorm.Dot(m_pBrush->m_Points[Index(nTestPt)] - m_pBrush->m_Points[Index(nCurrPt)]) > 0.001f)
			{
				return true;
			}
		}
	}

	return false;

}


//determines if this polygon lies entirely within a single plane
bool CEditPoly::IsCoplanar()
{
	//run through all the points, projecting them onto the normal. The
	//value should be relatively the same for all the points
	if(NumVerts() < 4)
	{
		//if we have 3 or less points, we mathmatically have to be coplanar, so just bail to
		//prevent mathmatical errors from causing problems
		return true;
	}

	//the epsilon value that we allow points to stray from the plane
	static const float kfPlaneEpsilon = 0.01f;

	LTVector vNormal = Normal();

	CReal fDist = m_pBrush->m_Points[Index(0)].Dot(Normal());

	for(uint32 nCurrPt = 1; nCurrPt < NumVerts(); nCurrPt++)
	{
		CReal fTestDist = m_pBrush->m_Points[Index(nCurrPt)].Dot(Normal());

		fTestDist -= fDist;

		if((fTestDist < -kfPlaneEpsilon) || (fTestDist > kfPlaneEpsilon))
		{
			return false;
		}
	}

	return true;
}


//determines the surface area of the polygon
CReal CEditPoly::GetSurfaceArea()
{
	//accumulate the area occupied by each triangle
	CReal fArea = 0.0f;

	uint32 nNumPts = NumVerts();

	//safety first...
	if(nNumPts < 3)
		return 0.0f;

	//fan out from vertex 0 to all other verts
	LTVector vFanPt = m_pBrush->m_Points[Index(0)];

	for(uint32 nCurrPt = 1; nCurrPt < nNumPts - 1; nCurrPt++)
	{
		//get the two edges
		LTVector vEdge1 = m_pBrush->m_Points[Index(nCurrPt)] - vFanPt;
		LTVector vEdge2 = m_pBrush->m_Points[Index(nCurrPt + 1)] - vFanPt;

		//the cross product gives us the area of the rect, so we want to add
		//half of that
		fArea += vEdge1.Cross(vEdge2).Mag() / 2;
	}

	return fArea;
}

