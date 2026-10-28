// talon: the 2001 classes EditRegion2001.cpp (0x410000..0x411670) writes or
// calls, which the shared (Build 69) headers do not have yet. Stand-ins under
// names of their own so nothing collides at link time; to be folded into
// the shared headers (EditRegion.h, BasePoly.h, EditPoly.h, EditBrush.h,
// EditObjects.h) later.

#ifndef __EDITREGION2001_DECL_H__
#define __EDITREGION2001_DECL_H__

	#include "editregion.h"


	// The 2001 CBasePoly (BasePoly.h): its vtable (0x474470) has one entry,
	// CopyAttributes (0x44d400, `ret 8`); there is no virtual destructor
	// (0x40fec0 destroys its temporary polies inline and frees them with a
	// plain operator delete). CopyEditPoly (0x410000) calls slot 0.
	class CBasePoly2001_ER
	{
		public:

			virtual void		CopyAttributes( CBasePoly2001_ER *pOther, CStringHolder *pStringHolder=NULL )	{}

		public:

			CEditPlane			m_Plane;		// 0x04
			CMoWordArray		m_Indices;		// 0x14
			CEditBrush			*m_pBrush;		// 0x28
	};


	// The 2001 CEditPoly (EditPoly.h): what this module uses of it.
	class CEditPoly2001_ER : public CBasePoly2001_ER
	{
		public:

			// 0x410000: Build 69's CopyEditPoly.
			BOOL				CopyEditPoly( CEditPoly2001_ER *pPoly, BOOL bCopyIndices=TRUE, CStringHolder *pStringHolder=NULL );

			// 0x40fec0 (EditObjects2001.cpp's range; the name ours): remakes
			// the poly's triangle list (CBasePoly::GetTriangles into the
			// CMoArray<CBasePoly*> at +0x5c) from a temporary CBasePoly.
			void				UpdateTriangles();
	};


	// The 2001 CEditBrush's ED file methods (EditBrush.h; EditBrush2001.cpp's
	// range, the names ours). The layout is the shared one (sizeof 0xb0).
	class CEditBrush2001_ER : public CEditBrush
	{
		public:

			// 0x40da20 (thiscall, ret 0xc).
			BOOL				LoadFile( CAbstractIO &InFile, CStringHolder *pStringHolder, uint32 version );
			// 0x40dc00 (thiscall, ret 8); the region passes 0.
			void				SaveFile( CAbstractIO &OutFile, BOOL bUnknown );
	};


	// The 2001 CBaseEditObj (EditObjects.h): 0x88 bytes (the operator new
	// at 0x410de5), its constructor 0x40f420 (EditObjects2001.cpp's range).
	class CBaseEditObj2001_ER : public CWorldNode
	{
		public:

								CBaseEditObj2001_ER();

			uint8				m_Unknown_60[0x88 - 0x60];
	};


	// A node's properties in the ED file (EditObjects2001.cpp's range, cdecl;
	// LoadNodeProperties2001 and SaveNodeProperties2001 there): 0x40ecd0
	// reads them (the project, Build 69's third argument, is the region
	// loader's third: Processing passes NULL), 0x40f100 writes them.
	BOOL	LoadNodeProperties_40ecd0( CAbstractIO &InFile, CWorldNode *pNode, CEditProjectMgr *pProject );
	void	SaveNodeProperties_40f100( CAbstractIO &OutFile, CWorldNode *pNode );


	// The 2001 CEditRegion (EditRegion.h): 0x128 bytes, laid out from its
	// constructor (0x410110): Build 69's members in Build 69's order without
	// the prefab manager (and without DEdit's).
	class CEditRegion2001
	{
		public:

									CEditRegion2001();
									~CEditRegion2001();

			void					Term();

		// Loading/saving: the 2001 ED file.
		public:

			RegionLoadStatus		LoadFile( CAbstractIO &InFile, uint32 *pVersion, CEditProjectMgr *pProject );
			void					SaveFile( CAbstractIO &OutFile, BOOL bCompress );

			void					SaveHeader( CAbstractIO &OutFile, uint32 version, BOOL bCompressed );
			BOOL					RecurseAndLoadNode( CAbstractIO &InFile, CWorldNode *pNode, CEditProjectMgr *pProject );
			void					RecurseAndSaveNode( CAbstractIO &OutFile, CWorldNode *pNode );

		// Geometry helpers.
		public:

			uint32					GetTotalNumPolies();
			uint32					GetTotalNumPoints();

			void					UpdatePolyTriangles();

		// Node functions.
		public:

			void					DetachNode( CWorldNode *pNode, BOOL bAttachToParent=TRUE );
			void					ClearSelections();

			void					SelectNode( CWorldNode *pNode, BOOL bUpdateSelectionArray=TRUE );
			void					UnselectNode( CWorldNode *pNode, BOOL bUpdateSelectionArray=TRUE );

		public:

			CMoArray<CWorldNode*>		m_PathNodes;			// 0x00
			CWorldNode					m_RootNode;				// 0x14
			CMoArray<CWorldNode*>		m_Selections;			// 0x74
			CNavigatorPosArray			m_NavigatorPosArray;	// 0x88
			int							m_nLastNavigatorPos;	// 0x9c
			CVector						m_vMarker;				// 0xa0

			CMoArray<CBaseEditObj*>		m_Objects;				// 0xac
			CLinkedList<CEditBrush*>	m_Brushes;				// 0xc0
			CMoArray<CEditBrush*>		m_QuickBrushIndexer;	// 0xf0

			CStringHolder				*m_pStringHolder;		// 0x104
			CStringHolder				m_DefaultStringHolder;	// 0x108
			char						*m_pInfoString;			// 0x120
			int							m_LoadErrorCode;		// 0x124
	};

#endif  // __EDITREGION2001_DECL_H__
