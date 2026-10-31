//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : EditVert.h
//
//	PURPOSE	  : Defines the CEditVert class.
//
//	CREATED	  : October 5 1996
//
//
//------------------------------------------------------------------

#ifndef __EDITVERT_H__
#define __EDITVERT_H__


	// Includes....


	// talon: 40 bytes in 2001, with a trivial default constructor: a
	// CMoArray<CEditVert> allocates n * 40 bytes and constructs nothing
	// (0x40eac0). The position, the transformed and projected positions,
	// and four bytes Processor never shows the use of (DEdit should); no
	// per-vertex colour.
	class CEditVert : public LTVector
	{
	public:

						CEditVert() {}

						CEditVert( const CVector &vec ) : LTVector(vec) {}

			// talon: a CEditVert assigned from a CEditVert is taken whole by
			// value and only its position copied: every CEditVert assigned in
			// CMoArray<CEditVert>'s code is first copied into a 40-byte
			// temporary, then x, y and z stored from it (0x40e1e4, 0x40e271,
			// 0x40e5d5, 0x40ded8). From an LTVector, by reference
			// (CBaseEditObj::DoCopy, 0x40f540).
			void		operator = ( CEditVert other )			{ x=other.x; y=other.y; z=other.z; }
#ifdef DIRECTEDITOR_BUILD
			// talon: by value in DEdit (2001): a vector assigned to a CEditVert
			// is copied to a 12-byte temporary first (ViewFunctions,
			// 0x4678d6..0x4678fd).
			void		operator = ( LTVector other )			{ x=other.x; y=other.y; z=other.z; }
#else
			void		operator = ( const LTVector& other )	{ x=other.x; y=other.y; z=other.z; }
#endif

	public:

			// Transformed and projected positions..
			LTVector	m_Transformed;
			LTVector	m_Projected;

#ifdef DIRECTEDITOR_BUILD
			// talon: DEdit's (2001): whether the vertex is inside the view
			// frustum, a byte at 0x24 that the renderer stores from
			// CViewRender::InsideFrustum (`mov %al,0x24(%esi)`, 0x41ee4e) and
			// tests (`cmpb $0,0x24`, 0x41c0cd, 0x41e378); still 40 bytes.
			uint8		m_bInFrustum;

			// talon: stand-in: Build 69's vertex flags and clip-plane mask,
			// for DEdit's link until its renderer is brought back to 2001:
			// the in-frustum flag is the 2001 byte above, the clip planes in
			// the padding after it. Build 69's per-vertex colour is gone.
			enum	{	VERTFLAG_INFRUSTUM		= (1 << 0)	};

			bool		IsInFrustum() const						{ return IsFlagSet(VERTFLAG_INFRUSTUM); }
			void		SetInFrustum(bool bVal)					{ SetFlag(VERTFLAG_INFRUSTUM, bVal); }

			bool		IsFlagSet(uint8 nFlag) const			{ return (m_bInFrustum & nFlag) ? true : false; }
			void		EnableFlag(uint8 nFlag)					{ m_bInFrustum |= nFlag; }
			void		DisableFlag(uint8 nFlag)				{ m_bInFrustum &= ~nFlag; }

			void		SetFlag(uint8 nFlag, bool bVal)
			{
				if(bVal)
					EnableFlag(nFlag);
				else
					DisableFlag(nFlag);
			}

			uint8		m_nClipPlanes;
			uint8		m_Unknown_26[2];
#else
			uint32		m_Unknown_24;
#endif
	};



	typedef CMoArray<CEditVert>		CEditVertArray;


#endif  // __EDITVERT_H__


