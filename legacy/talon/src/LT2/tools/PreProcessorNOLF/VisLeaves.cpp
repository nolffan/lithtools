// talon: a module of its own in the image, 0x004498d0..0x0044acd0, after
// Threads (the name is ours): removing the polies no leaf from the inside
// sees (CUnseenPolyRemover, 0x4498d0..0x449b5c: the InsideDef objects mark
// the leaves they are in), then the PVS's helpers -- the leaf lists the
// vis-BSP walk collects, zone flood fills, the leaf spheres, the per-thread
// leaf loop and the portal flow (0x449c00..0x44ac50), and the FastApprox
// marking (0x44ac50). MakePVS and the rest of the PVS (0x44acd0..) are
// WorldApprox.cpp's; what they share with these is declared in VisLeaves.h.
//
// Written from the image; neither Build 69 nor nolfsrc has any of it.

#include "bdefs.h"
#include "preworld.h"
#include "preleaf.h"
#include "pregeometry.h"
#include "LeafPortals.h"
#include "prepoly.h"
#include "presurface.h"
#include "node.h"
#include "editobjects.h"
#include "processing.h"
#include "threads.h"
#include "VisLeaves.h"


// CPreLeaf::m_Flags (LEAFF_VISITED, 1<<1, is PreLeaf.h's).
// Reached from the inside leaf or an InsideDef (0x4498f0).
#define LEAFF_INSIDE		(1<<0)

// The bits of a leaf set: a byte array indexed by CPreLeaf::m_Index.
#define VIS_TESTBIT(arr, i)	((arr)[(i) >> 3] & (1 << ((i) & 7)))
#define VIS_SETBIT(arr, i)	((arr)[(i) >> 3] |= (1 << ((i) & 7)))
#define VIS_CLEARBIT(arr, i)	((arr)[(i) >> 3] &= ~(1 << ((i) & 7)))

// The flow's portal buffers: four portals of up to 100 vertices each,
// one set per level of the flow (0x4060 bytes, newed at 0x44a5af).
#define VIS_PORTALBUF_SIZE	(sizeof(CPrePortal) + sizeof(CPolyVert)*97)
struct VisPortalBuffers
{
	uint8					m_Portals[4][VIS_PORTALBUF_SIZE];
};

// The flow from one portal (0x44 bytes, VisFlowPortal's local).
struct VisFlowState
{
	// Inline; out of line at 0x44a400 (for the unwinding).
							~VisFlowState()		{ DeleteAndClearArray(m_Buffers); }

	CMoArray<VisPortalBuffers*>	m_Buffers;		// vtable 0x475c84
	uint32					m_iDepth;			// 0x14: the buffers in use
	CMoArray<uint8>			m_OnPath;			// 0x18: the leaves on the way here
	CMoArray<uint8>			m_Seen;				// 0x2c: what the portal sees
	CPreLeaf				*m_pLeaf;			// 0x40: set, not read
};

// 0x485ac0: the leaves the vis-BSP walk (0x449c00) collects.
extern CMoArray<CPreLeaf*>	g_VisWalkLeaves;
// 0x485ad8: the bit index of the vis-BSP walk.
extern uint32				g_iVisWalkBit;
// 0x485ae0: the bits that walk reads.
extern class CPreLeafList	*g_pVisWalkBits;


struct PortalBuffer;

void			VisCollectLeaves_R(CNode *pNode);																// 0x449c00
void			VisLeafDone();																					// 0x44a0f0
uint32			VisGetNextLeaf();																				// 0x44a160
void			VisFlowPortal(CPreLeaf *pLeaf, CPrePortal *pPortal);											// 0x44a200
void			VisFlow_R(VisFlowState *pState, CPrePortal *pSource, CPrePortal *pPass, uint8 *pMightSee,
					BOOL bCheckBlocker);																		// 0x44a510
BOOL			VisClipToSeparators(CPrePortal *pSource, CPrePortal *pPass, PortalBuffer *pBuf, BOOL bFlip);	// 0x44a930


