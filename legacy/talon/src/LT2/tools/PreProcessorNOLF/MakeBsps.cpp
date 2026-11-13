// talon: 0x0040fa60..0x00411dd0, after create_world_tree: making a world
// model's (and the physics) BSP, the visibility BSP and its leaves'
// polies, selecting the problem and out-of-bounds brushes. Written from the
// AvP2 processor's image and brought back to NOLF's (the addresses here are
// NOLF's); Build 69 has none of it. The file's name and every function name
// here are ours.
//
// The layout looks like two source files: the world model half
// (0x40fa60..0x410570) is followed by its inline copies (0x4101b0..), then
// the vis BSP half (GetVisBspBrushes 0x410570..MakeVisBSP) by its own
// (0x411d30, 0x411db0). Kept as one file until the split is shown.

#include "bdefs.h"
#include "editregion.h"
#include "editbrush.h"
#include "processing.h"
#include "preworld.h"
#include "preleaf.h"
#include "bspgen.h"
#include "buildhulls.h"
#include "findworldmodel.h"
#include "splitpoly.h"
#include "node.h"


// talon: other modules' interfaces, provisional until those modules are
// written (the names are ours).

// Processing.cpp.
BOOL BuildWorldHulls(CPreWorld *pWorld, CHullInfo *pInfo);	// 0x44e0b0
BOOL MakeHullsAndRemoveUnseen(CPreWorld *pWorld);			// 0x44e3c0
void FixWorldTJunctions(CPreWorld *pWorld);					// 0x44e560
BOOL SaveEDFile();											// 0x44dbc0

// LeakFile.cpp (0x420be0).
BOOL MakeLeakFile(CPreWorld *pWorld, const char *pFilename);

// The physics module (0x432a40): the physics blocks of a BSP.
void MakePhysicsBlocks_425d20(CPreWorld *pWorld, BOOL bUnknown, char *pInfoString, BOOL bUnknown2);


// talon: a walk of the BSP from a node down (0x410320, thiscall on the
// node) that does nothing in this build: it stops at NODE_IN and NODE_OUT
// (not at NULL) and touches nothing else. Inline: the copy expands its own
// first level, and the EH-framed callers call it (0x40fef6, 0x411bba). A
// stand-in on CNode's own class, whose header is shared.
class CMBNode : public CNode
{
public:
	void Walk_40bab0()
	{
		if(this == NODE_IN || this == NODE_OUT)
			return;

		((CMBNode*)m_Sides[0])->Walk_40bab0();
		((CMBNode*)m_Sides[1])->Walk_40bab0();
	}
};


BOOL SelectProblemBrushes(CEditRegion *pRegion);
BOOL SelectOutOfBoundsBrushes(CEditRegion *pRegion);
void GetBspBrushes(GenList<CEditBrush*> &brushes, GenList<CEditBrush*> &bspBrushes);
void RemoveTerrainOccluderPolies(CPreWorld *pWorld);
void FilterTerrainPolies_R(CPreWorld *pWorld, NODEREF pNode, GenList<CPrePoly*> &newPolies, GenList<CPrePoly*> &polies);
void FilterPolyIntoLeaves_R(CPrePoly *pPoly, CPrePoly *pOriginal, NODEREF pNode);


// Takes the polies on SURF_TERRAINOCCLUDER surfaces out of the world's
// list (0x40fa60). They are not freed. Through GenList: the shipped calls
// are virtual.
// talon: through pWorld->m_Polies each time, no reference: 0x40fa60
// recomputes the list's address for every call
void RemoveTerrainOccluderPolies(CPreWorld *pWorld)
{
	GenListPos			pos, curPos;
	CPrePoly			*pPoly;

	for(pos=pWorld->m_Polies.GenBegin(); pWorld->m_Polies.GenIsValid(pos); )
	{
		curPos = pos;
		pPoly = pWorld->m_Polies.GenGetNext(pos);
		if(pPoly->GetSurfaceFlags() & SURF_TERRAINOCCLUDER)
			pWorld->m_Polies.GenRemoveAt(curPos);
	}
}


