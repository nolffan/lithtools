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

// talon: the 2001 lightmap maker (0x415770..0x41e020), written from the
// image with Build 69's as a guide (Build 69 rewrote it: no lighting BSP or
// light groups of this kind here). The names of what Build 69 lacks are ours.


//the name to use for the main base lightmap animation (every world has this one)
#define BASE_LIGHTANIM_NAME "LightAnim_BASE"


TLightNameMap g_LightsByName;


// -------------------------------------------------------------------------------- //
// Helpers.
// -------------------------------------------------------------------------------- //

LMMWorld::LMMWorld() : WorldTreeObj(WTObj_DObject)
{
	m_vMin.Init();
	m_vMax.Init();
	m_bUsingTransform = FALSE;
}


// -------------------------------------------------------------------------------- //
// CLightMapMaker.
// -------------------------------------------------------------------------------- //

CLightMapMaker::CLightMapMaker()
{
	m_pWorld = NULL;
	m_FrameCode = 1;
	m_WorldTree.Init(this);
	m_hClassModule = NULL;
	m_pCurAnim = NULL;
	m_pCurFrame = NULL;
	m_fSunRayLength = 0.0f;
	m_ThreadCS = NULL;
	m_pMathLT = &m_MathLT;
}


CLightMapMaker::~CLightMapMaker()
{
	Term();
}


BOOL CLightMapMaker::Init(CPreMainWorld *pWorld, BOOL bClipLight,
	void (*ProgressFn)(float fPercent))
{
	PVector vMin, vMax;
	LTVector vAmbient;
	uint32 i, nTotalPolies, iOutPoly;
	CPreWorld *pModel;
	GPOS pos;


	Term();

	// Setup our critical section.
	m_ThreadCS = thd_CreateCriticalSection();
	if(!m_ThreadCS)
		return FALSE;

	m_bClipLight = bClipLight;
	m_pWorld = pWorld;
	m_ProgressFn = ProgressFn;

	pWorld->SetupPolyArrays();

	m_bOverSample = TRUE;
	m_ShiftU = m_ShiftV = -(pWorld->m_LMGridSize * 0.5f);

	// Get ambient light levels out of the world info string.
	ParseAmbientLight(pWorld->m_pInfoString, &vAmbient);
	m_AmbientLight = vAmbient;

	vMin.Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	vMax.Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);

	if(!m_LMMWorlds.SetSize(pWorld->m_WorldModels.GetSize()))
		return FALSE;

	nTotalPolies = 0;
	for(i=0; i < pWorld->m_WorldModels; i++)
	{
		pModel = pWorld->m_WorldModels[i];
		pModel->UpdateBoundingBox();
		m_LMMWorlds[i].m_pWorld = pModel;

		VEC_MIN(vMin, vMin, pModel->m_PosMin);
		VEC_MAX(vMax, vMax, pModel->m_PosMax);

		// Set its poly pointers.
		for(pos=pModel->m_Polies; pos; )
		{
			pModel->m_Polies.GetNext(pos)->m_pWorld = &m_LMMWorlds[i];
		}

		nTotalPolies += pModel->m_Polies.GetSize();
	}

	m_WorldTree.CreateNodes(&vMin, &vMax, CreateNodesCB, NULL);
	m_fSunRayLength = (vMax - vMin).Mag() + 1.0f;

	// Setup our 'all polies' list.
	if(!m_Polies.SetSize(nTotalPolies))
		return FALSE;

	iOutPoly = 0;
	for(i=0; i < pWorld->m_WorldModels; i++)
	{
		pModel = pWorld->m_WorldModels[i];

		for(pos=pModel->m_Polies; pos; )
		{
			m_Polies[iOutPoly] = pModel->m_Polies.GetNext(pos);
			m_Polies[iOutPoly]->m_nName = (uint16)iOutPoly;
			iOutPoly++;
		}
	}

	DrawStatusTextIfMainWorld("Ambient light (%.1f, %.1f, %.1f)", VEC_EXPAND(m_AmbientLight));
	return TRUE;
}


// Nodes are made down to 300 units.
LTBOOL CLightMapMaker::CreateNodesCB(WorldTree *pTree, WorldTreeNode *pNode, void *pUser)
{
	return pNode->m_MinSize > 300.0f;
}


void CLightMapMaker::Term()
{
	DeleteAndClearArray(m_FullLightMaps);
	DeleteAndClearArray(m_LightDefs);

	UnloadClassModule();
	m_ProgressFn = NULL;

	if(m_ThreadCS)
	{
		thd_DeleteCriticalSection(m_ThreadCS);
		m_ThreadCS = NULL;
	}
}


PVector CLightMapMaker::CalcSunLightSample(PVector dir)
{
	PVector ret;
	PReal dot, t;

	dot = -m_SunLightDir.Dot(dir);
	if(dot <= 0.0f)
		return m_SunLightOuter;

	t = (PReal)pow(dot * m_SunLightBrightScale, m_SunLightBias);
	if(t < 0.0f)
		t = 0.0f;
	else if(t > 1.0f)
		t = 1.0f;

	VEC_LERP(ret, m_SunLightOuter, m_SunLightInner, t);
	return ret;
}


BOOL CLightMapMaker::InitPolyLightmapFill(CPrePoly *pPoly, CFullLightMap *pMap)
{
	uint32 width, height, i;
	PVector fillColor;
	CPrePlane *pPlane;

	width = pPoly->m_LMWidth;
	height = pPoly->m_LMHeight;
	if(m_bOverSample)
	{
		width++;
		height++;
	}

	pMap->m_bAmbientOnly = FALSE;
	if(g_pGlobs->m_bVerticesOnly ||
		!(pPoly->GetSurface()->m_Flags & SURF_LIGHTMAP) ||
		(pPoly->GetSurface()->m_Flags & (SURF_INVISIBLE|SURF_SKY)))
	{
		return FALSE;
	}

	pMap->SetSize(width, height);

	if(pPoly->GetSurface()->m_Flags & SURF_DIRECTIONALLIGHT)
	{
		pPlane = pPoly->GetSurface()->m_pPlane;
		fillColor = m_AmbientLight;
		fillColor += CalcSunLightSample(PVector(pPlane->m_Normal.x, pPlane->m_Normal.y, pPlane->m_Normal.z));

		if(fillColor.x < 0.0f)
			fillColor.x = 0.0f;
		else if(fillColor.x > 255.0f)
			fillColor.x = 255.0f;

		if(fillColor.y < 0.0f)
			fillColor.y = 0.0f;
		else if(fillColor.y > 255.0f)
			fillColor.y = 255.0f;

		if(fillColor.z < 0.0f)
			fillColor.z = 0.0f;
		else if(fillColor.z > 255.0f)
			fillColor.z = 255.0f;
	}
	else
	{
		fillColor = m_AmbientLight;
		if(pPoly->GetSurface()->m_Flags & SURF_BRIGHT)
			fillColor.Init(255.0f, 255.0f, 255.0f);
		else
			pMap->m_bAmbientOnly = TRUE;
	}

	pMap->ColorFill(fillColor);

	pMap->m_Min.Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	pMap->m_Max.Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);
	for(i=0; i < pPoly->NumVerts(); i++)
	{
		VEC_MIN(pMap->m_Min, pMap->m_Min, pPoly->Pt(i));
		VEC_MAX(pMap->m_Max, pMap->m_Max, pPoly->Pt(i));
	}

	return TRUE;
}


BOOL CLightMapMaker::SetupNewLightAnim(const char *pName)
{
	CPreLightAnim *pAnim;
	uint32 iWorld, iPoly;
	CPrePolyRef newRef;
	CPreWorld *pModel;
	GPOS pos;


	// Setup the base light anim.
	pAnim = new CPreLightAnim;
	if(!pAnim)
		return FALSE;

	if(!m_pWorld->m_LightAnims.Append(pAnim))
	{
		delete pAnim;
		return FALSE;
	}

	SAFE_STRCPY(pAnim->m_Name, pName);
	pAnim->m_Polies.SetCacheSize(1024);

	for(iWorld=0; iWorld < m_pWorld->m_WorldModels; iWorld++)
	{
		pModel = m_pWorld->m_WorldModels[iWorld];
		if(pModel->m_WorldInfoFlags & WIF_VISBSP)
			continue;

		iPoly = 0;
		for(pos=pModel->m_Polies; pos; )
		{
			pModel->m_Polies.GetNext(pos);

			newRef.m_iWorld = iWorld;
			newRef.m_iPoly = iPoly;
			if(!pAnim->m_Polies.Append(newRef))
				return FALSE;

			iPoly++;
		}
	}

	m_pCurAnim = pAnim;
	return TRUE;
}


BOOL CLightMapMaker::AddAnimFrame()
{
	CPreLightFrame *pFrame;


	if(!m_pCurAnim)
		return FALSE;

	pFrame = new CPreLightFrame;
	if(!pFrame)
		return FALSE;

	if(!m_pCurAnim->m_Frames.Append(pFrame))
	{
		delete pFrame;
		return FALSE;
	}

	m_pCurFrame = pFrame;
	return TRUE;
}


BOOL CLightMapMaker::Process()
{
	m_pWorld->RemoveAllLightMaps();

	if(SetupNewLightAnim(BASE_LIGHTANIM_NAME) && AddAnimFrame())
	{
		ClearPolyAnimRefs();
		GetLightDefs();
		SetupWorldModels(NULL, 0);
		SetupVertNormals();

		if(CalculateLightFrame())
		{
			if(!g_pGlobs->m_bLightAnimations || DoObjectPreLights())
			{
				Term();
				return TRUE;
			}
		}
	}

	Term();
	return FALSE;
}