// pregeometry.cpp (0x42ccd0).
CPreLeaf* LocatePointInTree(CPreWorld *pWorld, PVector &point, NODEREF iRoot);


// ------------------------------------------------------------------ //
// CUnseenPolyRemover (0x4498d0..0x449b5c).
// ------------------------------------------------------------------ //

// talon: Processing.cpp declares the class with its one public entry point;
// the helpers are this module's (the names are ours).
class CUnseenPolyRemover
{
public:
	uint32					RemoveUnseenPolies(CPreWorld *pWorld, CMoArray<CBaseEditObj*> *pObjects);

	void					MarkInsideLeaves(CPreWorld *pWorld, CMoArray<CBaseEditObj*> *pObjects);
	void					MarkLeaf_R(CPreLeaf *pLeaf);
	uint32					RemovePolies(CPreWorld *pWorld);
};


// 0x4498d0; NOLF's (0x45ad80) checks the root first, a pointer compared
// "< 0": it is never true (retail bug, harmless: the test is dead).
uint32 CUnseenPolyRemover::RemoveUnseenPolies(CPreWorld *pWorld, CMoArray<CBaseEditObj*> *pObjects)
{
	if(pWorld->m_RootNode < 0)
		return 0;

	MarkInsideLeaves(pWorld, pObjects);
	return RemovePolies(pWorld);
}


// Marks (LEAFF_INSIDE) every leaf reachable through the portals from the
// inside leaf and from each InsideDef; with no inside leaf, all of them
// (0x4498f0).
void CUnseenPolyRemover::MarkInsideLeaves(CPreWorld *pWorld, CMoArray<CBaseEditObj*> *pObjects)
{
	CPreLeaf *pLeaf;
	CBaseEditObj *pObj;
	uint32 i, nInside;

	pLeaf = FindInsideLeaf(pWorld, NULL);
	if(!pLeaf)
	{
		for(i=0; i < pWorld->m_Leaves; i++)
			pWorld->m_Leaves[i]->m_Flags |= LEAFF_INSIDE;

		return;
	}

	for(i=0; i < pWorld->m_Leaves; i++)
		pWorld->m_Leaves[i]->m_Flags &= ~LEAFF_INSIDE;

	MarkLeaf_R(pLeaf);

	for(i=0; i < pObjects->GetSize(); i++)
	{
		pObj = (*pObjects)[i];

		if(stricmp(pObj->GetClassName(), "InsideDef") == 0)
		{
			pLeaf = LocatePointInTree(pWorld, pObj->GetPos(), pWorld->m_RootNode);
			if(pLeaf)
				MarkLeaf_R(pLeaf);
		}
	}

	// talon: a count nothing reads; all that is left of it is the pointer
	// walk at 0x4499ba (an empty loop body leaves nothing at all).
	nInside = 0;
	for(i=0; i < pWorld->m_Leaves; i++)
	{
		if(pWorld->m_Leaves[i]->m_Flags & LEAFF_INSIDE)
			nInside++;
	}
}


// 0x4499e0.
void CUnseenPolyRemover::MarkLeaf_R(CPreLeaf *pLeaf)
{
	CPrePortal *pPortal;
	uint32 i;

	pLeaf->m_Flags |= LEAFF_INSIDE;

	for(i=0; i < pLeaf->m_Portals; i++)
	{
		pPortal = pLeaf->m_Portals[i];

		if(pPortal->m_pPartner && !(pPortal->m_pPartner->m_pLeaf->m_Flags & LEAFF_INSIDE))
			MarkLeaf_R(pPortal->m_pPartner->m_pLeaf);
	}
}


