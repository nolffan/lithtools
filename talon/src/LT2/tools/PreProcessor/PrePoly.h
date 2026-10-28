//------------------------------------------------------------------
//
//	FILE	  : PrePoly.h
//
//	PURPOSE	  : Defines the CPrePoly class, which is used 
//              in the preprocessing phase.
//
//	CREATED	  : 2nd May 1996
//
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------

#ifndef __PREPOLY_H__
#define __PREPOLY_H__

	// Defines....	
	#define CPrePolyPtrArray			CMoArray<CPrePoly*>
	
	
	// Includes....
	#include "bdefs.h"
	#include "preplane.h"
	#include "presurface.h"
	#include "prebasepoly.h"


	// Defines....
	class CPreWorld;
	class CPrePortal;
	class LMMWorld;
	
	#define PP_POSTREMOVE						(1<<0)	// Used in various places..
	// BrushToWorld (the names are ours): a face coplanar with a face of an
	// earlier brush, and one of those whose piece inside was thrown away.
	#define PP_REMOVED							(1<<1)
	#define PP_COPLANAR							(1<<2)

	class CPrePoly : public CGLLNode
	{
		public:

			// Constructor
								CPrePoly();
			virtual				~CPrePoly();


		public:

			// Member functions
			void				Term();
			
			void CopySplitAttributes(const CPrePoly *pPoly)
			{
				m_pSurface				= pPoly->m_pSurface;
				m_pOriginalBrushPoly	= pPoly->m_pOriginalBrushPoly;
				m_PPFlags				= pPoly->m_PPFlags;
			}
			
			// Sets up the poly lightmap origin.  If pPolyO is NULL, it uses m_PolyO.
			void				FindTextureOrigin(PVector *pPolyO, BOOL bAdjust, float lmGridSize);
			
			void				CalculateTextureSize(BOOL bCapSizes, 
													float maxTextureSize, float lmGridSize,
													uint16 &xPixels, uint16 &yPixels);

			void				RemoveCollinearVertices();
			BOOL				PostJoinFixup(PReal convexThreshold);

			// Make a copy of the poly.
			CPrePoly*			Clone() const;

			// talon: a portal of the given size on this poly's plane, for hull
			// making (0x42e200; name provisional).
			CPrePortal*			MakePortal(PReal fSize);


 
			const PVector&		P() const				{ ASSERT(GetSurface()); return GetSurface()->P; }
			const PVector&		Q()	const				{ ASSERT(GetSurface()); return GetSurface()->Q; }

			const PVector&		InverseP() const		{ ASSERT(GetSurface()); return GetSurface()->InverseP; }
			const PVector&		InverseQ() const		{ ASSERT(GetSurface()); return GetSurface()->InverseQ; }



			const PVector&		Normal() const			{ ASSERT(GetPlane()); return GetPlane()->m_Normal; }
			const PReal&		Dist() const			{ ASSERT(GetPlane()); return GetPlane()->m_Dist; }

			const PVector&		PolyO() const			{ return m_PolyO; }

			CPreSurface*		GetSurface()			{ return m_pSurface; }
			const CPreSurface*	GetSurface() const		{ return m_pSurface; }
			CPrePlane*			GetPlane()				{ ASSERT(GetSurface()); return GetSurface()->m_pPlane; }
			const CPrePlane*	GetPlane() const		{ ASSERT(GetSurface()); return GetSurface()->m_pPlane; }

			uint32&				Index(uint32 i)			{ ASSERT(i<NumVerts()); return Vert(i).m_Index; }
			const uint32&		Index(uint32 i)	const	{ ASSERT(i<NumVerts()); return Vert(i).m_Index; }

			uint32&				NextIndex(uint32 i)			{ return Index((i+1) % NumVerts());}
			const uint32&		NextIndex(uint32 i) const	{ return Index((i+1) % NumVerts());}

			inline uint32		GetSurfaceFlags() const	{ return GetSurface()->m_Flags;}

			// Set the PostRemove flag.
			inline BOOL			GetPostRemove() const	{ return !!(m_PPFlags & PP_POSTREMOVE);}
			inline void			SetPostRemove(BOOL bSet){ if(bSet) m_PPFlags |= PP_POSTREMOVE; else m_PPFlags &= ~PP_POSTREMOVE;}

			// Get/set PP_ flags.
			inline uint32		GetPPFlags() const		{ return m_PPFlags;}
			inline void			SetPPFlags(uint32 flags){ m_PPFlags = flags;}

			// talon: the poly's vertices, then the extra ones in m_Unknown_0c, as
			// one list (the lightmap maker's; the names are ours).
			uint32				NumAllVerts() const		{ return NumVerts() + m_Unknown_0c.GetSize(); }
			CPolyVert&			AllVert(uint32 i)		{ return (i >= NumVerts()) ? m_Unknown_0c[i - NumVerts()] : Vert(i); }

			// Find the center of the polygon

			// Extents tracking 


		public:

			// The 2001 class (its constructor, 0x42dc00) has a vertex array
			// and two more members ahead of m_pWorld; what they are used
			// for is not known yet.
			CMoArray<CPolyVert>	m_Unknown_0c;
			uint32				m_Unknown_20;
			uint32				m_Unknown_24;

			LMMWorld			*m_pWorld;		// Used by LightMapMaker.

			// The original poly this poly comes from.. after the BSP is generated,
			// all the polies that were split up in the BrushToWorld module are replaced
			// with their original polies.
			// If PP_DONTPRESERVEORIGINAL is set, this can't be used.
			CPrePoly			*m_pOriginalBrushPoly;

			CPrePoly			*m_pReplacement; // Used in BSP generation.

			// Used in the BSP generator's CPolyList.
			CMLLNode			m_PolyListNode;

			// Used for lightmapping .. tells the upper-left corner of the polygon in texture space.
			PVector				m_PolyO;

			// Extents tracking
			// Note : This is mutable because it's not really part of the polygon,
			// and everything that's going to be doing extents overlap checking might
			// require a different amount of overlap.  So it's more like processing-
			// specific state data associated with this specific poly.
			mutable PVector		m_vExtentsMin;
			mutable PVector		m_vExtentsMax;

			// talon: Build 69's (0x449200, in the T-junction module, which
			// defines it).
			void				CalcExtents(PReal = 0.0f) const;

		protected:
			
			// Combination of PP_ flags.
			uint32				m_PPFlags;


		public:
			
			// Width and height of lightmap data for this polygon.
			uint16				m_LMWidth, m_LMHeight;

			CPreSurface			*m_pSurface;
			
			// Which world it comes from.  Used by lighting and while saving files.
			uint32				m_WorldIndex;

			// This poly's index into the world's array.
			uint32				m_Index;

			// The ID for representing the "name" of this polygon
			uint32				m_nName;

			//this must come last
			BASEPOLY_MEMBER()

	};


	typedef CGLinkedList<CPrePoly*> CPrePolyList;
	typedef CMoArray<CPrePoly*> PrePolyArray;

#endif

