// talon: the 2001 one, with Build 69's as a guide. It starts with its
// static init for g_StringHolder (0x43e700) and the allocators, as Build
// 69's does; the code proper from 0x43e7e0.

#include "bdefs.h"
#include <stdarg.h>

#include "editregion.h"
#include "editpoly.h"
#include "processing.h"
#include "preworld.h"
#include "bspgen.h"
#include "brushtoworld.h"
#include "buildhulls.h"
#include "findworldmodel.h"
#include "parse_world_info.h"
#include "conparse.h"
#include "light_table.h"
#include "create_world_tree.h"
#include "gettextureflags.h"
#include "preleaf.h"
#include "node.h"
#include "node_ops.h"
#include "editobjects.h"
#include <float.h>
#include <errno.h>
#include "lightmapmaker.h"
#include "threads.h"
#include "EditRegion2001_decl.h"
#include "WorldApprox_decl.h"

// talon: the application's clock (ProcessorApp.cpp, 0x430210: winmm's
// timeGetTime through its thunk).
DWORD ProcessorApp_GetTime();


// talon: other modules' interfaces, provisional until those modules are
// written (the names are ours).

// The PVS maker (0x44b270; PVSInfo is WorldApprox_decl.h's).
uint32 MakePVS(PVSInfo *pInfo);

// Finds the vis BSP's leaves (0x40c640).
BOOL SetupVisBSPLeaves(CPreWorld *pVisBSP);

// What CTJunctionFixer's lists hold (FixTJunctions never touches them, so
// the type is not known; the name is ours). Their vtable (0x47575c) is not
// CMoArray<CPrePoly*>'s (0x4746b8), though its entries are the same code.
class CTJUnknown;

// The T-junction fixer (0x4479c0, its destructor 0x43f960). FixTJunctions
// returns nothing (0x4488c9; TJunctions_decl.h).
class CTJunctionFixer
{
public:
	void					FixTJunctions(CPreWorld *pWorld, BOOL bSplitPolys);

	uint32							m_Unknown_00;
	CMoArray< CMoArray<CTJUnknown*> >	m_Unknown_04;	// vtable 0x47578c
	CMoArray<CTJUnknown*>				m_Unknown_18;	// vtable 0x47575c
};

// talon: not matched yet: the template code this module emits for
// m_Unknown_04 (0x441d30..0x442e10) expands less than ours, and than
// LightMapMaker's CMoArray< CMoArray<uint32> > (0x41fb50..) whose code ours
// is: GenAppend (0x441ea0) leaves DefaultCache's GetCacheSize and
// SetCacheSize (0x4420b0, 0x4420c0) out of line in its deepest inline
// (measured: an inline depth of 7 instead of 8 for the template
// instantiations reproduces it and the two accessors exactly), and
// InternalNiceSetSize (0x442a80), _DeleteAndDestroyArray (0x442c20) and
// BaseDelete (0x442ce0) call the element's destructor (0x4429a0) where ours
// expands it (measured: an element class derived from CMoArray<X*>
// reproduces those three exactly, but then its constructor is a call in
// FixWorldTJunctions and BaseNew, and GenAppend expands less than
// shipped). The element type (a pointer, a 4-byte struct) and where the
// class is declared do not move it.

// The problem brush module: a world model's BSP (0x40b540; bPhysicsBSP
// for the physics BSP) and the vis BSP (0x40cbd0).
BOOL MakeWorldModelBSP(CEditRegion *pRegion, CWorldModelDef *pDef, CPreWorld *pWorld, BOOL bPhysicsBSP);
BOOL MakeVisBSP(CEditRegion *pRegion, CWorldModelDef *pDef, CPreWorld *pWorld);

// A terrain world model (0x446530).
BOOL MakeTerrain(CEditRegion *pRegion, CWorldModelDef *pDef, CPreWorld *pWorld, char *pInfoString);

// RaySelect: selects the brushes the RaySelecter objects' rays hit (0x4450c0).
uint32 DoRaySelect(CPreMainWorld *pMainWorld, CEditRegion *pRegion);

// Does nothing to the region (0x40a0a0, folded with cb_CheckClassDefs;
// the name is ours).
void CheckRegion(CEditRegion *pRegion);

// Copies the region's objects (without the hidden ones, with
// bIgnoreHidden) into objects (0x40f2e0).
void CopyRegionObjects(CMoArray<CBaseEditObj*> &regionObjects, CMoArray<CBaseEditObj*> &objects, BOOL bIgnoreHidden);

// ReplaceTextures (0x445700).
int ReplaceTextures(CEditRegion *pRegion, CPreMainWorld *pWorld);

// Removes the polies no leaf sees (0x4498d0).
class CUnseenPolyRemover
{
public:
	uint32					RemoveUnseenPolies(CPreWorld *pWorld, CMoArray<CBaseEditObj*> *pObjects);
};


// talon: the brushes the brush checks found, for SelectProblemBrushes and
// SelectOutOfBoundsBrushes (the names are ours; MakeBsps.cpp and
// PreBrush.cpp fill and use them). One declaration: one initializer
// (0x43e5b0) constructs both, g_ProblemBrushes (0x485758) first, and one
// (0x43e660) destroys both.
CMoArray<CEditBrush*>		g_ProblemBrushes, g_OutOfBoundsBrushes;

CStringHolder				g_StringHolder;

// talon: the 2001 region (EditRegion2001_decl.h). The other modules still
// take the Build 69 CEditRegion, so it is passed to them as one.
CEditRegion2001				*g_InputRegion = NULL;
FILE						*g_pLogFile = NULL;
static char					g_InputFileTitle[MAX_PATH];
CProcessorGlobs				*g_pGlobs = NULL;

// talon: always set (0x47eed4; the name is ours).
BOOL						g_bMinimizeTCoords = TRUE;


//---------------------Memory Allocation----------------------------//
void* dalloc(unsigned int size)
{
	void *pRet;

	pRet = malloc((size_t)size);
	if(!pRet)
		throw CLithMemException();

	return pRet;
}

void dfree(void *ptr)
{
	free(ptr);
}

void* dalloc_z(unsigned int size)
{
	void *pRet;

	pRet = malloc((size_t)size);
	if(!pRet)
		throw CLithMemException();

	memset(pRet, 0, size);
	return pRet;
}
//-----------------------------------------------------------------//