// Marks every poly, unmarks those in an inside leaf, and deletes the rest
// but hull makers; returns how many it deleted (0x449a30).
uint32 CUnseenPolyRemover::RemovePolies(CPreWorld *pWorld)
{
	GPOS pos;
	LPOS leafPos;
	CPrePoly *pPoly;
	CPreLeaf *pLeaf;
	uint32 i, nRemoved;

	nRemoved = 0;

	for(pos=pWorld->m_Polies; pos; )
	{
		pPoly = pWorld->m_Polies.GetNext(pos);

		pPoly->SetPostRemove(TRUE);
		if(pPoly->GetSurfaceFlags() & SURF_HULLMAKER)
			pPoly->SetPostRemove(FALSE);
	}

	for(i=0; i < pWorld->m_Leaves; i++)
	{
		pLeaf = pWorld->m_Leaves[i];

		if(pLeaf->m_Flags & LEAFF_INSIDE)
		{
			for(leafPos=pLeaf->m_Polies.GetHeadPosition(); leafPos; )
				pLeaf->m_Polies.GetNext(leafPos)->SetPostRemove(FALSE);
		}
	}

	for(pos=pWorld->m_Polies; pos; )
	{
		pPoly = pWorld->m_Polies.GetNext(pos);

		if(pPoly->GetPostRemove() && !(pPoly->GetSurfaceFlags() & SURF_HULLMAKER))
		{
			++nRemoved;
			pWorld->m_Polies.RemoveAt(pPoly);
			DeletePoly(pPoly);
		}
	}

	return nRemoved;
}


// ------------------------------------------------------------------ //
// The PVS's helpers (0x449b60..0x44ac50).
// ------------------------------------------------------------------ //

// talon: 0x485ac0..0x485aef, in image order (their static initializer is
// 0x449b60..0x449bff).
CMoArray<CPreLeaf*>		g_VisWalkLeaves;
uint32					g_nFastApproxLeaves;
uint32					g_iVisWalkBit;
CPrePortal				*g_pFloodPortal;
CPreLeafList			*g_pVisWalkBits;
PVSInfo					*g_pPVSInfo;
void					*g_pProgressCS;
void					*g_pLeafCS;


// Collects into g_VisWalkLeaves the leaves under pNode whose bits are set,
// taking the bits in g_pVisWalkBits one per leaf and one per child in
// walk order (0x449c00). talon: each bit is taken into a byte before the
// index moves on (the `and` into %cl at 0x449c31); with the test written
// inline the compiler tests memory instead.
void VisCollectLeaves_R(CNode *pNode)
{
	uint8 bSet;

	if(pNode->m_pLeaf)
	{
		bSet = VIS_TESTBIT(g_pVisWalkBits->m_Data, g_iVisWalkBit);
		g_iVisWalkBit++;
		if(bSet)
			g_VisWalkLeaves.Append(pNode->m_pLeaf);
	}

	if(IsValidNode(pNode->m_Sides[0]))
	{
		bSet = VIS_TESTBIT(g_pVisWalkBits->m_Data, g_iVisWalkBit);
		g_iVisWalkBit++;
		if(bSet)
			VisCollectLeaves_R(pNode->m_Sides[0]);
	}

	if(IsValidNode(pNode->m_Sides[1]))
	{
		bSet = VIS_TESTBIT(g_pVisWalkBits->m_Data, g_iVisWalkBit);
		g_iVisWalkBit++;
		if(bSet)
			VisCollectLeaves_R(pNode->m_Sides[1]);
	}
}


// The leaves pBits names, from pRoot down, into g_VisWalkLeaves; pWorld
// is not used (0x449cf0).
void VisCollectLeaves(CPreWorld *pWorld, CNode *pRoot, CPreLeafList *pBits)
{
	g_pVisWalkBits = pBits;
	g_iVisWalkBit = 0;

	g_VisWalkLeaves.Term();
	g_VisWalkLeaves.SetCacheSize(100);

	VisCollectLeaves_R(pRoot);
}


