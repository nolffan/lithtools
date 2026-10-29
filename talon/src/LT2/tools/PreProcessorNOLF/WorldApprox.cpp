// talon: a module of its own in the image, 0x0044acd0..0x0044c9d0, after
// Processing: the PVS maker from FastApproxArea on (the file's name is
// ours). The rest of the same module, before it (0x4498d0..0x44acd0), is
// VisLeaves.cpp: in the image they are one, their globals together in
// .data (0x485ac0..0x485aef). Here: the FastApproxArea marking (0x44acd0),
// the Zones (0x44add0..0x44b270), MakePVS (0x44b270), the leaf lists made
// with the vis-blocking surfaces (0x44b460..0x44c140), the portals' vis
// (0x44c140..0x44c580) and the portal setup (0x44c580).
//
// Written from the image; neither Build 69 nor nolfsrc has any of it.

#include "bdefs.h"
#include "preworld.h"
#include "preleaf.h"
#include "prepoly.h"
#include "presurface.h"
#include "node.h"
#include "editobjects.h"
#include "processing.h"
#include "threads.h"
#include "VisLeaves.h"


// pregeometry.cpp (0x42ccd0).
CPreLeaf* LocatePointInTree(CPreWorld *pWorld, PVector &point, NODEREF iRoot);



// The bits of a leaf set: a byte array indexed by CPreLeaf::m_Index.
#define WA_TESTBIT(arr, i)		((arr)[(i) >> 3] & (1 << ((i) & 7)))
#define WA_SETBIT(arr, i)		((arr)[(i) >> 3] |= (1 << ((i) & 7)))
#define WA_CLEARBIT(arr, i)		((arr)[(i) >> 3] &= ~(1 << ((i) & 7)))


static void		MakeLeafLists();												// 0x44b460
static void		WriteListBits(CMoArray<uint8> &vis, CPreLeafList *pList, uint32 nLeaves);	// 0x44bb20
static void		WriteTreeBits_R(CNode *pNode, CMoArray<uint8> &bits, uint32 &iBit);	// 0x44bc10
static void		ClearNodesVisible_R(CNode *pNode);								// 0x44be50
static void		FindSurfaceUsers(CPreWorld *pWorld, CMoArray<uint8> &vis, CMoArray<PreSurfaceUser*> &users);	// 0x44be90
static void		RemoveBits(CMoArray<uint8> &bits, CMoArray<uint8> &toRemove);	// 0x44c070
static BOOL		AnyBits(CMoArray<uint8> &bits);									// 0x44c0c0
static CPreLeaf*	FindSameLists(CPreWorld *pWorld, CMoArray<CPreLeafList*> &lists, uint32 nLeaves);	// 0x44c0f0
static void		MakePortalsVis();												// 0x44c140
static void		MakePortalVis(CPrePortal *pPortal);								// 0x44c2e0
static void		FloodPortalVis_R(CPreLeaf *pLeaf, CPrePortal *pFrom, float fDistSqr);	// 0x44c3b0
static BOOL		CanSeeThrough(CPrePortal *pPortal, CPrePortal *pTest);			// 0x44c4a0
static void		SetupPortals();													// 0x44c580


// ------------------------------------------------------------------ //
// FastApproxArea, Zones (0x44acd0..0x44b270).
// ------------------------------------------------------------------ //

// Marks the leaves the FastApproxArea objects are in (and what
// VisMarkFastApprox_R reaches from them) FastApprox (0x44acd0).
void MarkFastApproxAreas(CPreWorld *pWorld)
{
	CBaseEditObj *pObj;
	CPreLeaf *pLeaf;
	PVector vPos;
	uint32 i;

	g_nFastApproxLeaves = 0;

	for(i=0; i < pWorld->m_Leaves; i++)
		pWorld->m_Leaves[i]->m_Flags &= ~LEAFF_FASTAPPROX;

	for(i=0; i < pWorld->m_pMainWorld->m_Objects; i++)
	{
		pObj = pWorld->m_pMainWorld->m_Objects[i];

		if(strcmp(pObj->GetClassName(), "FastApproxArea") == 0)
		{
			VEC_COPY(vPos, pObj->GetPos());
			pLeaf = LocatePointInTree(pWorld, vPos, pWorld->m_RootNode);
			if(pLeaf)
				VisMarkFastApprox_R(pLeaf);
		}
	}

	DrawStatusText("Marked %d out of %d leaves as FastApprox", g_nFastApproxLeaves, pWorld->m_Leaves.GetSize());
}