void InitProcessOptions(CProcessorGlobs *pGlobs)
{
	int i;
	char *pArg;

	pGlobs->m_MaxBounds = 400000.0f;
	pGlobs->m_bPortals = TRUE;
	pGlobs->m_bTextureFlags = FALSE;
	pGlobs->m_bSelectProblemBrushes = FALSE;
	pGlobs->m_bSelectOutOfBoundsBrushes = FALSE;
	pGlobs->m_bRemoveProblemObjects = FALSE;
	pGlobs->m_LeakFile[0] = 0;
	pGlobs->m_PortalFile[0] = 0;
	pGlobs->m_bRaySelect = FALSE;
	pGlobs->m_bLightAnimations = FALSE;
	pGlobs->m_bSelectHullProblemBrushes = FALSE;
	pGlobs->m_bIgnoreHidden = FALSE;
	pGlobs->m_bFixTJunc = FALSE;
	pGlobs->m_bNoSnapNoCSG = FALSE;
	pGlobs->m_bBalanceBSPs = TRUE;
	pGlobs->m_bAIVolumes = FALSE;
	strcpy(pGlobs->m_ProjectDir, ".");

	for(i=0; i < (int)pGlobs->m_Argc; i++)
	{
		pArg = pGlobs->m_Argv[i];
		if(pArg[0] == '-' || pArg[0] == '+')
			pArg++;

		if(stricmp(pArg, "LeakFile") == 0)
		{
			if(i+1 < (int)pGlobs->m_Argc)
				strncpy(pGlobs->m_LeakFile, pGlobs->m_Argv[i+1], 255);
		}
		else if(stricmp(pArg, "PortalFile") == 0)
		{
			if(i+1 < (int)pGlobs->m_Argc)
				strncpy(pGlobs->m_PortalFile, pGlobs->m_Argv[i+1], 255);
		}
		else if(stricmp(pArg, "MaxBounds") == 0)
		{
			if(i+1 < (int)pGlobs->m_Argc)
				pGlobs->m_MaxBounds = (PReal)atof(pGlobs->m_Argv[i+1]);
		}
		else if(stricmp(pArg, "RaySelect") == 0)
		{
			pGlobs->m_bRaySelect = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "SelectProblemBrushes"))
		{
			pGlobs->m_bSelectProblemBrushes = TRUE;
		}
		else if(stricmp(pArg, "SelectOutOfBoundsBrushes") == 0)
		{
			pGlobs->m_bSelectOutOfBoundsBrushes = TRUE;
		}
		else if(stricmp(pArg, "SelectHullProblemBrushes") == 0)
		{
			pGlobs->m_bSelectHullProblemBrushes = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "RemoveProblemObjects"))
		{
			pGlobs->m_bRemoveProblemObjects = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "TextureFlags"))
		{
			pGlobs->m_bTextureFlags = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "ProjectDir"))
		{
			if(i+1 < (int)pGlobs->m_Argc)
				strncpy(pGlobs->m_ProjectDir, pGlobs->m_Argv[i+1], 199);
		}
		else if(CHelpers::UpperStrcmp(pArg, "NoPortals"))
		{
			pGlobs->m_bPortals = FALSE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "bsp"))
		{
			pGlobs->m_bBsp = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "aivolumes"))
		{
			pGlobs->m_bAIVolumes = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "replacetextures"))
		{
			pGlobs->m_bReplaceTextures = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "numthreads"))
		{
			if(i+1 < (int)pGlobs->m_Argc)
			{
				pGlobs->m_nThreads = atoi(pGlobs->m_Argv[i+1]);
				if((int)pGlobs->m_nThreads < 0)
					pGlobs->m_nThreads = 0;
				if((int)pGlobs->m_nThreads >= MAX_THREADS)
					pGlobs->m_nThreads = MAX_THREADS;
			}
		}
		else if(CHelpers::UpperStrcmp(pArg, "light"))
		{
			pGlobs->m_bLight = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "nocliplight"))
		{
			pGlobs->m_bClipLight = FALSE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "optimize"))
		{
			pGlobs->m_bOptimize = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "fast"))
		{
			pGlobs->m_bFast = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "LightAnims"))
		{
			pGlobs->m_bLightAnimations = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "onlylightverts"))
		{
			pGlobs->m_bVerticesOnly = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "Lambertian"))
		{
			pGlobs->m_bLambertian = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "maxdetail"))
		{
			if(i+1 < (int)pGlobs->m_Argc)
				pGlobs->m_MaxDetail = atoi(pGlobs->m_Argv[i+1]);
		}
		else if(CHelpers::UpperStrcmp(pArg, "splitweight"))
		{
			if(i+1 < (int)pGlobs->m_Argc)
			{
				pGlobs->m_SplitWeight = (PReal)atof(pGlobs->m_Argv[i+1]);
				pGlobs->m_BalanceWeight = 1.0f - pGlobs->m_SplitWeight;
			}
		}
		else if(CHelpers::UpperStrcmp(pArg, "logfile"))
		{
			pGlobs->m_bLogFile = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "ignorehidden"))
		{
			pGlobs->m_bIgnoreHidden = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "FixTJunc"))
		{
			pGlobs->m_bFixTJunc = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "NoSnapNoCSG"))
		{
			pGlobs->m_bNoSnapNoCSG = TRUE;
		}
		else if(CHelpers::UpperStrcmp(pArg, "NoBalanceBSPs"))
		{
			pGlobs->m_bBalanceBSPs = FALSE;
		}
	}
}


static void InternalDrawStatusText(const char *pMsg, va_list marker)
{
	char msg[500];

	vsprintf(msg, pMsg, marker);

	if(g_pLogFile)
	{
		fprintf(g_pLogFile, "%s\r\n", msg);
		fflush(g_pLogFile);
	}

	DrawStatusTextToScreen(msg);
}


void DrawStatusText(const char *pMsg, ...)
{
	va_list marker;

	va_start(marker, pMsg);
	InternalDrawStatusText(pMsg, marker);
	va_end(marker);
}


void DrawStatusTextIfMainWorld(const char *pMsg, ...)
{
	va_list marker;

	if(!g_pGlobs->m_bMakingWorldModel)
	{
		va_start(marker, pMsg);
		InternalDrawStatusText(pMsg, marker);
		va_end(marker);
	}
}


// Saves the ED file (with the selections) over the input (0x43edc0).
BOOL SaveEDFile()
{
	CMoFileIO	file;
	char		fileName[256];

	sprintf(fileName, "%s.ed", g_pGlobs->m_InputFile);
	if(file.Open(fileName, "wb"))
	{
		g_InputRegion->SaveFile(file, TRUE);
		return TRUE;
	}

	DrawStatusText("Can't open ED file %s for writing.", fileName);
	return FALSE;
}


void BspProgressFn(CBspGen *pBspGen, CReal fPercent);

BOOL MakeBsp(CPreWorld *pWorld, BOOL bMinimizePolies, BOOL bHullMakers, BOOL bUseOriginalPolies)
{
	CBspGen			bspGen;
	uint32			i;
	GPOS			pos;
	CPrePoly		*pPoly;
	CBspGenOptions	options;
	BOOL			bRet;

	StartProgressStage(PROGRESS_BSP);
	bspGen.m_ProgressFn = BspProgressFn;

	options.m_pWorld				= pWorld;
	options.m_bMinimizePolies		= bMinimizePolies;
	options.m_bHullMakers			= bHullMakers;
	options.m_nThreadsToUse			= g_pGlobs->m_nThreads;
	options.m_bUseOriginalPolies	= bUseOriginalPolies;

	bRet = bspGen.GenerateBspTree(&options);
	EndProgressStage(PROGRESS_BSP, FALSE);

	// Make all polies full brightness, so they're not required to light it.
	for(pos=pWorld->m_Polies; pos; )
	{
		pPoly = pWorld->m_Polies.GetNext(pos);
		
		for(i=0; i < pPoly->NumVerts(); i++)
		{
			VEC_SET(pPoly->Color(i), 255.0f, 255.0f, 255.0f);
		}
	}

	DrawStatusTextIfMainWorld("Num detail nodes: %d", pWorld->NumDetailNodes());
	return bRet;	
}


