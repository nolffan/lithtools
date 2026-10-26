// talon: a module of its own in the image, 0x00446170..0x004479c0, after Processing:
// terrain: TerrainSubDivSize, 'Creating terrain', merging terrain polies, terrain sections and their BSPs (the name ours).
// Written from the image (no source of any version survives: Build 69
// dropped terrain). Then the template code the module emits
// (0x446fc0..0x447220) and three helpers of the T-junction fixer that sit
// ahead of it (0x447220..0x4479c0), named by address: TJunctions.cpp's
// FixTJunctions (0x4479c0) calls them.

#include "bdefs.h"
#include "processing.h"
#include "editregion.h"
#include "editpoly.h"
#include "preworld.h"
#include "bspgen.h"
#include "brushtoworld.h"
#include "findworldmodel.h"
#include "conparse.h"
#include "splitpoly.h"
#include "node.h"


// talon: other modules' interfaces, provisional until those modules are
// written (the names are ours).

// create_world_tree.cpp (0x40a610).
BOOL DoesPolyIntersectNode(CPrePoly *pPoly, WorldTreeNode *pNode);

// RaySelecter.cpp (0x445b80): merges pWorld's surfaces (the
// caller prints how many went).
void MergeSurfaces_RS(CPreWorld *pWorld);

// The physics module (0x425d20): the physics blocks of a BSP.
void MakePhysicsBlocks_425d20(CPreWorld *pWorld, BOOL bUnknown, char *pInfoString, BOOL bUnknown2);

// The T-junction fixer (0x4479c0), as TJunctions_decl.h has it (its
// FixTJunctions returns nothing) but for m_Unknown_18, which is not a
// CMoArray<CPrePoly*>: its vtable (0x47575c, stored at 0x446d53) is not
// that one's (0x4746b8: CPreSubBsp::m_Polies's, 0x433b02), and only as
// another type does its inline destructor here share the failure paths'
// tail as the shipped code does (0x446d46).
class CTJunctionFixer
{
public:
	void					FixTJunctions(CPreWorld *pWorld, BOOL bSplitPolys);

	uint32							m_Unknown_00;
	CMoArray< CMoArray<uint32> >	m_Unknown_04;
	CMoArray<uint32>				m_Unknown_18;
};


// talon: the terrain section size (0x48577c), and the BSP being made of
// how many (0x485780, 0x485784), for the progress bar.
PReal	g_TerrainSubDivSize;
uint32	g_iCurTerrainBsp;
uint32	g_nTerrainBsps;


// Terrain sections are no smaller than this (0x446170).
PReal GetTerrainSubDivSize(char *pInfoString)
{
	ConParse	parse;
	PReal		ret;

	parse.Init(pInfoString);
	if(parse.ParseFind("TerrainSubDivSize", FALSE, 1))
	{
		ret = (PReal)atof(parse.m_Args[1]);
		return DCLAMP(ret, (PReal)1.0, (PReal)40000.0);
	}
	else
	{
		return (PReal)1000.0;
	}
}