// Floods from pLeaf through the leaves set in allowed, marking each in
// visited and in reached; it does not cross a portal on a vis-blocking
// surface other than iUser's (0x449d60). MakePVS passes -1 for none
// (0x44b6ed) or a user's m_Index.
void VisFloodZone(CMoArray<uint8> &visited, CMoArray<uint8> &allowed, CMoArray<uint8> &reached,
	uint32 iUser, CPreLeaf *pLeaf)
{
	CPrePortal *pPortal;
	PreSurfaceUser *pUser;
	CPreLeaf *pNext;
	uint32 i;

	if(VIS_TESTBIT(visited, pLeaf->m_Index))
		return;

	VIS_SETBIT(visited, pLeaf->m_Index);
	VIS_SETBIT(reached, pLeaf->m_Index);

	for(i=0; i < pLeaf->m_Unknown_00; i++)
	{
		pPortal = pLeaf->m_Unknown_00[i];
		pNext = pPortal->m_pLeaf;

		if(VIS_TESTBIT(allowed, pNext->m_Index))
		{
			if(pPortal->m_pNode && (pUser = (PreSurfaceUser*)pPortal->m_pNode->m_pPoly->m_pSurface->m_Unknown_80) != NULL)
			{
				if(pUser->m_Index == iUser)
					VisFloodZone(visited, allowed, reached, iUser, pNext);
			}
			else
			{
				VisFloodZone(visited, allowed, reached, iUser, pNext);
			}
		}
	}
}


// Looks from pLeaf, through the leaves set in allowed, for a portal on
// iUser's vis-blocking surface; with bCross it goes on through other
// users' surfaces too (0x449e60).
CPrePortal* VisFindZonePortal(CMoArray<uint8> &visited, CMoArray<uint8> &allowed, CPreLeaf *pLeaf,
	uint32 iUser, BOOL bCross)
{
	CPrePortal *pPortal, *pRet;
	PreSurfaceUser *pUser;
	uint32 i;

	if(VIS_TESTBIT(visited, pLeaf->m_Index) || !VIS_TESTBIT(allowed, pLeaf->m_Index))
		return NULL;

	VIS_SETBIT(visited, pLeaf->m_Index);

	for(i=0; i < pLeaf->m_Unknown_00; i++)
	{
		pPortal = pLeaf->m_Unknown_00[i];

		if(pPortal->m_pNode && (pUser = (PreSurfaceUser*)pPortal->m_pNode->m_pPoly->m_pSurface->m_Unknown_80) != NULL)
		{
			if(pUser->m_Index == iUser)
				return pPortal;

			if(!bCross)
				continue;
		}

		if(pPortal->m_pLeaf)
		{
			pRet = VisFindZonePortal(visited, allowed, pPortal->m_pLeaf, iUser, bCross);
			if(pRet)
				return pRet;
		}
	}

	return NULL;
}


// TRUE if every list in lists1 has an equal one (the same bits and the
// same m_Unknown_14) in lists2, which is as long (0x449f10).
BOOL VisCompareLeafLists(CMoArray<CPreLeafList*> &lists1, CMoArray<CPreLeafList*> &lists2)
{
	CPreLeafList *pList1, *pList2;
	uint32 i, j;

	if(lists1.GetSize() != lists2.GetSize())
		return FALSE;

	for(i=0; i < lists1.GetSize(); i++)
	{
		pList1 = lists1[i];

		for(j=0; j < lists2.GetSize(); j++)
		{
			pList2 = lists2[j];

			if(pList1->m_Unknown_14 == pList2->m_Unknown_14 &&
				pList1->m_Data.GetSize() == pList2->m_Data.GetSize() &&
				memcmp(pList1->m_Data.GetArray(), pList2->m_Data.GetArray(), pList1->m_Data.GetSize()) == 0)
			{
				break;
			}
		}

		if(j == lists2.GetSize())
			return FALSE;
	}

	return TRUE;
}