BOOL CLightMapMaker::DoObjectPreLights()
{
	ClassDef *pClass;
	IObjectPlugin *pPlugin;
	CBaseEditObj *pObj;
	uint32 i;

	m_AmbientLight.Init();

	if(!LoadClassModule())
	{
		DrawStatusText("Warning: couldn't load object.lto for light animations");
		return TRUE;
	}

	for(i=0; i < m_pWorld->m_Objects; i++)
	{
		pObj = m_pWorld->m_Objects[i];
		for(pClass=cb_FindClass(m_hClassModule, pObj->GetClassName()); pClass; pClass=pClass->m_ParentClass)
		{
			if(pClass->m_PluginFn)
			{
				pPlugin = pClass->m_PluginFn();
				if(pPlugin)
				{
					pPlugin->PreHook_Light(this, (HPREOBJECT)i);
					delete pPlugin;
				}

				break;
			}
		}
	}

	return TRUE;
}


void CLightMapMaker::ThreadCB(void *pUser)
{
	((CLightMapMaker*)pUser)->ThreadMain();
}


void CLightMapMaker::ThreadMain()
{
	uint32 iPoly;

	// Get polies to light..
	while(ThreadGetNextJob(iPoly))
	{
		if(!ThreadLightPoly(iPoly))
		{
			ThreadSignalError();
			break;
		}
	}
}


BOOL CLightMapMaker::ThreadLightPoly(uint32 iPoly)
{
	CPrePoly				*pPoly;
	CFullLightMap			*pLightMap, tempMap;
	CPreLightMap			*pPreLightMap;
	CMoArray<PVector>		positions;
	CLinkedList<CLightDef*>	lights;
	CLightDef				*pLight;
	LPOS					pos;
	uint32					i;


	pPoly = m_pWorld->GetLMPoly(&m_pCurAnim->m_Polies[iPoly]);
	if(!pPoly)
		return FALSE;

	pPoly->m_nName = iPoly;

	if(!LightPolygon(pPoly))
		return FALSE;

	if(!GetPolyVertexColors(pPoly, *m_pCurFrame->m_Unknown_14[iPoly]))
		return FALSE;

	if(g_pGlobs->m_bVerticesOnly || !(pPoly->GetSurfaceFlags() & SURF_LIGHTMAP) ||
		(pPoly->GetSurfaceFlags() & SURF_INVISIBLE) || (pPoly->GetSurfaceFlags() & SURF_SKY))
	{
		return TRUE;
	}

	// Make the full light map.
	pLightMap = m_FullLightMaps[iPoly];

	// Make the output lightmap.
	pPreLightMap = m_pCurFrame->m_PolyMaps[iPoly];
	if(!pPreLightMap)
		return FALSE;

	// Figure out how big this poly's lightmaps are.
	GetPolyLightmapSize(pPoly);

	// Setup the lightmap..
	if(!InitPolyLightmapFill(pPoly, pLightMap))
		return FALSE;

	// Make a list of lights that affect this poly.
	for(i=0; i < m_LightDefs; i++)
	{
		pLight = m_LightDefs[i];
		if(LightTouchesPoly(pLight, pPoly))
			lights.Append(pLight);
	}

	if(lights.GetSize() > 0)
	{
		CalcSamplePositions(pPoly, 3, &positions);

		for(pos=lights; pos; )
		{
			pLight = lights.GetNext(pos);
			if(!ApplyLightToPoly(pLight, pPoly, &positions))
				return FALSE;
		}
	}

	if(m_bStaticSunLight)
	{
		if(lights.GetSize() == 0)
			CalcSamplePositions(pPoly, 3, &positions);

		if(!ApplySunLightToPoly(pPoly, &positions))
			return FALSE;
	}

	if(m_bOverSample)
	{
		DownSample(pLightMap, &tempMap);
		pLightMap = &tempMap;
	}

	ClampMapValues(pLightMap, pLightMap->m_Width, pLightMap->m_Height);
	ConvertMap(pLightMap, pPreLightMap);
	return TRUE;
}


BOOL CLightMapMaker::ThreadGetNextJob(uint32 &iPoly)
{
	BOOL bRet;

	bRet = FALSE;
	if(!m_ThreadCS)
		return FALSE;

	thd_EnterCriticalSection(m_ThreadCS);
		
		if(!m_bThreadError && m_pCurAnim && m_iCurThreadPoly < m_pCurAnim->m_Polies.GetSize())
		{
			iPoly = m_iCurThreadPoly;
			m_iCurThreadPoly++;

			// Show progress..
			if(m_ProgressFn)
				m_ProgressFn((float)m_iCurThreadPoly / m_pCurAnim->m_Polies.GetSize());
			
			bRet = TRUE;
		}
	
	thd_LeaveCriticalSection(m_ThreadCS);

	return bRet;
}


void CLightMapMaker::ThreadSignalError()
{
	if(m_ThreadCS)
	{
		thd_EnterCriticalSection(m_ThreadCS);
			m_bThreadError = TRUE;
		thd_LeaveCriticalSection(m_ThreadCS);
	}
}


// ------------------------------------------------------------------------ //
// Light clipping: a segment against the world models' BSPs.
// ------------------------------------------------------------------------ //

#define LMM_STACK_SIZE	1024

// A piece of a segment still to be pushed down a BSP (the names are ours).
struct LMMStackEntry
{
	LMMStackEntry() {}

	NODEREF		m_pNode;		// Where the rest of the segment goes.
	NODEREF		m_pSplitNode;	// The node that split it.
	PVector		m_vStart;
	PVector		m_vEnd;
};

// What the world tree callbacks get.
struct LMMSegQuery
{
	LMMSegQuery() {}

	CLightMapMaker	*m_pMaker;
	BOOL			m_bBlocked;
	PVector			m_vStart;
	PVector			m_vEnd;

	// Sun queries: the closest poly hit.
	uint32			m_nTests;
	PReal			m_fBestDistSqr;
	CPrePoly		*m_pHitPoly;
	PVector			m_vHitPos;

	LMMStackEntry	m_Stack[LMM_STACK_SIZE];
};

static PVector CalcLightSample(const CLightDef *pLight, const PVector &lightPos,
	const PVector &lightDir, const PVector &samplePos, PReal distSqr, float fSurfaceDot);
static PVector CalcLightSample(const CLightDef *pLight, const PVector &lightPos,
	const PVector &lightDir, const PVector &samplePos, PReal distSqr, const PVector &surfNormal);
static float CalcSampleSurfaceDot(const PVector &lightPos, const PVector &samplePos, const PVector &vNormal);
static LTBOOL LightWorldModelCB(WorldTreeObj *pObj, void *pUser);
static BOOL TraceSegment(LMMStackEntry *pStack, CPreWorld *pWorld, NODEREF pNode,
	PVector vStart, PVector vEnd, BOOL *pbContinue, PVector *pNewStart);
static LTBOOL SunWorldModelCB(WorldTreeObj *pObj, void *pUser);
static CPrePoly* FindSegmentHit(NODEREF pNode, PVector *pStart, PVector *pEnd, PVector *pHit);
static BOOL IsPtInPoly(PVector vPt, CPrePoly *pPoly);


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
		MatVMul_3x3(&vQStep, &pPoly->m_pWorld->m_mTransform, &vQStep);
		MatVMul_3x3(&vPStep, &pPoly->m_pWorld->m_mTransform, &vPStep);
		MatVMul_H(&rowPos, &pPoly->m_pWorld->m_mTransform, &rowPos);
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
								rayDir = pos - pLight->m_Pos;
								rayDir.Norm();
								if(rayDir.Dot(pLight->m_Direction) > (PReal)sin(MATH_HALFPI - pLight->m_FOV * 0.5f))
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


static PVector CalcLightSample(const CLightDef *pLight, const PVector &lightPos,
	const PVector &lightDir, const PVector &samplePos, PReal distSqr, float fSurfaceDot)
{
	PVector rayDir;
	PReal distPercent, anglePercent, dot, fovDotLimit;

	if(pLight->m_bDirectional)
	{
		rayDir = samplePos - lightPos;
		rayDir.Norm();
		dot = rayDir.Dot(lightDir);
		fovDotLimit = (PReal)sin(MATH_HALFPI - pLight->m_FOV * 0.5f);
		if(dot > fovDotLimit)
		{
			anglePercent = (dot - fovDotLimit) / (1.0f - fovDotLimit);
			distPercent = 1.0f - ((PReal)sqrt(distSqr) / pLight->m_MaxDist);
			return ((pLight->m_InnerColor * anglePercent + pLight->m_OuterColor * (1.0f - anglePercent)) *
				(distPercent * pLight->m_BrightScale) * fSurfaceDot).FVec();
		}
		else
		{
			return PVector(0.0f, 0.0f, 0.0f);
		}
	}
	else
	{
		distPercent = 1.0f - ((PReal)sqrt(distSqr) / pLight->m_MaxDist);
		return ((pLight->m_InnerColor * distPercent + pLight->m_OuterColor * (1.0f - distPercent)) *
			(fSurfaceDot * pLight->m_BrightScale)).FVec();
	}
}


static PVector CalcLightSample(const CLightDef *pLight, const PVector &lightPos,
	const PVector &lightDir, const PVector &samplePos, PReal distSqr, const PVector &surfNormal)
{
	return CalcLightSample(pLight, lightPos, lightDir, samplePos, distSqr,
		CalcSampleSurfaceDot(lightPos, samplePos, surfNormal));
}


static float CalcSampleSurfaceDot(const PVector &lightPos, const PVector &samplePos, const PVector &vNormal)
{
	PVector vSampleDir = lightPos - samplePos;
	vSampleDir.Norm();
	float fSurfaceDot = vSampleDir.Dot(vNormal);
	return LTMAX(fSurfaceDot, 0.0f);
}


