// talon: 0x00417180..0x00419e20: the lighting of a poly by one light and by
// the sun, and the segment tests they use (the world tree callbacks, the
// lighting BSP walk, the sun's first hit), written from the image. To be
// folded into LightMapMaker.cpp, whose versions of these six functions
// (written before, from Build 69's shape, none of them matching) this file
// replaces. (The file name is ours and provisional.)
//
// The rest of the range is LightMapMaker.cpp's and matched already:
// MatVMul_H's COMDAT (0x417830), the light samples (0x4178b0, 0x417bd0),
// CalcSampleSurfaceDot (0x417c10), ApplySunLightToPoly (0x418260),
// SunWorldModelCB (0x418760), CalculateLightFrame (0x418b70) and on.
// Function-table rows 0x41746e, 0x417478 and 0x41756e are not functions:
// each starts inside an instruction of ApplyLightToPoly (the je at
// 0x417469, the mov at 0x417474, the sub at 0x41756d), which runs
// 0x417180..0x417829.

#include "bdefs.h"
#include "lightmapmaker.h"
#include "processing.h"
#include "editobjects.h"
#include "iobjectplugin.h"
#include "threads.h"
#include "prelightmap.h"
#include "lightmap_compress.h"
#include "gettextureinfo.h"
#include "geomroutines.h"
#include "node.h"
#include "conparse.h"
#include "genericprop_setup.h"
#include "ltquatbase.h"
#include "parse_world_info.h"
#include "pregeometry.h"
#include "lightmapmaker_r2_decl.h"

static LTBOOL LightWorldModelCB(WorldTreeObj *pObj, void *pUser);
static BOOL TraceSegment(LMMStackEntry *pStack, CPreWorld *pWorld, NODEREF pNode,
	PVector vStart, PVector vEnd, BOOL *pbContinue, PVector *pNewStart);
CPrePoly* FindSegmentHit(NODEREF pNode, PVector *pStart, PVector *pEnd, PVector *pHit);
static BOOL IsPtInPoly(PVector vPt, CPrePoly *pPoly);


// 0x417180..0x417829. Near, by the inline budget: the shipped code expands
// the query's stack construction to a loop calling the empty constructor
// (0x417190; ours calls ??_H, the vector constructor iterator) and leaves
// the directional shadow-map test's helpers as calls (the FVec of the
// position expanded but its _CVector constructor called, 0x41753b; FVec of
// the direction 0x41e090, operator- 0x402200, Norm 0x4044b0, Dot 0x4021e0),
// all of which ours expands; its frame is 0x1c bytes bigger for their
// temporaries, and this, pLight and pPoly are in edi, esi and ebx (ours
// esi, ebx, edi). The statements, calls and branches are the shipped ones.
// The other levers tried (the query's and the entry's constructors implicit,
// inline or out of line; the shadow test as locals, temporaries or
// assignments) don't move it.
BOOL CLightMapMaker::ApplyLightToPoly(CLightDef *pLight, CPrePoly *pPoly, CMoArray<PVector> *pPositions)
{
	LMMSegQuery		query;
	CFullLightMap	*pMap;
	PVector			lightPos, vPStep, vQStep, rowPos, pos, lightDir, lightPos2;
	PVector			color, rayDir;
	PVector			*pColor;
	PReal			distSqr;
	uint32			x, y;


	pMap = m_FullLightMaps[pPoly->m_nName];
	lightPos = pLight->m_Pos;

	vQStep = pPoly->GetSurface()->InverseQ * m_pWorld->m_LMGridSize;
	vPStep = pPoly->GetSurface()->InverseP * m_pWorld->m_LMGridSize;
	rowPos = pPoly->m_PolyO + pPoly->GetSurface()->InverseP * m_ShiftU +
		pPoly->GetSurface()->InverseQ * m_ShiftV;

	lightPos2 = pLight->m_Pos;
	lightDir = pLight->m_Direction;

	if(pPoly->m_pWorld->m_bUsingTransform)
	{
		// 0x417327: P's step, Q's, then the row through operator* (a copy
		// of rowPos in, MatVMul_H, 0x417830, out to a temporary).
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &vPStep);
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &vQStep);
		rowPos = pPoly->m_pWorld->m_mTransform * rowPos;
	}

	pColor = pMap->m_Map.GetArray();
	for(y=0; y < pMap->m_Height; y++)
	{
		pos = rowPos;
		for(x=0; x < pMap->m_Width; x++)
		{
			distSqr = (lightPos - pos).MagSqr();
			if(distSqr < pLight->m_MaxDistSqr)
			{
				if(m_bClipLight && pLight->m_bClip)
				{
					query.m_vStart = (*pPositions)[y*pMap->m_Width + x];
					query.m_vEnd = lightPos;
					query.m_pMaker = this;
					query.m_bBlocked = FALSE;
					m_WorldTree.IntersectSegment(&query.m_vStart, &query.m_vEnd,
						LightWorldModelCB, &query, NOA_Objects);

					if(!query.m_bBlocked)
					{
						if(m_pCurAnim->m_bShadowMap)
						{
							if(pLight->m_bDirectional)
							{
								// 0x417528: FVec of the position expanded
								// (its constructor called, 0x4021c0), of the
								// direction called (0x41e090).
								PVector vLightPos = pLight->m_Pos.FVec();
								PVector vLightDir = pLight->m_Direction.FVec();
								rayDir = pos - vLightPos;
								rayDir.Norm();
								if(rayDir.Dot(vLightDir) > (PReal)sin(MATH_HALFPI - pLight->m_FOV * 0.5f))
									color.Init(255.0f, 255.0f, 255.0f);
								else
									color.Init(0.0f, 0.0f, 0.0f);
							}
							else
							{
								color.Init(255.0f, 255.0f, 255.0f);
							}
						}
						else if(g_pGlobs->m_bLambertian)
						{
							color = CalcLightSample(pLight, lightPos2, lightDir, pos, distSqr,
								pPoly->Normal());
						}
						else
						{
							color = CalcLightSample(pLight, lightPos2, lightDir, pos, distSqr, 1.0f);
						}

						*pColor += color;
					}
				}
				else
				{
					if(g_pGlobs->m_bLambertian)
					{
						color = CalcLightSample(pLight, lightPos2, lightDir, pos, distSqr,
							pPoly->Normal());
					}
					else
					{
						color = CalcLightSample(pLight, lightPos2, lightDir, pos, distSqr, 1.0f);
					}

					*pColor += color;
				}
			}

			pos += vPStep;
			pColor++;
		}

		rowPos += vQStep;
	}

	return TRUE;
}


