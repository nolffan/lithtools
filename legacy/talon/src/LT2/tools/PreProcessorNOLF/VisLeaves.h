// talon: the PVS (the names are ours). In the image it is one module,
// VisLeaves.cpp (0x4498d0..0x44acd0) and WorldApprox.cpp (FastApproxArea
// and MakePVS, 0x44acd0..0x44c9d0): their globals sit together in .data
// (0x485ac0..0x485aef). What the two files and Processing's MakeWorldPVS
// share: the PVS maker's state and VisLeaves.cpp's helpers.

#ifndef __VISLEAVES_H__
#define __VISLEAVES_H__

	class CPreWorld;
	class CPreLeaf;
	class CPreLeafList;
	class CPrePortal;
	class CPreSurface;
	class CNode;
	class CBaseEditObj;

	// CPreLeaf::m_Flags (LEAFF_VISITED, 1<<1, is PreLeaf.h's: the PVS uses
	// it for "visible from the leaf being done", 0x44bb20).
	#ifndef LEAFF_FASTAPPROX
		// Marked by a FastApproxArea (VisMarkFastApprox_R, 0x44ac50).
		#define LEAFF_FASTAPPROX	(1<<2)
	#endif

	// CNode::m_Unknown_1a: on the way from the root to a visible leaf
	// (0x44bb20, 0x44be50).
	#define NODEF_PVSVISIBLE		(1<<1)


	// What CPreSurface::m_Unknown_80 points to: one of the world's
	// m_Unknown_04 (the surfaces that can block vis, as CPreWorld::LoadFile
	// fills them). talon: PreWorld.cpp defines it too, for LoadFile: in
	// PreWorld.h it would cost PreWorld.cpp's static CMoArray destructor
	// (0x430a90, the header declaration-count threshold, tools/guides/09).
	struct PreSurfaceUser : public CGLLNode
	{
		CPreSurface		*m_pSurface;
		const char		*m_pName;
		uint32			m_Index;			// 0x10
	};


	// A Zone object's world model box and the vis BSP leaves inside it
	// (0x30 bytes, 0x44aff0).
	struct PVSZone
	{
		CBaseEditObj			*m_pObj;
		PVector					m_vMin, m_vMax;
		CMoArray<CPreLeaf*>		m_Leaves;			// 0x1c
	};


	// The PVS maker's state: MakePVS's argument (0x84 bytes: Processing's
	// MakeWorldPVS, 0x43f170, zeroes it), kept in g_pPVSInfo while it runs.
	struct PVSInfo
	{
		uint32					m_iCurLeaf;			// 0x00: the next leaf a thread takes
		uint32					m_iFirstLeaf;		// 0x04
		uint32					m_iEndLeaf;			// 0x08: 0 (with m_iFirstLeaf 0) for all
		BOOL					m_bFast;			// 0x0c: no full PVS, just the approximation
		void					(*m_LeafDoneFn)(float fPercent);	// 0x10: as the threads finish leaves (0x44a0f0)
		void					(*m_ProgressFn)(float fPercent);	// 0x14: as the portals' vis is made (0x44c140)
		BOOL					m_Unknown_18;		// 0x18: set: the leaves' portal lists are freed after
		void					(*m_Unknown_1c)(uint32 iFrom, uint32 iTo);	// 0x1c: told the leaves done so far
		CPreWorld				*m_pWorld;			// 0x20
		uint32					m_nThreads;			// 0x24: clamped to 1..10
		uint32					m_nThreadsDone;		// 0x28
		uint32					m_nLeavesDone;		// 0x2c
		uint32					m_nLeavesToDo;		// 0x30
		uint32					m_nPortals;			// 0x34
		CMoArray<CPrePortal*>	m_Portals;			// 0x38: every leaf's m_Unknown_00
		uint32					m_nVisBytes;		// 0x4c: bytes in a leaf set (dword-rounded)
		uint32					m_nVisDWords;		// 0x50
		uint32					m_nPortalBytes;		// 0x54: bytes in a portal set
		uint8					*m_pCurVis;			// 0x58: the leaf set being flooded (0x44c3b0)
		CMoArray<uint8>			m_PortalVis;		// 0x5c
		CMoArray<PVSZone*>		m_Zones;			// 0x70
	};


	// The globals the module shares: VisLeaves.cpp defines them.
	// 0x485ad4: how many leaves VisMarkFastApprox_R marked FastApprox.
	extern uint32				g_nFastApproxLeaves;
	// 0x485adc: the portal whose vis FloodPortalVis_R floods (0x44c3b0).
	extern CPrePortal			*g_pFloodPortal;
	// 0x485ae4.
	extern PVSInfo				*g_pPVSInfo;
	// 0x485ae8, 0x485aec: critical sections (thd_CreateCriticalSection):
	// the progress count, the next leaf.
	extern void					*g_pProgressCS;
	extern void					*g_pLeafCS;


	// VisLeaves.cpp's functions (0x449c00..0x44ac50) that these call, with
	// its names. iUser is a PreSurfaceUser::m_Index: -1 for none, and what
	// 0x44b8a9 stores in CPreLeafList::m_Unknown_14 (which LoadFile reads
	// as a word).

	// The leaves pList names, from pRoot down, into its leaf walk list
	// (0x449cf0).
	void			VisCollectLeaves(CPreWorld *pWorld, CNode *pRoot, CPreLeafList *pList);

	// Floods from pLeaf through the portals into the leaves set in allowed,
	// stopping at vis-blocking surfaces other than iUser's; sets each leaf
	// reached in visited and out (0x449d60).
	void			VisFloodZone(CMoArray<uint8> &visited, CMoArray<uint8> &allowed, CMoArray<uint8> &out, uint32 iUser, CPreLeaf *pLeaf);

	// Floods the same way to the first portal on iUser's surface; through
	// other users' surfaces only with bThrough (0x449e60).
	CPrePortal*		VisFindZonePortal(CMoArray<uint8> &visited, CMoArray<uint8> &allowed, CPreLeaf *pLeaf, uint32 iUser, BOOL bThrough);

	// Whether two leaves' lists hold the same sets (0x449f10).
	BOOL			VisCompareLeafLists(CMoArray<CPreLeafList*> &lists1, CMoArray<CPreLeafList*> &lists2);

	// Sets pLeaf's m_Unknown_9c from the spheres of the leaves in vis
	// (0x449fc0).
	void			VisCalcLeafRadius(CPreWorld *pWorld, CPreLeaf *pLeaf, CMoArray<uint8> &vis);

	// The zone whose object is named pName (0x44a0a0).
	PVSZone*		VisFindZone(PVSInfo *pInfo, const char *pName);

	// The threads' function: takes leaves until there are none (0x44a1b0).
	void			VisThreadFn(void *pData);

	// Marks pLeaf and what it reaches FastApprox (0x44ac50).
	void			VisMarkFastApprox_R(CPreLeaf *pLeaf);


#endif