// The radius around pLeaf's center that holds every leaf set in visible:
// into m_Unknown_9c (a float; 0x449fc0).
void VisCalcLeafRadius(CPreWorld *pWorld, CPreLeaf *pLeaf, CMoArray<uint8> &visible)
{
	CPreLeaf *pTest;
	PReal fMax, fDist;
	uint32 i;

	fMax = 0.0f;
	for(i=0; i < pWorld->m_Leaves; i++)
	{
		pTest = pWorld->m_Leaves[i];

		if(VIS_TESTBIT(visible, pTest->m_Index))
		{
			fDist = (pTest->m_vCenter - pLeaf->m_vCenter).Mag() + pTest->m_fRadius;
			if(fDist > fMax)
				fMax = fDist;
		}
	}

	// talon: CPreLeaf::m_Unknown_9c is a float (fstps at 0x44a08e); PreLeaf.h
	// declares it uint32.
	*((PReal*)&pLeaf->m_Unknown_9c) = fMax + pLeaf->m_fRadius;
}


// The zone whose object is named pName (0x44a0a0).
PVSZone* VisFindZone(PVSInfo *pInfo, const char *pName)
{
	PVSZone *pZone;
	uint32 i;

	for(i=0; i < pInfo->m_Zones; i++)
	{
		pZone = pInfo->m_Zones[i];

		if(stricmp(pZone->m_pObj->GetName(), pName) == 0)
			return pZone;
	}

	return NULL;
}


// One more leaf done: the progress callback (0x44a0f0).
void VisLeafDone()
{
	thd_EnterCriticalSection(g_pProgressCS);

	g_pPVSInfo->m_nLeavesDone++;
	if(g_pPVSInfo->m_LeafDoneFn)
		g_pPVSInfo->m_LeafDoneFn((float)g_pPVSInfo->m_nLeavesDone / g_pPVSInfo->m_nLeavesToDo);

	thd_LeaveCriticalSection(g_pProgressCS);
}


// The next leaf for a thread to do, or BAD_INDEX when they are all taken
// (0x44a160). talon: one exit in the source; the compiler gives each arm
// its own Leave and return.
uint32 VisGetNextLeaf()
{
	uint32 iLeaf;

	thd_EnterCriticalSection(g_pLeafCS);

	if(g_pPVSInfo->m_iCurLeaf == g_pPVSInfo->m_iEndLeaf)
		iLeaf = BAD_INDEX;
	else
		iLeaf = g_pPVSInfo->m_iCurLeaf++;

	thd_LeaveCriticalSection(g_pLeafCS);
	return iLeaf;
}


void VisFlowPortal(CPreLeaf *pLeaf, CPrePortal *pPortal);

// A PVS thread: flows the portals of each leaf it takes (0x44a1b0).
void VisThreadFn(void *pData)
{
	CPreLeaf *pLeaf;
	uint32 iLeaf, i;

	while(1)
	{
		iLeaf = VisGetNextLeaf();
		if(iLeaf == BAD_INDEX)
			break;

		pLeaf = g_pPVSInfo->m_pWorld->m_Leaves[iLeaf];

		for(i=0; i < pLeaf->m_Unknown_00; i++)
			VisFlowPortal(pLeaf, pLeaf->m_Unknown_00[i]);

		VisLeafDone();
	}

	g_pPVSInfo->m_nThreadsDone++;
}


// The portal flow (0x44a200..0x44ac48).

BOOL VisClipToSeparators(CPrePortal *pSource, CPrePortal *pPass, PortalBuffer *pBuf, BOOL bFlip);


// From pLeaf through pPortal: what the leaves behind it can see, into the
// portal's m_Unknown_48 (0x44a200). FastApprox leaves are left alone.
void VisFlowPortal(CPreLeaf *pLeaf, CPrePortal *pPortal)
{
	VisFlowState state;

	if(!(pLeaf->m_Flags & LEAFF_FASTAPPROX))
	{
		state.m_OnPath.SetSize(g_pPVSInfo->m_nVisBytes);
		memset(state.m_OnPath.GetArray(), 0, g_pPVSInfo->m_nVisBytes);
		state.m_Seen.SetSize(g_pPVSInfo->m_nVisBytes);
		memset(state.m_Seen.GetArray(), 0, g_pPVSInfo->m_nVisBytes);

		state.m_pLeaf = pLeaf;
		state.m_iDepth = 0;

		VisFlow_R(&state, pPortal, pPortal, pPortal->m_Unknown_48.GetArray(), FALSE);

		memcpy(pPortal->m_Unknown_48.GetArray(), state.m_Seen.GetArray(), g_pPVSInfo->m_nVisBytes);
	}
}