// Whether a world model is between a texel and a light.
static LTBOOL LightWorldModelCB(WorldTreeObj *pObj, void *pUser)
{
	LMMWorld	*pWorld = (LMMWorld*)pObj;
	LMMSegQuery	*pQuery = (LMMSegQuery*)pUser;
	PVector		vStart, vEnd, vNewStart;
	BOOL		bContinue;

	bContinue = TRUE;
	if(pWorld->m_bUsingTransform)
	{
		vStart = pQuery->m_vStart;
		vEnd = pQuery->m_vEnd;
		MatVMul_InPlace_H(&pWorld->m_mInverseTransform, &vStart);
		MatVMul_InPlace_H(&pWorld->m_mInverseTransform, &vEnd);
	}
	else
	{
		vStart = pQuery->m_vStart;
		vEnd = pQuery->m_vEnd;
	}

	do
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
	while(bContinue);

	return bContinue;
}


// Pushes a segment down a BSP. FALSE if it gets through; TRUE if it hits
// something solid, and then, if what it hit lets light through, *pbContinue
// is set and *pNewStart is where to go on from.
static BOOL TraceSegment(LMMStackEntry *pStack, CPreWorld *pWorld, NODEREF pNode,
	PVector vStart, PVector vEnd, BOOL *pbContinue, PVector *pNewStart)
{
	LMMStackEntry	*pCur;
	PVector			vDir;
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
	vDir = vEnd - vStart;
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
			pNode = pCur->m_pNode;
			x1 = pCur->m_vStart.x;
			y1 = pCur->m_vStart.y;
			z1 = pCur->m_vStart.z;
			x2 = pCur->m_vEnd.x;
			y2 = pCur->m_vEnd.y;
			z2 = pCur->m_vEnd.z;
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
			midX = (x2 - x1) * t + x1;
			midY = (y2 - y1) * t + y1;
			midZ = (z2 - z1) * t + z1;
			pCur->m_vStart.x = midX;
			pCur->m_vStart.y = midY;
			pCur->m_vStart.z = midZ;
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
	if(pCur > pStack && (uint16)(uint32)(pCur-1)->m_pSplitNode->m_pPoly != 0xFFFF &&
		((pCur-1)->m_pSplitNode->m_pPoly->GetSurfaceFlags() & SURF_NOBLOCKLIGHT))
	{
		*pNewStart = vDir + (pCur-1)->m_vStart;
		*pbContinue = TRUE;
	}

	return TRUE;
}


static inline void CalcPolyExtents(const CPrePoly *pPoly, PReal fExpand = 0.0f)
{
	pPoly->m_vExtentsMin = pPoly->Pt(0);
	pPoly->m_vExtentsMax = pPoly->m_vExtentsMin;

	for (uint32 nVertLoop = 1; nVertLoop < pPoly->NumVerts(); ++nVertLoop)
	{
		const PVector& vPt = pPoly->Pt(nVertLoop);
		VEC_MIN(pPoly->m_vExtentsMin, pPoly->m_vExtentsMin, vPt);
		VEC_MAX(pPoly->m_vExtentsMax, pPoly->m_vExtentsMax, vPt);
	}

	pPoly->m_vExtentsMin -= fExpand;
	pPoly->m_vExtentsMax += fExpand;
}


BOOL CLightMapMaker::ApplySunLightToPoly(CPrePoly *pPoly, CMoArray<PVector> *pPositions)
{
	PVector			color, center, pos, sample;
	CFullLightMap	*pMap;
	PVector			*pColor;
	uint32			x, y, i;

	if(-m_SunLightDir.Dot(pPoly->Normal()) > 0.0f)
	{
		if(g_pGlobs->m_bLambertian)
		{
			color = CalcSunLightSample(pPoly->Normal());
		}
		else
		{
			color = m_SunLightInner;
		}

		pMap = m_FullLightMaps[pPoly->m_nName];
		pColor = pMap->m_Map.GetArray();

		if(!m_bClipLight)
		{
			CalcPolyExtents(pPoly);
			center = pPoly->m_vExtentsMin + (pPoly->m_vExtentsMax - pPoly->m_vExtentsMin) * 0.5f;
			if(DoesSunLightHit(center))
			{
				for(i=pMap->m_Width*pMap->m_Height; i; i--)
				{
					*pColor += color;
					pColor++;
				}
			}
		}
		else
		{
			for(y=0; y < pMap->m_Height; y++)
			{
				for(x=0; x < pMap->m_Width; x++)
				{
					pos = (*pPositions)[y*pMap->m_Width + x];
					if(DoesSunLightHit(pos))
					{
						if(m_pCurAnim->m_bShadowMap)
							sample.Init(255.0f, 255.0f, 255.0f);
						else
							sample = color;

						*pColor += sample;
					}

					pColor++;
				}
			}
		}
	}

	return TRUE;
}


