// ------------------------------------------------------------------ //
//	BuildHulls.cpp (the name is ours; it sorts between BspGen and classbind,
//	its neighbours in the image).
//
//	Hull making: portalizes the BSP. Starting from the portals of a box
//	around the world, each node's leaf is split by the node's plane, the
//	node's own poly makes the portal between the two sides, and the front
//	sides that end at a leaf become the world's leaves.
//
//	No source of this survives (Build 69 has no PVS); it is written from
//	the shipped code (0x408370..0x409f80), and the names are ours.
// ------------------------------------------------------------------ //

#include "bdefs.h"
#include "preworld.h"
#include "preleaf.h"
#include "LeafPortals.h"
#include "BuildHulls.h"
#include "processing.h"
#include "splitpoly.h"


static CPreWorld	*g_pHullWorld;
static PReal		g_fPortalSize;
static CHullInfo	*g_pHullInfo;


static void CheckLeaves(CPreWorld *pWorld);
static void MakeHulls_R(NODEREF iRoot, CPreLeaf *pLeaf);
static BOOL IsHullNode(CPreWorld *pWorld, CNode *pNode);
static void AddNodePoly(CPreWorld *pWorld, CNode *pNode, CLinkedList<CPrePoly*> &polies);
static CPrePortal* MakeNodePortal(CPreLeaf *pLeaf, CPrePoly *pPoly);
static CPrePortal* ClipPortal(CPreLeaf *pLeaf, CPrePortal *pPortal, CPrePoly *pPoly);


// 0x408370. The shipped code calls CMoArray::Clear in unused's constructor
// (0x408397) where a plain body expands it: the inline budget. Three more
// expansions that compile to nothing make it byte-identical (the stand-in
// below); what the 2001 source spent them on is unknown. Tried as real
// spellings and not it: vMax.Dist(vMin), `i < pLeaf->m_Portals`, GetNode
// on the root, RemoveAll for Term, PVector temporaries for the -= 10 and
// the Inits (they move other code or overspend).
uint32 MakeHulls(CHullInfo *pInfo, CPreWorld *pWorld, void (*ProgressFn)(float fPercent))
{
	CMoArray<CNode*> unused;
	PVector vMin, vMax;
	CPreLeaf *pLeaf;
	GPOS pos;
	uint32 i;

	pInfo->m_ProblemPolies.Term();

	// Portals big enough to cross the whole world.
	pWorld->GetBoundingBox(&vMin, &vMax);
	g_fPortalSize = (vMax - vMin).Mag() * 2.0f;

	pInfo->m_ProgressFn = ProgressFn;
	pInfo->m_nNodesDone = 0;
	pInfo->m_nNodes = pWorld->NumNodes();

	pWorld->TermLeaves();

	// The first leaf is a box around everything.
	pLeaf = new CPreLeaf;

	vMin.Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	vMax.Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);
	for(pos=pWorld->m_Polies; pos; )
	{
		pWorld->m_Polies.GetNext(pos)->ExtendBounds(vMin, vMax);
	}

	vMin -= 10.0f;
	vMax += 10.0f;
	MakeBoxPortals(pLeaf->m_Portals, vMin, vMax);

	for(i=0; i < pLeaf->m_Portals.GetSize(); i++)
		pLeaf->m_Portals[i]->m_Index = (uint16)i;

	g_pHullWorld = pWorld;
	g_pHullInfo = pInfo;
	MakeHulls_R(pWorld->m_RootNode, pLeaf);

	CheckLeaves(pWorld);

	// talon: stand-in: three inline calls whose values are dropped (they
	// compile to nothing). They only spend the inline budget the shipped
	// code spent here on something unknown, so that unused's constructor
	// calls Clear out of line (0x408397) as shipped does. A dead
	// `TRACE("%d %d %d\n", ...)` of the same three works too.
	pLeaf->m_Portals.GetSize();
	pLeaf->m_Portals.GetSize();
	pLeaf->m_Portals.GetSize();
	return 0;
}


// 0x4085a0. The shipped loop keeps storing each leaf into the dead
// parameter slot (0x4085b4) after the add and the dec: the leaf is a local
// whose address escapes into code that compiled to nothing. Any dead call
// taking &pLeaf gives exactly that (an inline or intrinsic one, or a plain
// `&pLeaf`, is dropped with the store; volatile keeps the store but pins it
// right after the load). What the 2001 body did with the leaf is unknown;
// the free() is a stand-in for that dead use, not a recovered statement.
static void CheckLeaves(CPreWorld *pWorld)
{
	CPreLeaf *pLeaf;
	uint32 i;

	for(i=0; i < pWorld->m_Leaves.GetSize(); i++)
	{
		pLeaf = pWorld->m_Leaves[i];
		if(0) free(&pLeaf);
	}
}