void BspProgressFn(CBspGen *pBspGen, CReal fPercent)
{
	SetProgressBar(fPercent);
}


void ProgressFn(float fPercent);

// Makes the hulls (0x43f020; the name is ours). pInfo gets the brushes
// whose hulls failed, if it's set.
BOOL BuildWorldHulls(CPreWorld *pWorld, CHullInfo *pInfo)
{
	CHullInfo	info;
	uint32		status;

	if(!pInfo)
		pInfo = &info;

	StartProgressStage(PROGRESS_HULLS);
	status = MakeHulls(pInfo, pWorld, ProgressFn);
	if(status)
		DrawStatusText("Error making hulls.");

	EndProgressStage(PROGRESS_HULLS, FALSE);
	return status == 0;
}


void ProgressFn(float fPercent)
{
	SetProgressBar(fPercent);
}


void PVSProgressFn(float fPercent);

static BOOL g_bFullOptimization;

// Makes the PVS (0x43f170; the name is ours).
BOOL MakeWorldPVS(CPreMainWorld *pMainWorld)
{
	PVSInfo					info;
	CHullInfo				hullInfo;
	CPreWorld				*pVisBSP;
	uint32					status;

	pVisBSP = pMainWorld->GetVisBSP();
	if(!pVisBSP)
	{
		DrawStatusText("ERROR: No visibility BSP!");
		return FALSE;
	}

	if(!SetupVisBSPLeaves(pVisBSP))
		return FALSE;

	StartProgressStage(PROGRESS_PVS);

	memset(&info, 0, sizeof(info));
	info.m_pWorld = pVisBSP;
	info.m_ProgressFn = ProgressFn;
	info.m_LeafDoneFn = PVSProgressFn;
	info.m_bFast = g_pGlobs->m_bFast;
	info.m_nThreads = g_pGlobs->m_nThreads;

	g_bFullOptimization = FALSE;
	// talon: the result is named: compared with the zero register
	// (0x43f363), not tested.
	status = MakePVS(&info);
	if(status != 0)
	{
		DrawStatusText("Error making PVS.");
		return FALSE;
	}

	EndProgressStage(PROGRESS_PVS, FALSE);
	return TRUE;
}


void PVSProgressFn(float fPercent)
{
	if(!g_bFullOptimization)
	{
		SetFullOptimization();
		g_bFullOptimization = TRUE;
	}

	SetProgressBar(fPercent);
}


CEditBrush* FindBrushWithPoly(CEditRegion2001 *pRegion, CEditPoly *pPoly);

// Makes the hulls; with SelectHullProblemBrushes, selects the brushes they
// failed on and saves the ED file instead (0x43f5f0; the name is ours).
BOOL MakeHullsAndRemoveUnseen(CPreWorld *pWorld)
{
	CHullInfo			info;
	CUnseenPolyRemover	remover;
	CEditBrush			*pBrush;
	uint32				i, nSelected;

	BuildWorldHulls(pWorld, &info);

	if(g_pGlobs->m_bSelectHullProblemBrushes && g_InputRegion)
	{
		nSelected = 0;
		for(i=0; i < info.m_ProblemBrushes; i++)
		{
			pBrush = FindBrushWithPoly(g_InputRegion, (CEditPoly*)info.m_ProblemBrushes[i]);
			if(pBrush)
			{
				g_InputRegion->SelectNode(pBrush);
				nSelected++;
			}
		}

		DrawStatusText("");
		DrawStatusText("---------------");
		DrawStatusText("SelectHullProblemBrushes: Selected %d brushes.", nSelected);
		DrawStatusText("---------------");
		SaveEDFile();
		return FALSE;
	}

	g_pGlobs->m_nPoliesRemoved = remover.RemoveUnseenPolies(pWorld, &pWorld->m_pMainWorld->m_Objects);
	DrawStatusTextIfMainWorld("Number of (unseen) polies removed: %d", g_pGlobs->m_nPoliesRemoved);
	return TRUE;
}


// The brush pPoly is in (0x43f7c0).
CEditBrush* FindBrushWithPoly(CEditRegion2001 *pRegion, CEditPoly *pPoly)
{
	LPOS		pos;
	CEditBrush	*pBrush;
	uint32		i;

	for(pos=pRegion->m_Brushes; pos; )
	{
		pBrush = pRegion->m_Brushes.GetNext(pos);

		for(i=0; i < pBrush->m_Polies; i++)
		{
			if(pBrush->m_Polies[i] == pPoly)
				return pBrush;
		}
	}

	return NULL;
}


// (0x43f810; the name is ours.)
void FixWorldTJunctions(CPreWorld *pWorld)
{
	CTJunctionFixer fixer;

	fixer.FixTJunctions(pWorld, g_pGlobs->m_bFixTJunc);
}


// Finds a plane in the list that matches the specifications you pass in.
// The actual floating point values must match exactly for a plane to be returned.
CPrePlane* FindPlaneExact(CLinkedList<CPrePlane*> *pList, PVector &normal, PReal dist)
{
	LPOS		pos;
	CPrePlane	*pTestPlane;

	for(pos=pList->GetHeadPosition(); pos; )
	{
		pTestPlane = pList->GetNext(pos);
		
		if(pTestPlane->m_Normal.x == normal.x &&
			pTestPlane->m_Normal.y == normal.y &&
			pTestPlane->m_Normal.z == normal.z && 
			pTestPlane->m_Dist == dist)
		{
			return pTestPlane;
		}
	}

	return NULL;
}


// talon: the 2001 CEditPoly (0x88 bytes; EditObjects2001.cpp has the class
// as CEditPoly2001): what GenerateSurfaces reads of it, on
// EditRegion2001_decl.h's stand-in (the names of the unknown members by
// offset).
class CEditPoly2001_PR : public CEditPoly2001_ER
{
	public:

		uint32						m_Unknown_2c[2];
		PVector						O, P, Q;			// 0x34: the texture vectors
		uint8						m_Ambient[3];		// 0x58: the surface's ambient light
		// talon: m_Triangles (0x5c, a CMoArray<CBasePoly*>) and a dword
		// (0x70) as padding: with them declared, DoProcessing's inline
		// CMoArray destructor adds its two counts the other way round (the
		// declaration-count threshold, tools/guides/09).
		uint8						m_Unknown_5b[0x19];
		char						*m_pTextureName;	// 0x74
		uint32						m_Unknown_78;
		void						*m_pUser1;			// 0x7c: the surface made for it
		uint32						m_Unknown_80;
		uint32						m_SurfaceFlags;		// 0x84: the poly's own surface flags
};


