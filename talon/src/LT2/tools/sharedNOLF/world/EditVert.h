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
			void		operator = ( const LTVector& other )	{ x=other.x; y=other.y; z=other.z; }

	public:

			// Transformed and projected positions..
			LTVector	m_Transformed;
			LTVector	m_Projected;

			uint32		m_Unknown_24;
	};



	typedef CMoArray<CEditVert>		CEditVertArray;


#endif  // __EDITVERT_H__