// talon: 0x40c4d0, the 2001 version (EH-framed: new CPreLeaf). Through
// the world's GetNode (0x40c522), the blocker flag read from the surface
// (0x40c62e), the portals' leaves set with a separate counter (signed,
// 0x40c99b), the leaf's portals SetSize(0) (0x40c93e) and its index LastI
// (0x40cac2); the back side by the loop (the shipped back edge stores into
// both parameters, 0x40cb15).
//
// Near (1704 bytes, shipped 1650): the same calls in the same order, under
// this file's /Ob0. Left: our icl rotates the loop, putting the parameter
// stores at its top behind a jump in (shipped: at the bottom, 0x40cb15;
// while, goto and tail recursion tried: ours does not eliminate the tail
// call, shipped does or loops); it keeps pPortal in edx across the second
// test (shipped reloads it, 0x40c68a) and increments the counters through
// a register where shipped has incl on the slot (0x40c913, 0x40c9c2).
static void MakeHulls_R(NODEREF iRoot, CPreLeaf *pLeaf)
{
	static SplitStruct splitStruct;
	CNode *pRoot;
	CPrePoly *pNodePoly;
	CPreLeaf *sides[2];
	BOOL bBlocker;
	CPrePortal *pPortal, *pReverse;
	LPOS pos;
	CPrePoly *pPoly;
	PolySide side;
	uint32 i;
	CPrePortal *pTest;
	CPrePortal *portalSides[2];
	CPrePortal *pPartner;
	CPreLeaf *pPartnerLeaf;
	uint32 k;
	CPrePortal *newPartners[2];
	uint32 j;

	for(;;)
	{
		if(pLeaf->m_Portals.GetSize() == 0)
			return;

		pRoot = g_pHullWorld->GetNode(iRoot);
		pNodePoly = pRoot->m_pPoly;

		sides[BackSide] = new CPreLeaf;
		sides[FrontSide] = new CPreLeaf;

		++g_pHullInfo->m_nNodesDone;
		if(g_pHullInfo->m_ProgressFn)
			g_pHullInfo->m_ProgressFn((float)g_pHullInfo->m_nNodesDone / g_pHullInfo->m_nNodes);

		if(pRoot->m_pPoly->m_pSurface->m_Flags & SURF_VISBLOCKER)
			bBlocker = TRUE;
		else
			bBlocker = FALSE;

		// The node's portal: its poly's plane, inside this leaf.
		pPortal = MakeNodePortal(pLeaf, pNodePoly);
		if(pPortal)
			pPortal = ClipPortal(pLeaf, pPortal, pNodePoly);

		if(pPortal)
		{
			pReverse = ClonePortal(pPortal);
			pReverse->Flip();

			sides[FrontSide]->m_Portals.Append(pPortal);
			sides[BackSide]->m_Portals.Append(pReverse);

			// You can't see through a vis blocker.
			pPortal->m_pPartner = bBlocker ? NULL : pReverse;
			pReverse->m_pPartner = bBlocker ? NULL : pPortal;
			if(bBlocker)
			{
				pPortal->m_Flags |= PORTALF_BLOCKER;
				pReverse->m_Flags |= PORTALF_BLOCKER;
			}

			pReverse->m_pNode = pRoot;
			pPortal->m_pNode = pRoot;
		}

		// Sort the polies.
		for(pos=pLeaf->m_Polies; pos; )
		{
			pPoly = pLeaf->m_Polies.GetNext(pos);

			side = GetPolySide(pNodePoly->GetPlane(), pPoly, &splitStruct);
			if(side == Intersect)
			{
				sides[FrontSide]->m_Polies.AddTail(pPoly);
				sides[BackSide]->m_Polies.AddTail(pPoly);
			}
			else
			{
				sides[side]->m_Polies.AddTail(pPoly);
			}
		}

		// Split the portals.
		for(i=0; i < pLeaf->m_Portals.GetSize(); i++)
		{
			pTest = pLeaf->m_Portals[i];

			side = GetPolySide(pNodePoly->GetPlane(), pTest, &splitStruct);
			if(side != Intersect)
			{
				sides[side]->m_Portals.Append(pTest);
				continue;
			}

			SplitPoly(pNodePoly->GetPlane(), pTest, portalSides, &splitStruct);
			sides[BackSide]->m_Portals.Append(portalSides[BackSide]);
			sides[FrontSide]->m_Portals.Append(portalSides[FrontSide]);

			// Its partner, in the leaf on its other side, is split the same way.
			pPartner = pTest->m_pPartner;
			if(pPartner)
			{
				pPartnerLeaf = pPartner->m_pLeaf;
				for(k=0; k < 2; k++)
				{
					newPartners[k] = ClonePortal(portalSides[k]);
					newPartners[k]->Flip();
					pPartnerLeaf->m_Portals.Append(newPartners[k]);

					newPartners[k]->m_pLeaf = pPartner->m_pLeaf;
					newPartners[k]->m_pPartner = portalSides[k];
					portalSides[k]->m_pPartner = newPartners[k];
				}

				pPartnerLeaf->m_Portals.Remove(pPartnerLeaf->m_Portals.FindElement(pPartner));
				DeletePoly(pPartner);
				pTest->m_pPartner = NULL;
			}

			DeletePoly(pTest);
		}

		pLeaf->m_Portals.SetSize(0);
		delete pLeaf;

		sides[FrontSide]->m_Polies.AddTail(pRoot->m_pPoly);

		for(i=0; i < 2; i++)
		{
			for(j=0; (int)j < (int)sides[i]->m_Portals.GetSize(); j++)
				sides[i]->m_Portals[j]->m_pLeaf = sides[i];
		}

		// The front side.
		if(!IsHullNode(g_pHullWorld, pRoot->m_Sides[FrontSide]))
		{
			// It's a leaf.
			if(pRoot->m_Unknown_1a & 1)
			{
				for(i=0; i < sides[FrontSide]->m_Portals.GetSize(); i++)
					sides[FrontSide]->m_Portals[i]->m_Flags |= PORTALF_BLOCKER;
			}

			AddNodePoly(g_pHullWorld, pRoot->m_Sides[FrontSide], sides[FrontSide]->m_Polies);

			pRoot->m_pLeaf = sides[FrontSide];
			sides[FrontSide]->m_pNode = pRoot;
			g_pHullWorld->m_Leaves.Append(sides[FrontSide]);
			sides[FrontSide]->m_Index = (uint16)g_pHullWorld->m_Leaves.LastI();
		}
		else
		{
			MakeHulls_R(pRoot->m_Sides[FrontSide], sides[FrontSide]);
		}

		// The back side.
		if(!IsHullNode(g_pHullWorld, pRoot->m_Sides[BackSide]))
		{
			delete sides[BackSide];
			return;
		}

		iRoot = pRoot->m_Sides[BackSide];
		pLeaf = sides[BackSide];
	}
}