// Whether a world model is between a texel and a light (0x417cd0; the
// segment is in the world model's own space, pushed down its BSP until it
// gets through or hits something that blocks light).
static LTBOOL LightWorldModelCB(WorldTreeObj *pObj, void *pUser)
{
	LMMWorld	*pWorld = (LMMWorld*)pObj;
	LMMSegQuery	*pQuery = (LMMSegQuery*)pUser;
	PVector		vStart, vEnd, vNewStart;
	BOOL		bContinue;

	bContinue = TRUE;

	// The trace loop is in the image twice, once after each arm
	// (0x417ea0..0x417f12 and 0x417f35..0x417fa3), with its own exit.
	if(pWorld->m_bUsingTransform)
	{
		// 0x417cf2: LTMatrix::operator*, each point copied into the
		// argument (0x14(%esp)) and MatVMul_H expanded.
		vStart = pWorld->m_mInverseTransform * pQuery->m_vStart;
		vEnd = pWorld->m_mInverseTransform * pQuery->m_vEnd;

		while(bContinue)
		{
			if(TraceSegment(pQuery->m_Stack, pWorld->m_pWorld, pWorld->m_pWorld->m_RootNode,
				vStart, vEnd, &bContinue, &vNewStart))
			{
				if(!bContinue)
				{
					pQuery->m_bBlocked = TRUE;
					return TRUE;
				}

				vStart = vNewStart;
			}
		}

		return bContinue;
	}
	else
	{
		vStart = pQuery->m_vStart;
		vEnd = pQuery->m_vEnd;

		while(bContinue)
		{
			if(TraceSegment(pQuery->m_Stack, pWorld->m_pWorld, pWorld->m_pWorld->m_RootNode,
				vStart, vEnd, &bContinue, &vNewStart))
			{
				if(!bContinue)
				{
					pQuery->m_bBlocked = TRUE;
					return TRUE;
				}

				vStart = vNewStart;
			}
		}

		return bContinue;
	}
}