// The box around a world model's polies (0x44add0).
static void GetPolyBounds(CPreWorld *pWorld, PVector *pMin, PVector *pMax)
{
	GPOS pos;

	pMin->Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	pMax->Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);

	for(pos=pWorld->m_Polies; pos; )
	{
		pWorld->m_Polies.GetNext(pos)->ExtendBounds(*pMin, *pMax);
	}
}


// Whether the box around the leaf's portals is inside the zone's box
// (0x44aec0).
static BOOL IsLeafInZone(CPreLeaf *pLeaf, PVSZone *pZone)
{
	PVector vMin, vMax;
	uint32 i;

	vMin.Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	vMax.Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);

	for(i=0; i < pLeaf->m_Portals; i++)
		pLeaf->m_Portals[i]->m_BasePoly.ExtendBounds(vMin, vMax);

	for(i=0; i < 3; i++)
	{
		if(vMin[i] < pZone->m_vMin[i] || vMax[i] > pZone->m_vMax[i])
			return FALSE;
	}

	return TRUE;
}


// Makes a zone for each Zone object (its world model's box) and gives the
// vis BSP leaves inside it its index (CPreLeaf::m_Unknown_a0, -1 for none;
// 0x44aff0).
static void MakeZones(PVSInfo *pInfo)
{
	CPreWorld *pWorld, *pModel;
	CBaseEditObj *pObj;
	CPreLeaf *pLeaf;
	PVSZone *pZone;
	uint32 i, j;

	pWorld = pInfo->m_pWorld;
	for(i=0; i < pWorld->m_Leaves; i++)
		pInfo->m_pWorld->m_Leaves[i]->m_Unknown_a0 = (uint32)-1;

	for(i=0; i < pWorld->m_pMainWorld->m_Objects; i++)
	{
		pObj = pWorld->m_pMainWorld->m_Objects[i];

		if(stricmp(pObj->GetClassName(), "Zone") != 0)
			continue;

		pModel = pWorld->m_pMainWorld->FindWorldModel(pObj->GetName());
		if(!pModel)
		{
			DrawStatusText("Can't find world model for zone '%s'", pObj->GetName());
			continue;
		}

		pZone = new PVSZone;
		pZone->m_pObj = pObj;
		pInfo->m_Zones.Append(pZone);

		GetPolyBounds(pModel, &pZone->m_vMin, &pZone->m_vMax);

		for(j=0; j < pWorld->m_Leaves; j++)
		{
			pLeaf = pWorld->m_Leaves[j];

			if(IsLeafInZone(pLeaf, pZone))
			{
				pLeaf->m_Unknown_a0 = pInfo->m_Zones.GetSize() - 1;
				pZone->m_Leaves.Append(pLeaf);
			}
		}
	}
}


// ------------------------------------------------------------------ //
// MakePVS (0x44b270).
// ------------------------------------------------------------------ //

#define MAX_PVS_THREADS		10