// (Build 69's.)
#define NUM_PLANE_LISTS		128

inline uint32 GetHashCode(PVector &normal, PReal &dist)
{
	return	(uint32)((double)normal.x * 10000.0f) +
			(uint32)((double)normal.y * 10000.0f) +
			(uint32)((double)normal.z * 10000.0f) +
			(uint32)((double)dist * 10000.0f);
}


// Makes a surface in pWorld for each brush poly (0x43fa90): Build 69's,
// with the 2001 poly's one texture, the Effect properties and the portals'
// surface users.
void GenerateSurfaces(GenList<CEditBrush*> &brushes, CPreWorld *pWorld)
{
	uint32				j;
	CEditBrush			*pBrush;
	CEditPoly2001_PR	*pPoly;
	CPreSurface			*pSurface;
	CStringProp			*pProp;
	PreSurfaceUser		*pUser;
	char				baseTextureName[512];
	PVector				normal;
	uint32				iList;
	PReal				dist;
	GenListPos			pos;

	CLinkedList<CPrePlane*> planeLists[NUM_PLANE_LISTS];


	for(pos=brushes.GenBegin(); brushes.GenIsValid(pos); )
	{
		pBrush = brushes.GenGetNext(pos);

		for(j=0; j < pBrush->m_Polies; j++)
		{
			pPoly = (CEditPoly2001_PR*)pBrush->m_Polies[j];

			// Setup the surface.
			pSurface = new CPreSurface;

			// talon: each vector through a temporary, by FVec (0x43fb73).
			pSurface->m_TextureO = pPoly->O.FVec();
			pSurface->m_TextureP = pPoly->P.FVec();
			pSurface->m_TextureQ = pPoly->Q.FVec();
			pSurface->m_pTextureName = pPoly->m_pTextureName;

			normal = pPoly->m_Plane.m_Normal.FVec();
			dist = (PReal)pPoly->m_Plane.m_Dist;

			// (Using the hash table makes this about 40 times faster for large levels).
			// NOTE: it still only accepts EXACT duplicate planes so it doesn't lose 
			// precision here.. the hash table is only for speed (at one point, it didn't
			// always use exact duplicates and some levels with rotated brushes had tons
			// of problems).
			iList = GetHashCode(normal, dist) % NUM_PLANE_LISTS;
			pSurface->m_pPlane = FindPlaneExact(&planeLists[iList], normal, dist);
			if(!pSurface->m_pPlane)
			{
				pSurface->m_pPlane = pWorld->AddPlane(
					pPoly->m_Plane.m_Normal.FVec(), pPoly->m_Plane.m_Dist);
				
				planeLists[iList].AddTail(pSurface->m_pPlane);
			}

			pSurface->m_Flags = GetBrushType(pBrush) | (pPoly->m_SurfaceFlags & ~SURF_TERRAINOCCLUDER);
			
			// Hack Mc. Hackelson.. force SURF_INVISIBLE if using a certain texture name..
			if(pPoly->m_pTextureName)
			{
				CHelpers::ExtractNames(pPoly->m_pTextureName, NULL, baseTextureName, NULL, NULL);
				if(stricmp(baseTextureName, "invisible.dtx") == 0)
				{
					pSurface->m_Flags |= SURF_INVISIBLE;
				}
			}
			
			if((pProp = (CStringProp*)pBrush->m_PropList.GetProp("Effect")) && (pProp->m_Type == PT_STRING) && (pProp->m_String[0] != 0))
			{
				pSurface->m_Unknown_a0 = (uint32)pWorld->m_StringHolder.AddString(pProp->m_String);

				if((pProp = (CStringProp*)pBrush->m_PropList.GetProp("EffectParam")) && (pProp->m_Type == PT_STRING))
					pSurface->m_Unknown_a4 = (uint32)pWorld->m_StringHolder.AddString(pProp->m_String);
				else
					pSurface->m_Unknown_a4 = (uint32)"";
			}

			pSurface->m_bDetail = (uint8)GetDetailLevel(pBrush);
			pSurface->m_Ambient[0] = pPoly->m_Ambient[0];
			pSurface->m_Ambient[1] = pPoly->m_Ambient[1];
			pSurface->m_Ambient[2] = pPoly->m_Ambient[2];

			pPoly->m_pUser1 = pSurface;
			// talon: CPreSurface::m_pBrush is the poly it came from.
			pSurface->m_pBrush = (CEditBrush*)pPoly;

			pWorld->m_Surfaces.AddTail(pSurface);

			// A portal's surface goes on the world's list of surface users,
			// by its brush's name.
			if((pSurface->m_Flags & SURF_PORTAL) && g_pGlobs->m_bPortals)
			{
				if((pProp = (CStringProp*)pBrush->m_PropList.GetProp("Name")) && (pProp->m_Type == PT_STRING))
				{
					pUser = new PreSurfaceUser;
					pUser->m_Index = pWorld->m_Unknown_04.GetSize();
					pUser->m_pName = pWorld->m_StringHolder.AddString(pProp->m_String);
					pWorld->m_Unknown_04.AddTail(pUser);

					pSurface->m_Unknown_80 = (uint32)pUser;
					pUser->m_pSurface = pSurface;
				}
			}
		}
	}

	pWorld->SetupSurfaceTextureVectors();
}


PReal GetMaxLMSize(char *pInfoString)
{
	ConParse parse;
	int size;

	parse.Init(pInfoString);
	if(parse.ParseFind("MaxLMSize", FALSE, 2))
	{
		size = atoi(parse.m_Args[1]);
		if(size == 16 || size == 32)
			return (PReal)size;
	}

	return 32.0f;
}


PReal GetLMGridSize(char *pInfoString)
{
	ConParse parse;

	parse.Init(pInfoString);
	if(parse.ParseFind("LMGridSize", FALSE, 2))
		return (PReal)atoi(parse.m_Args[1]);

	return 20.0f;
}


BOOL MergeBrushes(
	GenList<CEditBrush*> &brushes,
	CPreWorld *pWorld)
{
	DrawStatusTextIfMainWorld("Merging %d brushes.", brushes.GenGetSize());

	StartProgressStage(PROGRESS_MERGEBRUSHES);
	if(!BrushToWorld(brushes, pWorld, ProgressFn))
	{
		DrawStatusText("");
		DrawStatusText("---------------");
		DrawStatusText("Found an invalid (unprocessable) brush!");
		DrawStatusText("---------------");
		return FALSE;
	}

	EndProgressStage(PROGRESS_MERGEBRUSHES, FALSE);
	return TRUE;
}