// -SelectProblemBrushes: selects the brushes the brush checks found
// problems in, and the out-of-bounds ones, and saves the ED file (0x40fb10).
BOOL SelectProblemBrushes(CEditRegion *pRegion)
{
	uint32 i;

	DrawStatusText("");
	DrawStatusText("---------------");
	DrawStatusText("Selecting %d problem brushes", g_ProblemBrushes.GetSize() + g_OutOfBoundsBrushes.GetSize());
	DrawStatusText("---------------");

	for(i=0; i < g_ProblemBrushes; i++)
		pRegion->SelectNode(g_ProblemBrushes[i]);

	for(i=0; i < g_OutOfBoundsBrushes; i++)
		pRegion->SelectNode(g_OutOfBoundsBrushes[i]);

	return SaveEDFile();
}


// -SelectOutOfBoundsBrushes: selects the out-of-bounds brushes and saves
// the ED file (0x40fbd0).
BOOL SelectOutOfBoundsBrushes(CEditRegion *pRegion)
{
	uint32 i;

	DrawStatusText("");
	DrawStatusText("---------------");
	DrawStatusText("Selecting %d out-of-bounds brushes", g_OutOfBoundsBrushes.GetSize());
	DrawStatusText("---------------");

	for(i=0; i < g_OutOfBoundsBrushes; i++)
		pRegion->SelectNode(g_OutOfBoundsBrushes[i]);

	return SaveEDFile();
}


// The brushes that go into a world model's BSP: all but the hull makers
// and terrain occluders (0x40fc60).
void GetBspBrushes(GenList<CEditBrush*> &brushes, GenList<CEditBrush*> &bspBrushes)
{
	GenListPos	pos;
	CEditBrush	*pBrush;

	for(pos=brushes.GenBegin(); brushes.GenIsValid(pos); )
	{
		pBrush = brushes.GenGetNext(pos);
		if(!(GetBrushType(pBrush) & (SURF_HULLMAKER|SURF_TERRAINOCCLUDER)))
			bspBrushes.GenAppend(pBrush);
	}
}


// A world model's BSP (0x40fcf0, a $$1 pair): bPhysicsBSP for the physics BSP, which
// also loses the polies no leaf sees. Brushes the brush checks found
// problems in stop it with SelectProblemBrushes or
// SelectOutOfBoundsBrushes (after saving the ED file with them selected).
BOOL MakeWorldModelBSP(CEditRegion *pRegion, CWorldModelDef *pDef, CPreWorld *pWorld, BOOL bPhysicsBSP)
{
	// talon: an initialized static (0x489420, TRUE, read only here), passed
	// as MakeBsp's bUseOriginalPolies; the name is ours.
	static BOOL					bUseOriginalPolies = TRUE;
	CLinkedList<CEditBrush*>	brushes;
	PVector						vMin, vMax;
	CEditBrush					*pBrush;
	uint32						i;

	GetBspBrushes(pDef->m_Brushes, brushes);
	GenerateSurfaces(brushes, pWorld);
	if(!MergeBrushes(brushes, pWorld))
		return FALSE;

	// talon: GenRemoveAll, not RemoveAll: both are 0x40a820, but only this
	// leaves the inline budget the shipped code has (the destructors'
	// RemoveAll calls at the early returns, expanded at the end).
	brushes.GenRemoveAll();

	DrawStatusTextIfMainWorld("Found %d problem brushes", g_ProblemBrushes.GetSize());
	if(g_pGlobs->m_bSelectProblemBrushes)
	{
		SelectProblemBrushes(pRegion);
		return FALSE;
	}

	if(g_pGlobs->m_bSelectOutOfBoundsBrushes)
	{
		SelectOutOfBoundsBrushes(pRegion);
		return FALSE;
	}

	for(i=0; i < g_ProblemBrushes; i++)
	{
		pBrush = g_ProblemBrushes[i];
		if(pBrush->m_Points.GetSize() == 0)
			DrawStatusTextIfMainWorld("  #%d : <Empty>", i+1);
		else
			DrawStatusTextIfMainWorld("  #%d : %0.1f %0.1f %0.1f", i+1, pBrush->m_Points[0].x, pBrush->m_Points[0].y, pBrush->m_Points[0].z);
	}

	DrawStatusTextIfMainWorld("Creating BSP.");
	if(!MakeBsp(pWorld, FALSE, FALSE, FALSE))
		return FALSE;

	((CMBNode*)pWorld->m_RootNode)->Walk_40bab0();

	if(g_pGlobs->m_LeakFile[0] && (pWorld->m_WorldInfoFlags & WIF_MAINWORLD) && !(pWorld->m_WorldInfoFlags & WIF_TERRAIN))
	{
		BuildWorldHulls(pWorld, NULL);
		DrawStatusText("Making PhysicsBSP leak file.");
		MakeLeakFile(pWorld, g_pGlobs->m_LeakFile);
	}

	if(bPhysicsBSP)
	{
		DrawStatusTextIfMainWorld("Removing unseen geometry.");
		if(!MakeHullsAndRemoveUnseen(pWorld))
			return FALSE;
	}

	RemoveTerrainOccluderPolies(pWorld);
	pWorld->JoinPolies(0.0001f);
	pWorld->TermLeaves();
	pWorld->RemoveAllNodes();

	DrawStatusTextIfMainWorld("Recreating BSP.");
	if(!MakeBsp(pWorld, TRUE, FALSE, bUseOriginalPolies))
		return FALSE;

	((CMBNode*)pWorld->m_RootNode)->Walk_40bab0();
	FixWorldTJunctions(pWorld);

	DrawStatusTextIfMainWorld("Physics precalculation...");
	MakePhysicsBlocks_425d20(pWorld, FALSE, pWorld->m_pMainWorld->m_pInfoString, TRUE);

	pWorld->GetBoundingBox(&vMin, &vMax);
	pWorld->m_WorldTranslation = vMin + (vMax - vMin) * 0.5f;

	DeleteAndClearArray(pWorld->m_Leaves);
	DrawStatusText("-  %s (%d polies)", pWorld->m_WorldName, pWorld->m_Polies.GetSize());
	return TRUE;
}


