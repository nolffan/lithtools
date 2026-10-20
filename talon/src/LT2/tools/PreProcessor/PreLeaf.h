// ------------------------------------------------------------------ //
//	The leaves and portals the 2001 Processor makes from the BSP (hull
//	making, then the PVS). Build 69 has neither; it keeps only the name
//	CPreLeaf. The layouts are the shipped code's: CPreLeaf's constructor
//	(0x42d060), CPrePortal's (inlined in CreatePoly, 0x409de0). The names
//	of their members are ours, and m_Unknown_* are members whose use is
//	not known yet.
// ------------------------------------------------------------------ //

#ifndef __PRELEAF_H__
#define __PRELEAF_H__

	#include "preplane.h"
	#include "prebasepoly.h"

	class CNode;
	class CPreLeaf;
	class CPrePoly;


	// A convex face of a leaf: the part of a node's plane between two leaves.
	class CPrePortal
	{
		public:

			CPrePortal()
			{
				m_pPartner	= NULL;
				m_pLeaf		= NULL;
				m_pNode		= NULL;
				m_Index		= 0xFFFF;
				m_Flags		= 0;
			}

			~CPrePortal()
			{
				if(m_pPartner)
					m_pPartner->m_pPartner = NULL;
			}

			void CopySplitAttributes(const CPrePortal *pPortal)
			{
				m_pNode		= pPortal->m_pNode;
				m_Plane		= pPortal->m_Plane;
				m_Index		= pPortal->m_Index;
				m_Flags		= pPortal->m_Flags;
				m_pPartner	= pPortal->m_pPartner;
			}

			// Reverses the vertices and the plane (0x414080).
			void				Flip();

			const PVector&		Normal() const		{ return m_Plane.m_Normal; }

		public:

			// PORTALF_ flags.
			uint32				m_Flags;

			uint32				m_Unknown_04[3];

			// The node whose plane it lies on.
			CNode				*m_pNode;

			uint16				m_Index;

			CPrePlane			m_Plane;

			CPreLeaf			*m_pLeaf;

			// The same portal seen from the leaf on its other side.
			CPrePortal			*m_pPartner;

			uint32				m_Unknown_3c[3];
			CMoArray<uint8>		m_Unknown_48;

			//this must come last
			BASEPOLY_MEMBER()
	};

	// A portal on a vis blocker: it has no partner.
	#define PORTALF_BLOCKER		(1<<0)

	// A copy of pPortal: its vertices, plane and index (0x413a10).
	CPrePortal* ClonePortal(CPrePortal *pPortal);

	// pSrc's vertices and split attributes into pDest (0x413a70, the name
	// is ours).
	void CopyPortal(CPrePortal *pSrc, CPrePortal *pDest);

	// Two portals to clip back and forth between (the PVS's); m_iCur is
	// the one holding the current shape. The names are ours.
	struct PortalBuffer
	{
		CPrePortal*		Cur()		{return m_pPortals[m_iCur];}
		CPrePortal*		Other()		{return m_pPortals[!m_iCur];}

		int				m_iCur;
		CPrePortal		*m_pPortals[2];
	};

	// Clips the current portal to pPlane's front side; FALSE when nothing
	// is left (0x413ad0, the name is ours).
	BOOL ClipPortalToPlane(CPrePlane *pPlane, PortalBuffer *pBuf);

	// The six portals of a box (0x413c90).
	void MakeBoxPortals(CMoArray<CPrePortal*> &portals, PVector &vMin, PVector &vMax);

	// Sets the portal's plane from its vertices (0x4143d0, the name is ours).
	void CalcPortalPlane(CPrePortal *pPortal);


	// talon (W2): what CPreLeaf::m_Unknown_58 holds (CPreWorld::LoadFile
	// reads each as two uint16s; the names are ours).
	struct LeafPolyRef
	{
		uint16		m_Unknown_00;
		uint16		m_Unknown_02;
	};

	// talon (W2): what CPreLeaf::m_Unknown_6c holds: 0x18 bytes, a byte array
	// and a word after it (CPreWorld::LoadFile; the constructor is out of
	// line at 0x43afb0, in PreWorld.cpp). The names are ours.
	class CPreLeafList
	{
		public:

							CPreLeafList();

			// talon: so the parts that used m_Unknown_6c[i]->GetSize() compile.
			uint32			GetSize() const		{ return m_Data.GetSize(); }

			CMoArray<uint8>	m_Data;
			uint32			m_Unknown_14;
	};

	class CPreLeaf
	{
		public:

							CPreLeaf();
							~CPreLeaf();

		public:

			CMoArray<CPrePortal*>	m_Unknown_00;
			CMoArray<CPrePortal*>	m_Portals;
			CLinkedList<CPrePoly*>	m_Polies;
			// talon (W2): two uint16s each (CPreWorld::LoadFile).
			CMoArray<LeafPolyRef>	m_Unknown_58;
			// talon: arrays freed with the leaf (0x42d1c0); what they hold is
			// not known yet.
			CMoArray<CPreLeafList*>	m_Unknown_6c;
			uint16					m_Unknown_80;
			uint16					m_Unknown_82;

			// The node whose front side this leaf is.
			CNode					*m_pNode;

			// LEAFF_ flags.
			uint16					m_Flags;

			// Index into the world's m_Leaves.
			uint16					m_Index;

			// talon, W3: the sphere around its portals (CPreWorld::CalcLeafSpheres;
			// the names are ours).
			PVector					m_vCenter;
			PReal					m_fRadius;
			uint32					m_Unknown_9c;
			uint32					m_Unknown_a0;
	};

	// Reached while looking for a leak (the name is ours).
	#define LEAFF_VISITED		(1<<1)

	class CBaseEditObj;
	class CPreWorld;

	// The leaf holding the first object of class pClassName that is in one,
	// and that object (0x42cd70, the name is ours).
	CPreLeaf* FindObjectLeaf(CPreWorld *pWorld, const char *pClassName, CBaseEditObj **ppObj=NULL);

	// The leaf of a GameStartPoint, a StartPoint, a Light or a DirLight,
	// whichever is found first (0x42ce40, the name is ours).
	CPreLeaf* FindInsideLeaf(CPreWorld *pWorld, CBaseEditObj **ppObj);

#endif