BOOL CreatePhysicsBSPs(CEditRegion *pRegion, CPreMainWorld *pMainWorld, CWorldModelDef *pMainDef)
{
	CPreWorld *pPhysicsBSP, *pVisBSP;

	DrawStatusText("Creating physics BSP");
	pPhysicsBSP = new CPreWorld(pMainWorld);
	pPhysicsBSP->m_WorldInfoFlags = WIF_PHYSICSBSP | WIF_MAINWORLD;
	pMainWorld->m_WorldModels.GenAppend(pPhysicsBSP);
	SAFE_STRCPY(pPhysicsBSP->m_WorldName, "PhysicsBSP");
	if(!MakeWorldModelBSP(pRegion, pMainDef, pPhysicsBSP, TRUE))
		return FALSE;

	DrawStatusText("Creating visibility BSP");
	pVisBSP = new CPreWorld(pMainWorld);
	pVisBSP->m_WorldInfoFlags = WIF_VISBSP;
	pMainWorld->m_WorldModels.GenAppend(pVisBSP);
	SAFE_STRCPY(pVisBSP->m_WorldName, "VisBSP");
	return MakeVisBSP(pRegion, pMainDef, pVisBSP) != 0;
}


// Adds a world model for each of worldModels to pMainWorld (the AIVolumes;
// 0x440220, the name is ours).
BOOL AddWorldModels(CEditRegion *pRegion, CMoArray<CWorldModelDef*> &worldModels, CPreMainWorld *pMainWorld)
{
	uint32			i, iModel, iFirst;
	// talon: the second loop's world is a variable of its own (in a
	// register there; the first loop's lives on the stack, 0x44028e).
	CPreWorld		*pWorld, *pModelWorld;
	CWorldModelDef	*pDef;

	if(worldModels.GetSize() == 0)
		return TRUE;

	iFirst = pMainWorld->m_WorldModels.GetSize();
	for(i=0; i < worldModels; i++)
	{
		pWorld = new CPreWorld(pMainWorld);
		pMainWorld->m_WorldModels.Append(pWorld);
		GenerateSurfaces(worldModels[i]->m_Brushes, pWorld);
	}

	g_pGlobs->m_bMakingWorldModel = TRUE;
	for(iModel=0; iModel < worldModels; iModel++)
	{
		pDef = worldModels[iModel];
		pModelWorld = pMainWorld->m_WorldModels[iFirst + iModel];

		pModelWorld->m_WorldInfoFlags = pDef->m_WorldInfoFlags;
		SAFE_STRCPY(pModelWorld->m_WorldName, pDef->m_WorldName);
		if(!MakeWorldModelBSP(pRegion, pDef, pModelWorld, FALSE))
		{
			g_pGlobs->m_bMakingWorldModel = FALSE;
			return FALSE;
		}
	}

	g_pGlobs->m_bMakingWorldModel = FALSE;
	return TRUE;
}


void GetWorldStats(CPreMainWorld *pMainWorld);

BOOL ImportGeometry(CEditRegion *pRegion, CMoArray<CWorldModelDef*> &worldModels, CPreMainWorld *pMainWorld)
{
	uint32			i, nSelected;
	CPreWorld		*pWorld;
	CWorldModelDef	*pDef, *pMainDef;

	if(worldModels.GetSize() == 0)
	{
		DrawStatusText("Must have at least one brush!");
		return FALSE;
	}

	if(g_pGlobs->m_bRaySelect)
	{
		nSelected = DoRaySelect(pMainWorld, (CEditRegion*)g_InputRegion);

		DrawStatusText("");
		DrawStatusText("---------------");
		DrawStatusText("RaySelect: Did %d ray selections", nSelected);
		DrawStatusText("---------------");
		SaveEDFile();
		return FALSE;
	}

	pMainDef = worldModels[0];
	for(i=0; i < worldModels; i++)
	{
		pDef = worldModels[i];
		pWorld = new CPreWorld(pMainWorld);
		pMainWorld->m_WorldModels.Append(pWorld);
		GenerateSurfaces(pDef->m_Brushes, pWorld);
	}

	if(!CreateWorldTree(&pMainWorld->m_WorldTree, worldModels, pMainWorld->m_pInfoString))
		return FALSE;

	DrawStatusText("WorldTree nodes: %d", pMainWorld->m_WorldTree.m_nNodes);
	DrawStatusText("WorldTree depth: %d", pMainWorld->m_WorldTree.m_nDepth);
	DrawStatusText("WorldTree root size: %.2f", pMainWorld->m_WorldTree.m_RootNode.m_MinSize);

	// The main world's BSP is made below, as the physics and vis BSPs.
	if(pMainWorld->m_WorldModels[0])
		delete pMainWorld->m_WorldModels[0];

	pMainWorld->m_WorldModels.Remove(0);
	worldModels.Remove(0);

	for(i=0; i < worldModels; i++)
	{
		pDef = worldModels[i];
		pWorld = pMainWorld->m_WorldModels[i];

		pWorld->m_WorldInfoFlags = pDef->m_WorldInfoFlags;
		SAFE_STRCPY(pWorld->m_WorldName, pDef->m_WorldName);
		g_pGlobs->m_bMakingWorldModel = !(pWorld->m_WorldInfoFlags & WIF_MAINWORLD);

		if(pWorld->m_WorldInfoFlags & WIF_TERRAIN)
		{
			if(!MakeTerrain(pRegion, pDef, pWorld, pMainWorld->m_pInfoString))
				return FALSE;
		}
		else
		{
			if(!MakeWorldModelBSP(pRegion, pDef, pWorld, FALSE))
				return FALSE;
		}
	}

	g_pGlobs->m_bMakingWorldModel = FALSE;
	if(!CreatePhysicsBSPs(pRegion, pMainWorld, pMainDef))
		return FALSE;

	if(g_pGlobs->m_bTextureFlags)
		GetTextureFlags(pMainWorld);

	if(g_bMinimizeTCoords)
		pMainWorld->MinimizeSurfaceTCoords();

	GetWorldStats(pMainWorld);
	g_pGlobs->m_bMakingWorldModel = FALSE;
	return TRUE;
}


void GetWorldStats(CPreMainWorld *pMainWorld)
{
	LightTable	lightTable;
	CPreWorld	*pWorld;
	uint32		i;

	g_pGlobs->m_nOutputPolies		= 0;
	g_pGlobs->m_nOutputVertices		= 0;
	g_pGlobs->m_TreeDepth			= 0;
	g_pGlobs->m_VisListMemory		= 0;
	g_pGlobs->m_nLeaves				= 0;
	g_pGlobs->m_nVisibleLeaves		= 0;
	g_pGlobs->m_LMDataSize			= pMainWorld->CalcLMDataSize();
	g_pGlobs->m_LMCompressionRatio	= pMainWorld->CalcLMCompressionRatio();

	lightTable.InitLightTableSize(&pMainWorld->m_PosMin, &pMainWorld->m_PosMax,
		ParseLightTableRes(pMainWorld->m_pInfoString));
	g_pGlobs->m_LightTableSize = lightTable.m_FullLookupSize * sizeof(LTRGBColor);
	
	for(i=0; i < pMainWorld->m_WorldModels; i++)
	{
		pWorld = pMainWorld->m_WorldModels[i];

		if(pWorld->m_WorldInfoFlags & WIF_MAINWORLD)
		{
			g_pGlobs->m_nOutputPolies	+= pWorld->m_Polies;
			g_pGlobs->m_nOutputVertices += pWorld->m_nPointsSaved;
			g_pGlobs->m_TreeDepth		+= pWorld->GetTreeDepth();
			g_pGlobs->m_VisListMemory	+= pWorld->CalcVisListMemory();
			g_pGlobs->m_nLeaves			+= pWorld->m_Leaves;
			g_pGlobs->m_nVisibleLeaves	+= pWorld->NumVisibleLeaves();
		}
	}
}