// A node that splits space for the hulls (not a physics blocker).
static BOOL IsHullNode(CPreWorld *pWorld, CNode *pNode)
{
	if(!IsValidNode(pNode))
		return FALSE;

	return !(pNode->m_pPoly->GetSurfaceFlags() & SURF_PHYSICSBLOCKER);
}


static void AddNodePoly(CPreWorld *pWorld, CNode *pNode, CLinkedList<CPrePoly*> &polies)
{
	if(!IsValidNode(pNode))
		return;

	polies.AddTail(pNode->m_pPoly);
}


static CPrePortal* MakeNodePortal(CPreLeaf *pLeaf, CPrePoly *pPoly)
{
	return pPoly->MakePortal(g_fPortalSize);
}


// Clips pPortal to the inside of pLeaf.
static CPrePortal* ClipPortal(CPreLeaf *pLeaf, CPrePortal *pPortal, CPrePoly *pPoly)
{
	SplitStruct splitStruct;
	CPrePortal *pClip, *portalSides[2];
	PolySide side;
	uint32 i;

	for(i=0; i < pLeaf->m_Portals.GetSize(); i++)
	{
		pClip = pLeaf->m_Portals[i];

		side = GetPolySide(&pClip->m_Plane, pPortal, &splitStruct);

		// On the leaf's boundary already.
		if(splitStruct.m_nIntersect == pPortal->NumVerts())
		{
			DeletePoly(pPortal);
			pPortal = NULL;
			break;
		}

		if(side == BackSide)
		{
			DeletePoly(pPortal);
			return NULL;
		}

		if(side == Intersect)
		{
			if(!SplitPoly(&pClip->m_Plane, pPortal, portalSides, &splitStruct))
				g_pHullInfo->m_ProblemPolies.Append(pPoly->m_pSurface->m_pEditPoly);

			DeletePoly(portalSides[BackSide]);
			DeletePoly(pPortal);
			pPortal = portalSides[FrontSide];
		}
	}

	return pPortal;
}