// Makes a terrain section (a CPreSubBsp in pWorld->m_PolyArray) for each
// world tree node under pNode no smaller than g_TerrainSubDivSize, with the
// polies that touch it (0x4461f0; the image's 0x44626b row is its interior).
// Near, 826 bytes against 824: register allocation in childPolies.Append's
// inline Insert2 (0x4462f1: j*4 in edi, &polies[j] in esi, newSize in ebx;
// ours j*4 in esi and one more mov). The leaf case is written first: with
// the recursion first, Insert2 stays a call and CopyArray2 is expanded,
// the shipped code's opposite.
void MakeTerrainSections_R(WorldTree *pTree, CPreWorld *pWorld, WorldTreeNode *pNode,
	CMoArray<CPrePoly*> &polies, uint32 depth, uint32 *pMaxDepth)
{
	CMoArray<CPrePoly*>	childPolies;
	NodePath			path;
	CPreSubBsp			*pSubBsp;
	uint32				j, i;

	if(polies.GetSize() == 0)
		return;

	if(pNode->m_MinSize * 0.5f < g_TerrainSubDivSize || !pNode->HasChildren())
	{
		if(pTree->GetNodePath(pNode, &path))
		{
			pSubBsp = new CPreSubBsp;
			pSubBsp->m_Unknown_2c = (uint32)pWorld;
			// talon: CPreSubBsp's first 8 bytes are a NodePath.
			*((NodePath*)pSubBsp->m_Unknown_00) = path;
			pSubBsp->m_Polies.CopyArray(polies);
			pSubBsp->m_Unknown_30 = pNode->m_vCenter;
			pWorld->m_PolyArray.Append(pSubBsp);

			if(depth > *pMaxDepth)
				*pMaxDepth = depth;
		}
		else
		{
			DrawStatusText("****ERROR: can't get node path!");
		}
	}
	else
	{
		for(i=0; i < MAX_WTNODE_CHILDREN; i++)
		{
			childPolies.Term();
			childPolies.SetCacheSize(4096);

			for(j=0; j < polies; j++)
			{
				if(DoesPolyIntersectNode(polies[j], pNode->m_ChildrenA[i]))
					childPolies.Append(polies[j]);
			}

			MakeTerrainSections_R(pTree, pWorld, pNode->m_ChildrenA[i], childPolies, depth+1, pMaxDepth);
		}
	}
}


// The BSPs' progress (0x446f80).
void TerrainBspProgressFn(CBspGen *pBspGen, CReal fPercent)
{
	SetProgressBar((g_iCurTerrainBsp + fPercent) / g_nTerrainBsps);
}


// talon: the 2001 PBlockTable's copy and Clear, inline in MakeTerrain
// (0x446b13..0x446c04): the header's Clear is PreWorld.cpp's (0x433b80).
inline void TakePBlocks(PBlockTable &dest, PBlockTable &src)
{
	dest.m_vMin			= src.m_vMin;
	dest.m_Unknown_0c	= src.m_Unknown_0c;
	dest.m_nBlocks[0]	= src.m_nBlocks[0];
	dest.m_nBlocks[1]	= src.m_nBlocks[1];
	dest.m_nBlocks[2]	= src.m_nBlocks[2];
	dest.m_nBlocksXY	= src.m_nBlocksXY;
	dest.m_nBlocksTotal	= src.m_nBlocksTotal;
	dest.m_pBlocks		= src.m_pBlocks;

	src.m_vMin.Init();
	src.m_Unknown_0c.Init();
	src.m_nBlocks[0] = src.m_nBlocks[1] = src.m_nBlocks[2] = 0;
	src.m_nBlocksXY = 0;
	src.m_nBlocksTotal = 0;
	src.m_pBlocks = NULL;
}