// The next bit of a leaf's visibility list (0x440890; the name is ours).
BOOL ReadVisBit(CPreLeafList *pList, uint32 *pBit)
{
	uint32 bit;
	BOOL bRet;

	bit = *pBit;
	bRet = (pList->m_Data.GetArray()[bit >> 3] & (1 << (bit & 7))) != 0;
	*pBit = bit + 1;
	return bRet;
}


// (0x4408d0; the name is ours.)
void WriteLeafIndex(CNode *pNode, FILE *fp)
{
	fprintf(fp, "%d ", pNode->m_pLeaf->m_Index);
}


void OpenLogFile();
void CloseLogFile();
BOOL LoadEDFile();
void RemoveDetailBrushes(CEditRegion2001 *pRegion);
BOOL LoadDATFile(CPreMainWorld *pMainWorld, BOOL bWarn, BOOL *pbLoaded, BOOL bLoadObjects);
BOOL SaveDATFile(CPreMainWorld *pMainWorld, BOOL bObjectsOnly);
void ShowStatusText(DWORD startTime, DWORD endTime);
BOOL SavePortalFile(CPreMainWorld *pMainWorld, const char *pFilename);
void WriteVisibleLeaves(CPreLeafList *pList, uint32 *pBit, CNode *pNode, FILE *fp);


// (0x440900; its two catch blocks are 0x440f16 and 0x440f3c.)
void DoProcessing(CProcessorGlobs *pGlobs)
{
	CPreMainWorld				mainWorld;
	CMoArray<CWorldModelDef*>	worldModels;
	CLightMapMaker				lightMapMaker;
	DWORD						startTime, endTime;
	char						timeStr[128], dateStr[128];
	BOOL						bLoaded;
	uint32						nProcessors;

	g_pGlobs = pGlobs;
	pGlobs->m_bMakingWorldModel = FALSE;
	startTime = ProcessorApp_GetTime();

	g_InputRegion = new CEditRegion2001;

	CHelpers::ExtractNames(g_pGlobs->m_InputFile, NULL, NULL, g_InputFileTitle, NULL);
	OpenLogFile();

	DrawStatusText("Processing %s.ed", g_pGlobs->m_InputFile);
	DrawStatusText("");

	thd_GetTimeString(timeStr);
	thd_GetDateString(dateStr);
	DrawStatusText("Date: %s, Time: %s", dateStr, timeStr);

	nProcessors = thd_Init();
	DrawStatusText("This machine has %d processor%s.", nProcessors, (nProcessors > 1) ? "s" : "");
	DrawStatusText("Using %d threads.", g_pGlobs->m_nThreads);
	DrawStatusText("Max detail level: %d.", g_pGlobs->m_MaxDetail);

	try
	{
		try
		{
			g_InputRegion->m_pStringHolder = &g_StringHolder;
			bLoaded = FALSE;

			DrawStatusText("Getting objects from ED file.");
			if(!LoadEDFile())
				goto Done;

			if(g_pGlobs->m_bRemoveProblemObjects)
			{
				DrawStatusText("");
				DrawStatusText("---------------");
				DrawStatusText("Removing problem objects");
				DrawStatusText("---------------");
				SaveEDFile();
				goto Done;
			}

			if(g_pGlobs->m_bBsp || g_pGlobs->m_bAIVolumes || g_pGlobs->m_bReplaceTextures ||
				g_pGlobs->m_bLight || g_pGlobs->m_bOptimize)
			{
				DrawStatusText("Loading DAT file.");
				if(!LoadDATFile(&mainWorld, g_pGlobs->m_bBsp, &bLoaded, FALSE) && !g_pGlobs->m_bBsp)
					goto Done;
			}

			mainWorld.m_pInfoString = mainWorld.m_StringHolder.AddString(g_InputRegion->m_pInfoString, TRUE);
			CopyRegionObjects(g_InputRegion->m_Objects, mainWorld.m_Objects, g_pGlobs->m_bIgnoreHidden);
			SetupLightGroups(&g_InputRegion->m_RootNode, &mainWorld);

			if(pGlobs->m_bReplaceTextures)
			{
				DrawStatusText("Replacing textures...");
				DrawStatusText("Replaced %d textures", ReplaceTextures((CEditRegion*)g_InputRegion, &mainWorld));

				if(pGlobs->m_bTextureFlags)
					GetTextureFlags(&mainWorld);

				goto SaveIt;
			}

			if(g_pGlobs->m_bBsp)
			{
				DrawStatusText("");
				DrawStatusText("-------------------");

				mainWorld.TermGeometry();
				g_pGlobs->m_MaxLMSize = GetMaxLMSize(mainWorld.m_pInfoString);
				mainWorld.m_LMGridSize = GetLMGridSize(mainWorld.m_pInfoString);
				DrawStatusText("Max Lightmap Size: %.2f", g_pGlobs->m_MaxLMSize);
				DrawStatusText("Lightmap Grid Size: %.2f", mainWorld.m_LMGridSize);

				FindWorldModels((CEditRegion*)g_InputRegion, worldModels);
				if(!ImportGeometry((CEditRegion*)g_InputRegion, worldModels, &mainWorld))
					goto Done;

				bLoaded = TRUE;
			}
			else if(g_pGlobs->m_bAIVolumes)
			{
				DrawStatusText("");
				DrawStatusText("-------------------");

				g_pGlobs->m_MaxLMSize = mainWorld.m_LMGridSize;
				mainWorld.RemoveAIVolumes();
				FindAIVolumes((CEditRegion*)g_InputRegion, worldModels);
				if(!AddWorldModels((CEditRegion*)g_InputRegion, worldModels, &mainWorld))
					goto Done;
			}

			if(g_pGlobs->m_bLight)
			{
				DrawStatusText("");
				DrawStatusText("-------------------");
				DrawStatusText("Lighting world");

				StartProgressStage(1);
				lightMapMaker.Init(&mainWorld, g_pGlobs->m_bClipLight, ProgressFn);
				lightMapMaker.Process();
				EndProgressStage(1, FALSE);
			}

			if(g_pGlobs->m_bOptimize)
			{
				DrawStatusText("");
				DrawStatusText("-------------------");
				DrawStatusText("Making PVS.");
				if(!MakeWorldPVS(&mainWorld))
					goto Done;
			}

SaveIt:
			if(g_pGlobs->m_PortalFile[0])
			{
				DrawStatusText("-------------------");
				DrawStatusText("PortalFile - saving %s", g_pGlobs->m_PortalFile);
				DrawStatusText("-------------------");
				SavePortalFile(&mainWorld, g_pGlobs->m_PortalFile);
			}

			DrawStatusText("Saving DAT file.");
			if(!SaveDATFile(&mainWorld, !bLoaded))
				goto Done;

			GetWorldStats(&mainWorld);
		}
		catch(CMemoryException *pException)
		{
			pException->Delete();
			throw CLithMemException();
		}
	}
	catch(CLithException &exception)
	{
		if(exception.GetExceptionType() == LITH_MEMEXCEPTION)
			DrawStatusText("Ran out of memory!");
		else
			DrawStatusText("Internal error!  (Old file format?)");
		goto Done;
	}

	endTime = ProcessorApp_GetTime();
	DrawStatusText("");
	DrawStatusText("-------------------");
	ShowStatusText(startTime, endTime);

Done:
	g_InputRegion->Term();
	delete g_InputRegion;

	thd_Term();
	CloseLogFile();
}


