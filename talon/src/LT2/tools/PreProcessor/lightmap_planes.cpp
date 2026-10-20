#include "bdefs.h"
#include "lightmap_planes.h"


LMPlane g_LMPlanes[NUM_LM_PLANES] =
{
	LMPlane(PVector(1.0f, 0.0f, 0.0f), PVector(0.0f, 0.0f, -1.0f), PVector(0.0f, 1.0f, 0.0f)),
	LMPlane(PVector(1.0f, 0.0f, 0.0f), PVector(0.0f, 0.0f, 1.0f), PVector(0.0f, -1.0f, 0.0f)),
	LMPlane(PVector(1.0f, 0.0f, 0.0f), PVector(0.0f, 1.0f, 0.0f), PVector(0.0f, 0.0f, 1.0f)),
	LMPlane(PVector(1.0f, 0.0f, 0.0f), PVector(0.0f, -1.0f, 0.0f), PVector(0.0f, 0.0f, -1.0f)),
	LMPlane(PVector(0.0f, 0.0f, 1.0f), PVector(0.0f, -1.0f, 0.0f), PVector(1.0f, 0.0f, 0.0f)),
	LMPlane(PVector(0.0f, 0.0f, -1.0f), PVector(0.0f, -1.0f, 0.0f), PVector(-1.0f, 0.0f, 0.0f))
};


uint32 FindLMPlane(PVector vNormal)
{
	uint32 i, iBest;
	PReal dot, maxDot;

	iBest = 0;
	maxDot = -2.0f;
	for(i=0; i < NUM_LM_PLANES; i++)
	{
		dot = g_LMPlanes[i].m_Normal.Dot(vNormal);
		if(dot > maxDot)
		{
			maxDot = dot;
			iBest = i;
		}
	}

	return iBest;
}


void GetLMPlaneVectors(uint32 iPlane, PVector &vNormal, PVector &P, PVector &Q)
{
	P = vNormal.Cross(g_LMPlanes[iPlane].m_Up);
	Q = P.Cross(vNormal);
}