// A terrain world model (0x446530): its polies split into sections on the
// world tree, a BSP for each, then the world's T-junctions fixed.
// Near (2703 bytes against 2634), the calls in the shipped order but for
// the inline budget: the shipped code calls m_Unknown_0c.Init (0x446bce)
// and, at the end, ObjectBank::Free, AddHead (0x446de3), allPolies' SetSize2
// and polies' and the fixer's _DeleteAndDestroyArray, where ours expands
// them; and it places the allPolies.SetSize failure path (0x44696f) after
// the BSP loop's entry test, ours after the test. The rest is registers.
// Append, not AddTail, in the brush and section loops: it leaves AddTail
// (0x4466bd, 0x4466e4) and InsertAfter (0x446a73) calls, as shipped.
BOOL MakeTerrain(CEditRegion *pRegion, CWorldModelDef *pDef, CPreWorld *pWorld, char *pInfoString)
{
	CTJunctionFixer				tjFixer;
	CMoArray<CPrePoly*>			polies;
	CMoArray<CPrePoly*>			allPolies;
	CBspGenOptions				options;
	CBspGen						bspGen;
	CLinkedList<CEditBrush*>	occluders;
	CPreWorld					tempWorld(pWorld->m_pMainWorld);
	CEditBrush					*pBrush;
	CPrePoly					*pPoly;
	CPreSurface					*pSurface;
	CPreSubBsp					*pSubBsp;
	GPOS						pos;
	uint32						i, j, nBefore, nPolies, nTotal;
	BOOL						bWarned, bOldMakingWorldModel;

	g_TerrainSubDivSize = GetTerrainSubDivSize(pInfoString);
	DrawStatusText("## Creating terrain '%s'.", pWorld->m_WorldName);

	for(i=0; i < pDef->m_Brushes; i++)
	{
		pBrush = pDef->m_Brushes[i];
		if(GetBrushType(pBrush) & (SURF_HULLMAKER|SURF_TERRAINOCCLUDER))
		{
			occluders.Append(pBrush);
		}
		else
		{
			for(j=0; j < pBrush->m_Polies; j++)
			{
				pPoly = EditPolyToPrePoly(pBrush->m_Polies[j]);
				if(pPoly)
					pWorld->m_Polies.Append(pPoly);
			}
		}
	}

	pWorld->InitPolyVertexIndices();

	nBefore = pWorld->m_Surfaces.GetSize();
	MergeSurfaces_RS(pWorld);
	DrawStatusText("%d surfaces merged.", nBefore - pWorld->m_Surfaces.GetSize());

	nBefore = pWorld->m_Polies.GetSize();
	pWorld->JoinPolies(0.0001f);
	DrawStatusText("%d terrain polies merged.", nBefore - pWorld->m_Polies.GetSize());
	DrawStatusText("%d polies left.", pWorld->m_Polies.GetSize());

	// Terrain is never subdivided.
	bWarned = FALSE;
	for(pos=pWorld->m_Surfaces; pos; )
	{
		pSurface = pWorld->m_Surfaces.GetNext(pos);
		if((pSurface->m_Flags & SURF_LIGHTMAP) && !bWarned)
		{
			bWarned = TRUE;
			DrawStatusText("** Found terrain surface(s) with Lightmap TRUE.  Lightmaps will overflow on large polies..");
		}

		pSurface->m_Flags |= SURF_NOSUBDIV;
	}

	// The polies that go in the sections.
	nPolies = 0;
	polies.SetSize(pWorld->m_Polies.GetSize());
	for(pos=pWorld->m_Polies; pos; )
	{
		pPoly = pWorld->m_Polies.GetNext(pos);
		if(!(pPoly->GetSurfaceFlags() & SURF_HULLMAKER) && !(pPoly->GetSurfaceFlags() & SURF_TERRAINOCCLUDER))
		{
			polies[nPolies] = pPoly;
			nPolies++;
		}
	}
	polies.NiceSetSize(nPolies);

	MakeTerrainSections_R(&pWorld->m_pMainWorld->m_WorldTree, pWorld,
		pWorld->m_pMainWorld->m_WorldTree.GetRootNode(), polies, 0,
		&pWorld->m_pMainWorld->m_WorldTree.m_TerrainDepth);

	nTotal = 0;
	for(i=0; i < pWorld->m_PolyArray; i++)
		nTotal += pWorld->m_PolyArray[i]->m_Polies.GetSize();

	DrawStatusText("%d terrain sections (avg. %.1f polies/section)",
		pWorld->m_PolyArray.GetSize(), (float)pWorld->m_Polies.GetSize() / pWorld->m_PolyArray.GetSize());
	DrawStatusText("%.1fx poly overlap (%d extra polies)",
		(float)nTotal / pWorld->m_Polies.GetSize(), nTotal - pWorld->m_Polies.GetSize());

	// Every poly, to put back in the world after the BSPs.
	if(!allPolies.SetSize(pWorld->m_Polies.GetSize()))
		return FALSE;

	i = 0;
	for(pos=pWorld->m_Polies; pos; )
	{
		allPolies[i] = pWorld->m_Polies.GetNext(pos);
		i++;
	}

	StartProgressStage(PROGRESS_BSP);
	DrawStatusText("Generating %d terrain BSPs", pWorld->m_PolyArray.GetSize());
	bspGen.m_ProgressFn = TerrainBspProgressFn;
	g_nTerrainBsps = pWorld->m_PolyArray.GetSize();

	bOldMakingWorldModel = g_pGlobs->m_bMakingWorldModel;
	g_pGlobs->m_bMakingWorldModel = TRUE;

	for(i=0; i < pWorld->m_PolyArray; i++)
	{
		pSubBsp = pWorld->m_PolyArray[i];
		g_iCurTerrainBsp = i;

		tempWorld.m_Polies.RemoveAll();
		for(j=0; j < pSubBsp->m_Polies; j++)
			tempWorld.m_Polies.Append(pSubBsp->m_Polies[j]);

		options.m_pWorld				= &tempWorld;
		options.m_bMinimizePolies		= FALSE;
		options.m_bHullMakers			= FALSE;
		options.m_bPostProcess			= TRUE;
		options.m_nThreadsToUse			= 0;
		options.m_bUseOriginalPolies	= FALSE;
		options.m_bInitVertexIndices	= FALSE;
		if(!bspGen.GenerateBspTree(&options))
		{
			DrawStatusText("CBspGen::GenerateBspTree returned FALSE");
			return FALSE;
		}

		MakePhysicsBlocks_425d20(&tempWorld, TRUE, "", FALSE);

		// The section takes the BSP.
		pSubBsp->m_Nodes.StealList(tempWorld.m_Nodes);
		pSubBsp->m_RootNode = tempWorld.m_RootNode;
		TakePBlocks(pSubBsp->m_PBlocks, tempWorld.m_PBlocks);

		tempWorld.m_Polies.RemoveAll();
		tempWorld.TermGeometry();
	}

	g_pGlobs->m_bMakingWorldModel = bOldMakingWorldModel;
	EndProgressStage(PROGRESS_BSP, FALSE);

	pWorld->m_Polies.RemoveAll();
	for(i=0; i < allPolies; i++)
		pWorld->m_Polies.AddTail(allPolies[i]);

	tjFixer.FixTJunctions(pWorld, g_pGlobs->m_bFixTJunc);
	return TRUE;
}



