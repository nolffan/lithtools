//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : EditBrush.h
//
//	PURPOSE	  : Defines the CEditBrush class.
//
//	CREATED	  : November 12 1996
//
//
//------------------------------------------------------------------

#ifndef __EDITBRUSH_H__
#define __EDITBRUSH_H__


	// Includes....
	#include "editvert.h"
	#include "boundingbox.h"
	#include "editplane.h"
	#include "worldnode.h"
	#include "boundingsphere.h"
	

	// Defines....
	class CEditPoly;
	class CEditRegion;
	class CLTANode;
	class CLTAFile;


	// Different flags for the brushes
	enum EBrushFlags
	{
		BRUSHFLAG_VISIBLE			= (NODEFLAG_USER << 0)
	};

	// EditBrush stuff.
	// talon: the 2001 class. Its vtable (0x4747c8): the scalar deleting
	// destructor (0x40d870), Term (0x40d960), AllocateSameKind (0x40d810),
	// DoCopy (0x40da00), GetUpperLeftCornerPos (0x40df80): Rotate is not
	// virtual.
	class CEditBrush : public CWorldNode
	{
		public:
	
									CEditBrush();
			virtual					~CEditBrush();
			
			virtual void			Term();
			virtual CWorldNode*		AllocateSameKind()	{return new CEditBrush;}
			virtual void			DoCopy(CWorldNode *pOther);			

		// Loading/saving.
		public:


			// talon: in 2001 the third argument is handed to each poly's
			// loader (0x40dbb2), and with bUnknown, SaveTBW saves each poly as
			// its texture (0x40fcf0) and the CBasePolys of its triangle array
			// (0x40dcfb).
			BOOL					LoadTBW( CAbstractIO& InFile, CStringHolder *pStringHolder, uint32 dwUnknown );
			void					SaveTBW( CAbstractIO& OutFile, BOOL bUnknown );


		// Geometry stuff.
		// talon: none of these but CopyEditBrush, UpdateBSphere and
		// GetUpperLeftCornerPos is in the image. Build 69's IsOpen,
		// FindClosestVert, RemoveUnusedVerts, FlipNormals, UpdatePlanes,
		// CalcBoundingBox, DoCopy(pOther, pStringHolder) and FindPolyWithEdge
		// are left out: with them declared, the inline CMoArray destructors
		// of WorldNode.cpp's static initializer (0x44e350) and of
		// MakeWorldPVS (0x43f170) add their two counts the other way round
		// (the header declaration-count threshold, tools/guides/09).
		public:

			void					CopyEditBrush( CEditBrush *pOther, CStringHolder *pStringHolder=NULL );
			
			uint32					FindPolyWithPoints( uint32 index1, uint32 index2 );
			BOOL					FindPoliesWithEdge( uint32 v1, uint32 v2, CEditPoly*&pPoly1, CEditPoly*&pPoly2, uint32 &nContaining );

			//this will add a vertex to the list, unless it can find one that is within
			//fThreshold distance of the one trying to be added, in which case it will use that
			//point
			uint32					AddVertOrGetClosest(const LTVector& vPos, CReal fThreshold);
			
			void					RemovePolyVerts( CMoDWordArray &indices, CMoArray<CEditPoly*> &exclude, CMoArray<CEditPoly*> &changedPolies );
			void					RemoveVertex( uint32 iVert );
			void					RemoveExtraEdges( CReal normalThresh, CReal distThresh );
			void					RemoveDuplicatePoints( CReal distThresh );
			void					ChangeVertexReferences( uint32 from, uint32 to );


		public:
			
			CEditPoly*				JoinPolygons( CEditPoly *pPoly1, CEditPoly *pPoly2, uint32 v1, uint32 v2 );

			//called to update all bounding volume information
			void					UpdateBoundingInfo()		{ UpdateBSphere(); }

			//updates the bounding sphere of this brush
			void					UpdateBSphere();

			//calculates a bounding box for this brush
		
			// Returns the upper left corner of the brush as it would appear in DEdit
			CVector					GetUpperLeftCornerPos();

			// Moves a brush to a position.
			void					MoveBrush(const LTVector& vPos);
			void					OffsetBrush(const LTVector& vDelta);

			void					Rotate(LTMatrix &mMatrix, LTVector &vCenter);

		// Accessors.
		public:

			//handles determining if the brush is visible or not
			bool					IsVisible()				{ return IsFlagSet(BRUSHFLAG_VISIBLE) != 0; }
			void					SetVisible( BOOL bVis )	{ SetFlag(BRUSHFLAG_VISIBLE, bVis != 0); }

		// Brush data.
		public:

			// talon: the 2001 layout, from CEditBrush() (0x40d6f0; sizeof 0xb0 by
			// the operator new of 0x40d826 and 0x4107b2): the polies at +0x60,
			// the points at +0x74, a bounding box at +0x88 (its constructor zeroes
			// six floats), the region index, the colour at +0xa4, the region at
			// +0xa8 (set NULL), and a dword at +0xac that the constructor sets to 1
			CMoArray<CEditPoly*>	m_Polies;
			CEditVertArray			m_Points;

			CBoundingBox			m_BoundingBox;

			// The brush's index into the world..
			// Only valid during saving.
			uint32					m_RegionBrushIndex;

			// The brush display color.
			uint8					r, g, b;

			//the region that holds this brush
			CEditRegion*			m_pRegion;

			uint32					m_Unknown_ac;
		
		protected:
			//don't allow the copy constructor
			CEditBrush( CEditBrush& pCopyFrom )	{}

#ifdef DIRECTEDITOR_BUILD
		// Notification that a property of this object has changed
		virtual void			OnPropertyChanged(CBaseProp* pProperty, bool bNotifyGame, const char *pModifiers);

#endif // DIRECTEDITOR_BUILD

				
	};


	void SimpleSplitBrush( CEditRegion *pRegion, CEditPoly *pRoot, CEditBrush *pToSplit, CEditBrush *newBrushes[2] );


#endif  // __EDITBRUSH_H__


