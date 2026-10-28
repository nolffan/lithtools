// talon: stand-ins for LightMapMaker_r1.cpp, to fold into the shared
// headers (PreLeaf.h, lightmap_planes.h) when the file is folded in.

#ifndef __LIGHTMAPMAKER_R1_DECL_H__
#define __LIGHTMAPMAKER_R1_DECL_H__

	// PreLeaf.h's MakeBoxPortals, as shipped: it returns TRUE (0x414027).
	BOOL MakeBoxPortals_r1(CMoArray<CPrePortal*> &portals, PVector &vMin, PVector &vMax);

	// lightmap_planes.h's LMPlane, as shipped: the constructor out of line
	// (0x4155d0), as Build 69's is.
	class LMPlane_r1
	{
		public:

					LMPlane_r1(LTVector vRight, LTVector vUp, LTVector vNormal);

			PVector	m_Right;
			PVector	m_Up;
			PVector	m_Normal;
	};

	extern LMPlane_r1 g_LMPlanes_r1[NUM_LM_PLANES];

#endif
