// talon: 0x0040b540..0x0040cff0, after create_world_tree: making a world
// model's (and the physics) BSP, the visibility BSP and its leaves'
// polies, selecting the problem and out-of-bounds brushes. Written from the
// image; Build 69 has none of it. The file's name and every function name
// here are ours.
//
// 0x40cff0 (22 bytes) is not this module's: it is TextureMipData's
// constructor (the 2001 one zeroes six fields), dtxmgr's first function,
// called by TextureData's constructor (0x40d022).

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
BOOL BuildWorldHulls(CPreWorld *pWorld, CHullInfo *pInfo);	// 0x43f020
BOOL MakeHullsAndRemoveUnseen(CPreWorld *pWorld);			// 0x43f5f0
void FixWorldTJunctions(CPreWorld *pWorld);					// 0x43f810
BOOL SaveEDFile();											// 0x43edc0

// LeakFile.cpp (0x414790).
BOOL MakeLeakFile(CPreWorld *pWorld, const char *pFilename);

// The physics module (0x425d20): the physics blocks of a BSP.
void MakePhysicsBlocks_425d20(CPreWorld *pWorld, BOOL bUnknown, char *pInfoString, BOOL bUnknown2);


// talon: a walk of the BSP from a node down (0x40bab0, thiscall on the
// node) that does nothing in this build: it stops at NODE_IN and NODE_OUT
// (not at NULL) and touches nothing else. Inline, so its callers expand
// the first level (0x40b7cd, 0x40cd13). A stand-in on CNode's own class,
// whose header is shared.
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


// A world model's BSP (0x40b540): bPhysicsBSP for the physics BSP, which
// also loses the polies no leaf sees. Brushes the brush checks found
// problems in stop it with SelectProblemBrushes or
// SelectOutOfBoundsBrushes (after saving the ED file with them selected).
BOOL MakeWorldModelBSP(CEditRegion *pRegion, CWorldModelDef *pDef, CPreWorld *pWorld, BOOL bPhysicsBSP)
{
	// talon: an initialized static (0x47e2e8, TRUE, read only here), passed
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

	// talon: GenRemoveAll, not RemoveAll: both are 0x407bd0, but only this
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


// Takes the polies on SURF_TERRAINOCCLUDER surfaces out of the world's
// list (0x40bad0). They are not freed. Through GenList: the shipped calls
// are virtual.
void RemoveTerrainOccluderPolies(CPreWorld *pWorld)
{
	GenList<CPrePoly*>	&polies = pWorld->m_Polies;
	GenListPos			pos, curPos;
	CPrePoly			*pPoly;

	for(pos=polies.GenBegin(); polies.GenIsValid(pos); )
	{
		curPos = pos;
		pPoly = polies.GenGetNext(pos);
		if(pPoly->GetSurfaceFlags() & SURF_TERRAINOCCLUDER)
			polies.GenRemoveAt(curPos);
	}
}


// -SelectProblemBrushes: selects the brushes the brush checks found
// problems in, and the out-of-bounds ones, and saves the ED file (0x40bb50).
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
// the ED file (0x40bbf0).
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
// and terrain occluders (0x40bc60).
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


void FilterTerrainPolies_R(CPreWorld *pWorld, NODEREF pNode, GenList<CPrePoly*> &newPolies, GenList<CPrePoly*> &polies);
void SplitPolyList(const CPrePlane *pPlane, GenList<CPrePoly*> &polies, GenList<CPrePoly*> &newPolies, GenList<CPrePoly*> *sideLists[2]);

// talon: a GenList's GDeleteAndRemoveElements. Not known to be in the
// source: as an inline (and its own block-scoped GenListPos), it gives
// AddTerrainBlockers the shipped frame (pos at 0x10, GenBegin's temporary
// at 0x18, 0x40c12b) and the virtual calls 0x40c1c3..0x40c227 have; the
// name is ours.
template<class T>
inline void GenDeleteAndRemoveElements(GenList<T> &theList)
{
	GenListPos pos;

	for(pos=theList.GenBegin(); theList.GenIsValid(pos); )
		delete theList.GenGetNext(pos);

	theList.GenRemoveAll();
}


// Makes the terrain block visibility in the vis BSP (0x40bfb0): the
// terrain world models' polies are filtered down it, and where a piece
// lands in front of a node whose back is outside, a node marked as a
// blocker (CNode::m_Unknown_1a & 1; BuildHulls.cpp) is added behind it.
// FALSE if there's no physics BSP.
// Near (804 bytes against 797): the inline budget. Shipped (0x40c22a..):
// both lists' destructors at the end call RemoveAll and expand ObjectBank's
// destructor; ours expands RemoveAll and calls ~ObjectBank. Append (not
// AddTail) leaves InsertAfter and AddHead calls as shipped (0x40c16f,
// 0x40c17a); with AddTail the end comes out as shipped but InsertAfter is
// expanded. The GetPhysicsBSP test is `cmp %ebx,%eax` in the shipped code
// (0x40c066), `test` in ours, and pWorld is held in esi there.
// talon, retail bug: the pieces are malloc'd (CreatePoly, in SplitPoly)
// and freed with delete (0x40c209, the virtual destructor with 1), not
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
		{
			GenListPos pos;

			for(pos=pModel->m_Polies.GenBegin(); pModel->m_Polies.GenIsValid(pos); )
				polies.Append(pModel->m_Polies.GenGetNext(pos));
		}
	}

	FilterTerrainPolies_R(pWorld, pWorld->m_RootNode, newPolies, polies);

	// The pieces split off are the walk's own.
	GenDeleteAndRemoveElements(newPolies);
	return TRUE;
}