// ------------------------------------------------------------------------ //
// talon: three helpers of the T-junction fixer (TJunctions.cpp's
// FixTJunctions, 0x4479c0, calls them; nothing else does). They sit here,
// after the template code of this module, so they are in this file; named
// by address.
// ------------------------------------------------------------------------ //

// talon: Build 69's CPrePoly::OverlapExtents, inline in 0x447220 (not in
// our header: it moves BrushToWorld's declaration count).
inline bool OverlapExtents_447220(const CPrePoly &thisPoly, const CPrePoly &pOtherPoly)
{
	return	(pOtherPoly.m_vExtentsMin.x < thisPoly.m_vExtentsMax.x) && (pOtherPoly.m_vExtentsMax.x > thisPoly.m_vExtentsMin.x) &&
			(pOtherPoly.m_vExtentsMin.y < thisPoly.m_vExtentsMax.y) && (pOtherPoly.m_vExtentsMax.y > thisPoly.m_vExtentsMin.y) &&
			(pOtherPoly.m_vExtentsMin.z < thisPoly.m_vExtentsMax.z) && (pOtherPoly.m_vExtentsMax.z > thisPoly.m_vExtentsMin.z);
}

// Adds to touching each poly of the BSP under pRoot that lies on pPoly's
// plane and overlaps its extents (0x447220). Near, 898 bytes against 888:
// the shipped code calls Insert2's _DeleteAndDestroyArray (0x4474e1), ours
// expands it (inline budget; Dot, 0x4472b9, is a call in both only with
// OverlapExtents an inline function).
void sub_447220(NODEREF pRoot, CPrePoly *pPoly, CMoArray<CPrePoly*> &touching)
{
	static SplitStruct	splitStruct;
	static PolySide		side;

	if(!IsValidNode(pRoot))
		return;

	if(pRoot->m_pPoly == pPoly)
	{
		side = Intersect;
	}
	else
	{
		side = GetPolySide(pRoot->m_pPoly->GetPlane(), pPoly, &splitStruct);
		if(splitStruct.m_nIntersect > 0)
		{
			if(OverlapExtents_447220(*pPoly, *pRoot->m_pPoly))
			{
				if(touching.FindElement(pRoot->m_pPoly) == BAD_INDEX)
					touching.Append(pRoot->m_pPoly);
			}

			side = Intersect;
		}
	}

	if(side == Intersect)
	{
		sub_447220(pRoot->m_Sides[0], pPoly, touching);
		sub_447220(pRoot->m_Sides[1], pPoly, touching);
	}
	else
	{
		sub_447220(pRoot->m_Sides[side], pPoly, touching);
	}
}