// The brushes that go into the vis BSP: the detail level 0 ones (0x410570).
void GetVisBspBrushes(GenList<CEditBrush*> &brushes, GenList<CEditBrush*> &bspBrushes)
{
	GenListPos	pos;
	CEditBrush	*pBrush;

	for(pos=brushes.GenBegin(); brushes.GenIsValid(pos); )
	{
		pBrush = brushes.GenGetNext(pos);
		if(!GetDetailLevel(pBrush))
			bspBrushes.GenAppend(pBrush);
	}
}


// Sorts polies by pPlane into sideLists (0x410600); the ones it cuts go in
// pieces into both, and the pieces into newPolies too.
// talon, retail bug: SplitPoly's result is not looked at (its "Bad poly"
// path, 0x410991, goes on to the appends at 0x410bbc), so a poly it gave
// up on (more than two edge splits) still goes on in pieces.
void SplitPolyList(CPrePlane *pPlane, GenList<CPrePoly*> &polies, GenList<CPrePoly*> &newPolies, GenList<CPrePoly*> *sideLists[2])
{
	SplitStruct		splitStruct;
	CPrePoly		*sides[2];
	CPrePoly		*pPoly;
	GenListPos		pos;
	PolySide		side;

	for(pos=polies.GenBegin(); polies.GenIsValid(pos); )
	{
		pPoly = polies.GenGetNext(pos);

		side = GetPolySide(pPlane, pPoly, &splitStruct);
		if(side == Intersect)
		{
			SplitPoly(pPlane, pPoly, sides, &splitStruct);
			newPolies.GenAppend(sides[0]);
			newPolies.GenAppend(sides[1]);
			sideLists[0]->GenAppend(sides[0]);
			sideLists[1]->GenAppend(sides[1]);
		}
		else
		{
			sideLists[side]->GenAppend(pPoly);
		}
	}
}


// The walk (0x410d80): polies split on pNode's plane into the lists of its
// two sides (the new pieces into newPolies too), then down each side. The
// node's plane through CNode::GetPlane (0x40fa50, called at 0x410e3a), the
// back list's size through GenGetSize (virtual, 0x410f17).
void FilterTerrainPolies_R(CPreWorld *pWorld, NODEREF pNode, GenList<CPrePoly*> &newPolies, GenList<CPrePoly*> &polies)
{
	CLinkedList<CPrePoly*>	sides[2];
	GenList<CPrePoly*>		*pSides[2];
	CNode					*pNewNode;

	if(!IsValidNode(pNode))
		return;

	pSides[0] = &sides[0];
	pSides[1] = &sides[1];
	// talon: a cast, as below: NOLF's LTPlane::DistTo isn't const and
	// Node.h's GetPlane is (the 2001 one likely wasn't)
	SplitPolyList((CPrePlane*)pNode->GetPlane(), polies, newPolies, pSides);

	if(pNode->m_Sides[BackSide] == NODE_OUT && pSides[BackSide]->GenGetSize() > 0)
	{
		pNewNode = AddBacksideNode(pWorld, pNode);
		if(pNewNode)
			pNewNode->m_Unknown_1a |= 1;

		FilterTerrainPolies_R(pWorld, pNode->m_Sides[FrontSide], newPolies, sides[FrontSide]);
	}
	else
	{
		FilterTerrainPolies_R(pWorld, pNode->m_Sides[BackSide], newPolies, sides[BackSide]);
		FilterTerrainPolies_R(pWorld, pNode->m_Sides[FrontSide], newPolies, sides[FrontSide]);
	}
}


