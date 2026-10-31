
#ifndef __PREGEOMETRY_H__
#define __PREGEOMETRY_H__

	#include "prepoly.h"

	class CPreWorld;
	class CBaseEditObj;

	// ----------------------------------------------------------------------- //
	//      PURPOSE:        Tells if the given poly is convex.
	// ----------------------------------------------------------------------- //
	// talon: NOLF's returns BOOL, not bool: inlined in CPrePoly::PostJoinFixup
	// (0x43ff40) its FALSE goes straight to the caller's return FALSE.
	template<class T>
	BOOL IsConvex(T *pPoly, PReal convexThreshold)
	{

		uint32		i, j, nextI;
		CPrePlane	edgePlane;
		PReal		dot, pointDots[200];

		for(i=0; i < pPoly->NumVerts(); i++)
		{
			nextI = (i+1) % pPoly->NumVerts();

			// Make an edge plane.
			edgePlane.m_Normal = pPoly->Pt(nextI) - pPoly->Pt(i);
			edgePlane.m_Normal = edgePlane.m_Normal.Cross(pPoly->Normal());
			edgePlane.m_Normal.Norm();
			if(edgePlane.m_Normal.Mag() < 0.1f)
				return FALSE;

			edgePlane.m_Dist = edgePlane.m_Normal.Dot(pPoly->Pt(i));

			// Now see if all points are on the front side;
			for(j=0; j < pPoly->NumVerts(); j++)
			{
				pointDots[j] = edgePlane.m_Normal.Dot(pPoly->Pt(j)) - edgePlane.m_Dist;
			}

			for(j=0; j < pPoly->NumVerts(); j++)
			{
				if(j == i || j == nextI)
					continue;

				dot = pointDots[j];
				if(dot < convexThreshold)
				{
					return FALSE;
				}
			}
		}

		return TRUE;
	}

	
	class CPreLeaf;

	// talon: pregeometry.cpp's leaf searches (the leak file's and the
	// PVS's).
	// The leaf holding the first object of class pClassName that is in one,
	// and that object (0x42cd70, the name is ours).
	CPreLeaf* FindObjectLeaf(CPreWorld *pWorld, const char *pClassName, CBaseEditObj **ppObj=NULL);

	// The leaf of a GameStartPoint, a StartPoint, a Light or a DirLight,
	// whichever is found first (0x42ce40, the name is ours).
	CPreLeaf* FindInsideLeaf(CPreWorld *pWorld, CBaseEditObj **ppObj);

	// Helper functions.
	void GetPerpendicularVector(PVector *pVec, PVector *pRef, PVector *pPerp);
	void BuildFrameOfReference(PVector *pVec, PVector *pUpRef, PVector *pRight, PVector *pUp, PVector *pForward);

	PReal SnapNumberToGrid(PReal num, PReal gridSize);
	BOOL InsideConvex(CPrePoly *pPoly, PVector &pt);


#endif  // __PREGEOMETRY_H__


