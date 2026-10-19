//------------------------------------------------------------------
//
//	FILE	  : BspGen.h
//
//	PURPOSE	  : Defines the CBspGen class, which creates a BSP
//				tree out of a simple list of polygons (CModel).
//
//	CREATED	  : 2nd May 1996
//
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------

#ifndef __BSPGEN_H__
	#define __BSPGEN_H__


	// Defines....
	#define FILTER_ADDREVERSE	(1<<0)	// Add an extra poly for the reverse side (hullmakers do this).
	#define FILTER_ALWAYSADD	(1<<1)	// Always add the poly regardless of which side of the BSP it's on.


	typedef enum
	{

		BSPGEN_OK,
		BSPGEN_ERROR,
		BSPGEN_OUTOFMEMORY

	} BspGenStatus;

	class CBspGen;
	class CPrePoly;
	class CPreWorld;
	
	// Includes....
	#include "bdefs.h"
	#include "node.h"


	class CPolyList;


	// The default for CBspGenOptions::m_PlaneDistFn (0x430950, defined with
	// the surface code): the plane's distance. Name provisional.
	PReal GetPlaneDist(const CPrePlane *pPlane);

	// The 2001 options (40 bytes; CBspGen's constructor at 0x404c40 has them
	// inline). Names follow what DoPostProcessing and GenerateBspTree use
	// them for; m_Unknown_* are not read by BspGen.
	class CBspGenOptions
	{
	public:

						CBspGenOptions()
						{
							m_pWorld				= NULL;
							m_pCustomPolyList		= NULL;
							m_bMinimizePolies		= FALSE;
							m_bHullMakers			= FALSE;
							m_bPostProcess			= TRUE;
							m_nThreadsToUse			= 0;
							m_bUseOriginalPolies	= FALSE;
							m_bInitVertexIndices	= TRUE;
							m_PlaneDistFn			= GetPlaneDist;
							m_Unknown_24			= 0;
						}

		CPreWorld		*m_pWorld;

		// If this is set, it uses this list instead of m_pWorld->m_Polies.
		CGLinkedList<CPrePoly*>	*m_pCustomPolyList;

		// Split polies to the lightmap size (DoPostProcessing).
		BOOL			m_bMinimizePolies;

		// Put hull maker polies in the tree.
		BOOL			m_bHullMakers;

		BOOL			m_bPostProcess;
		uint32			m_nThreadsToUse;
		BOOL			m_bInitVertexIndices;

		// Replace the BrushToWorld fragments with the original brush polies.
		BOOL			m_bUseOriginalPolies;

		PReal			(*m_PlaneDistFn)(const CPrePlane *pPlane);
		uint32			m_Unknown_24;
	};


	class CBspGen
	{
		public:

							CBspGen();
							~CBspGen();

		public:

			BOOL			GenerateBspTree(CBspGenOptions *pOptions);

		public:

			BOOL				MinimizeNodePolySize(NODEREF iNode, CReal maxX, CReal maxY);

		public:

			void				RecurseOnList(CPolyList &polies, NODEREF *pRootNode, uint32 nLevel);
			void				ThreadedRecurseOnList(CPolyList &polies, NODEREF *pRootNode, uint32 nLevel);
			CPrePoly*			GetBestSplit(CPolyList &polies, uint32 nLevel);
			void				SplitPolyArray(CPolyList &polies, CPrePoly *pSplitOn, CPolyList *sideArrays[2]);
			void				SplitPoly(CPrePoly *pSplitOn, CPrePoly *pToSplit, CPolyList *sideArrays[2]);
			void				DoPreProcessing();
			BOOL				DoPostProcessing();

		public:

			CBspGenOptions		m_Options;
			CPreWorld			*m_pWorld;
			uint32				m_nPolySplits;
			uint32				m_nSplitPolyArrayCalls;

		public:

			uint32				m_nThreadsBeingUsed;
			BOOL				m_bRemoveIntersectingPolies;
			void				*m_AppendCS;
			// Called with the progress as polies are split (SplitPolyArray).
			void				(*m_ProgressFn)(CBspGen *pBspGen, CReal fPercent);
			CMoDWordArray		m_PortalPolyPts;
	};


	void FilterPolyIntoBSP(
		CPreWorld			*pWorld, 
		NODEREF				pRoot, 
		CPrePoly			*pPoly, 
		uint32				flags,
		CMoArray<CPrePoly*> *pInsideFragments=NULL);

	CNode* AddNode(CNodeList &nodeList, const CPrePlane *pPlane, CPrePoly *pPoly);

	// Returns TRUE if there is a node with the given poly.
	CNode* NodeWithPoly(CPreWorld *pWorld, CPrePoly *pPoly);

	// Adds a node, surface, and poly to the backside of the specified node 
	// (and keeps the BSP structure intact).
	CNode* AddBacksideNode(CPreWorld *pWorld, CNode *pNode);

	// Used for HullMakers. Creates a poly that faces the opposite way and adds it to the world.
	CPrePoly* AddReversedPoly(
		CPreWorld *pWorld,
		const CPrePoly *pPoly);

	
#endif


