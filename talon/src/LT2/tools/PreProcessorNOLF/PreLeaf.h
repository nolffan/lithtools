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

			// talon: the portal's center, the average of its vertices (the PVS
			// sets it, 0x44c66c; the name ours).
			PVector				m_vCenter;
			CMoArray<uint8>		m_Unknown_48;

			//this must come last
			BASEPOLY_MEMBER()
	};

	// A portal on a vis blocker: it has no partner.
	#define PORTALF_BLOCKER		(1<<0)

	// The portal functions are LeafPortals.h's.


	// talon (W2): what CPreLeaf::m_Unknown_58 holds (CPreWorld::LoadFile
	// reads each as two uint16s; the names are ours).
	struct LeafPolyRef
	{
		uint16		m_Unknown_00;
		uint16		m_Unknown_02;
	};

	// talon (W2): what CPreLeaf::m_Unknown_6c holds: 0x18 bytes, a byte array
	// and a word after it (CPreWorld::LoadFile). The names are ours. No
	// constructor of its own: `new CPreLeafList` constructs the byte array
	// in place, inline (0x44b69f, 0x44b8c7) or, where the inline budget is
	// spent, by a call to CMoArray<uint8>'s (0x43afb0, from 0x434bd7).
	class CPreLeafList
	{
		public:

			// talon: so the parts that used m_Unknown_6c[i]->GetSize() compile.
			uint32			GetSize() const		{ return m_Data.GetSize(); }

			CMoArray<uint8>	m_Data;
			uint32			m_Unknown_14;
	};

	// talon: a leaf's lists of visible leaves (CPreLeaf's 0x6c, 0x18
	// bytes): its own (an array of its own: Init is called out of line,
	// its destructor 0x42d370 has its own EH state), or, when m_iSameAs
	// isn't 0xFFFF, those of the leaf with that index (WorldApprox's
	// MakeLeafLists, 0x44b460).
	class CLeafListArray
	{
		public:
							CLeafListArray() : m_Lists(0)
							{
								m_iSameAs = 0xFFFF;
							}

							~CLeafListArray()
							{
								DeleteAndClearArray(m_Lists);
							}

			CMoArray<CPreLeafList*>	m_Lists;
			uint16					m_iSameAs;	// then two bytes' padding
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
			CLeafListArray			m_LeafLists;

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

	// FindObjectLeaf and FindInsideLeaf (pregeometry.cpp's) are
	// PreGeometry.h's.

#endif