BOOL CLightMapMaker::DoesSunLightHit(const PVector &vPos)
{
	LMMSegQuery	query;

	query.m_pMaker = this;
	query.m_pHitPoly = NULL;
	query.m_nTests = 0;
	query.m_fBestDistSqr = 0.0f;
	query.m_vEnd = vPos - m_SunLightDir * m_fSunRayLength;
	query.m_vStart = vPos - m_SunLightDir;
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


// The closest poly of a world model between the sky and a point.
static LTBOOL SunWorldModelCB(WorldTreeObj *pObj, void *pUser)
{
	LMMWorld	*pWorld = (LMMWorld*)pObj;
	LMMSegQuery	*pQuery = (LMMSegQuery*)pUser;
	CPrePoly	*pPoly;
	PVector		vHit;
	PReal		distSqr;

	pQuery->m_nTests++;
	pPoly = FindSegmentHit(pWorld->m_pWorld->m_RootNode, &pQuery->m_vStart, &pQuery->m_vEnd, &vHit);
	if(pPoly)
	{
		distSqr = (vHit - pQuery->m_vStart).MagSqr();
		if(distSqr < pQuery->m_fBestDistSqr || !pQuery->m_pHitPoly)
		{
			pQuery->m_pHitPoly = pPoly;
			pQuery->m_fBestDistSqr = distSqr;
			pQuery->m_vHitPos = vHit;
		}
	}

	return pQuery->m_pHitPoly != NULL;
}


// The first poly a segment hits in a BSP, and where.
static CPrePoly* FindSegmentHit(NODEREF pNode, PVector *pStart, PVector *pEnd, PVector *pHit)
{
	CPrePoly	*pRet;
	PReal		dot1, dot2, t;
	PVector		vHit;
	uint32		side1, side2;

	while(pNode && pNode != NODE_IN && pNode != NODE_OUT)
	{
		dot1 = (pNode->m_pPlane->m_Normal.x*(*pStart).x + pNode->m_pPlane->m_Normal.y*(*pStart).y + pNode->m_pPlane->m_Normal.z*(*pStart).z) - pNode->m_pPlane->m_Dist;
		dot2 = (pNode->m_pPlane->m_Normal.x*(*pEnd).x + pNode->m_pPlane->m_Normal.y*(*pEnd).y + pNode->m_pPlane->m_Normal.z*(*pEnd).z) - pNode->m_pPlane->m_Dist;
		side1 = dot1 > 0.0f;
		side2 = dot2 > 0.0f;
		if(side1 == side2)
		{
			pNode = pNode->m_Sides[side1];
		}
		else
		{
			pRet = FindSegmentHit(pNode->m_Sides[side1], pStart, pEnd, pHit);
			if(pRet)
				return pRet;

			if(!(pNode->m_pPoly->GetSurfaceFlags() & SURF_INVISIBLE))
			{
				t = dot1 / (dot1 - dot2);
				vHit = (*pEnd - *pStart) * t + *pStart;
				if(IsPtInPoly(vHit, pNode->m_pPoly))
				{
					*pHit = vHit;
					return pNode->m_pPoly;
				}
			}

			pNode = pNode->m_Sides[side2];
		}
	}

	return NULL;
}


// Whether a point on a poly's plane is inside its edges.
static BOOL IsPtInPoly(PVector vPt, CPrePoly *pPoly)
{
	PVector	edge, edgeNormal;
	uint32	i, nVerts;

	nVerts = pPoly->NumVerts();
	for(i=0; i < nVerts; )
	{
		PVector &vCur = pPoly->Pt(i);
		++i;
		edge = vCur - pPoly->Pt(i % nVerts);
		edgeNormal = pPoly->Normal().Cross(edge);
		edgeNormal.Norm();

		if(vPt.Dot(edgeNormal) - vCur.Dot(edgeNormal) < 0.0f)
			return FALSE;
	}

	return TRUE;
}



#define POLYANIMREF_NONE	0xFFFFFFFF

// How far a texture light is tiled across a poly, in each direction.
#define TEXTURE_LIGHT_ITERATIONS	100

// talon: in the PreWorld module (not written yet); the names are ours.
// val rounded down to a multiple of mod (0x42cea0).
PReal FloorToMultiple(PReal val, PReal mod);
// Whether vPt (on its plane) is inside pPoly (0x42cee0).
BOOL IsPtOverPoly(CPrePoly *pPoly, PVector &vPt);

static BOOL CalcVertNormals(CPreWorld *pWorld, CMoArray<PVector> &normals);


BOOL CLightMapMaker::CalculateLightFrame()
{
	CMoArray<THREAD_ID> threads;
	uint32 i, nThreads;

	if(!m_pCurAnim || !m_pCurFrame || !m_ThreadCS)
		return FALSE;

	DeleteAndClearArray(m_FullLightMaps);

	// Create all the objects.
	if(!m_FullLightMaps.SetSizeInit2(m_pCurAnim->m_Polies.GetSize(), NULL) ||
		!m_pCurFrame->m_PolyMaps.SetSizeInit2(m_pCurAnim->m_Polies.GetSize(), NULL) ||
		!m_pCurFrame->m_Unknown_14.SetSizeInit2(m_pCurAnim->m_Polies.GetSize(), NULL) ||
		!AllocateArray(m_FullLightMaps, (CFullLightMap*)NULL) ||
		!AllocateArray(m_pCurFrame->m_PolyMaps, (CPreLightMap*)NULL) ||
		!AllocateArray(m_pCurFrame->m_Unknown_14, (CMoArray<uint32>*)NULL))
	{
		DeleteAndClearArray(m_FullLightMaps);
		DeleteAndClearArray(m_pCurFrame->m_PolyMaps);
		DeleteAndClearArray(m_pCurFrame->m_Unknown_14);
		return FALSE;
	}

	// Spawn each thread.  Each thread will ask for a poly and provide notification
	// when it's done.
	m_iCurThreadPoly = 0;
	m_bThreadError = FALSE;

	nThreads = DMAX(1, (int)g_pGlobs->m_nThreads);

	if(!threads.SetSize(nThreads))
		return FALSE;

	for(i=0; i < threads; i++)
		threads[i] = 0;

	for(i=0; i < nThreads; i++)
	{
		threads[i] = thd_BeginThread(ThreadCB, this);
		if(!threads[i])
			break;
	}

	if(i != nThreads)
	{
		if(i == 0)
		{
			DrawStatusText("Can't create any threads to process lighting!");
			return FALSE;
		}

		DrawStatusText("Warning: could only create %d threads for lighting.", i);
	}

	thd_WaitForMultipleToFinish(threads.GetArray(), i);
	return !m_bThreadError;
}


void CLightMapMaker::GetPolyLightmapSize(CPrePoly *pPoly)
{
	uint16 xPixels, yPixels;

	pPoly->CalculateTextureSize(FALSE, g_pGlobs->m_MaxLMSize, m_pWorld->GetLMGridSize(),
		xPixels, yPixels);

	pPoly->m_LMWidth = xPixels / (uint16)m_pWorld->GetLMGridSize() + 1;
	pPoly->m_LMHeight = yPixels / (uint16)m_pWorld->GetLMGridSize() + 1;

	if(pPoly->m_LMWidth > LIGHTMAP_MAX_PIXELS || pPoly->m_LMHeight > LIGHTMAP_MAX_PIXELS)
	{
		DrawStatusText("WARNING: Invalid poly lightmap size, clamping.");
		pPoly->m_LMWidth = DMIN(pPoly->m_LMWidth, LIGHTMAP_MAX_PIXELS);
		pPoly->m_LMHeight = DMIN(pPoly->m_LMHeight, LIGHTMAP_MAX_PIXELS);
	}
}


void CLightMapMaker::ClearPolyAnimRefs()
{
	uint32 i;

	for(i=0; i < m_Polies; i++)
		m_Polies[i]->m_nName = POLYANIMREF_NONE;
}


BOOL CLightMapMaker::SetupVertNormals()
{
	uint32 i;
	CPreWorld *pWorld;

	if(!m_VertNormals.SetSize(m_pWorld->m_WorldModels.GetSize()))
		return FALSE;

	for(i=0; i < m_pWorld->m_WorldModels; i++)
	{
		CMoArray<PVector> &normals = m_VertNormals[i];
		pWorld = m_pWorld->m_WorldModels[i];

		normals.SetCacheSize(2048);
		if(!CalcVertNormals(pWorld, normals))
			return FALSE;

		pWorld->SetPolyWorldIndices();
	}

	return TRUE;
}


// Each point's normal: the average of the normals of the polies on it.
static BOOL CalcVertNormals(CPreWorld *pWorld, CMoArray<PVector> &normals)
{
	CMoArray< CMoArray<CPrePoly*> > pointPolies;
	GPOS pos;
	CPrePoly *pPoly;
	uint32 i, j;

	if(!normals.SetSize(pWorld->m_Points.GetSize()))
		return FALSE;

	if(!pointPolies.SetSize(pWorld->m_Points.GetSize()))
		return FALSE;

	for(pos=pWorld->m_Polies; pos; )
	{
		pPoly = pWorld->m_Polies.GetNext(pos);

		for(i=0; i < pPoly->NumAllVerts(); i++)
		{
			if(!pointPolies[pPoly->AllVert(i).m_Index].Append(pPoly))
				return FALSE;
		}
	}

	for(i=0; i < pointPolies; i++)
	{
		normals[i].Init();
		for(j=0; j < pointPolies[i]; j++)
			normals[i] += pointPolies[i][j]->Normal();

		normals[i].Norm();
	}

	return TRUE;
}


BOOL CLightMapMaker::LightPolygon(CPrePoly *pPoly)
{
	uint32 i, flags;

	flags = pPoly->GetSurfaceFlags();

	if(flags & SURF_BRIGHT)
	{
		for(i=0; i < pPoly->NumAllVerts(); i++)
			pPoly->AllVert(i).m_Color.Init(255.0f, 255.0f, 255.0f);

		return TRUE;
	}

	if(m_bStaticSunLight && (flags & SURF_DIRECTIONALLIGHT))
	{
		// Only the sun (and the ambient light).
		for(i=0; i < pPoly->NumAllVerts(); i++)
		{
			pPoly->AllVert(i).m_Color = m_AmbientLight;
			pPoly->AllVert(i).m_Color += CalcSunLightSample(m_VertNormals[pPoly->m_Index][pPoly->AllVert(i).m_Index]);
			VEC_CLAMP(pPoly->AllVert(i).m_Color, 0.0f, 255.0f);
		}

		return TRUE;
	}

	if(flags & (SURF_GOURAUDSHADE|SURF_LIGHTMAP|SURF_DIRECTIONALLIGHT))
	{
		for(i=0; i < pPoly->NumAllVerts(); i++)
			LightVertex(&pPoly->AllVert(i), TRUE);

		if(m_bStaticSunLight)
		{
			for(i=0; i < pPoly->NumAllVerts(); i++)
			{
				CPolyVert &vert = pPoly->AllVert(i);

				if(DoesSunLightHit(vert.m_Vec))
				{
					vert.m_Color += CalcSunLightSample(vert.m_Normal);
					VEC_CLAMP(vert.m_Color, 0.0f, 255.0f);
				}
			}
		}
	}
	else if(flags & SURF_FLATSHADE)
	{
		CPolyVert center;

		center.m_Vec = pPoly->GetCenter();
		center.m_Normal = pPoly->Normal();
		LightVertex(&center, FALSE);

		if(m_bStaticSunLight && DoesSunLightHit(center.m_Vec))
		{
			center.m_Color += CalcSunLightSample(center.m_Normal);

			if(center.m_Color.x < 0.0f)
				center.m_Color.x = 0.0f;
			else if(center.m_Color.x > 255.0f)
				center.m_Color.x = 255.0f;

			if(center.m_Color.y < 0.0f)
				center.m_Color.y = 0.0f;
			else if(center.m_Color.y > 255.0f)
				center.m_Color.y = 255.0f;

			if(center.m_Color.z < 0.0f)
				center.m_Color.z = 0.0f;
			else if(center.m_Color.z > 255.0f)
				center.m_Color.z = 255.0f;
		}

		for(i=0; i < pPoly->NumAllVerts(); i++)
			pPoly->AllVert(i).m_Color = center.m_Color;
	}

	return TRUE;
}


BOOL CLightMapMaker::GetPolyVertexColors(CPrePoly *pPoly, CMoArray<uint32> &colors)
{
	uint32 i;

	if(!colors.SetSize(pPoly->NumAllVerts()))
		return FALSE;

	for(i=0; i < pPoly->NumAllVerts(); i++)
	{
		CPolyVert &vert = pPoly->AllVert(i);

		colors[i] = (((uint32)vert.m_Color.x & 0xFF) << 16) + (((uint32)vert.m_Color.y & 0xFF) << 8) + ((uint32)vert.m_Color.z & 0xFF);
	}

	return TRUE;
}


void CLightMapMaker::AddLightDef(TLightDef *pLightDef, PVector pos, CPrePoly *pPoly)
{
	CLightDef *pLight;

	pLight = new CLightDef;
	m_LightDefs.Append(pLight);

	pLight->m_Pos = pos;
	pLight->m_Direction = pPoly->Normal();
	pLight->m_FOV = MATH_HALFPI;
	pLight->m_InnerColor = pLightDef->m_InnerColor;
	pLight->m_OuterColor = pLightDef->m_OuterColor;
	pLight->m_MaxDist = pLightDef->m_Radius;
	pLight->m_MaxDistSqr = pLight->m_MaxDist * pLight->m_MaxDist;
	pLight->m_bClip = TRUE;
	pLight->m_bDirectional = pLightDef->m_bDirectional;
}


// Makes a light for each texture light of each poly's texture: at the
// poly's center, or tiled across it as the texture is.
void CLightMapMaker::GetTextureLights()
{
	CGLinkedList<TInfo*> texInfo;
	CPrePoly *pPoly;
	CPreSurface *pSurface;
	TInfo *pInfo;
	TLightDef *pDef;
	PVector vRowStart, vPos, vTest;
	PReal fDot, fU, fV, t;
	uint32 i, j, iDef, iU, iV, nLights;
	BOOL bHit, bAnyHit;
	GPOS pos;


	if(!GetTextureInfo(m_pWorld, texInfo))
	{
		DrawStatusTextIfMainWorld("** Unable to get texture lights.");
		DrawStatusTextIfMainWorld("** (Is project path set?)");
		return;
	}

	nLights = 0;
	for(i=0; i < m_Polies; i++)
	{
		pPoly = m_Polies[i];

		if(pPoly->GetSurface()->m_pTextureName &&
			(pInfo = FindTInfo(texInfo, pPoly->GetSurface()->m_pTextureName)))
		{
			PVector vTexP = pPoly->GetSurface()->m_TextureP / pPoly->GetSurface()->m_TextureP.Mag();
			PVector vTexQ = pPoly->GetSurface()->m_TextureQ / pPoly->GetSurface()->m_TextureQ.Mag();

			// The poly's corner furthest back along the texture vectors.
			PVector vMin = pPoly->Pt(0);
			for(j=1; j < pPoly->NumVerts(); j++)
			{
				fDot = vTexP.Dot(pPoly->Pt(j) - vMin);
				if(fDot < 0.0f)
					vMin += vTexP * fDot;

				fDot = vTexQ.Dot(pPoly->Pt(j) - vMin);
				if(fDot < 0.0f)
					vMin += vTexQ * fDot;
			}

			pSurface = pPoly->GetSurface();
			PVector vNormal = pSurface->m_TextureP.Cross(pSurface->m_TextureQ);
			vNormal.Norm();

			fDot = vNormal.Mag();
			if(fDot >= 0.1f && fDot <= 1.1f)
			{
				for(iDef=0; iDef < pInfo->m_LightDefs; iDef++)
				{
					pDef = pInfo->m_LightDefs[iDef];

					if(pDef->m_bCentered)
					{
						AddLightDef(pDef, pPoly->GetCenter(), pPoly);
						nLights++;
					}
					else
					{
						// Tile it across the poly as the texture is tiled.
						pSurface = pPoly->GetSurface();
						fU = pSurface->m_TextureP.Dot(vMin - pSurface->m_TextureO);
						fV = pSurface->m_TextureQ.Dot(vMin - pSurface->m_TextureO);

						fU = FloorToMultiple(fU, (PReal)pInfo->m_Width) + pDef->m_UOffset;
						fV = FloorToMultiple(fV, (PReal)pInfo->m_Height) + pDef->m_VOffset;

						vRowStart = pSurface->InverseP * fU + pSurface->m_TextureO + pSurface->InverseQ * fV;
						PVector vStepU = pSurface->InverseP * (PReal)pInfo->m_Width;
						PVector vStepV = pSurface->InverseQ * (PReal)pInfo->m_Height;

						bAnyHit = FALSE;
						for(iV=0; iV < TEXTURE_LIGHT_ITERATIONS; iV++)
						{
							vPos = vRowStart;
							bHit = FALSE;
							for(iU=0; iU < TEXTURE_LIGHT_ITERATIONS; iU++)
							{
								// Onto the poly's plane, along the texture's normal.
								CPrePlane *pPlane = pPoly->GetPlane();
								t = (pPlane->m_Dist - VEC_DOT(vPos, pPlane->m_Normal)) / VEC_DOT(vNormal, pPlane->m_Normal);
								vTest = vPos + vNormal * t;

								if(IsPtOverPoly(pPoly, vTest))
								{
									AddLightDef(pDef, vTest + pPoly->Normal() * pDef->m_ZOffset, pPoly);
									bHit = TRUE;
									nLights++;
								}
								else if(bHit)
								{
									break;
								}

								vPos += vStepU;
							}

							bAnyHit |= bHit;
							if(bAnyHit && !bHit)
								break;

							vRowStart += vStepV;
						}
					}
				}
			}
		}
	}

	DrawStatusTextIfMainWorld("Created %d texture lights.", nLights);

	for(pos=texInfo; pos; )
		delete texInfo.GetNext(pos);
}




void CLightMapMaker::GetLightDefs()
{
	uint32			i;
	CPropList		*pPropList;
	CLightDef		*pLight;
	CColorProp		*pColor;
	CVectorProp		*pVectorProp;
	CBoolProp		*pBoolProp;
	CBaseEditObj	*pObject;
	CRealProp		*pRealProp;
	CStringProp		*pStringProp;
	CRotationProp	*pRot;
	LTVector		vUp, vRight, vForward;
	LTMatrix		mTemp;


	ClearLightDefs();
	GetTextureLights();

	for(i=0; i < m_pWorld->m_Objects.GetSize(); i++)
	{
		pObject = m_pWorld->m_Objects[i];
		pPropList = &pObject->m_PropList;

		if((strcmp(pObject->GetClassName(), "Light") == 0) ||
			(strcmp(pObject->GetClassName(), "DirLight") == 0))
		{
			pLight = new CLightDef;

			pLight->m_pObject = pObject;

			//see if it is a directional light
			pLight->m_bDirectional = (strcmp(pObject->GetClassName(), "DirLight") == 0);

			// LightColor maps to InnerColor.
			if(pColor = (CColorProp*)pPropList->GetProp("LightColor"))
			{
				pLight->m_InnerColor = pColor->m_Vector.FVec();
			}
			if(pColor = (CColorProp*)pPropList->GetProp("InnerColor"))
			{
				pLight->m_InnerColor = pColor->m_Vector.FVec();
			}
			if(pColor = (CColorProp*)pPropList->GetProp("OuterColor"))
			{
				pLight->m_OuterColor = pColor->m_Vector.FVec();
			}

			if(pVectorProp = (CVectorProp*)pPropList->GetProp("Pos"))
			{
				pLight->m_Pos = pVectorProp->m_Vector.FVec();
			}

			if((pRot = (CRotationProp*)pPropList->GetProp("Rotation")) && pRot->m_Type == PT_ROTATION)
			{
				gr_GetEulerVectors(
					pRot->GetEulerAngles(),
					vRight,
					vUp,
					vForward);
				VEC_COPY(pLight->m_Direction, vForward);
			}

			if((pBoolProp = (CBoolProp*)pPropList->GetProp("ClipLight")) && pBoolProp->m_Type == PT_BOOL)
			{
				pLight->m_bClip = pBoolProp->m_Value;
			}

			if((pRealProp = (CRealProp*)pPropList->GetProp("FOV")) && pRealProp->m_Type == PT_REAL)
			{
				pLight->m_FOV = (pRealProp->m_Value / 360.0f) * MATH_CIRCLE;
			}

			if((pRealProp = (CRealProp*)pPropList->GetProp("LightRadius")) && pRealProp->m_Type == PT_REAL)
			{
				pLight->m_MaxDist = pRealProp->m_Value;
			}

			if((pRealProp = (CRealProp*)pPropList->GetProp("BrightScale")) && pRealProp->m_Type == PT_REAL)
			{
				pLight->m_BrightScale = pRealProp->m_Value;
			}

			pLight->m_MaxDistSqr = pLight->m_MaxDist * pLight->m_MaxDist;

			// A light named in the lights by name goes there instead.
			pStringProp = (CStringProp*)pPropList->GetProp("Name");
			TLightNameMap::iterator iter = g_LightsByName.find(std::string(pStringProp->m_String));
			if(iter == g_LightsByName.end())
				m_LightDefs.Append(pLight);
			else
				(*iter).second = pLight;
		}
		else if(strcmp(pObject->GetClassName(), "StaticSunLight") == 0)
		{
			m_bStaticSunLight = TRUE;

			if((pColor = (CColorProp*)pPropList->GetProp("InnerColor")) && pColor->m_Type == PT_COLOR)
			{
				m_SunLightInner = pColor->m_Vector;
			}

			if((pColor = (CColorProp*)pPropList->GetProp("OuterColor")) && pColor->m_Type == PT_COLOR)
			{
				m_SunLightOuter = pColor->m_Vector;
			}

			if((pRot = (CRotationProp*)pPropList->GetProp("Rotation")) && pRot->m_Type == PT_ROTATION)
			{
				gr_SetupMatrixEuler(pRot->GetEulerAngles(), mTemp.m);
				mTemp.GetBasisVectors(&vRight, &vUp, &m_SunLightDir);
			}

			if((pRealProp = (CRealProp*)pPropList->GetProp("BrightScale")) && pRealProp->m_Type == PT_REAL)
			{
				m_SunLightBrightScale = pRealProp->m_Value;
			}

			if((pRealProp = (CRealProp*)pPropList->GetProp("Bias")) && pRealProp->m_Type == PT_REAL)
			{
				m_SunLightBias = pRealProp->m_Value;
			}
		}
	}
}


BOOL CLightMapMaker::SetupFrameLights(PreLightAnimFrameInfo *pFrame)
{
	CLightDef *pDef, *pSrc;
	PreLightInfo *pInfo;
	uint32 i;


	ClearLightDefs();

	m_bStaticSunLight = pFrame->m_bSunLight;
	m_SunLightDir = pFrame->m_vSunLightDir;
	m_SunLightInner = pFrame->m_vSunLightInnerColor;
	m_SunLightOuter = pFrame->m_vSunLightOuterColor;
	m_AmbientLight = pFrame->m_vAmbientColor;

	// Copies of the global lights it names.
	for(std::vector<std::string>::iterator iName = pFrame->m_LightList.begin();
		iName != pFrame->m_LightList.end(); iName++)
	{
		TLightNameMap::iterator iter = g_LightsByName.find(*iName);
		if(iter != g_LightsByName.end())
		{
			pSrc = (*iter).second;
			CLightDef *pCopy = new CLightDef(*pSrc);
			m_LightDefs.Append(pCopy);
		}
	}

	for(i=0; i < pFrame->m_nLights; i++)
	{
		pInfo = &pFrame->m_Lights[i];

		pDef = new CLightDef;
		if(!pDef)
			return FALSE;

		if(!m_LightDefs.Append(pDef))
		{
			delete pDef;
			return FALSE;
		}

		pDef->m_Pos = pInfo->m_vPos;
		pDef->m_MaxDist = pInfo->m_Radius;
		pDef->m_MaxDistSqr = pDef->m_MaxDist * pDef->m_MaxDist;
		pDef->m_bDirectional = pInfo->m_bDirectional;
		pDef->m_InnerColor = pInfo->m_vInnerColor;
		pDef->m_OuterColor = pInfo->m_vOuterColor;
		pDef->m_Direction = pInfo->m_vDirection;
		pDef->m_FOV = pInfo->m_FOV;
		pDef->m_BrightScale = pInfo->m_BrightScale;
	}

	return TRUE;
}


BOOL CLightMapMaker::ShouldBlockLight(CPreWorld *pWorld)
{
	CBaseEditObj *pObj;
	CBoolProp *pProp;
	uint32 i;

	if(pWorld->m_WorldInfoFlags & WIF_PHYSICSBSP)
		return TRUE;

	if(!(pWorld->m_WorldInfoFlags & WIF_VISBSP))
	{
		for(i=0; i < m_pWorld->m_Objects; i++)
		{
			pObj = m_pWorld->m_Objects[i];

			if(stricmp(pObj->GetName(), pWorld->m_WorldName) == 0)
			{
				pProp = (CBoolProp*)pObj->m_PropList.GetMatchingProp("BlockLight", PT_BOOL);
				if(pProp && pProp->m_Value)
					return TRUE;
			}
		}
	}

	return FALSE;
}


static PreWorldModelInfo* FindWorldModelInfo(const char *pName, PreWorldModelInfo *pInfos, uint32 nInfos)
{
	uint32 i;

	for(i=0; i < nInfos; i++, pInfos++)
	{
		if(stricmp(pInfos->m_WorldModelName, pName) == 0)
			return pInfos;
	}

	return NULL;
}


BOOL CLightMapMaker::SetupWorldModels(PreWorldModelInfo *pInfos, uint32 nInfos)
{
	LMMWorld *pWorld;
	PreWorldModelInfo *pInfo;
	PVector pts[8];
	BOOL bInvisible;
	uint32 i, iPt;


	for(i=0; i < m_LMMWorlds; i++)
	{
		pWorld = &m_LMMWorlds[i];

		pInfo = FindWorldModelInfo(pWorld->m_pWorld->m_WorldName, pInfos, nInfos);
		if(pInfo)
		{
			pWorld->m_bUsingTransform = TRUE;
			gr_SetupWMTransform(&pWorld->m_pWorld->m_WorldTranslation, &pInfo->m_Pos, &pInfo->m_Rot,
				&pWorld->m_mTransform, &pWorld->m_mInverseTransform);

			// Where the world's box ends up.
			PVector &vMin = m_pWorld->m_PosMin;
			PVector &vMax = m_pWorld->m_PosMax;
			pts[0].Init(vMin.x, vMin.y, vMin.z);
			pts[1].Init(vMin.x, vMax.y, vMin.z);
			pts[2].Init(vMax.x, vMin.y, vMin.z);
			pts[3].Init(vMax.x, vMax.y, vMin.z);
			pts[4].Init(vMin.x, vMin.y, vMax.z);
			pts[5].Init(vMin.x, vMax.y, vMax.z);
			pts[6].Init(vMax.x, vMin.y, vMax.z);
			pts[7].Init(vMax.x, vMax.y, vMax.z);

			pWorld->m_vMax = pts[0];
			pWorld->m_vMin = pts[0];
			for(iPt=0; iPt < 8; iPt++)
			{
				pts[iPt] = pWorld->m_mTransform * pts[iPt];
				VEC_MIN(pWorld->m_vMin, pWorld->m_vMin, pts[iPt]);
				VEC_MAX(pWorld->m_vMax, pWorld->m_vMax, pts[iPt]);
			}

			bInvisible = pInfo->m_bInvisible;
		}
		else
		{
			pWorld->m_bUsingTransform = FALSE;
			pWorld->m_mTransform.Identity();
			bInvisible = FALSE;
			pWorld->m_vMin = pWorld->m_pWorld->m_PosMin;
			pWorld->m_vMax = pWorld->m_pWorld->m_PosMax;
		}

		pWorld->m_mInverseTransform = pWorld->m_mTransform.MakeInverse();

		if(!ShouldBlockLight(pWorld->m_pWorld) || bInvisible)
			pWorld->RemoveFromWorldTree();
		else
			m_WorldTree.InsertObject(pWorld);
	}

	return TRUE;
}


void CLightMapMaker::ClearLightDefs()
{
	VEC_INIT(m_SunLightInner);
	VEC_INIT(m_SunLightOuter);
	m_bStaticSunLight = FALSE;
	DeleteAndClearArray(m_LightDefs);
}


BOOL CLightMapMaker::LightTouchesPoly(CLightDef *pLight, CPrePoly *pPoly)
{
	CFullLightMap *pMap;
	PVector vPos;
	uint32 i;

	// The light in the world model's space.
	vPos = pPoly->m_pWorld->m_mInverseTransform * pLight->m_Pos;

	// Behind the poly?
	if(m_bClipLight && pLight->m_bClip && (pPoly->GetPlane()->m_Normal.Dot(vPos) - pPoly->GetPlane()->m_Dist) < 0.0f)
		return FALSE;

	pMap = m_FullLightMaps[pPoly->m_nName];
	for(i=0; i < 3; i++)
	{
		if(vPos[i] - pLight->m_MaxDist > pMap->m_Max[i])
			return FALSE;

		if(vPos[i] + pLight->m_MaxDist < pMap->m_Min[i])
			return FALSE;
	}

	return TRUE;
}




static PReal CalcLightDot(PVector *pToLight, PReal distSqr, PVector *pNormal)
{
	PReal dot;

	dot = pToLight->Dot(*pNormal) / (PReal)sqrt(distSqr);
	if(dot <= 0.0f)
		dot = 0.0f;

	return dot;
}


void CLightMapMaker::LightVertex(CPolyVert *pVert, BOOL bClip)
{
	LMMSegQuery query;
	CLightDef *pLight;
	PReal distSqr, dot;
	uint32 i;

	VEC_SET(pVert->m_Color, m_AmbientLight.x, m_AmbientLight.y, m_AmbientLight.z);

	for(i=0; i < m_LightDefs; i++)
	{
		pLight = m_LightDefs[i];

		PVector vToLight = pLight->m_Pos - pVert->m_Vec;
		distSqr = vToLight.MagSqr();
		if(distSqr < pLight->m_MaxDistSqr)
		{
			dot = CalcLightDot(&vToLight, distSqr, &pVert->m_Normal);
			if(dot > 0.0f)
			{
				// Anything in the way?
				if(m_bClipLight && bClip && pLight->m_bClip)
				{
					query.m_pMaker = this;
					query.m_bBlocked = FALSE;
					query.m_vStart = pVert->m_Vec;
					query.m_vEnd = pLight->m_Pos;
					query.m_vStart += vToLight * (1.0f / (PReal)sqrt(distSqr));
					m_WorldTree.IntersectSegment(&query.m_vStart, &query.m_vEnd, LightWorldModelCB, &query);

					if(query.m_bBlocked)
						continue;
				}

				pVert->m_Color += CalcLightSample(pLight, pLight->m_Pos, pLight->m_Direction,
					pVert->m_Vec, distSqr, dot);
			}
		}
	}

	pVert->m_Color.x = LTCLAMP(pVert->m_Color.x, 0.0f, 255.0f);
	pVert->m_Color.y = LTCLAMP(pVert->m_Color.y, 0.0f, 255.0f);
	pVert->m_Color.z = LTCLAMP(pVert->m_Color.z, 0.0f, 255.0f);
}


void CLightMapMaker::ClampMapValues(CFullLightMap *pMap, uint32 nSamplesU, uint32 nSamplesV)
{
	uint32 u, v;
	PVector *pColor;

	for(u=0; u < nSamplesU; u++)
	{
		for(v=0; v < nSamplesV; v++)
		{
			pColor = &pMap->m_Map[pMap->m_Width*v + u];

			if(pColor->x > 255.0f)
				pColor->x = 255.0f;

			if(pColor->y > 255.0f)
				pColor->y = 255.0f;

			if(pColor->z > 255.0f)
				pColor->z = 255.0f;
		}
	}
}


// Averages each 2x2 of samples (0x41bed0).
void CLightMapMaker::DownSample(CFullLightMap *pIn, CFullLightMap *pOut)
{
	uint32 x, y, width, height;
	PVector *pDest, *pRow, *pNextRow;

	if(!pIn->m_Width || !pIn->m_Height)
		return;

	width = pIn->m_Width - 1;
	height = pIn->m_Height - 1;
	if(width * height > pOut->m_Map.GetSize())
		pOut->m_Map.SetSize(width * height);

	pOut->m_Width = width;
	pOut->m_Height = height;

	pDest = pOut->m_Map.GetArray();
	for(y=0; y < pOut->m_Height; y++)
	{
		pRow = &pIn->m_Map[pIn->m_Width * y];
		pNextRow = &pIn->m_Map[(y+1) * pIn->m_Width];

		for(x=0; x < pOut->m_Width; x++)
		{
			*pDest = (pNextRow[0] + pRow[0] + pRow[1] + pNextRow[1]) * 0.25f;
			pDest++;
			pRow++;
			pNextRow++;
		}
	}
}


BOOL CLightMapMaker::ConvertMap(CFullLightMap *pIn, CPreLightMap *pOut)
{
	uint32 data[LIGHTMAP_MAX_TOTAL_PIXELS];
	uint32 x, y, width, height, *pData;
	PVector *pColor;

	width = pIn->m_Width;
	if(width <= LIGHTMAP_MAX_PIXELS)
	{
		height = pIn->m_Height;
		if(height <= LIGHTMAP_MAX_PIXELS)
		{
			pColor = pIn->m_Map.GetArray();
			pData = data;
			for(y=0; y < height; y++)
			{
				for(x=0; x < width; x++)
				{
					*pData = (((uint32)pColor->x << 8 | (uint32)pColor->y) << 8) | (uint32)pColor->z;
					pColor++;
					pData++;
				}
			}

			if(m_pCurAnim->m_bShadowMap)
				return pOut->CompressShadowMap(width, height, data);
			else
				return pOut->CompressLightMap(width, height, data);
		}
	}

	return FALSE;
}


// Looks along a row of the sample grid, from xStart up to xEnd, for a
// valid sample (0x41cd10).
static BOOL FindRowSample(CMoArray<uint32> *pValid, int xStart, int xEnd, int y,
	int width, int height, int *pX, int *pY)
{
	if(y >= 0 && y < height)
	{
		*pY = y;
		for(*pX=xStart; *pX < xEnd; (*pX)++)
		{
			if(*pX >= 0 && *pX < width && (*pValid)[y*width + *pX])
				return TRUE;
		}
	}

	return FALSE;
}


// The same down a column (0x41cd80).
static BOOL FindColumnSample(CMoArray<uint32> *pValid, int yStart, int yEnd, int x,
	int width, int height, int *pX, int *pY)
{
	if(x >= 0 && x < width)
	{
		*pX = x;
		for(*pY=yStart; *pY < yEnd; (*pY)++)
		{
			if(*pY >= 0 && *pY < height && (*pValid)[*pY*width + x])
				return TRUE;
		}
	}

	return FALSE;
}


// What PointInsideCB gets (the names are ours).
struct LMMPointQuery
{
	CLightMapMaker	*m_pMaker;
	PVector			m_vPos;
	uint32			*m_pbValid;
	uint32			m_nWorlds;
};


static BOOL IsPointInBSP(NODEREF pNode, PVector &vPos, PReal fEpsilon);


// A world model the point is in: it stays valid only if it is in the clear
// in each one (0x41cdf0).
static void PointInsideCB(WorldTreeObj *pObj, void *pUser)
{
	LMMPointQuery *pQuery = (LMMPointQuery*)pUser;

	pQuery->m_nWorlds++;
	if(*pQuery->m_pbValid)
		*pQuery->m_pbValid = IsPointInBSP(((LMMWorld*)pObj)->m_pWorld->m_RootNode, pQuery->m_vPos, 0.1f);
}


// TRUE if the point is in empty space (0x41ce30).
static BOOL IsPointInBSP(NODEREF pNode, PVector &vPos, PReal fEpsilon)
{
	PReal dist;

	if(pNode == NODE_IN)
		return TRUE;

	if(pNode == NODE_OUT)
		return FALSE;

	dist = VEC_DOT(pNode->m_pPlane->m_Normal, vPos) - pNode->m_pPlane->m_Dist;
	if(dist > fEpsilon)
	{
		return IsPointInBSP(pNode->m_Sides[FrontSide], vPos, fEpsilon);
	}
	else if(dist >= -fEpsilon)
	{
		if(IsPointInBSP(pNode->m_Sides[FrontSide], vPos, fEpsilon))
			return TRUE;

		return IsPointInBSP(pNode->m_Sides[BackSide], vPos, fEpsilon);
	}
	else
	{
		return IsPointInBSP(pNode->m_Sides[BackSide], vPos, fEpsilon);
	}
}


// The positions of the poly's lightmap samples. A sample in a wall moves to
// the closest of nSub*nSub supersamples that is in the clear (0x41c160).
BOOL CLightMapMaker::CalcSamplePositions(CPrePoly *pPoly, uint32 nSub, CMoArray<PVector> *pPositions)
{
	CMoArray<uint32> valid;
	CFullLightMap *pMap;
	CPreSurface *pSurface;
	CPrePlane *pPlane;
	LMMPointQuery query;
	PVector dU, dV, vGridStart, vRowPos, vPos, vPt, *pOut;
	uint32 nSamplesU, nSamplesV, u, v, iRow;
	int gx, gy, r, x, y;
	PReal fSub;


	pMap = m_FullLightMaps[pPoly->m_nName];
	nSamplesU = pMap->m_Width * nSub + 64;
	nSamplesV = pMap->m_Height * nSub + 64;
	if(valid.SetSize(nSamplesU * nSamplesV))
	{

	// The supersample grid, with a border of 32 supersamples.
	pSurface = pPoly->GetSurface();
	fSub = (PReal)nSub;
	dU = pSurface->InverseP * (m_pWorld->m_LMGridSize / fSub);
	dV = pSurface->InverseQ * (m_pWorld->m_LMGridSize / fSub);
	vGridStart = pPoly->PolyO() + pSurface->InverseP * m_ShiftU + pSurface->InverseQ * m_ShiftV - dU * 32.0f - dV * 32.0f;

	if(pPoly->m_pWorld->m_bUsingTransform)
	{
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &dU);
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &dV);
		vGridStart = pPoly->m_pWorld->m_mTransform * vGridStart;
	}

	vRowPos = vGridStart;
	iRow = 0;
	for(v=0; v < nSamplesV; v++)
	{
		vPos = vRowPos;
		for(u=0; u < nSamplesU; u++)
		{
			if(m_bClipLight)
			{
				// Which world models' solid space is it in?
				pPlane = pPoly->GetPlane();
				vPt = vPos;
				vPt -= pPlane->m_Normal * (pPlane->m_Normal.Dot(vPos) - pPlane->m_Dist);

				query.m_pMaker = this;
				query.m_nWorlds = 0;
				query.m_pbValid = &valid[iRow + u];
				*query.m_pbValid = TRUE;
				query.m_vPos = vPt;
				m_WorldTree.FindObjectsOnPoint(&vPt, PointInsideCB, &query);

				if(query.m_nWorlds == 0)
					*query.m_pbValid = FALSE;
			}
			else
			{
				valid[iRow + u] = TRUE;
			}

			vPos += dU;
		}

		vRowPos += dV;
		iRow += nSamplesU;
	}

	if(pPositions->SetSize(pMap->m_Width * pMap->m_Height))
	{

	// The samples.
	PVector dU2 = pSurface->InverseP * m_pWorld->m_LMGridSize;
	PVector dV2 = pSurface->InverseQ * m_pWorld->m_LMGridSize;
	vRowPos = pPoly->PolyO() + pSurface->InverseP * m_ShiftU + pSurface->InverseQ * m_ShiftV;

	if(pPoly->m_pWorld->m_bUsingTransform)
	{
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &dU2);
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &dV2);
		vRowPos = pPoly->m_pWorld->m_mTransform * vRowPos;
	}

	for(v=0; v < pMap->m_Height; v++)
	{
		vPos = vRowPos;
		for(u=0; u < pMap->m_Width; u++)
		{
			pOut = &(*pPositions)[v*pMap->m_Width + u];
			*pOut = vPos;

			if(m_bClipLight)
			{
				gx = 32 + u*nSub;
				gy = 32 + v*nSub;
				if(!valid[gy*nSamplesU + gx])
				{
					// The closest valid supersample, ring by ring.
					for(r=1; r < 32; r++)
					{
						if(FindRowSample(&valid, gx-r, gx+r, gy-r, nSamplesU, nSamplesV, &x, &y) ||
							FindRowSample(&valid, gx-r, gx+r, gy+r, nSamplesU, nSamplesV, &x, &y) ||
							FindColumnSample(&valid, gy-r, gy+r, gx-r, nSamplesU, nSamplesV, &x, &y) ||
							FindColumnSample(&valid, gy-r, gy+r, gx+r, nSamplesU, nSamplesV, &x, &y))
						{
							*pOut = vGridStart + dU * (PReal)x + dV * (PReal)y;
							break;
						}
					}
				}

				// Onto the poly's plane.
				pPlane = pPoly->GetPlane();
				*pOut -= pPlane->m_Normal * (pPlane->m_Normal.Dot(*pOut) - pPlane->m_Dist);
			}

			vPos += dU2;
		}

		vRowPos += dV2;
	}

	return TRUE;
	}
	}

	return FALSE;
}