// talon: a GenList's GDeleteAndRemoveElements, out of line at 0x411d30
// (AddTerrainBlockers calls it, 0x41107a): virtual GenBegin, GenIsValid,
// GenGetNext, the deleting destructor and GenRemoveAll. stdlith has no such
// helper (linklist.h's DeleteAndRemoveElements is CLinkedList's); the name
// is ours.
template<class T>
inline void GenDeleteAndRemoveElements(GenList<T> &theList)
{
	GenListPos pos;

	for(pos=theList.GenBegin(); theList.GenIsValid(pos); )
		delete theList.GenGetNext(pos);

	theList.GenRemoveAll();
}


// Makes the terrain block visibility in the vis BSP (0x410f80): the
// terrain world models' polies are filtered down it, and where a piece
// lands in front of a node whose back is outside, a node marked as a
// blocker (CNode::m_Unknown_1a & 1; BuildHulls.cpp) is added behind it.
// FALSE if there's no physics BSP. NOLF's takes each terrain model's
// polies with GenAppendList (0x411029, 0x42ba20).
// talon, retail bug: the pieces are malloc'd (CreatePoly, in SplitPoly)
// and freed with delete (0x411d7f, the virtual destructor with 1), not
// DeletePoly; it works only because operator delete is free.
BOOL AddTerrainBlockers(CPreWorld *pWorld)
{
	CLinkedList<CPrePoly*>	polies;
	CLinkedList<CPrePoly*>	newPolies;
	CPreMainWorld			*pMainWorld;
	CPreWorld				*pModel;
	uint32					i;

	pMainWorld = pWorld->m_pMainWorld;
	if(!pMainWorld->GetPhysicsBSP())
		return FALSE;

	for(i=0; i < pMainWorld->m_WorldModels; i++)
	{
		pModel = pMainWorld->m_WorldModels[i];
		if(pModel->m_WorldInfoFlags & WIF_TERRAIN)
			polies.GenAppendList(pModel->m_Polies);
	}

	FilterTerrainPolies_R(pWorld, pWorld->m_RootNode, newPolies, polies);

	// The pieces split off are the walk's own.
	GenDeleteAndRemoveElements(newPolies);
	return TRUE;
}


// Empties each leaf's poly list (0x411100; the calls are virtual).
// talon: through pWorld->m_Leaves each time, as 0x411100 has it
void ClearLeafPolies(CPreWorld *pWorld)
{
	GenListPos			pos;

	for(pos=pWorld->m_Leaves.GenBegin(); pWorld->m_Leaves.GenIsValid(pos); )
	{
		pWorld->m_Leaves.GenGetNext(pos)->m_Polies.GenRemoveAll();
	}
}


// pPoly (pOriginal or a piece of it) down the BSP from pNode; pOriginal
// goes into the leaf of each node it's not behind (0x411180, a $$1 pair).
// talon, retail bug: when SplitPoly gives up (more than two pieces' worth
// of edge splits, 0x411717) it returns without freeing the two pieces
// SplitPoly made.
void FilterPolyIntoLeaves_R(CPrePoly *pPoly, CPrePoly *pOriginal, NODEREF pNode)
{
	static SplitStruct	splitStruct;		// 0x48cf20 (.bss)
	CPrePoly			*sides[2];
	PolySide			side;
	uint32				i;

	if(!IsValidNode(pNode))
		return;

	side = GetPolySide((CPrePlane*)pNode->m_pPlane, pPoly, &splitStruct);

	if(pNode->m_pLeaf && side != BackSide)
		pNode->m_pLeaf->m_Polies.GenAppend(pOriginal);

	if(side == Intersect)
	{
		if(!SplitPoly((CPrePlane*)pNode->m_pPlane, pPoly, sides, &splitStruct))
			return;

		for(i=0; i < 2; i++)
		{
			FilterPolyIntoLeaves_R(sides[i], pOriginal, pNode->m_Sides[i]);
			DeletePoly(sides[i]);
		}
	}
	else
	{
		FilterPolyIntoLeaves_R(pPoly, pOriginal, pNode->m_Sides[side]);
	}
}