// Makes the vis BSP's PVS; returns 0 (0x44b270).
uint32 MakePVS(PVSInfo *pInfo)
{
	THREAD_ID threadIDs[MAX_PVS_THREADS];
	uint32 i, iDone, iCur;

	pInfo->m_nThreads = LTCLAMP(pInfo->m_nThreads, 1, MAX_PVS_THREADS);

	if(pInfo->m_iFirstLeaf == 0 && pInfo->m_iEndLeaf == 0)
		pInfo->m_iEndLeaf = pInfo->m_pWorld->m_Leaves.GetSize();

	g_pProgressCS = thd_CreateCriticalSection();
	g_pLeafCS = thd_CreateCriticalSection();

	g_pPVSInfo = pInfo;
	g_pPVSInfo->m_nLeavesDone = 0;
	g_pPVSInfo->m_nLeavesToDo = pInfo->m_iEndLeaf - pInfo->m_iFirstLeaf;

	pInfo->m_pWorld->SetPolyIndices();

	// Bytes in a leaf set, a dword more than enough.
	g_pPVSInfo->m_nVisBytes = (pInfo->m_pWorld->m_Leaves.GetSize() >> 3) + 1;
	g_pPVSInfo->m_nVisBytes = (g_pPVSInfo->m_nVisBytes - (g_pPVSInfo->m_nVisBytes % 4)) + 4;
	g_pPVSInfo->m_nVisDWords = g_pPVSInfo->m_nVisBytes / 4;

	MakeZones(pInfo);
	SetupPortals();
	MakePortalsVis();

	if(!g_pPVSInfo->m_bFast)
	{
		MarkFastApproxAreas(pInfo->m_pWorld);

		g_pPVSInfo->m_iCurLeaf = pInfo->m_iFirstLeaf;
		g_pPVSInfo->m_nThreadsDone = 0;

		if(g_pPVSInfo->m_nThreads == 1)
		{
			VisThreadFn(NULL);
		}
		else
		{
			for(i=0; i < g_pPVSInfo->m_nThreads; i++)
				threadIDs[i] = thd_BeginThread(VisThreadFn, NULL);
		}

		iDone = g_pPVSInfo->m_iFirstLeaf;
		for(;;)
		{
			iCur = g_pPVSInfo->m_iCurLeaf;
			if(iCur > iDone)
			{
				if(g_pPVSInfo->m_Unknown_1c)
					g_pPVSInfo->m_Unknown_1c(iDone, iCur);

				iDone = iCur;
			}

			if(g_pPVSInfo->m_nThreadsDone == g_pPVSInfo->m_nThreads)
				break;

			thd_Sleep(100);
		}
	}

	MakeLeafLists();

	if(g_pPVSInfo->m_Unknown_18)
	{
		for(i=0; i < pInfo->m_pWorld->m_Leaves; i++)
			pInfo->m_pWorld->m_Leaves[i]->m_Unknown_00.Term();
	}

	thd_DeleteCriticalSection(g_pProgressCS);
	thd_DeleteCriticalSection(g_pLeafCS);
	g_pProgressCS = NULL;

	return 0;
}


// ------------------------------------------------------------------ //
// The leaf lists (0x44b460..0x44c140).
// ------------------------------------------------------------------ //

// talon: stand-in: a dead inline call that spends MakeLeafLists' inline
// budget (see below). It does nothing and compiles to nothing.
static inline int StandIn_InlineBudget(int a, int b, int c, int d)
{
	if(a < c && b < c)
		return 0;
	else if (a > d && b > d)
		return 1;
	else
		return 2;
}