// Opens the log file, the first <world>_<n>.log not taken (0x441010).
void OpenLogFile()
{
	char	fileName[256];
	uint32	i;

	if(g_pGlobs->m_bLogFile)
	{
		for(i=0; i < 5000; i++)
		{
			sprintf(fileName, "%s_%d.log", g_InputFileTitle, i);
			if(!fopen(fileName, "rt"))
				break;
		}

		sprintf(fileName, "%s_%d.log", g_InputFileTitle, i);
		g_pLogFile = fopen(fileName, "wt");
	}
}


void CloseLogFile()
{
	if(g_pGlobs->m_bLogFile && g_pLogFile)
	{
		fclose(g_pLogFile);
		g_pLogFile = NULL;
	}
}


// Loads the ED file into g_InputRegion (0x4410c0).
BOOL LoadEDFile()
{
	CMoFileIO			file;
	char				fileName[256];
	uint32				i, version;
	RegionLoadStatus	status;
	CBaseEditObj		*pObj;
	LTVector			*pPos;

	sprintf(fileName, "%s.ed", g_pGlobs->m_InputFile);
	if(file.Open(fileName, "rb"))
	{
		status = g_InputRegion->LoadFile(file, &version, NULL);
		file.Close();

		// Objects outside the world break things.
		for(i=0; i < g_InputRegion->m_Objects; i++)
		{
			pObj = g_InputRegion->m_Objects[i];

			pPos = &pObj->GetPos();
			if(pPos)
			{
				if(pPos->x > g_pGlobs->m_MaxBounds || pPos->x < -g_pGlobs->m_MaxBounds ||
					pPos->y > g_pGlobs->m_MaxBounds || pPos->y < -g_pGlobs->m_MaxBounds ||
					pPos->z > g_pGlobs->m_MaxBounds || pPos->z < -g_pGlobs->m_MaxBounds ||
					!_finite(pPos->x) || !_finite(pPos->y) || !_finite(pPos->z))
				{
					DrawStatusText("* Object '%s' (class '%s') has an invalid position (%.2f %.2f %.2f).",
						pObj->GetName(), pObj->GetClassName(), pPos->x, pPos->y, pPos->z);
					no_DestroyNode((CEditRegion*)g_InputRegion, pObj, FALSE);
				}
			}
		}

		g_pGlobs->m_nInputPolies = g_InputRegion->GetTotalNumPolies();
		g_pGlobs->m_nInputVertices = g_InputRegion->GetTotalNumPoints();
		g_pGlobs->m_nInputObjects = g_InputRegion->m_Objects.GetSize();

		if(status == REGIONLOAD_INVALIDFILEVERSION)
		{
			DrawStatusText("Invalid ED file version (%d).", version);
			return FALSE;
		}
		else if(status == REGIONLOAD_INVALIDFILE)
		{
			DrawStatusText("The ED file is corrupt (error code %d).", g_InputRegion->m_LoadErrorCode);
			return FALSE;
		}

		g_InputRegion->ClearSelections();
		RemoveDetailBrushes(g_InputRegion);
		CheckRegion((CEditRegion*)g_InputRegion);
		return TRUE;
	}

	DrawStatusText("Can't open ED file: %s.", fileName);
	return FALSE;
}


// Removes the brushes with a detail level above MaxDetail (0x441370; the
// name is ours). Their children go to the root.
void RemoveDetailBrushes(CEditRegion2001 *pRegion)
{
	LPOS		pos, curPos;
	CEditBrush	*pBrush;
	GPOS		childPos;

	for(pos=pRegion->m_Brushes; pos; )
	{
		curPos = pos;
		pBrush = pRegion->m_Brushes.GetNext(pos);

		if(GetDetailLevel(pBrush) > g_pGlobs->m_MaxDetail)
		{
			for(childPos=pBrush->m_Children; childPos; )
				pRegion->DetachNode(pBrush->m_Children.GetNext(childPos), TRUE);

			pRegion->DetachNode(pBrush, FALSE);
			pRegion->m_Brushes.RemoveAt(curPos);
			delete pBrush;
		}
	}
}


// Loads the DAT file (0x441470). *pbLoaded tells if it could.
BOOL LoadDATFile(CPreMainWorld *pMainWorld, BOOL bWarn, BOOL *pbLoaded, BOOL bLoadObjects)
{
	CMoFileIO	file;
	uint32		status;

	*pbLoaded = FALSE;
	pMainWorld->Term();

	if(!file.Open(g_pGlobs->m_OutputFile, "rb"))
	{
		if(bWarn)
			DrawStatusText("Error loading %s.", g_pGlobs->m_OutputFile);

		return FALSE;
	}

	status = pMainWorld->LoadFile(file, bLoadObjects);
	if(status != 0)
	{
		file.Close();
		if(status == 1)
			DrawStatusText("DAT file %s is an invalid version.", g_pGlobs->m_OutputFile);
		else
			DrawStatusText("DAT file %s is corrupt.", g_pGlobs->m_OutputFile);

		return FALSE;
	}

	*pbLoaded = TRUE;
	file.Close();
	return TRUE;
}