BOOL CLightMapMaker::LoadClassModule()
{
	char fileName[512];
	int version;

	UnloadClassModule();

	sprintf(fileName, "%s\\object.lto", g_pGlobs->m_ProjectDir);
	if(cb_LoadModule(fileName, NULL, &m_hClassModule, &version) == CB_NOERROR)
		return TRUE;

	m_hClassModule = NULL;
	return FALSE;
}


static void ProcessLightGroup(CWorldNode *pGroup, CPreMainWorld *pWorld);
static uint32 FindObjectByName(const char *pName, CPreMainWorld *pWorld);


void CLightMapMaker::UnloadClassModule()
{
	if(m_hClassModule)
	{
		cb_UnloadModule(m_hClassModule);
		m_hClassModule = NULL;
	}
}


BOOL CLightMapMaker::RemoveExcessPolies()
{
	uint32 iFrame, iPoly, i;
	BOOL bAllBlack, bNoValues;
	CPreLightFrame *pFrame;
	CMoArray<uint32> *pValues;

	if(!m_pCurAnim)
		return FALSE;

	for(iPoly=0; iPoly < m_pCurAnim->m_Polies; iPoly++)
	{
		// Are all the frames black?
		bAllBlack = TRUE;
		for(iFrame=0; iFrame < m_pCurAnim->m_Frames; iFrame++)
		{
			pFrame = m_pCurAnim->m_Frames[iFrame];

			if(pFrame->m_PolyMaps.GetSize() != m_pCurAnim->m_Polies.GetSize())
				return FALSE;

			if(pFrame->m_PolyMaps[iPoly] && !pFrame->m_PolyMaps[iPoly]->AllBlack())
			{
				bAllBlack = FALSE;
				break;
			}
		}

		bNoValues = TRUE;
		for(iFrame=0; iFrame < m_pCurAnim->m_Frames; iFrame++)
		{
			pFrame = m_pCurAnim->m_Frames[iFrame];

			if(pFrame->m_Unknown_14.GetSize() != m_pCurAnim->m_Polies.GetSize())
				return FALSE;

			pValues = pFrame->m_Unknown_14[iPoly];
			if(pValues)
			{
				for(i=0; i < pValues->GetSize(); i++)
				{
					if((*pValues)[i])
					{
						bNoValues = FALSE;
						goto Done;
					}
				}
			}
		}

	Done:;
		if(bAllBlack)
		{
			for(iFrame=0; iFrame < m_pCurAnim->m_Frames; iFrame++)
			{
				pFrame = m_pCurAnim->m_Frames[iFrame];

				delete pFrame->m_PolyMaps[iPoly];
				pFrame->m_PolyMaps[iPoly] = NULL;

				if(bNoValues)
				{
					delete pFrame->m_Unknown_14[iPoly];
					pFrame->m_Unknown_14[iPoly] = NULL;
				}

				if(bNoValues)
				{
					pFrame->m_PolyMaps.Remove(iPoly);
					pFrame->m_Unknown_14.Remove(iPoly);
				}
			}

			if(bNoValues)
			{
				m_pCurAnim->m_Polies.Remove(iPoly);
				--iPoly;
			}
		}
	}

	return TRUE;
}