// The walk (0x40c2d0): polies split on pNode's plane into the lists of its
// two sides (the new pieces into newPolies too), then down each side.
// Near, size-exact (324 bytes): the scratch registers at the three
// recursive calls and the array destructor (shipped puts the side list's
// address in eax/edx and the child in ecx, 0x40c3a8..0x40c3eb; ours
// ecx/edx and edx/eax). The parameter as a CLinkedList& doesn't move it.
void FilterTerrainPolies_R(CPreWorld *pWorld, NODEREF pNode, GenList<CPrePoly*> &newPolies, GenList<CPrePoly*> &polies)
{
	CLinkedList<CPrePoly*>	sides[2];
	GenList<CPrePoly*>		*pSides[2];
	CNode					*pNewNode;

	if(!IsValidNode(pNode))
		return;

	pSides[0] = &sides[0];
	pSides[1] = &sides[1];
	SplitPolyList(pNode->m_pPlane, polies, newPolies, pSides);

	if(pNode->m_Sides[BackSide] == NODE_OUT && sides[BackSide].GetSize() > 0)
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


// Sorts polies by pPlane into sideLists (0x40c420); the ones it cuts go in
// pieces into both, and the pieces into newPolies too.
// talon, retail bug: SplitPoly's result is not looked at (0x40c5fb), so a
// poly it gave up on (more than two edge splits) still goes on in pieces.
void SplitPolyList(const CPrePlane *pPlane, GenList<CPrePoly*> &polies, GenList<CPrePoly*> &newPolies, GenList<CPrePoly*> *sideLists[2])
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


void ClearLeafPolies(CPreWorld *pWorld);
void AddPoliesToLeaves(CPreWorld *pWorld, GenList<CPrePoly*> &polies);

// Finds the vis BSP's leaves (0x40c640): makes its hulls and gives each
// leaf the polies of the physics BSP and the terrain that are in it.
// MakeVisBSP and MakeWorldPVS (0x43f272) call it.
// Near (606 bytes against 618): the inline budget. Shipped calls
// ObjectBank::Allocate (0x40c7c6) where ours expands it to sb_Allocate,
// expands ~ObjectBank at the failure return (0x40c6eb) where ours calls it,
// and calls sb_Free in the final RemoveAll (0x40c851) where ours expands
// it. The model poly position is block-scoped: it shares GenBegin's
// temporary's slot (0x10) as shipped.
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
		{
			GenListPos polyPos;

			for(polyPos=pModel->m_Polies.GenBegin(); pModel->m_Polies.GenIsValid(polyPos); )
				polies.AddTail(pModel->m_Polies.GenGetNext(polyPos));
		}
	}

	AddPoliesToLeaves(pVisBSP, polies);
	return TRUE;
}


// Empties each leaf's poly list (0x40c8d0; the calls are virtual).
void ClearLeafPolies(CPreWorld *pWorld)
{
	GenList<CPreLeaf*>	&leaves = pWorld->m_Leaves;
	GenListPos			pos;

	for(pos=leaves.GenBegin(); leaves.GenIsValid(pos); )
	{
		leaves.GenGetNext(pos)->m_Polies.GenRemoveAll();
	}
}


