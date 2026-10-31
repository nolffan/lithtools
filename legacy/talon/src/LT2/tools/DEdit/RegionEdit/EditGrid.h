//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : EditGrid.h
//
//	PURPOSE	  : Defines the CEditGrid class.
//
//	CREATED	  : October 3 1996
//
//
//------------------------------------------------------------------

#ifndef __EDITGRID_H__
	#define __EDITGRID_H__


	// Includes....
	#include "oldtypes.h"
	#include "editray.h"
	#include "ccs.h"



	// Defines....
	class CEditRender;
	class CRegionView;
	#define CEditGridArray		CMoArray<CEditGrid*>


	class CEditGrid : public CCS
	{
		public:

								CEditGrid();			
								CEditGrid( LTVector pos, LTVector forward, LTVector up, CReal drawSize=300.0f );

		
		public:

			LTVector&			Normal()			{ return Forward(); }
			CReal				Dist()				{ return Normal().Dot( Pos() ); }

			// talon: 2001 (0x4230d8, `ret $0x24`): no bSnapToGrid
			BOOL				IntersectRay( DWORD snapSize, CEditRay ray, LTVector &intersection, CReal *pT=NULL );
		
		public:

			CReal				m_DrawSize;

	};



#endif  // __EDITGRID_H__