uint32 CLightMapMaker::IncFrameCode()
{
	++m_FrameCode;
	return m_FrameCode;
}


uint32 CLightMapMaker::GetFrameCode()
{
	return m_FrameCode;
}


LTRESULT CLightMapMaker::CreateLightAnim(const char *pName, PreLightAnimFrameInfo *pFrames,
	uint32 nFrames, LTBOOL bShadowMap)
{
	uint32 i;

	m_pCurAnim = NULL;

	if(!m_pWorld)
		return LT_NOTINITIALIZED;

	if(nFrames == 0)
		return LT_INVALIDPARAMS;

	if(m_pWorld->FindLightAnim(pName))
		return LT_ALREADYEXISTS;

	if(SetupNewLightAnim(pName))
	{
		DrawStatusText("LAnim - %s (%d frames)", pName, nFrames);
		m_pCurAnim->m_bShadowMap = bShadowMap;

		for(i=0; i < nFrames; i++)
		{
			if(!AddAnimFrame())
				goto Error;

			if(!SetupFrameLights(&pFrames[i]))
				goto Error;

			SetupWorldModels(pFrames[i].m_WorldModels, pFrames[i].m_nWorldModels);

			if(!CalculateLightFrame())
				goto Error;
		}

		if(RemoveExcessPolies())
		{
			if(m_pCurAnim->m_Polies.GetSize() && m_pCurAnim->m_Frames.GetSize())
			{
				DrawStatusText("      - %d bytes", m_pCurAnim->CalcFrameDataSize());
				m_pCurAnim = NULL;
				m_pCurFrame = NULL;
				return LT_OK;
			}

			DrawStatusText("      - discarding (nothing lit)");
		}

	Error:;
		if(m_pCurAnim)
		{
			m_pWorld->RemoveLightAnim(m_pCurAnim);
			m_pCurAnim = NULL;
		}

		m_pCurFrame = NULL;
	}

	return LT_ERROR;
}