// Gives each leaf its lists (CPreLeaf::m_LeafLists): the leaves it sees
// with every vis-blocking surface closed (m_Unknown_14 -1), and for each
// such surface it sees, the ones more it sees through it (m_Unknown_14 the
// surface's index). A leaf whose lists are an earlier leaf's gets that
// leaf's index (m_LeafLists.m_iSameAs) instead (0x44b460).
//
// talon: the inline budget: users' destructor calls _DeleteAndDestroyArray
// (0x44ba24; the four byte arrays' destructors after it expand it), which
// only the stand-in below reproduces (8 bytes long without it). The memsets
// through &x[0] (the shipped budget: with GetArray() the second array's
// Init is expanded too) and the rest are the spellings that came closest
// (a search over the equivalent ones: Get/GetAt/[], SetSize/SetSize2,
// Append/Add/Push, DeleteAndClearArray or the loop written out; then
// users.GetSize() as the loop's bound, the leaf counts and through's size
// by operator conversion, Get(i), WA_SETBIT through operator[], and the
// leaf's bit set through an inline function, which spends too much).
static void MakeLeafLists()
{
	CMoArray<uint8> leafVis, seen, through, visited;
	CMoArray<PreSurfaceUser*> users;
	CPreLeaf *pLeaf, *pSame;
	CPrePortal *pPortal;
	CPreLeafList *pList;
	uint32 iLeaf, i, j;

	// talon: stand-in: two dead calls (no code, no behaviour) that spend
	// as much of the inline budget as the 2001 source spent on something
	// we have not found: with them users' destructor calls
	// _DeleteAndDestroyArray (0x44ba24) as shipped and the function is
	// byte-identical; with one it is expanded, with four the first
	// array's Init is called too. Real spellings tried are listed above
	// the function.
	StandIn_InlineBudget(0,0,0,0);
	StandIn_InlineBudget(0,0,0,0);

	g_pPVSInfo->m_pWorld->CalcLeafSpheres();
	g_pPVSInfo->m_pWorld->m_RootNode->SetParentNodes(NULL);

	leafVis.SetSize(g_pPVSInfo->m_nVisBytes);
	seen.SetSize(g_pPVSInfo->m_nVisBytes);
	through.SetSize(g_pPVSInfo->m_nVisBytes);
	visited.SetSize(g_pPVSInfo->m_nVisBytes);

	g_pPVSInfo->m_pWorld->SetLeafIndices();

	for(iLeaf=g_pPVSInfo->m_iFirstLeaf; iLeaf < g_pPVSInfo->m_iEndLeaf; iLeaf++)
	{
		pLeaf = g_pPVSInfo->m_pWorld->m_Leaves[iLeaf];

		// What its portals see, and itself.
		memset(leafVis.GetArray(), 0, g_pPVSInfo->m_nVisBytes);
		for(i=0; i < pLeaf->m_Unknown_00; i++)
		{
			pPortal = pLeaf->m_Unknown_00[i];
			for(j=0; j < g_pPVSInfo->m_nVisBytes; j++)
				leafVis[j] |= pPortal->m_Unknown_48[j];
		}

		WA_SETBIT(leafVis.GetArray(), pLeaf->m_Index);

		VisCalcLeafRadius(g_pPVSInfo->m_pWorld, pLeaf, leafVis);

		// With every surface closed.
		pList = new CPreLeafList;
		pLeaf->m_LeafLists.m_Lists.Append(pList);

		memset(&seen[0], 0, g_pPVSInfo->m_nVisBytes);
		memset(&visited[0], 0, g_pPVSInfo->m_nVisBytes);
		VisFloodZone(visited, leafVis, seen, (uint32)-1, pLeaf);

		pList->m_Unknown_14 = (uint32)-1;
		WriteListBits(seen, pList, g_pPVSInfo->m_pWorld->m_Leaves.GetSize());
		VisCollectLeaves(g_pPVSInfo->m_pWorld, g_pPVSInfo->m_pWorld->m_RootNode, pList);

		// Through each surface.
		FindSurfaceUsers(g_pPVSInfo->m_pWorld, leafVis, users);
		for(i=0; i < users; i++)
		{
			memset(&through[0], 0, through.GetSize());
			memset(&visited[0], 0, g_pPVSInfo->m_nVisBytes);

			pPortal = VisFindZonePortal(visited, leafVis, pLeaf, users[i]->m_Index, FALSE);
			if(!pPortal)
			{
				memset(&visited[0], 0, g_pPVSInfo->m_nVisBytes);
				pPortal = VisFindZonePortal(visited, leafVis, pLeaf, users[i]->m_Index, TRUE);
			}

			if(pPortal && pPortal->m_pLeaf)
			{
				memset(&visited[0], 0, g_pPVSInfo->m_nVisBytes);
				VisFloodZone(visited, leafVis, through, (uint32)-1, pPortal->m_pLeaf);
			}

			RemoveBits(through, seen);
			if(AnyBits(through))
			{
				pList = new CPreLeafList;
				pLeaf->m_LeafLists.m_Lists.Append(pList);

				pList->m_Unknown_14 = users[i]->m_Index;
				WriteListBits(through, pList, g_pPVSInfo->m_pWorld->m_Leaves.GetSize());
				VisCollectLeaves(g_pPVSInfo->m_pWorld, g_pPVSInfo->m_pWorld->m_RootNode, pList);
			}
		}

		if(iLeaf > 0)
		{
			pSame = FindSameLists(g_pPVSInfo->m_pWorld, pLeaf->m_LeafLists.m_Lists, iLeaf-1);
			if(pSame)
			{
				pLeaf->m_LeafLists.m_iSameAs = pSame->m_Index;
				DeleteAndClearArray(pLeaf->m_LeafLists.m_Lists);
			}
		}
	}
}


