// talon: what TJunctions.cpp needs that the shared headers do not declare
// yet (the names by address or ours). To be folded into those headers.

#ifndef __TJUNCTIONS_DECL_H__
#define __TJUNCTIONS_DECL_H__

	#include "preworld.h"
	#include "splitpoly.h"

	// The T-junction fixer (0x4479c0; built and destroyed by 0x43f810, its
	// destructor 0x43f960). FixTJunctions returns nothing (0x4488c9: eax is
	// not set on the way out) and never touches this; Processing.cpp's
	// declaration says BOOL.
	class CTJunctionFixer
	{
	public:
		void							FixTJunctions(CPreWorld *pWorld, BOOL bSplitPolys);

		uint32							m_Unknown_00;
		CMoArray< CMoArray<uint32> >	m_Unknown_04;	// vtable 0x47578c
		CMoArray<CPrePoly*>				m_Unknown_18;	// vtable 0x47575c
	};

	// CPrePoly::CalcExtents (0x449200, thiscall): Build 69's, which PrePoly.h
	// does not have yet. Here on a class of its own so it can be written
	// without touching PrePoly.h; it belongs in CPrePoly.
	class CTJPrePoly : public CPrePoly
	{
	public:
		// Extents tracking
		inline void CalcExtents(PReal fExpand = 0.0f) const
		{
			m_vExtentsMin = Pt(0);
			m_vExtentsMax = m_vExtentsMin;

			for (uint32 nVertLoop = 1; nVertLoop < NumVerts(); ++nVertLoop)
			{
				VEC_MIN(m_vExtentsMin, m_vExtentsMin, Pt(nVertLoop));
				VEC_MAX(m_vExtentsMax, m_vExtentsMax, Pt(nVertLoop));
			}

			m_vExtentsMin -= fExpand;
			m_vExtentsMax += fExpand;
		}
	};

	// A CPreSurface method not declared yet (0x430740, thiscall): FALSE for
	// SURF_SKY or SURF_HULLMAKER surfaces, otherwise !(m_Flags &
	// SURF_NOSUBDIV): whether the poly may be split.
	class CTJPreSurface : public CPreSurface
	{
	public:
		BOOL							sub_430740() const;
	};

	// The functions before 0x4479c0 that only the T-junction code calls
	// (0x447220..0x4479c0, written in Terrain.cpp under these names; cdecl):
	// adds the polies of the BSP under pRoot whose extents overlap pPoly's to
	// touching (recursive);
	void	sub_447220(NODEREF pRoot, CPrePoly *pPoly, CMoArray<CPrePoly*> &touching);
	// splits pPoly in two at pPt, a point on one of its edges, with pStruct
	// the classification of its points ("> 1 edge split!");
	BOOL	sub_4475a0(PVector *pPt, CPrePoly *pPoly, CPrePoly *sides[2], SplitStruct *pStruct);
	// puts pNew1 in place of pOld in the nodes of nodes and adds a node for
	// pNew2 under each (pWorld unused).
	void	sub_447940(CPreWorld *pWorld, CNodeList &nodes, CPrePoly *pOld, CPrePoly *pNew1, CPrePoly *pNew2);

#endif