LTRESULT CLightMapMaker::FindObject(const char *pName, HPREOBJECT &hObject)
{
	uint32 i;

	if(!m_pWorld)
		return LT_ERROR;

	for(i=0; i < m_pWorld->m_Objects; i++)
	{
		if(stricmp(m_pWorld->m_Objects[i]->GetName(), pName) == 0)
		{
			hObject = i;
			return LT_OK;
		}
	}

	return LT_NOTFOUND;
}


LTRESULT CLightMapMaker::GetClassName(HPREOBJECT hObject, char *pClassName, uint32 maxCount)
{
	if(maxCount == 0)
		return LT_ERROR;

	pClassName[0] = 0;
	if(!m_pWorld || hObject >= m_pWorld->m_Objects.GetSize())
		return LT_ERROR;

	strncpy(pClassName, m_pWorld->m_Objects[hObject]->GetClassName(), maxCount);
	pClassName[maxCount-1] = 0;
	return LT_OK;
}


LTRESULT CLightMapMaker::GetPropGeneric(HPREOBJECT hObject, const char *pName, GenericProp *pProp)
{
	if(pName && pProp)
	{
		gp_Init(pProp);
		if(m_pWorld && hObject < m_pWorld->m_Objects.GetSize())
		{
			CBaseProp *pBaseProp = m_pWorld->m_Objects[hObject]->m_PropList.GetProp(pName);
			if(pBaseProp)
			{
				pBaseProp->SetupGenericProp(pProp);
				return LT_OK;
			}

			return LT_NOTFOUND;
		}
	}

	return LT_ERROR;
}


