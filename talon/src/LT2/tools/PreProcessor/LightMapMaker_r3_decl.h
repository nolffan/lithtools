// talon: stand-ins for LightMapMaker_r3.cpp: LightMapMaker.cpp's static
// helpers that the r3 functions call, declared here under names of their
// own until the file is folded into LightMapMaker.cpp (where each is the
// static named in its comment).

#ifndef __LIGHTMAPMAKER_R3_DECL_H__
#define __LIGHTMAPMAKER_R3_DECL_H__

	// LightMapMaker.cpp's.
	#define TEXTURE_LIGHT_ITERATIONS	100

	// FindRowSample (0x41cd10).
	BOOL FindRowSample_41cd10(CMoArray<uint32> *pValid, int xStart, int xEnd, int y,
		int width, int height, int *pX, int *pY);

	// FindColumnSample (0x41cd80).
	BOOL FindColumnSample_41cd80(CMoArray<uint32> *pValid, int yStart, int yEnd, int x,
		int width, int height, int *pX, int *pY);

	// PointInsideCB (0x41cdf0).
	void PointInsideCB_41cdf0(WorldTreeObj *pObj, void *pUser);

	// FindObjectByName (0x41dfa0).
	uint32 FindObjectByName_41dfa0(const char *pName, CPreMainWorld *pWorld);

	// What PointInsideCB gets (LightMapMaker.cpp's LMMPointQuery).
	struct LMMPointQuery_r3
	{
		CLightMapMaker	*m_pMaker;
		PVector			m_vPos;
		uint32			*m_pbValid;
		uint32			m_nWorlds;
	};

	// ProcessLightGroup (0x41d910; static in LightMapMaker.cpp).
	void ProcessLightGroup_r3(CWorldNode *pGroup, CPreMainWorld *pWorld);

#endif
