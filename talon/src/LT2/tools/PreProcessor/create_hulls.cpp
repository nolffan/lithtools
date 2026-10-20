// ------------------------------------------------------------------ //
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
#include "create_hulls.h"
#include "processing.h"
#include "splitpoly.h"


static CPreWorld	*g_pHullWorld;
static PReal		g_fPortalSize;
static CHullInfo	*g_pHullInfo;


static void CheckLeaves(CPreWorld *pWorld);
static void MakeHulls_R(CNode *pRoot, CPreLeaf *pLeaf);
static BOOL IsHullNode(CPreWorld *pWorld, CNode *pNode);
static void AddNodePoly(CPreWorld *pWorld, CNode *pNode, CLinkedList<CPrePoly*> &polies);
static CPrePortal* MakeNodePortal(CPreLeaf *pLeaf, CPrePoly *pPoly);
static CPrePortal* ClipPortal(CPreLeaf *pLeaf, CPrePortal *pPortal, CPrePoly *pPoly);


uint32 MakeHulls(CHullInfo *pInfo, CPreWorld *pWorld, void (*ProgressFn)(float fPercent))
{
	CMoArray<CNode*> unused;
	PVector vMin, vMax;
	CPreLeaf *pLeaf;
	GPOS pos;
	uint32 i;

	pInfo->m_ProblemBrushes.Term();

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
	return 0;
}


static void CheckLeaves(CPreWorld *pWorld)
{
	CPreLeaf *pLeaf;
	uint32 i;

	for(i=0; i < pWorld->m_Leaves.GetSize(); i++)
	{
		pLeaf = pWorld->m_Leaves[i];
	}
}


static void MakeHulls_R(CNode *pRoot, CPreLeaf *pLeaf)
{
	static SplitStruct splitStruct;
	CPreLeaf *sides[2];
	CPrePortal *portalSides[2], *newPartners[2];
	CPrePortal *pPortal, *pReverse, *pPartner;
	CPrePoly *pNodePoly, *pPoly;
	CMoArray<CPrePortal*> *pPartnerPortals;
	PolySide side;
	BOOL bBlocker;
	LPOS pos;
	uint32 i, k;

	if(pLeaf->m_Portals.GetSize() == 0)
		return;

	for(;;)
	{
		pNodePoly = pRoot->m_pPoly;

		sides[BackSide] = new CPreLeaf;
		sides[FrontSide] = new CPreLeaf;

		++g_pHullInfo->m_nNodesDone;
		if(g_pHullInfo->m_ProgressFn)
			g_pHullInfo->m_ProgressFn((float)g_pHullInfo->m_nNodesDone / g_pHullInfo->m_nNodes);

		bBlocker = !!(pRoot->m_pPoly->GetSurfaceFlags() & SURF_VISBLOCKER);

		// The node's portal: its poly's plane, inside this leaf.
		pPortal = MakeNodePortal(pLeaf, pNodePoly);
		if(pPortal)
		{
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
			pPortal = pLeaf->m_Portals[i];

			side = GetPolySide(pNodePoly->GetPlane(), pPortal, &splitStruct);
			if(side != Intersect)
			{
				sides[side]->m_Portals.Append(pPortal);
				continue;
			}

			SplitPoly(pNodePoly->GetPlane(), pPortal, portalSides, &splitStruct);
			sides[BackSide]->m_Portals.Append(portalSides[BackSide]);
			sides[FrontSide]->m_Portals.Append(portalSides[FrontSide]);

			// Its partner, in the leaf on its other side, is split the same way.
			pPartner = pPortal->m_pPartner;
			if(pPartner)
			{
				pPartnerPortals = &pPartner->m_pLeaf->m_Portals;
				for(k=0; k < 2; k++)
				{
					newPartners[k] = ClonePortal(portalSides[k]);
					newPartners[k]->Flip();
					pPartnerPortals->Append(newPartners[k]);

					newPartners[k]->m_pLeaf = pPartner->m_pLeaf;
					newPartners[k]->m_pPartner = portalSides[k];
					portalSides[k]->m_pPartner = newPartners[k];
				}

				pPartnerPortals->Remove(pPartnerPortals->FindElement(pPartner));
				DeletePoly(pPartner);
				pPortal->m_pPartner = NULL;
			}

			DeletePoly(pPortal);
		}

		pLeaf->m_Portals.Term();
		delete pLeaf;

		sides[FrontSide]->m_Polies.AddTail(pNodePoly);

		for(k=0; k < 2; k++)
		{
			for(i=0; (int)i < (int)sides[k]->m_Portals.GetSize(); i++)
				sides[k]->m_Portals[i]->m_pLeaf = sides[k];
		}

		// The front side.
		if(IsHullNode(g_pHullWorld, pRoot->m_Sides[FrontSide]))
		{
			MakeHulls_R(pRoot->m_Sides[FrontSide], sides[FrontSide]);
		}
		else
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
			sides[FrontSide]->m_Index = (uint16)(g_pHullWorld->m_Leaves.GetSize() - 1);
		}

		// The back side.
		if(!IsHullNode(g_pHullWorld, pRoot->m_Sides[BackSide]))
		{
			delete sides[BackSide];
			return;
		}

		pRoot = pRoot->m_Sides[BackSide];
		pLeaf = sides[BackSide];
		if(pLeaf->m_Portals.GetSize() == 0)
			return;
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
	if(IsValidNode(pNode))
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
				g_pHullInfo->m_ProblemBrushes.Append(pPoly->m_pSurface->m_pBrush);

			DeletePoly(portalSides[BackSide]);
			DeletePoly(pPortal);
			pPortal = portalSides[FrontSide];
		}
	}

	return pPortal;
}
