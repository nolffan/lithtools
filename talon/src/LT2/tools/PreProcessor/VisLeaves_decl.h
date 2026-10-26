// talon: what VisLeaves.cpp (0x4498d0..0x44acd0) shares with the rest of
// the PVS (MakePVS, 0x44b270, and its helpers after 0x44acd0). In the
// image these are one module: the globals below sit together in .data
// (0x485ac0..0x485aef). All names are ours.

#ifndef __VISLEAVES_DECL_H__
#define __VISLEAVES_DECL_H__

	class CPreWorld;
	class CPreLeaf;
	class CPrePortal;
	class CNode;
	class CBaseEditObj;

	// CPreLeaf::m_Flags (LEAFF_VISITED, 1<<1, is PreLeaf.h's).
	// Reached from the inside leaf or an InsideDef (0x4498f0).
	#define LEAFF_INSIDE		(1<<0)
	// Marked FastApprox (0x44ac50): the portal flow skips it (0x44a25f).
	#define LEAFF_FASTAPPROX	(1<<2)

	// The bits of a leaf set: a byte array indexed by CPreLeaf::m_Index.
	#define VIS_TESTBIT(arr, i)	((arr)[(i) >> 3] & (1 << ((i) & 7)))
	#define VIS_SETBIT(arr, i)	((arr)[(i) >> 3] |= (1 << ((i) & 7)))
	#define VIS_CLEARBIT(arr, i)	((arr)[(i) >> 3] &= ~(1 << ((i) & 7)))

	// The PVS maker's state (MakePVS's argument, kept in g_pPVSInfo while
	// it runs; 0x84 bytes). The layout and names as WorldApprox_decl.h has
	// them (MakePVS's side, 0x44b270..); this module uses 0x00..0x30, 0x4c,
	// 0x50 and 0x70.
	struct PVSInfo
	{
		uint32					m_iCurLeaf;			// 0x00: the next leaf a thread takes (0x44a160)
		uint32					m_iFirstLeaf;		// 0x04
		uint32					m_iEndLeaf;			// 0x08
		BOOL					m_bFast;			// 0x0c
		void					(*m_LeafDoneFn)(float fPercent);	// 0x10: as the threads finish leaves (0x44a0f0)
		void					(*m_ProgressFn)(float fPercent);	// 0x14
		BOOL					m_Unknown_18;
		void					(*m_Unknown_1c)(uint32 iFrom, uint32 iTo);
		CPreWorld				*m_pWorld;			// 0x20
		uint32					m_nThreads;			// 0x24: more than one: the flow sleeps(0) (0x44a631)
		uint32					m_nThreadsDone;		// 0x28 (0x44a1fa)
		uint32					m_nLeavesDone;		// 0x2c
		uint32					m_nLeavesToDo;		// 0x30
		uint32					m_nPortals;			// 0x34
		CMoArray<CPrePortal*>	m_Portals;			// 0x38
		uint32					m_nVisBytes;		// 0x4c: bytes in a leaf set (dword-rounded)
		uint32					m_nVisDWords;		// 0x50
		uint32					m_nPortalBytes;		// 0x54
		uint8					*m_pCurVis;			// 0x58
		CMoArray<uint8>			m_PortalVis;		// 0x5c
		CMoArray<struct PVSZone*>	m_Zones;		// 0x70 (0x44a0a0)
	};

	// A Zone object's zone (0x30 bytes; 0x44aff0 makes them).
	struct PVSZone
	{
		CBaseEditObj			*m_pObj;
		PVector					m_vMin, m_vMax;		// 0x44add0 sets them from the world model
		CMoArray<CPreLeaf*>		m_Leaves;			// 0x1c
	};

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
	// 0x485ad4: how many leaves 0x44ac50 marked FastApprox.
	extern uint32				g_nFastApproxLeaves;
	// 0x485ad8: the bit index of the vis-BSP walk.
	extern uint32				g_iVisWalkBit;
	// 0x485adc: not used by these.
	extern uint32				g_Unknown_485adc;
	// 0x485ae0: the bits that walk reads.
	extern class CPreLeafList	*g_pVisWalkBits;
	// 0x485ae4.
	extern PVSInfo				*g_pPVSInfo;
	// 0x485ae8, 0x485aec: critical sections (thd_CreateCriticalSection):
	// the progress count, the next leaf.
	extern void					*g_pProgressCS;
	extern void					*g_pLeafCS;


	class CPreLeafList;
	struct PortalBuffer;

	void			VisCollectLeaves_R(CNode *pNode);																// 0x449c00
	void			VisCollectLeaves(CPreWorld *pWorld, CNode *pRoot, CPreLeafList *pBits);						// 0x449cf0
	void			VisFloodZone(CMoArray<uint8> &visited, CMoArray<uint8> &allowed, CMoArray<uint8> &reached,
						uint32 iUser, CPreLeaf *pLeaf);																// 0x449d60
	CPrePortal*		VisFindZonePortal(CMoArray<uint8> &visited, CMoArray<uint8> &allowed, CPreLeaf *pLeaf,
						uint32 iUser, BOOL bCross);																	// 0x449e60
	BOOL			VisCompareLeafLists(CMoArray<CPreLeafList*> &lists1, CMoArray<CPreLeafList*> &lists2);		// 0x449f10
	void			VisCalcLeafRadius(CPreWorld *pWorld, CPreLeaf *pLeaf, CMoArray<uint8> &visible);			// 0x449fc0
	PVSZone*		VisFindZone(PVSInfo *pInfo, const char *pName);												// 0x44a0a0
	void			VisLeafDone();																					// 0x44a0f0
	uint32			VisGetNextLeaf();																				// 0x44a160
	void			VisThreadFn(void *pData);																		// 0x44a1b0
	void			VisFlowPortal(CPreLeaf *pLeaf, CPrePortal *pPortal);											// 0x44a200
	void			VisFlow_R(VisFlowState *pState, CPrePortal *pSource, CPrePortal *pPass, uint8 *pMightSee,
						BOOL bCheckBlocker);																		// 0x44a510
	BOOL			VisClipToSeparators(CPrePortal *pSource, CPrePortal *pPass, PortalBuffer *pBuf, BOOL bFlip);	// 0x44a930
	void			VisMarkFastApprox_R(CPreLeaf *pLeaf);															// 0x44ac50

#endif
