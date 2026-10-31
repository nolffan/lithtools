// talon: the lightmap planes (0x415200..0x415770), written from the image;
// the names are ours. A surface's lightmap is laid out on whichever of six
// axis planes faces most like it.

#ifndef __LIGHTMAP_PLANES_H__
#define __LIGHTMAP_PLANES_H__

	#define NUM_LM_PLANES	6

	class LMPlane
	{
		public:

					// talon: out of line (0x4155d0), defined after the table:
					// the table's initializer (0x415210) builds each argument
					// in a temporary and copies it.
					LMPlane(PVector vRight, PVector vUp, PVector vNormal);

			PVector	m_Right;
			PVector	m_Up;
			PVector	m_Normal;
	};

	extern LMPlane g_LMPlanes[NUM_LM_PLANES];

	// The plane whose normal is closest to vNormal's.
	uint32 FindLMPlane(PVector vNormal);

	// Lightmap texture vectors for a surface with normal vNormal on plane
	// iPlane.
	void GetLMPlaneVectors(uint32 iPlane, PVector &vNormal, PVector &P, PVector &Q);

#endif