// Writes pList's bits: the visible leaves, as the walk of the BSP down the
// nodes that lead to them meets them (0x44bb20).
static void WriteListBits(CMoArray<uint8> &vis, CPreLeafList *pList, uint32 nLeaves)
{
	CNode *pNode;
	uint32 i, iBit;

	ClearNodesVisible_R(g_pPVSInfo->m_pWorld->m_RootNode);

	for(i=0; i < g_pPVSInfo->m_pWorld->m_Leaves; i++)
		g_pPVSInfo->m_pWorld->m_Leaves[i]->m_Flags &= ~LEAFF_VISITED;

	for(i=0; i < nLeaves; i++)
	{
		if(WA_TESTBIT(vis.GetArray(), i))
		{
			g_pPVSInfo->m_pWorld->m_Leaves[i]->m_Flags |= LEAFF_VISITED;

			for(pNode=g_pPVSInfo->m_pWorld->m_Leaves[i]->m_pNode; pNode; pNode=pNode->m_pParentNode)
				pNode->m_Unknown_1a |= NODEF_PVSVISIBLE;
		}
	}

	iBit = 0;
	WriteTreeBits_R(g_pPVSInfo->m_pWorld->m_RootNode, pList->m_Data, iBit);
}


// Appends a bit.
inline void WriteBit(CMoArray<uint8> &bits, uint32 &iBit, BOOL bSet)
{
	if((iBit >> 3) >= bits.GetSize())
		bits.Append(0);

	bits[iBit >> 3] |= (bSet != 0) << (iBit & 7);
	iBit++;
}

// Down the visible nodes: a bit for each one's leaf, and for each side
// whether it's visible (0x44bc10).
static void WriteTreeBits_R(CNode *pNode, CMoArray<uint8> &bits, uint32 &iBit)
{
	CNode *pFront, *pBack;

	if(!IsValidNode(pNode) || !(pNode->m_Unknown_1a & NODEF_PVSVISIBLE))
		return;

	if(pNode->m_pLeaf)
		WriteBit(bits, iBit, (pNode->m_pLeaf->m_Flags >> 1) & 1);

	pFront = pNode->m_Sides[0];
	pBack = pNode->m_Sides[1];

	if(IsValidNode(pFront))
		WriteBit(bits, iBit, (pFront->m_Unknown_1a >> 1) & 1);

	WriteTreeBits_R(pFront, bits, iBit);

	if(IsValidNode(pBack))
		WriteBit(bits, iBit, (pBack->m_Unknown_1a >> 1) & 1);

	WriteTreeBits_R(pBack, bits, iBit);
}


// 0x44be50.
static void ClearNodesVisible_R(CNode *pNode)
{
	if(!IsValidNode(pNode))
		return;

	pNode->m_Unknown_1a &= ~NODEF_PVSVISIBLE;

	if(pNode->m_Sides[1])
		ClearNodesVisible_R(pNode->m_Sides[1]);

	if(pNode->m_Sides[0])
		ClearNodesVisible_R(pNode->m_Sides[0]);
}


// The vis-blocking surfaces on the portals of the leaves in vis (0x44be90).
static void FindSurfaceUsers(CPreWorld *pWorld, CMoArray<uint8> &vis, CMoArray<PreSurfaceUser*> &users)
{
	CPreLeaf *pLeaf;
	CPrePortal *pPortal;
	PreSurfaceUser *pUser;
	uint32 i, iBit, iLeaf, j;

	iLeaf = 0;
	for(i=0; i < vis; i++)
	{
		for(iBit=0; iBit < 8; iLeaf++, iBit++)
		{
			if(!(vis[i] & (1 << iBit)))
				continue;

			pLeaf = pWorld->m_Leaves[iLeaf];
			for(j=0; j < pLeaf->m_Unknown_00; j++)
			{
				pPortal = pLeaf->m_Unknown_00[j];

				if(pPortal->m_pNode)
				{
					pUser = (PreSurfaceUser*)pPortal->m_pNode->m_pPoly->GetSurface()->m_Unknown_80;
					if(pUser)
					{
						if(users.FindElement(pUser) == BAD_INDEX)
							users.Append(pUser);
					}
				}
			}
		}
	}
}