// Splits pToSplit in two at pPt, the point of a T-junction on one of its
// edges, as GetPolySide classified its points (0x4475a0). The new vertex is
// pPt itself; t, for its normal, is its distance from the edge's start
// over its distance from the edge's end.
BOOL sub_4475a0(PVector *pPt, CPrePoly *pToSplit, CPrePoly *sides[2], SplitStruct *pStruct)
{
	uint32		iCur, iNext, nFront, nBack, nEdgeSplits;
	PolySide	curSide, nextSide;
	PVector		vNormal;
	PReal		t;

	nEdgeSplits = 0;
	nFront = pStruct->m_nFront + pStruct->m_nIntersect + 2;
	nBack = pStruct->m_nBack + pStruct->m_nIntersect + 2;

	sides[FrontSide] = CreatePoly(CPrePoly, nFront, FALSE);
	sides[BackSide]  = CreatePoly(CPrePoly, nBack, FALSE);

	nextSide = pStruct->m_PointSides[0];
	for(iCur=0; iCur < pToSplit->NumVerts(); iCur++)
	{
		iNext = (iCur+1) % pToSplit->NumVerts();

		curSide = nextSide;
		nextSide = pStruct->m_PointSides[iNext];

		if(curSide == Intersect)
		{
			sides[0]->AddVert(pToSplit->Pt(iCur), pToSplit->PtNormal(iCur));
			sides[1]->AddVert(pToSplit->Pt(iCur), pToSplit->PtNormal(iCur));
			continue;
		}

		sides[curSide]->AddVert(pToSplit->Pt(iCur), pToSplit->PtNormal(iCur));
		if(nextSide == Intersect || nextSide == curSide)
			continue;

		++nEdgeSplits;
		if(nEdgeSplits > 1)
		{
			DrawStatusText("*** Bad poly found! > 1 edge split! (%0.1f %0.1f %0.1f)", VEC_EXPAND(pToSplit->Pt(iCur)));
			break;
		}

		PVector &A = pToSplit->Pt(iCur);
		PVector &B = pToSplit->Pt(iNext);

		t = (*pPt - A).Mag() / (*pPt - B).Mag();

		VEC_LERP(vNormal, pToSplit->PtNormal(iCur), pToSplit->PtNormal(iNext), t);
		vNormal.Norm();

		sides[0]->AddVert(*pPt, vNormal);
		sides[1]->AddVert(*pPt, vNormal);
	}

	sides[0]->CopySplitAttributes(pToSplit);
	sides[1]->CopySplitAttributes(pToSplit);

	return nEdgeSplits <= 2;
}


// Replaces pOld with pNew1 in the nodes of nodes that have it, and adds a
// node for pNew2 on each one's front side (0x447940). pWorld is not used.
void sub_447940(CPreWorld *pWorld, CNodeList &nodes, CPrePoly *pOld, CPrePoly *pNew1, CPrePoly *pNew2)
{
	GPOS	pos;
	CNode	*pNode, *pNewNode;

	for(pos=nodes; pos; )
	{
		pNode = nodes.GetAt(pos);
		if(pNode->m_pPoly == pOld)
		{
			pNode->m_pPoly = pNew1;

			pNewNode = AddNode(nodes, pNew2->GetPlane(), pNew2);
			pNewNode->m_Sides[1] = pNode->m_Sides[1];
			pNewNode->m_Sides[0] = NODE_OUT;
			pNewNode->m_pParentNode = pNode;
			if(IsValidNode(pNode->m_Sides[1]))
				pNode->m_Sides[1]->m_pParentNode = pNewNode;

			pNode->m_Sides[1] = pNewNode;
		}

		nodes.GetNext(pos);
	}
}