// talon: near. The might-see array's constructor: the shipped code calls
// Clear() (0x420650) and Init(0, 0); ours expands Clear, and the registers
// of the whole function come out rotated from there (pState in esi, not
// edi). MakeHulls (BuildHulls.cpp) is near the same way; neither the inline
// budget levers (more or fewer expansions after it, helpers for the bit
// tests, another constructor) nor the declaration order move it.
//
// One step of the flow: from pPass (the clipped portal into the current
// leaf), through each portal out of it to a leaf pMightSee allows and
// that adds to what is seen, clipped to what pSource lets through
// (0x44a510). bCheckBlocker stops at a blocker's portal.
void VisFlow_R(VisFlowState *pState, CPrePortal *pSource, CPrePortal *pPass, uint8 *pMightSee, BOOL bCheckBlocker)
{
	CMoArray<uint8> mightSee;
	PortalBuffer source, pass;
	VisPortalBuffers *pBuffers;
	CPrePortal *pPortal;
	CPreLeaf *pLeaf, *pNext;
	CPrePlane backPlane;
	uint32 *pPortalBits, *pSeen, *pOut;
	uint32 i, j, more;

	source.m_iCur = 0;
	pass.m_iCur = 0;

	if(bCheckBlocker && pPass->m_pPartner && (pPass->m_pPartner->m_Flags & PORTALF_BLOCKER))
		return;

	if(pState->m_iDepth == pState->m_Buffers.GetSize())
	{
		pBuffers = new VisPortalBuffers;
		pState->m_Buffers.Append(pBuffers);
	}

	pBuffers = pState->m_Buffers[pState->m_iDepth];
	pState->m_iDepth++;

	source.m_pPortals[0] = (CPrePortal*)pBuffers->m_Portals[0];
	source.m_pPortals[1] = (CPrePortal*)pBuffers->m_Portals[1];
	pass.m_pPortals[0] = (CPrePortal*)pBuffers->m_Portals[2];
	pass.m_pPortals[1] = (CPrePortal*)pBuffers->m_Portals[3];

	pLeaf = pPass->m_pLeaf;
	mightSee.SetSize(g_pPVSInfo->m_nVisBytes);

	if(g_pPVSInfo->m_nThreads > 1)
		thd_Sleep(0);

	VIS_SETBIT(pState->m_OnPath, pLeaf->m_Index);
	VIS_SETBIT(pState->m_Seen, pLeaf->m_Index);

	for(i=0; i < pLeaf->m_Unknown_00; i++)
	{
		pPortal = pLeaf->m_Unknown_00[i];
		pNext = pPortal->m_pLeaf;

		if(VIS_TESTBIT(pState->m_OnPath, pNext->m_Index))
			continue;

		if(!VIS_TESTBIT(pMightSee, pNext->m_Index))
			continue;

		pPortalBits = (uint32*)pPortal->m_Unknown_48.GetArray();
		pSeen = (uint32*)pState->m_Seen.GetArray();
		pOut = (uint32*)mightSee.GetArray();

		more = 0;
		for(j=0; j < g_pPVSInfo->m_nVisDWords; j++)
		{
			pOut[j] = pPortalBits[j] & ((uint32*)pMightSee)[j];
			more |= pOut[j] & ~pSeen[j];
		}

		if(!more)
			continue;

		CopyPortal(pPortal, pass.Cur());
		CopyPortal(pSource, source.Cur());

		if(!ClipPortalToPlane(&pSource->m_Plane, &pass))
			continue;

		if(pSource != pPass)
		{
			if(!ClipPortalToPlane(&pSource->m_Plane, &pass))
				continue;

			if(!ClipPortalToPlane(&pPass->m_Plane, &pass))
				continue;

			backPlane.m_Normal = -pass.Cur()->m_Plane.m_Normal;
			backPlane.m_Dist = -pass.Cur()->m_Plane.m_Dist;
			if(!ClipPortalToPlane(&backPlane, &source))
				continue;

			if(!VisClipToSeparators(source.Cur(), pPass, &pass, FALSE))
				continue;

			if(!VisClipToSeparators(pass.Cur(), pPass, &source, TRUE))
				continue;
		}

		pass.Cur()->m_pLeaf = pPortal->m_pLeaf;
		VisFlow_R(pState, source.Cur(), pass.Cur(), mightSee.GetArray(), TRUE);
	}

	VIS_CLEARBIT(pState->m_OnPath, pLeaf->m_Index);
	pState->m_iDepth--;
}