// Clears the bits of toRemove in bits (0x44c070).
static void RemoveBits(CMoArray<uint8> &bits, CMoArray<uint8> &toRemove)
{
	uint32 i, iBit;

	for(i=0; i < toRemove; i++)
	{
		for(iBit=0; iBit < 8; iBit++)
		{
			if(toRemove[i] & (1 << iBit))
				bits[i] &= ~(1 << iBit);
		}
	}
}


// 0x44c0c0.
static BOOL AnyBits(CMoArray<uint8> &bits)
{
	uint32 i;

	for(i=0; i < bits; i++)
	{
		if(bits[i])
			return TRUE;
	}

	return FALSE;
}


// The first of the first nLeaves leaves whose lists are the same as lists
// (0x44c0f0).
static CPreLeaf* FindSameLists(CPreWorld *pWorld, CMoArray<CPreLeafList*> &lists, uint32 nLeaves)
{
	CPreLeaf *pLeaf;
	uint32 i;

	for(i=0; i < nLeaves; i++)
	{
		pLeaf = pWorld->m_Leaves[i];
		if(VisCompareLeafLists(pLeaf->m_LeafLists.m_Lists, lists))
			return pLeaf;
	}

	return NULL;
}


// ------------------------------------------------------------------ //
// The portals' vis (0x44c140..0x44c580).
// ------------------------------------------------------------------ //

// Makes each portal's leaf set (CPrePortal::m_Unknown_48), less the
// leaves of the zones its leaf's Zone has a FALSE bool property for
// (0x44c140).
static void MakePortalsVis()
{
	CPreWorld *pWorld;
	CPreLeaf *pLeaf;
	CPrePortal *pPortal;
	PVSZone *pZone, *pOther;
	CBaseProp *pProp;
	uint32 i, j, k, iLeaf;

	pWorld = g_pPVSInfo->m_pWorld;
	g_pPVSInfo->m_PortalVis.SetSize(g_pPVSInfo->m_nPortalBytes);

	for(i=0; i < pWorld->m_Leaves; i++)
	{
		pLeaf = pWorld->m_Leaves[i];

		for(j=0; j < pLeaf->m_Unknown_00; j++)
		{
			pPortal = pLeaf->m_Unknown_00[j];
			MakePortalVis(pPortal);

			if(pLeaf->m_Unknown_a0 == (uint32)-1)
				continue;

			pZone = g_pPVSInfo->m_Zones[pLeaf->m_Unknown_a0];
			for(k=0; k < pZone->m_pObj->m_PropList.m_Props; k++)
			{
				pProp = pZone->m_pObj->m_PropList.m_Props[k];

				if(pProp->m_Type == PT_BOOL && !((CBoolProp*)pProp)->m_Value)
				{
					pOther = VisFindZone(g_pPVSInfo, pProp->m_Name);
					if(pOther)
					{
						for(iLeaf=0; iLeaf < pOther->m_Leaves; iLeaf++)
							WA_CLEARBIT(pPortal->m_Unknown_48.GetArray(), pOther->m_Leaves[iLeaf]->m_Index);
					}
				}
			}
		}

		if(g_pPVSInfo->m_ProgressFn)
			g_pPVSInfo->m_ProgressFn((float)i / (float)pWorld->m_Leaves.GetSize());
	}

	g_pPVSInfo->m_PortalVis.Term();
}


// pPortal's leaf set: the leaves reached through the portals on its other
// side that it can see (0x44c2e0).
static void MakePortalVis(CPrePortal *pPortal)
{
	pPortal->m_Unknown_48.SetSize(g_pPVSInfo->m_nVisBytes);
	memset(pPortal->m_Unknown_48.GetArray(), 0, g_pPVSInfo->m_nVisBytes);
	memset(g_pPVSInfo->m_PortalVis.GetArray(), 0, g_pPVSInfo->m_nPortalBytes);

	g_pFloodPortal = pPortal;
	g_pPVSInfo->m_pCurVis = pPortal->m_Unknown_48.GetArray();
	FloodPortalVis_R(pPortal->m_pLeaf, pPortal, 0.0f);
}