// Pushes a segment down a BSP. FALSE if it gets through; TRUE if it hits
// something solid, and then, if what it hit lets light through, *pbContinue
// is set and *pNewStart is where to go on from (0x417fc0..0x418257).
//
// Near: size and instructions exact (664/664); the shipped frame is four
// bytes bigger (sub $0x18: vDir at 0x0, an unused dword at 0xc, y2 and z2 at
// 0x10 and 0x14; ours has no unused dword and vDir above y2 and z2), so every
// frame offset differs, and z2 is loaded through edx where ours uses ecx
// (0x417fdf). Declaring the coordinates initialized, or vDir first, changes
// the instruction order instead.
//
// Retail bug (harmless): the test of the splitting node's poly at 0x4181f8
// compares the low WORD of the CPrePoly pointer with 0xFFFF (a no-poly index,
// it seems, from when the field was a WORD index); a pointer never has that
// low word, so the test is always true and a node without a poly would
// crash on the surface flags.
static BOOL TraceSegment(LMMStackEntry *pStack, CPreWorld *pWorld, NODEREF pNode,
	PVector vStart, PVector vEnd, BOOL *pbContinue, PVector *pNewStart)
{
	LMMStackEntry	*pCur;
	CPrePoly		*pPoly;
	PReal			dot1, dot2, t, midX, midY, midZ;
	PReal			x1, y1, z1, x2, y2, z2;
	BOOL			bFront;

	x1 = vStart.x;
	y1 = vStart.y;
	z1 = vStart.z;
	x2 = vEnd.x;
	y2 = vEnd.y;
	z2 = vEnd.z;

	pCur = pStack;
	// 0x417fd3: built in its own frame slots (0x0(%esp)), not copied there.
	PVector vDir = vEnd - vStart;
	vDir.Norm();

	for(;;)
	{
		while(pNode == NODE_IN)
		{
			if(pCur == pStack)
			{
				*pbContinue = FALSE;
				return FALSE;
			}

			--pCur;
			x1 = pCur->m_vStart.x;
			y1 = pCur->m_vStart.y;
			z1 = pCur->m_vStart.z;
			x2 = pCur->m_vEnd.x;
			y2 = pCur->m_vEnd.y;
			z2 = pCur->m_vEnd.z;
			pNode = pCur->m_pNode;
		}

		if(pNode == NODE_OUT)
			break;

		dot1 = (x1*pNode->m_pPlane->m_Normal.x + y1*pNode->m_pPlane->m_Normal.y + z1*pNode->m_pPlane->m_Normal.z) - pNode->m_pPlane->m_Dist;
		dot2 = (x2*pNode->m_pPlane->m_Normal.x + y2*pNode->m_pPlane->m_Normal.y + z2*pNode->m_pPlane->m_Normal.z) - pNode->m_pPlane->m_Dist;
		if(dot1 > -0.1 && dot2 > -0.1)
		{
			pNode = pNode->m_Sides[FrontSide];
		}
		else if(dot1 < 0.1 && dot2 < 0.1)
		{
			pNode = pNode->m_Sides[BackSide];
		}
		else
		{
			bFront = dot1 > 0.0f;

			pCur->m_pSplitNode = pNode;
			t = -(dot1 / (dot2 - dot1));
			pCur->m_vStart.x = midX = (x2 - x1) * t + x1;
			pCur->m_vStart.y = midY = (y2 - y1) * t + y1;
			pCur->m_vStart.z = midZ = (z2 - z1) * t + z1;
			pCur->m_pNode = pNode->m_Sides[!bFront];
			pCur->m_vEnd.x = x2;
			pCur->m_vEnd.y = y2;
			pCur->m_vEnd.z = z2;
			++pCur;

			pNode = pNode->m_Sides[bFront];
			x2 = midX;
			y2 = midY;
			z2 = midZ;
		}
	}

	*pbContinue = FALSE;
	if(pCur > pStack)
	{
		pPoly = (pCur-1)->m_pSplitNode->m_pPoly;
		if((uint16)(uint32)pPoly != 0xFFFF && (pPoly->GetSurfaceFlags() & SURF_NOBLOCKLIGHT))
		{
			*pNewStart = (pCur-1)->m_vStart + vDir;
			*pbContinue = TRUE;
		}
	}

	return TRUE;
}


