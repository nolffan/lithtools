#include "bdefs.h"
#include "preworld.h"
#include "pregeometry.h"
#include "preleaf.h"
#include "prelightmap.h"
#include "lightmap_compress.h"
#include "node.h"
#include "editobjects.h"
#include "brushtoworld.h"
#include "splitpoly.h"
#include "editbrush.h"
#include "editpoly.h"
#include "findworldmodel.h"
#include "proputils.h"
#include "processing.h"

// talon: 0x42c9c0..0x42d060 in the image.

void GetPerpendicularVector(PVector *pVec, PVector *pRef, PVector *pPerp)
{
	PReal	dot;
	PVector tempRef;

	if(!pRef)
	{
		tempRef.Init(0, 1, 0);
		pRef = &tempRef;
	}

	VEC_COPY(*pPerp, *pRef);

	// Are pRef and pVec the same?  If not, we can exit.
	dot = VEC_DOT(*pVec, *pPerp);
	if(dot > 0.99f || dot < -0.99f)
	{
		pPerp->x += 5.0f;
		pPerp->y += 5.0f;
		pPerp->z += 5.0f;
		VEC_NORM(*pPerp);

		dot = VEC_DOT(*pVec, *pPerp);
		if(dot > 0.99f || dot < -0.99f)
		{
			pPerp->x += 5.0f;
			pPerp->y -= 2.0f;
			pPerp->z -= 5.0f;
			VEC_NORM(*pPerp);
		}
	}

	// Make pVec and pPerp linear independent.
	*pPerp += *pVec * -VEC_DOT(*pVec, *pPerp);
	VEC_NORM(*pPerp);
}


void BuildFrameOfReference(PVector *pVec, PVector *pUpRef, PVector *pRight, PVector *pUp, PVector *pForward)
{
	PVector tempRef;

	VEC_COPY(*pForward, *pVec);
	VEC_NORM(*pForward);

	// Treat the vector as the forward vector and come up with 2 other vectors.
	if(pUpRef)
	{
		VEC_COPY(tempRef, *pUpRef);
		VEC_NORM(tempRef);
		GetPerpendicularVector(pForward, &tempRef, pUp);
	}
	else
	{
		GetPerpendicularVector(pForward, NULL, pUp);
	}

	// Create the right vector.
	VEC_CROSS(*pRight, *pForward, *pUp);
}


// The leaf the point is in: the last one passed on the way down, if the
// point ends up in front of the last plane (0x42ccd0).
CPreLeaf* LocatePointInTree(CPreWorld *pWorld, PVector &point, NODEREF iRoot)
{
	PReal		dot;
	CNode		*pRoot;
	uint32		side;
	CPreLeaf	*pLeaf;

	pLeaf = NULL;
	while(1)
	{
		if(!IsValidNode(iRoot))
			return pLeaf;

		pRoot = iRoot;
		if(pRoot->m_pLeaf)
			pLeaf = pRoot->m_pLeaf;

		dot = pRoot->Normal().Dot(point) - pRoot->Dist();
		side = (dot >= -0.0001f) ? FrontSide : BackSide;

		iRoot = pRoot->m_Sides[side];
		if(!IsValidNode(iRoot))
			return (side == FrontSide) ? pLeaf : NULL;
	}
}


CPreLeaf* FindObjectLeaf(CPreWorld *pWorld, const char *pClassName, CBaseEditObj **ppObj)
{
	uint32 i;
	CBaseEditObj *pObj;
	PVector vPos;
	CPreLeaf *pLeaf;

	for(i=0; i < pWorld->m_pMainWorld->m_Objects; i++)
	{
		pObj = pWorld->m_pMainWorld->m_Objects[i];
		VEC_COPY(vPos, pObj->GetPos());

		if(strcmp(pObj->GetClassName(), pClassName) == 0)
		{
			pLeaf = LocatePointInTree(pWorld, vPos, pWorld->m_RootNode);
			if(pLeaf)
			{
				if(ppObj)
					*ppObj = pObj;

				return pLeaf;
			}
		}
	}

	return NULL;
}


CPreLeaf* FindInsideLeaf(CPreWorld *pWorld, CBaseEditObj **ppObj)
{
	CPreLeaf *pLeaf;

	pLeaf = FindObjectLeaf(pWorld, "GameStartPoint", ppObj);
	if(!pLeaf)
	{
		pLeaf = FindObjectLeaf(pWorld, "StartPoint", ppObj);
		if(!pLeaf)
		{
			pLeaf = FindObjectLeaf(pWorld, "Light", ppObj);
			if(!pLeaf)
			{
				pLeaf = FindObjectLeaf(pWorld, "DirLight", ppObj);
				if(!pLeaf)
					return NULL;
			}
		}
	}

	return pLeaf;
}


PReal SnapNumberToGrid(PReal num, PReal gridSize)
{
	if(num > (PReal)0.0)
		return num - (PReal)fmod(num, gridSize);
	else
		return num - (gridSize + (PReal)fmod(num, gridSize));
}


BOOL InsideConvex(CPrePoly *pPoly, PVector &pt)
{
	uint32		i, nextI;
	CPrePlane	edgePlane;
	PReal		edgeDot;

	for( i=0; i < pPoly->NumVerts(); i++ )
	{
		nextI = (i+1) % pPoly->NumVerts();

		edgePlane.m_Normal = pPoly->Normal().Cross(pPoly->Pt(i) - pPoly->Pt(nextI));
		edgePlane.m_Normal.Norm();
		edgePlane.m_Dist = VEC_DOT(edgePlane.m_Normal, pPoly->Pt(i));

		edgeDot = edgePlane.m_Normal.Dot(pt) - edgePlane.m_Dist;

		if( edgeDot < 0.0f )
			return FALSE;
	}

	return TRUE;
}