// Saves the DAT file through a temporary file (0x4415a0). With
// bObjectsOnly, only the objects in the old DAT file are replaced.
BOOL SaveDATFile(CPreMainWorld *pMainWorld, BOOL bObjectsOnly)
{
	CMoFileIO	outFile;
	char		tempFileName[200];
	char		msg[512];
	uint32		len, response;
	BOOL		bRet;

	strncpy(tempFileName, g_pGlobs->m_OutputFile, 200);
	tempFileName[199] = 0;

	len = strlen(tempFileName);
	if((int)len < 3)
		len = 5;
	strcpy(&tempFileName[len-4], ".tmp");

	// talon: while(TRUE), not for(;;): the loop is not rotated, the retry
	// jumps back to the Open (0x44166a).
	while(TRUE)
	{
		if(outFile.Open(tempFileName, "wb"))
		{
			if(!bObjectsOnly)
			{
				bRet = pMainWorld->SaveFile(outFile);
			}
			else
			{
				CMoFileIO inFile;

				if(!inFile.Open(g_pGlobs->m_OutputFile, "rb"))
				{
					bRet = FALSE;
					goto Done;
				}

				bRet = pMainWorld->SaveObjectsOnly(outFile, inFile);
			}

			if(!bRet)
				DrawStatusText("ERROR IN CPreWorld::SaveFile!");
			break;
		}

		response = AskQuestion("Error opening output file.  Try again?", QUES_YES|QUES_NO|QUES_CANCEL);
		if(response == QUES_CANCEL)
		{
			bRet = FALSE;
			break;
		}
		else if(response == QUES_NO)
		{
			// RETAIL BUG: bRet is not set on this path (0x44170d loads its
			// stack slot, never written), so whether the rename below is
			// tried and what the caller is told is whatever was there.
			break;
		}
	}

Done:
	outFile.Close();

	if(bRet)
	{
		do
		{
			if(remove(g_pGlobs->m_OutputFile) != 0)
			{
				_snprintf(msg, sizeof(msg), "Could not remove \"%s\", %s.",
					g_pGlobs->m_OutputFile, _sys_errlist[errno]);
				msg[sizeof(msg)-1] = 0;
			}

			if(rename(tempFileName, g_pGlobs->m_OutputFile) == 0)
				break;

			// RETAIL BUG: msg is only set if the remove failed, so a failed
			// rename after a good remove prints the stack's garbage first;
			// and each retry puts the last message in front of the new one
			// (msg is formatted into itself, 0x4417c5).
			_snprintf(msg, sizeof(msg), "%s\nCould not rename \"%s\" to \"%s\", %s.\nTry again?\n(If you answer no, output file left as \"%s\".)",
				msg, tempFileName, g_pGlobs->m_OutputFile, _sys_errlist[errno], tempFileName);
			msg[sizeof(msg)-1] = 0;

			response = AskQuestion(msg, QUES_YES|QUES_NO);
			DrawStatusText("%s  %s", msg, (response == QUES_NO) ? "No." : "Yes.");
		}
		while(response != QUES_NO);
	}

	return bRet;
}


void ShowStatusText(DWORD startTime, DWORD endTime)
{
	DrawStatusText("Done in %.2f minutes", ((endTime - startTime) / 1000.0f) / 60.0f);
	
	DrawStatusText("");
	
	DrawStatusText("Number of input polies: %d", g_pGlobs->m_nInputPolies);
	DrawStatusText("Number of input vertices: %d", g_pGlobs->m_nInputVertices);
	
	DrawStatusText("");
	
	DrawStatusText("Number of output polies: %d", g_pGlobs->m_nOutputPolies);
	DrawStatusText("Number of output vertices: %d", g_pGlobs->m_nOutputVertices);
	DrawStatusText("Tree depth: %d", g_pGlobs->m_TreeDepth);
	DrawStatusText("Memory for vis lists: %d", g_pGlobs->m_VisListMemory);
	DrawStatusText("Number of (unseen) polies removed: %d", g_pGlobs->m_nPoliesRemoved);

	DrawStatusText("");

	DrawStatusText("Number of leaves: %d", g_pGlobs->m_nLeaves);
	DrawStatusText("Number of visible leaves: %d", g_pGlobs->m_nVisibleLeaves);

	DrawStatusText("");

	DrawStatusText("Lightmap data size: %d", g_pGlobs->m_LMDataSize);
	DrawStatusText("Lightmap compression ratio: %.2f", g_pGlobs->m_LMCompressionRatio);
	DrawStatusText("Number of objects: %d", g_pGlobs->m_nInputObjects);

	DrawStatusText("");

	DrawStatusText("Light table size (from LightTableRes): %dM %dk",
		g_pGlobs->m_LightTableSize >> 20, ((g_pGlobs->m_LightTableSize & 0xFFFFF) + 512) >> 10);

	DrawStatusText("");
}


// Writes the vis BSP's leaves, portals and visibility (PortalFile;
// 0x441a50, the name is ours).
BOOL SavePortalFile(CPreMainWorld *pMainWorld, const char *pFilename)
{
	CPreWorld	*pWorld;
	CPreLeaf	*pLeaf;
	CPrePortal	*pPortal;
	FILE		*fp;
	uint32		i, j, k, nLeaves;

	pWorld = pMainWorld->GetVisBSP();
	if(!pWorld)
		return FALSE;

	nLeaves = pWorld->m_Leaves.GetSize();

	fp = fopen(pFilename, "wt");
	if(fp)
	{
		fprintf(fp, "File Version: %d\n\n", 1);
		fprintf(fp, "Leaves: %d\n", nLeaves);

		for(i=0; i < pWorld->m_Leaves; i++)
		{
			pLeaf = pWorld->m_Leaves[i];

			fprintf(fp, "\n\nLeaf #%lu\n\n", i);
			fprintf(fp, "  Polies: %d\n", pLeaf->m_Portals.GetSize());

			for(j=0; j < pLeaf->m_Portals; j++)
			{
				pPortal = pLeaf->m_Portals[j];

				if(!(pPortal->m_Flags & PORTALF_BLOCKER) && pPortal->m_pPartner && pPortal->m_pPartner->m_pLeaf)
					fprintf(fp, "    Connection: %lu\n", pWorld->m_Leaves.FindElement(pPortal->m_pPartner->m_pLeaf));
				else
					fprintf(fp, "    Connection: %lu\n", BAD_INDEX);

				fprintf(fp, "    Verts: %d\n", pPortal->NumVerts());
				for(k=0; k < pPortal->NumVerts(); k++)
				{
					fprintf(fp, "      %f %f %f\n", VEC_EXPAND(pPortal->Pt(k)));
				}
			}

			if(pLeaf->m_Unknown_6c.GetSize() == 0)
			{
				fputs("    Visibility: \n", fp);
			}
			else
			{
				fputs("    Visibility: ", fp);
				uint32 bit = 0;
				WriteVisibleLeaves(pLeaf->m_Unknown_6c[0], &bit, pWorld->m_RootNode, fp);
				fputc('\n', fp);
			}

			fprintf(fp, "    Center: %f %f %f\n", VEC_EXPAND(pLeaf->m_vCenter));
		}

		fclose(fp);
	}

	return TRUE;
}


// The leaves a visibility list sees, in the order of a walk down the BSP
// (0x441ca0; the name is ours).
void WriteVisibleLeaves(CPreLeafList *pList, uint32 *pBit, CNode *pNode, FILE *fp)
{
	CNode *pFront, *pBack;

	if(pNode->m_pLeaf && ReadVisBit(pList, pBit))
		WriteLeafIndex(pNode, fp);

	pFront = pNode->m_Sides[0];
	pBack = pNode->m_Sides[1];

	if(IsValidNode(pFront) && ReadVisBit(pList, pBit))
		WriteVisibleLeaves(pList, pBit, pFront, fp);

	if(IsValidNode(pBack) && ReadVisBit(pList, pBit))
		WriteVisibleLeaves(pList, pBit, pBack, fp);
}