// talon: near, size-exact (792 bytes): scheduling only -- where the pass
// portal's vertex pointer and j start, the base of the source vertex
// walk, and the order of the dot products' terms; the shipped frame has
// one more dword (0x90, ours 0x8c). Named v1/v2 locals, DistTo, const
// pointers, the declaration order and a break-and-test for the goto do not
// close it. The cross product is v2 x v1 in the SDK's Cross (0x44aa3b).
//
// Clips the portal in pBuf to each plane through an edge of pPass and a
// vertex of pSource that has pSource behind it and pPass in front;
// bFlip turns the edges around. FALSE when nothing is left (0x44a930).
BOOL VisClipToSeparators(CPrePortal *pSource, CPrePortal *pPass, PortalBuffer *pBuf, BOOL bFlip)
{
	CPrePlane plane;
	PVector *pts[2];
	PReal fMag;
	uint32 i, j, k;

	for(i=0; i < pPass->NumVerts(); i++)
	{
		pts[bFlip] = &pPass->Pt(i);
		pts[!bFlip] = &pPass->NextPt(i);

		for(j=0; j < pSource->NumVerts(); j++)
		{
			plane.m_Normal = (pSource->Pt(j) - *pts[0]).Cross(*pts[1] - *pts[0]);

			fMag = plane.m_Normal.Mag();
			if(fMag < 0.1f)
				continue;

			plane.m_Normal /= fMag;
			plane.m_Dist = plane.m_Normal.Dot(*pts[0]);

			for(k=0; k < pSource->NumVerts(); k++)
			{
				if(plane.m_Normal.Dot(pSource->Pt(k)) - plane.m_Dist > 0.1)
					goto NextVert;
			}

			for(k=0; k < pPass->NumVerts(); k++)
			{
				if(plane.m_Normal.Dot(pPass->Pt(k)) - plane.m_Dist < -0.1)
					goto NextVert;
			}

			if(!ClipPortalToPlane(&plane, pBuf))
				return FALSE;

NextVert:;
		}
	}

	return TRUE;
}


// Marks pLeaf and the leaves its portals lead to, but through hull maker
// surfaces, FastApprox, counting them in g_nFastApproxLeaves (0x44ac50).
// talon: NOLF's (0x45d550) reads the surface's flags directly; through
// GetSurfaceFlags icl tests m_pSurface twice.
void VisMarkFastApprox_R(CPreLeaf *pLeaf)
{
	CPrePortal *pPortal;
	uint32 i;

	if(pLeaf->m_Flags & LEAFF_FASTAPPROX)
		return;

	pLeaf->m_Flags |= LEAFF_FASTAPPROX;
	g_nFastApproxLeaves++;

	for(i=0; i < pLeaf->m_Unknown_00; i++)
	{
		pPortal = pLeaf->m_Unknown_00[i];

		if(pPortal->m_pNode && pPortal->m_pNode->m_pPoly && pPortal->m_pNode->m_pPoly->m_pSurface &&
			!(pPortal->m_pNode->m_pPoly->m_pSurface->m_Flags & SURF_HULLMAKER) && pPortal->m_pLeaf)
		{
			VisMarkFastApprox_R(pPortal->m_pLeaf);
		}
	}
}
