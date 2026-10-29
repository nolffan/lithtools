#include "bdefs.h"
#include "lightmap_planes.h"


// -------------------------------------------------------------------------------- //
// 0x00415210 (949 bytes): the static initializer of the lightmap planes
// (lightmap_planes.cpp's g_LMPlanes; 0x415200 is its jmp thunk). It matches
// as Build 69 writes it (tools/shared/engine/lightmap_planes.cpp): LMPlane's
// constructor out of line and defined after the table (0x4155d0, which is
// why the image has it after the initializer), so each argument is built in
// a temporary and copied.
// -------------------------------------------------------------------------------- //

LMPlane g_LMPlanes[NUM_LM_PLANES] =
{
	LMPlane(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, 0.0f, -1.0f), LTVector(0.0f, 1.0f, 0.0f)),
	LMPlane(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, 0.0f, 1.0f), LTVector(0.0f, -1.0f, 0.0f)),
	LMPlane(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, 1.0f, 0.0f), LTVector(0.0f, 0.0f, 1.0f)),
	LMPlane(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, -1.0f, 0.0f), LTVector(0.0f, 0.0f, -1.0f)),
	LMPlane(LTVector(0.0f, 0.0f, 1.0f), LTVector(0.0f, -1.0f, 0.0f), LTVector(1.0f, 0.0f, 0.0f)),
	LMPlane(LTVector(0.0f, 0.0f, -1.0f), LTVector(0.0f, -1.0f, 0.0f), LTVector(-1.0f, 0.0f, 0.0f))
};


// 0x004155d0.
LMPlane::LMPlane(LTVector vRight, LTVector vUp, LTVector vNormal)
{
	m_Right		= vRight;
	m_Up		= vUp;
	m_Normal	= vNormal;
}


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
