//------------------------------------------------------------------
//
//	FILE	  : BrushToWorld.h
//
//	PURPOSE	  : Defines the class that converts a brush list (CEditRegion)
//              to a nonintersecting polygon list (CPreWorld).
//
//	CREATED	  : February 5, 1997
//
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------

#ifndef __BRUSH_TO_WORLD_H__
	#define __BRUSH_TO_WORLD_H__


	// ----------------------------------------------------------------------- //
	// Includes....
	// ----------------------------------------------------------------------- //

	#include "editpoly.h"
	#include "editregion.h"
	#include "preworld.h"


	// ----------------------------------------------------------------------- //
	// The 2001 BrushToWorld is CSG: every brush becomes a TempBrush, a list of
	// TempPolies (its faces) and a chain of TempNodes (its distinct planes).
	// The faces of each brush are clipped against the planes of the brushes
	// it intersects and the pieces inside are thrown away. The layouts are
	// the shipped code's; the names are ours.
	// ----------------------------------------------------------------------- //

	class TempPoly : public CGLLNode
	{
		public:

			TempPoly() { m_pOriginalPoly = NULL; }

			void CopySplitAttributes(const TempPoly *pPoly)
			{
				m_pSurface		= pPoly->m_pSurface;
				m_pOriginalPoly	= pPoly->m_pOriginalPoly;
			}

			const PVector&		Normal() const			{ ASSERT(GetPlane()); return GetPlane()->m_Normal; }
			const PReal&		Dist() const			{ ASSERT(GetPlane()); return GetPlane()->m_Dist; }

			CPreSurface*		GetSurface()			{ return m_pSurface; }
			const CPreSurface*	GetSurface() const		{ return m_pSurface; }

			CPrePlane*			GetPlane()				{ ASSERT(GetSurface()); return GetSurface()->m_pPlane; }
			const CPrePlane*	GetPlane() const		{ ASSERT(GetSurface()); return GetSurface()->m_pPlane; }

		public:

			// The face this poly started as (in the world's m_OriginalBrushPolies).
			CPrePoly			*m_pOriginalPoly;

			uint32				m_Unknown_0c;
			uint32				m_Unknown_10;

			CPreSurface			*m_pSurface;

			//this must come last
			BASEPOLY_MEMBER()
	};


	// A node of a brush's plane chain: a poly behind m_pPlane goes on to
	// m_Sides[BackSide] (the next plane), one in front is outside the brush.
	// The chain ends in TEMPNODE_OUT; a poly that gets there is inside.
	struct TempNode
	{
		CPrePlane			*m_pPlane;
		TempNode			*m_Sides[2];
	};

	#define TEMPNODE_IN		((TempNode*)-1)
	#define TEMPNODE_OUT	((TempNode*)-2)


	struct TempBrush
	{
		// Index into the brush array.
		uint16					m_Index;

		// The brushes this one intersects (they point into one shared array).
		TempBrush				**m_Intersecters;
		uint16					m_nIntersecters;

		CGLinkedList<TempPoly*>	m_Polies;

		// Bounding box (grown by 1).
		PVector					m_Min, m_Max;

		TempNode				*m_pTree;

		// One node per poly, allocated with the brush.
		uint16					m_nNodes;
		TempNode				m_Nodes[1];
	};


	// ----------------------------------------------------------------------- //
	// Functions.
	// ----------------------------------------------------------------------- //

	// Makes the TempBrush for a brush (snapping it unless it's NoSnap).
	TempBrush* MakeTempBrush(CEditBrush *pBrush);

	// Convert a CEditPoly to a CPrePoly.
	CPrePoly* EditPolyToPrePoly(CEditPoly *pEditPoly);

	BOOL BrushToWorld(
		GenList<CEditBrush*> &brushes,
		CPreWorld *pOut,
		void (*ProgressCB)(float fPercent));


#endif  // __BRUSH_TO_WORLD_H__