// 0x418610..0x418753.
BOOL CLightMapMaker::DoesSunLightHit(const PVector &vPos)
{
	LMMSegQuery	query;

	query.m_pMaker = this;
	query.m_pHitPoly = NULL;
	query.m_nTests = 0;
	query.m_fBestDistSqr = 0.0f;
	// 0x41861a: the two ends are locals, the far one first, copied into
	// the query after (the sun direction copied once more, 0x18(%esp), for
	// the near end's operator-).
	PVector vEnd = vPos - m_SunLightDir * m_fSunRayLength;
	PVector vStart = vPos - m_SunLightDir;
	query.m_vStart = vStart;
	query.m_vEnd = vEnd;
	m_WorldTree.IntersectSegment(&query.m_vStart, &query.m_vEnd, SunWorldModelCB, &query, NOA_Objects);

	if(!query.m_nTests)
		return TRUE;

	if(query.m_pHitPoly && query.m_pHitPoly->GetSurface() &&
		(query.m_pHitPoly->GetSurfaceFlags() & SURF_SKY))
	{
		return TRUE;
	}

	return FALSE;
}


// The first poly a segment hits in a BSP, and where (0x418830..0x4189d4;
// called by SunWorldModelCB, 0x418760, so not static).
CPrePoly* FindSegmentHit(NODEREF pNode, PVector *pStart, PVector *pEnd, PVector *pHit)
{
	CPrePoly	*pRet;
	PReal		dot1, dot2, t;
	PVector		vHit;
	uint32		side1, side2;

	// The two tail calls are jumps back to the top (0x4188d7, 0x4189a7):
	// written recursive, the loop is VC6's (no rotation, as shipped).
	if(!pNode || pNode == NODE_IN || pNode == NODE_OUT)
		return NULL;

	// Near (0x418860): dot1's x and z products load their operands the other
	// way round (VC6 orders them whatever the source order; the grouping
	// a + (b + c) is what gives the shipped z, y, x term order).
	dot1 = (pNode->m_pPlane->m_Normal.x*(*pStart).x+(pNode->m_pPlane->m_Normal.y*(*pStart).y+pNode->m_pPlane->m_Normal.z*(*pStart).z)) - pNode->m_pPlane->m_Dist;
	dot2 = (pNode->m_pPlane->m_Normal.x*(*pEnd).x+(pNode->m_pPlane->m_Normal.y*(*pEnd).y+pNode->m_pPlane->m_Normal.z*(*pEnd).z)) - pNode->m_pPlane->m_Dist;
	side1 = dot1 > 0.0f;
	side2 = dot2 > 0.0f;
	if(side1 == side2)
		return FindSegmentHit(pNode->m_Sides[side1], pStart, pEnd, pHit);

	pRet = FindSegmentHit(pNode->m_Sides[side1], pStart, pEnd, pHit);
	if(pRet)
		return pRet;

	if(!(pNode->m_pPoly->GetSurfaceFlags() & SURF_INVISIBLE))
	{
		t = dot1 / (dot1 - dot2);
		// 0x418913: only the subtraction copies *pStart.
		vHit = *pStart + (*pEnd - *pStart) * t;
		if(IsPtInPoly(vHit, pNode->m_pPoly))
		{
			*pHit = vHit;
			return pNode->m_pPoly;
		}
	}

	return FindSegmentHit(pNode->m_Sides[side2], pStart, pEnd, pHit);
}


// Whether a point on a poly's plane is inside its edges (0x4189e0..0x418b6b).
//
// Near: 381 bytes against 396. The shipped frame is 0x10 bytes bigger (an
// unused 16 bytes above edgeNormal, 0x30..0x3f), the count is kept in
// pPoly's dead slot for the loop test with the divisor zero-extended again
// into ebp (0x4189f8, 0x418a0a) where ours keeps one copy in ebx, and the
// shipped code computes the vertex's Dot first and leaves it and
// edgeNormal.z on the FPU stack (two fstp at 0x418b3d, 0x418b42). Tried: an
// int count, the loop bound or the divisor as NumVerts() or the local, the
// edge and its normal initialized in the loop, the vertex's Dot in a local.
static BOOL IsPtInPoly(PVector vPt, CPrePoly *pPoly)
{
	PVector	edge, edgeNormal;
	uint32	i, nVerts;

	// 0x4189eb: the count is kept (in pPoly's slot) for the loop test; the
	// modulus takes NumVerts() again (0x418a0a, zero-extended once more).
	nVerts = pPoly->NumVerts();
	for(i=0; i < nVerts; i++)
	{
		PVector &vCur = pPoly->Pt(i);
		edge = vCur - pPoly->Pt((i+1) % pPoly->NumVerts());
		edgeNormal = pPoly->Normal().Cross(edge);
		edgeNormal.Norm();

		// 0x418ae9: the vertex is Dot's (copied) argument, the point its this.
		if(vPt.Dot(edgeNormal) - edgeNormal.Dot(vCur) < 0.0f)
			return FALSE;
	}

	return TRUE;
}



