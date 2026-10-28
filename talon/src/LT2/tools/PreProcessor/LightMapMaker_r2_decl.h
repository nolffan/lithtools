// talon: what LightMapMaker_r2.cpp needs from LightMapMaker.cpp that no
// header declares (the segment queries and the light sample functions are
// file-local there). To be folded into LightMapMaker.cpp with the r2 code.

#ifndef __LIGHTMAPMAKER_R2_DECL_H__
#define __LIGHTMAPMAKER_R2_DECL_H__

	#include "lightmapmaker.h"

	#define LMM_STACK_SIZE	1024

	// A piece of a segment still to be pushed down a BSP (0x20 bytes: the
	// array construction at 0x417190 steps 0x20, 0x400 times). Its empty
	// constructor is the 0x40df70 that loop calls (where the inline budget
	// is spent; DoesSunLightHit, 0x418610, expands it to nothing). This and
	// LMMSegQuery are LightMapMaker.cpp's, as they are there.
	struct LMMStackEntry
	{
		LMMStackEntry() {}

		NODEREF		m_pNode;		// 0x00 Where the rest of the segment goes.
		NODEREF		m_pSplitNode;	// 0x04 The node that split it.
		PVector		m_vStart;		// 0x08
		PVector		m_vEnd;			// 0x14
	};

	// What the world tree callbacks get (0x8038 bytes: DoesSunLightHit's
	// frame, 0x418610; the offsets are from its stores and ApplyLightToPoly's,
	// 0x4174b8..0x4174ef).
	struct LMMSegQuery
	{
		LMMSegQuery() {}

		CLightMapMaker	*m_pMaker;			// 0x00
		BOOL			m_bBlocked;			// 0x04
		PVector			m_vStart;			// 0x08
		PVector			m_vEnd;				// 0x14

		// Sun queries: the closest poly hit.
		uint32			m_nTests;			// 0x20
		PReal			m_fBestDistSqr;		// 0x24
		CPrePoly		*m_pHitPoly;		// 0x28
		PVector			m_vHitPos;			// 0x2c

		LMMStackEntry	m_Stack[LMM_STACK_SIZE];	// 0x38
	};

	// LightMapMaker.cpp's light samples (static there, so they must stop
	// being static, or this file be folded in, before the link; cdecl,
	// returning the color through a hidden pointer): 0x4178b0, with the
	// surface factor given, and 0x417bd0, with it from the poly's normal.
	PVector CalcLightSample(const CLightDef *pLight, const PVector &lightPos,
		const PVector &lightDir, const PVector &samplePos, PReal distSqr, float fSurfaceDot);
	PVector CalcLightSample(const CLightDef *pLight, const PVector &lightPos,
		const PVector &lightDir, const PVector &samplePos, PReal distSqr, const PVector &surfNormal);

	// The sun's world tree callback (0x418760, static in LightMapMaker.cpp,
	// matched; the same applies): the closest poly of a world model between
	// the sky and a point, through FindSegmentHit (0x418830, defined in
	// LightMapMaker_r2.cpp and so not static here).
	LTBOOL SunWorldModelCB(WorldTreeObj *pObj, void *pUser);

#endif  // __LIGHTMAPMAKER_R2_DECL_H__