void FilterPolyIntoLeaves_R(CPrePoly *pPoly, CPrePoly *pOriginal, NODEREF pNode);

// Each visible poly into the leaves it's in (0x40c930).
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


// pPoly (pOriginal or a piece of it) down the BSP from pNode; pOriginal
// goes into the leaf of each node it's not behind (0x40c9b0).
// Near (525 bytes against 530): the inline budget. Shipped calls
// _CVector::Dot (0x40ca27, the point copied to the stack, the plane kept in
// a slot) in GetPolySide's DistTo, where ours expands it; the second Dot
// (the poly's normal, 0x40cac1) is expanded in both.
// talon, retail bug: when SplitPoly gives up (0x40cb61) it returns without
// freeing the two pieces SplitPoly made.
void FilterPolyIntoLeaves_R(CPrePoly *pPoly, CPrePoly *pOriginal, NODEREF pNode)
{
	static SplitStruct	splitStruct;		// 0x4845c0 (.bss)
	CPrePoly			*sides[2];
	PolySide			side;
	uint32				i;

	if(!IsValidNode(pNode))
		return;

	side = GetPolySide(pNode->m_pPlane, pPoly, &splitStruct);

	if(pNode->m_pLeaf && side != BackSide)
		pNode->m_pLeaf->m_Polies.GenAppend(pOriginal);

	if(side == Intersect)
	{
		if(!SplitPoly(pNode->m_pPlane, pPoly, sides, &splitStruct))
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


void GetVisBspBrushes(GenList<CEditBrush*> &brushes, GenList<CEditBrush*> &bspBrushes);

// The visibility BSP (0x40cbd0): made of the detail level 0 brushes, with
// the unseen polies removed; the terrain blocks it, and its leaves get
// their polies.
BOOL MakeVisBSP(CEditRegion *pRegion, CWorldModelDef *pDef, CPreWorld *pWorld)
{
	// talon: as MakeWorldModelBSP's (0x47e414, TRUE).
	static BOOL					bUseOriginalPolies = TRUE;
	CLinkedList<CEditBrush*>	brushes;
	char						leakFile[MAX_PATH + 10];

	GetVisBspBrushes(pDef->m_Brushes, brushes);
	GenerateSurfaces(brushes, pWorld);
	if(!MergeBrushes(brushes, pWorld))
		return FALSE;

	brushes.GenRemoveAll();

	DrawStatusTextIfMainWorld("Creating BSP.");
	if(!MakeBsp(pWorld, FALSE, FALSE, FALSE))
		return FALSE;

	((CMBNode*)pWorld->m_RootNode)->Walk_40bab0();

	if(g_pGlobs->m_LeakFile[0] && (pWorld->m_WorldInfoFlags & WIF_VISBSP))
	{
		BuildWorldHulls(pWorld, NULL);
		strncpy(leakFile, g_pGlobs->m_LeakFile, MAX_PATH);
		// talon: strncat with the whole buffer's size (0x40cd6b, 270), not
		// what is left of it; m_LeakFile's 255 characters keep it inside.
		strncat(leakFile, "_Vis.lek", sizeof(leakFile));
		DrawStatusText("Making VisBSP leak file: %s", leakFile);
		MakeLeakFile(pWorld, leakFile);
	}

	DrawStatusTextIfMainWorld("Removing unseen geometry.");
	if(!MakeHullsAndRemoveUnseen(pWorld))
		return FALSE;

	pWorld->JoinPolies(0.0001f);
	pWorld->TermLeaves();
	pWorld->RemoveAllNodes();

	// talon, retail bug: what this BSP returns is not looked at (0x40ce37;
	// MakeWorldModelBSP's is): a failed vis BSP goes on to the PVS.
	DrawStatusTextIfMainWorld("Recreating BSP.");
	MakeBsp(pWorld, TRUE, TRUE, bUseOriginalPolies);

	((CMBNode*)pWorld->m_RootNode)->Walk_40bab0();

	if(!AddTerrainBlockers(pWorld))
		return FALSE;

	pWorld->InitPolyVertexIndices();

	// talon: nor what this returns (0x40cec7); MakeWorldPVS calls it again
	// and does look.
	DrawStatusTextIfMainWorld("Finding leaves.");
	SetupVisBSPLeaves(pWorld);

	DrawStatusText("-  %s (%d polies)", pWorld->m_WorldName, pWorld->m_Polies.GetSize());
	return TRUE;
}


// The brushes that go into the vis BSP: the detail level 0 ones (0x40cf70).
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