LTRESULT CLightMapMaker::Parse(ConParse *pParse)
{
	return pParse->Parse() ? LT_OK : LT_FINISHED;
}


LTRESULT CLightMapMaker::CPrint(const char *pMsg, ...)
{
	char msg[2048];
	va_list marker;

	va_start(marker, pMsg);
	vsprintf(msg, pMsg, marker);
	va_end(marker);

	DrawStatusText(msg);
	return LT_OK;
}


LTRESULT CLightMapMaker::GetRotationVectors(LTRotation &rot, LTVector &right, LTVector &up, LTVector &forward)
{
	quat_GetVectors((float*)&rot, (float*)&up, (float*)&right, (float*)&forward);
	return LT_OK;
}


LTRESULT CLightMapMaker::InterpolateRotation(LTRotation &rDest, LTRotation &rot1, LTRotation &rot2, float t)
{
	gr_InterpolateRotation(&rDest, &rot1, &rot2, t);
	return LT_OK;
}


LTRESULT CLightMapMaker::GetWorldModelRuntimePos(HPREOBJECT hObj, LTVector &vPos)
{
	CPreWorld *pWorld;

	vPos.Init();
	if(m_pWorld && hObj < m_pWorld->m_Objects.GetSize())
	{
		pWorld = m_pWorld->FindWorldModel(m_pWorld->m_Objects[hObj]->GetName());
		if(pWorld)
		{
			vPos = pWorld->m_WorldTranslation;
			return LT_OK;
		}
	}

	return LT_ERROR;
}


// ------------------------------------------------------------------------ //
// Light groups.
// ------------------------------------------------------------------------ //

// Copies the lights under each LightGroup object into its numbered
// properties (Object1, Position1, Color1, Radius1, ModelLight1, ...) in
// the processed world's copy of it.
void SetupLightGroups(CWorldNode *pNode, CPreMainWorld *pWorld)
{
	GPOS pos;

	if(pNode->GetType() == Node_Object)
	{
		if(std::string(pNode->GetClassName()) == std::string("LightGroup"))
			ProcessLightGroup(pNode, pWorld);
	}
	else
	{
		for(pos=pNode->m_Children; pos; )
			SetupLightGroups(pNode->m_Children.GetNext(pos), pWorld);
	}
}


static void ProcessLightGroup(CWorldNode *pGroup, CPreMainWorld *pWorld)
{
	GPOS pos;
	uint32 iObject, iLight;
	CBaseEditObj *pObj;
	CWorldNode *pChild;
	CBaseProp *pPos, *pColor, *pRadius, *pLightObjects, *pProp;
	char num[8];
	char *pName;

	pName = ((CStringProp*)pGroup->m_PropList.GetProp("Name"))->m_String;
	iObject = FindObjectByName(pName, pWorld);
	if(iObject == 0xFFFFFFFF)
		return;

	pObj = pWorld->m_Objects[iObject];
	pos = pGroup->m_Children;

	static std::string sObject("Object");
	static std::string sPosition("Position");
	static std::string sColor("Color");
	static std::string sRadius("Radius");
	static std::string sModelLight("ModelLight");

	iLight = 1;
	while(pos)
	{
		pChild = pGroup->m_Children.GetNext(pos);

		std::string name(((CStringProp*)pChild->m_PropList.GetProp("Name"))->m_String);

		pPos = pChild->m_PropList.GetProp("Pos");
		pColor = pChild->m_PropList.GetProp("LightColor");
		if(!pColor)
			pColor = pChild->m_PropList.GetProp("InnerColor");
		pRadius = pChild->m_PropList.GetProp("LightRadius");
		pLightObjects = pChild->m_PropList.GetProp("LightObjects");

		g_LightsByName[name] = NULL;

		std::string objectName = sObject + itoa(iLight, num, 10);
		std::string positionName = sPosition + itoa(iLight, num, 10);
		std::string colorName = sColor + itoa(iLight, num, 10);
		std::string radiusName = sRadius + itoa(iLight, num, 10);
		std::string modelLightName = sModelLight + itoa(iLight, num, 10);

		pProp = pObj->m_PropList.GetProp(objectName.c_str());
		if(pProp)
			strcpy(((CStringProp*)pProp)->m_String, name.c_str());

		pProp = pObj->m_PropList.GetProp(positionName.c_str());
		if(pProp && pPos)
			((CVectorProp*)pProp)->m_Vector = ((CVectorProp*)pPos)->m_Vector;

		pProp = pObj->m_PropList.GetProp(colorName.c_str());
		if(pProp && pColor)
			((CColorProp*)pProp)->m_Vector = ((CColorProp*)pColor)->m_Vector;

		pProp = pObj->m_PropList.GetProp(radiusName.c_str());
		if(pProp && pRadius)
			((CRealProp*)pProp)->m_Value = ((CRealProp*)pRadius)->m_Value;

		pProp = pObj->m_PropList.GetProp(modelLightName.c_str());
		if(pProp && pLightObjects)
			((CBoolProp*)pProp)->m_Value = ((CBoolProp*)pLightObjects)->m_Value;

		iLight++;
	}
}


// The index of the object named pName in pWorld, or 0xFFFFFFFF.
static uint32 FindObjectByName(const char *pName, CPreMainWorld *pWorld)
{
	uint32 i;

	for(i=0; i < pWorld->m_Objects; i++)
	{
		if(strcmp(pName, ((CStringProp*)pWorld->m_Objects[i]->GetPropertyList()->GetProp("Name"))->m_String) == 0)
			return i;
	}

	return 0xFFFFFFFF;
}