// 0x44c3b0.
//
// talon: near, size-exact: the head loads g_pPVSInfo into ecx before the
// frame where the shipped code loads it into edx after (and so the lea's
// operands swap). The index and set locals and VEC_MAGSQR (the member
// MagSqr sums the terms in another order) are what brought the rest in.
static void FloodPortalVis_R(CPreLeaf *pLeaf, CPrePortal *pFrom, float fDistSqr)
{
	CPrePortal *pPortal;
	uint32 iIndex;
	uint8 *pVis;
	PVector vDiff;
	uint32 i;

	iIndex = pLeaf->m_Index;
	pVis = g_pPVSInfo->m_pCurVis;
	if(WA_TESTBIT(pVis, iIndex))
		return;

	WA_SETBIT(pVis, iIndex);

	for(i=0; i < pLeaf->m_Unknown_00; i++)
	{
		pPortal = pLeaf->m_Unknown_00[i];

		vDiff = pPortal->m_vCenter - pFrom->m_vCenter;
		if(CanSeeThrough(g_pFloodPortal, pPortal))
			FloodPortalVis_R(pPortal->m_pLeaf, pPortal, fDistSqr + VEC_MAGSQR(vDiff));
	}
}


// Whether pTest is partly in front of pPortal and pPortal partly behind
// pTest (0x44c4a0).
static BOOL CanSeeThrough(CPrePortal *pPortal, CPrePortal *pTest)
{
	CPrePlane *pPlane;
	uint32 i;

	pPlane = &pPortal->m_Plane;
	for(i=0; i < pTest->NumVerts(); i++)
	{
		if(pPlane->m_Normal.Dot(pTest->Pt(i)) - pPlane->m_Dist > 0.1)
		{
			pPlane = &pTest->m_Plane;
			for(i=0; i < pPortal->NumVerts(); i++)
			{
				if(pPlane->m_Normal.Dot(pPortal->Pt(i)) - pPlane->m_Dist < -0.1)
					return TRUE;
			}

			return FALSE;
		}
	}

	return FALSE;
}


// Gives each leaf (m_Unknown_00) the partners of its portals that have
// them, puts those in the info's list, and sets their centers (0x44c580).
static void SetupPortals()
{
	CPreWorld *pWorld;
	CPreLeaf *pLeaf;
	CPrePortal *pPortal;
	uint32 i, j, iPortal;

	pWorld = g_pPVSInfo->m_pWorld;
	g_pPVSInfo->m_nPortals = 0;
	g_pPVSInfo->m_pWorld->SetLeafIndices();

	for(i=0; i < pWorld->m_Leaves; i++)
	{
		pLeaf = pWorld->m_Leaves[i];
		pLeaf->m_Unknown_00.SetSize(0);

		for(j=0; j < pLeaf->m_Portals; j++)
		{
			pPortal = pLeaf->m_Portals[j];

			if(pPortal->m_pPartner && pPortal->m_pPartner->m_pPartner == pPortal)
			{
				pLeaf->m_Unknown_00.Append(pPortal->m_pPartner);
				g_pPVSInfo->m_nPortals++;
			}
		}
	}

	g_pPVSInfo->m_Portals.SetSize(g_pPVSInfo->m_nPortals);

	iPortal = 0;
	for(i=0; i < pWorld->m_Leaves; i++)
	{
		pLeaf = pWorld->m_Leaves[i];

		for(j=0; j < pLeaf->m_Unknown_00; j++)
		{
			g_pPVSInfo->m_Portals[iPortal] = pLeaf->m_Unknown_00[j];
			iPortal++;
		}
	}

	for(i=0; i < g_pPVSInfo->m_Portals; i++)
	{
		pPortal = g_pPVSInfo->m_Portals[i];

		pPortal->m_vCenter.Init();
		for(j=0; j < pPortal->NumVerts(); j++)
			pPortal->m_vCenter += pPortal->Pt(j);

		pPortal->m_vCenter /= (PReal)pPortal->NumVerts();
	}

	g_pPVSInfo->m_nPortalBytes = (g_pPVSInfo->m_nPortals >> 3) + 1;
}