// Each visible poly into the leaves it's in (0x4118f0).
void AddPoliesToLeaves(CPreWorld *pWorld, GenList<CPrePoly*> &polies)
{
	GenListPos	pos;
	CPrePoly	*pPoly;

	for(pos=polies.GenBegin(); polies.GenIsValid(pos); )
	{
		pPoly = polies.GenGetNext(pos);
		if(!(pPoly->GetSurfaceFlags() & SURF_INVISIBLE))
			FilterPolyIntoLeaves_R(pPoly, pPoly, pWorld->m_RootNode);
	}
}


// Finds the vis BSP's leaves (0x411980): makes its hulls and gives each
// leaf the polies of the physics BSP and the terrain that are in it, each
// model's with GenAppendList (0x411a74). MakeVisBSP and MakeWorldPVS
// (0x44e1eb) call it.
BOOL SetupVisBSPLeaves(CPreWorld *pVisBSP)
{
	CLinkedList<CPrePoly*>	polies;
	CPreMainWorld			*pMainWorld;
	CPreWorld				*pModel;
	GenListPos				pos;

	if(!BuildWorldHulls(pVisBSP, NULL))
		return FALSE;

	pMainWorld = pVisBSP->m_pMainWorld;
	ClearLeafPolies(pVisBSP);

	for(pos=pMainWorld->m_WorldModels.GenBegin(); pMainWorld->m_WorldModels.GenIsValid(pos); )
	{
		pModel = pMainWorld->m_WorldModels.GenGetNext(pos);
		if(pModel->m_WorldInfoFlags & (WIF_TERRAIN|WIF_PHYSICSBSP))
			polies.GenAppendList(pModel->m_Polies);
	}

	AddPoliesToLeaves(pVisBSP, polies);
	return TRUE;
}


// The visibility BSP (0x411ae0): made of the detail level 0 brushes, with
// the unseen polies removed; the terrain blocks it, and its leaves get
// their polies. NOLF's: no hulls before the leak file, which is made last,
// under the -LeakFile name itself (AvP2's adds "_Vis.lek" and is made
// before the unseen geometry goes).
BOOL MakeVisBSP(CEditRegion *pRegion, CWorldModelDef *pDef, CPreWorld *pWorld)
{
	// talon: as MakeWorldModelBSP's (0x4894a0, TRUE).
	static BOOL					bUseOriginalPolies = TRUE;
	CLinkedList<CEditBrush*>	brushes;

	GetVisBspBrushes(pDef->m_Brushes, brushes);
	GenerateSurfaces(brushes, pWorld);
	if(!MergeBrushes(brushes, pWorld))
		return FALSE;

	brushes.GenRemoveAll();

	DrawStatusTextIfMainWorld("Creating BSP.");
	if(!MakeBsp(pWorld, FALSE, FALSE, FALSE))
		return FALSE;

	((CMBNode*)pWorld->m_RootNode)->Walk_40bab0();

	DrawStatusTextIfMainWorld("Removing unseen geometry.");
	if(!MakeHullsAndRemoveUnseen(pWorld))
		return FALSE;

	pWorld->JoinPolies(0.0001f);
	pWorld->TermLeaves();
	pWorld->RemoveAllNodes();

	// talon, retail bug: what this BSP returns is not looked at (0x411c38;
	// MakeWorldModelBSP's is): a failed vis BSP goes on to the PVS.
	DrawStatusTextIfMainWorld("Recreating BSP.");
	MakeBsp(pWorld, TRUE, TRUE, bUseOriginalPolies);

	((CMBNode*)pWorld->m_RootNode)->Walk_40bab0();

	if(!AddTerrainBlockers(pWorld))
		return FALSE;

	pWorld->InitPolyVertexIndices();

	// talon: nor what this returns (0x411c95); MakeWorldPVS calls it again
	// and does look.
	DrawStatusTextIfMainWorld("Finding leaves.");
	SetupVisBSPLeaves(pWorld);

	DrawStatusText("-  %s (%d polies)", pWorld->m_WorldName, pWorld->m_Polies.GetSize());

	if(g_pGlobs->m_LeakFile[0] && (pWorld->m_WorldInfoFlags & WIF_VISBSP))
	{
		DrawStatusText("Making VisBSP leak file.");
		MakeLeakFile(pWorld, g_pGlobs->m_LeakFile);
	}

	return TRUE;
}
