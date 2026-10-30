// RegionView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRegionView view


#ifndef __REGIONVIEW_H__
#define __REGIONVIEW_H__


	#include "editgrid.h"
	#include "editpoly.h"
	#include "editregion.h"
	#include "editray.h"
	#include "navigator.h"
	#include "editbrush.h"
	#include "refs.h"
	#include "viewrender.h"
	#include "trackers\trackermgr.h"


	// In ViewFunctions.cpp
	void SetupBrushProperties(CEditBrush *pBrush, CEditRegion *pRegion);


	// Useful definitions.
	#define BEGIN_MEMORYEXCEPTION()		try {
	// talon: 2001 (the catch funclets, e.g. 0x4558b8, call GiveMemoryWarning only).
	#define END_MEMORYEXCEPTION()		} catch( CMemoryException x ) { GiveMemoryWarning(); }

	// Edit modes.
	#define BRUSH_EDITMODE						0
	#define GEOMETRY_EDITMODE					1
	#define OBJECT_EDITMODE						2
	// talon: 2001's fourth mode (CRegionView::OnSelectNone 0x45d3ed clears the
	// document's lightmap poly selection in it); MainFrm calls it LIGHT_EDITMODE (OnLightEditMode 0x4300b3).
	#define LIGHT_EDITMODE					3


	// Shade modes.
	#define SM_WIREFRAME						0
	#define SM_FLAT								1
	#define SM_TEXTURED							2
	#define SM_LIGHTMAPSONLY					3

	// Updating hints..
	#define REGIONVIEWUPDATE_EDITMODE			101
	#define REGIONVIEWUPDATE_REDRAW				102
	// talon: 2001's (CRegionView::OnUpdate 0x4562f1): redraws the Direct3D views.
	#define REGIONVIEWUPDATE_LIGHTMAPS			103
	
	// The number of move/size handles
	#define NUM_BOX_HANDLES						18

	enum EditState
	{
		EDIT_NOSTATE,
		EDIT_DRAWINGPOLY,
		// talon: 2001's third state (the DrawPoly tracker's split, which calls
		// UpdateSplitPolyVertex); the name is ours.
		EDIT_SPLITTINGPOLY
	};

	// Grid directions
	#define GRID_FORWARD						0
	#define GRID_RIGHT							1
	#define GRID_UP								2


	// Defines....
	class CRegionDoc;
	class CEditPoly;
	class CObjInterface;



	class CRegionView : public CView, public CViewRender
	{
	public:
		DECLARE_DYNCREATE(CRegionView)
		CRegionView();           // protected constructor used by dynamic creation
		virtual ~CRegionView();


		// Called by the Workspace doc to save some state info.
		void						LoadProjFile( CAbstractIO &file );
		void						SaveProjFile( CAbstractIO &file );


		void UpdateStatusBar();
		virtual BOOL PreTranslateMessage(MSG* pMsg);


	// Accessors
	public:

		// Set/Get the grid spacing
		DWORD						GetGridSpacing()				{ return m_dwGridSpacing; }
		void						SetGridSpacing( DWORD size )	{ m_dwGridSpacing = DMAX(size, 1); DrawRect(); }

		DWORD						GetMajorGridSpacing()				{ return m_dwMajorGridSpacing; }
		void						SetMajorGridSpacing(DWORD dwSize)	{ m_dwMajorGridSpacing=dwSize; DrawRect(); }

		float						GetGridSize()					{ return EditGrid().m_DrawSize; }
		void						SetGridSize( float fSize )		{ EditGrid().m_DrawSize= fSize; DrawRect(); }
		
		CRegionDoc*					GetRegionDoc()	{ return (CRegionDoc*)GetDocument(); }
		CEditRegion*				GetRegion()		{ return m_pRegion; }

		void						DeleteSelectedNodes();
	
	// View-ish stuff.
	public:
	
		void						CheckAMenuItem( CMenu *pMenu, UINT nItem, int testVal1, int testVal2 );
		void						EnableAMenuItem( CMenu *pMenu, UINT nItem, int testVal1, int testVal2 );

		BOOL						OnIdle( LONG lCount );
		CPoint						GetCurMousePos();
		

		// talon: 2001's view keeps these itself (ViewFunctions 0x46623c..0x466289;
		// the wireframe, grid, objects and marker flags have no out-of-line
		// accessors in the image).
		int							GetShadeMode();
		void						SetShadeMode( int mode );

		BOOL						IsWalkThroughMovement();
		void						SetWalkThroughMovement( BOOL bWalkThrough );

		BOOL						IsSelectBackfaces();
		void						SetSelectBackfaces( BOOL bBackface );

		BOOL						IsShowNormals();
		void						SetShowNormals( BOOL bShow );

		BOOL						IsShowWireframe()				{ return m_bShowWireframe; }
		void						SetShowWireframe( BOOL bShow )	{ m_bShowWireframe = bShow; }

		BOOL						IsShowGrid()					{ return m_bShowGrid; }
		void						SetShowGrid(BOOL bShow)			{ m_bShowGrid = bShow; }

		BOOL						IsShowObjects()					{ return m_bShowObjects; }
		void						SetShowObjects(BOOL bShow)		{ m_bShowObjects = bShow; }

		BOOL						IsShowMarker()					{ return m_bShowMarker; }
		void						SetShowMarker(BOOL bShow)		{ m_bShowMarker = bShow; }

		int							GetEditMode();

		//used to notify when the hot keys have been changed

		// Switch the navigator to one of the stored navigators for this level
		void						SwitchNavigator(int nNavIndex);

		// DrawRect override for focus drawing
		void						DrawRect(CRect *pRect=NULL);

		// Get the rendering rectangle
		void						GetRenderRect(LPRECT pRect, BOOL bClient = TRUE);

		BOOL						GetInFocus() const { return m_bInFocus; };
		void						SetInFocus(BOOL bInFocus) { m_bInFocus = bInFocus; };

		//given a list of polygons, it will back up all the nodes associated with them
		//and set up the undo
		void						MakeUndoForPolygons(CMoArray<CEditPoly*>* pPolyList);
	

	// Trackers and tracker functions.
	public:

		void						InitTracker();

		// Tracker command callbacks
		BOOL						OnDepthSelect(CUITracker *pTracker);
		BOOL						OnApplyTextureClick(CUITracker *pTracker);
		BOOL						OnSetGridOrientation(CUITracker *pTracker);
		BOOL						OnInitTextureSpace(CUITracker *pTracker);
		BOOL						OnSelectNone(CUITracker *pTracker);
		BOOL						OnDelete(CUITracker *pTracker);
		BOOL						OnNudge(CUITracker *pTracker);
		BOOL						OnNudgeRotate(CUITracker *pTracker);
		BOOL						OnSplitEdges(CUITracker *pTracker);
		BOOL						OnDeleteTaggedPolygons(CUITracker *pTracker);
		BOOL						OnDeleteEdges(CUITracker *pTracker);
		BOOL						OnGridToPoly(CUITracker *pTracker);
		BOOL						OnTagPolyVerts(CUITracker *pTracker);
		BOOL						OnFlipNormal(CUITracker *pTracker);
		BOOL						OnOrbitVertex(CUITracker *pTracker);

		BOOL						OnSelectLightNode(CUITracker *pTracker);
		BOOL						OnEditLightNode(CUITracker *pTracker);

		// talon: 2001's (ViewFunctions, 0x46c783; the name ours): selects the
		// lightmap texel under the cursor.
		void						SelectLightTexel();
		// talon: 2001's (ViewFunctions; names ours): 0x46c536 (OnApplyTextureClick
		// in the light edit mode), 0x46ca7c (the Tag tracker, 0x4790c8).
		void						PickLightPoly();
		void						TagLightTexelsInRect( CRect rect );

		// talon: 2001's (ViewFunctions 0x4677ce, 0x46790a; Build 69 has them in
		// CRVTrackerDrawPoly; names Build 69's).
		void						UpdateDrawPolyVertex( CPoint &point );
		void						UpdateSplitPolyVertex( CPoint &point );

	// Tagging stuff.
	public:
		
		// Used to help with immediate selection.
		BOOL						GetClosestPoint( CPoint &point, CVector &vClosest, BOOL bSelectedOnly, CReal *pDist = NULL );

		CVertRef					GetClosestVert( CEditBrush *pBrush, CPoint point, CReal *pDist=NULL );
		CVertRef					GetClosestVert( CPoint &point, BOOL bSelectedOnly, CReal *pDist=NULL );
		CVertRef					GetClosestVert( CVertRefArray &vertArray, CPoint point, CReal *pDist=NULL );
		
		CEdgeRef					GetClosestEdge( CPoint &point, BOOL bSelectedOnly = FALSE );
		CEdgeRef					GetClosestEdge( CEditBrush *pBrush, CPoint point, CReal *pDist=NULL );

		CPolyRef					CastRayAtPolies( CEditRay &ray, CReal *pDist=NULL, CReal rMinDist = 0.0f, BOOL bSelectedOnly = FALSE );
		CPolyRef					CastRayAtPolies( CEditBrush *pBrush, CEditRay &ray, CReal *pDist=NULL );

		CObjectRef					GetClosestObject( CPoint point, BOOL bSelectedOnly, CReal *pDist=NULL );



		// Makes a tag list out of either the immediate selection or the current tag list.
		// Returns TRUE if there's anything in the list.
		BOOL						GetSelectedPoly( CVertRefArray &list );
		BOOL						GetSelectedVerts( CVertRefArray &list );
		BOOL						GetImmediateVert( CVertRefArray &list );
		CBrushRef					GetMouseOverBrush( CReal *pDist = NULL, CReal rMinDist = 0.0f );
		BOOL						GetSelectedBrushes( CVertRefArray &list );
		BOOL						MakeTaggedEdgeList( CEdgeRefArray &list );

		// Tags all points/edges/polies in the tag rect.
		void						TagPointsInRect( CRect &rect );

		// Tags all points/edges/polies in the tag rect.
		void						GetObjectsInRect( CRect &rect, CMoArray<CWorldNode*> &objects );

		// Tags/untags the given object.
		void						ToggleVertexTag( CVertRef ref );
		void						ToggleEdgeTag( CEdgeRef edge );

		GenList<CVertRef>&			TaggedVerts()	{ return GetRegionDoc()->m_TaggedVerts; }
		CBrushRefArray&				TaggedBrushes()	{ return GetRegionDoc()->m_TaggedBrushes; }
		CPolyRefArray&				TaggedPolies() {return GetRegionDoc()->m_TaggedPolies;}
		
		CVertRef&					IVert()		{ return GetRegionDoc()->m_ImmediateVertexTag; }
		CEdgeRef&					IEdge()		{ return GetRegionDoc()->m_ImmediateEdgeTag; }
		CBrushRef&					IBrush()	{ return GetRegionDoc()->m_ImmediateBrushTag; }
		CPolyRef&					IPoly()		{ return GetRegionDoc()->m_TaggedPolies[0]; }
		
		CEditBrush&					DrawingBrush()	{ return GetRegionDoc()->m_DrawingBrush; }

	// UI helpers.
	public:

		//determines if any polygons are selected for editing. This occurs when
		//in geometry mode and the cursor is over a face, or if it is brush mode and
		//a brush is selected

		// Gets the current texture name from the controls.
		BOOL						GetCurrentTextureName( char* &pTextureName, BOOL bUseSprites=FALSE );

		// Gives a low memory warning.
		void						GiveMemoryWarning();

		// These will update any polies that contain any of these vertices.
		void						UpdatePlanesOfPolies( CVertRefArray &verts );
		
		// Finds the intersection with the right EditPlane.
		BOOL						GetVertexFromPoint( CPoint point, CVector &vertex );

		// Updates the current immediate selection.
		// Returns TRUE if it's changed.
		BOOL						UpdateImmediateSelection();

		// Updates the cursor to be a different one if necessary.
		void						UpdateCursor();

		// Gets all relevant information for moving selection handles.
		BOOL						GetHandleInfo( int theHandle, HCURSOR *pCursor, CVector &scaleNormal, CVector &scaleOrigin );

		// Gets the current selection box handle that the mouse is over.
		// -1 if none.
		int32						GetCurrentMouseOverHandle();

		//Returns the active region.  If none is active, just return the first region 
		//in the active document.

		//  maximizes a view.  0 = upper left, 1 = upper right, 2 = lower left, 3 = lower right

		// Tells if the given key is down.  Pass it in uppercase.
		BOOL						IsKeyDown( int key );
		
		// Updates all current trackers.
		void						UpdateTrackers( CPoint &point );
		
		// Splits any edges in the selection list.
		void						SplitEdges();			

		// Deletes any tagged vertices or polygons
		void						DeleteTaggedVertices();
		void						DeleteTaggedEdges( BOOL bRemoveOriginalPolyPoints );
		void						DeleteTaggedPolygons();

		// Does a carve operation (boolean subtraction).
		void						DoCarveOperation();

		// Does a flip operation (reverses polygon winding order).
		void						DoFlipOperation();

		// Offsets (moves them in the world) the selected nodes by a vector
		void						OffsetSelectedNodes(CVector vOffset);

		// Offsets the texture map for the selected polygon in the specified
		// direction (use 1 for positive, -1 for negative, or zero for no
		// movement).  The grid size is used to offset the texture.  If SHIFT
		// is pressed, then the texture is moved one unit at a time.
		void						OffsetTextureForPoly(int nPDirection, int nQDirection, BOOL bGrid = FALSE);

		// Scale the texture map for the selected polygon in the specified
		// direction (use 1 to expand, -1 to shrink, or zero for neither)
		// The grid size is used to scale the texture.
		void						ScaleTextureForPoly(int nPDirection, int nQDirection);

		// Mirrors the selected nodes along one of the axis specified in vAxis.
		// vAxis must either be (1, 0, 0) (0,1,0) or (0,0,1)
		void						MirrorSelectedNodes(CVector vAxis);

		// Returns TRUE if we are in a situation in which we can split a brush.
		// pnLastSelectedBrushIndex is filled in with the last selected brush index.
		BOOL						CanSplitBrush(int *pnLastSelectedBrush=NULL);

		// Generates unique names for the selected objects
		//
		// bUpdateRefProps		- Updates properties for objects that reference
		//						  other objects that have had their names change
		// bUpdateSelPropsOnly	- Only objects that are selected will have their
		//						  properties updated
		// bDisplayReport		- A dialog reporting the name changes is displayed
		// bGenerateUndoInfo	- Undo information is generated and added to the document
		void						GenerateUniqueNamesForSelected(BOOL bUpdateRefProps, BOOL bUpdateSelPropsOnly, BOOL bDisplayReport=FALSE, BOOL bGenerateUndoInfo=TRUE);

		// Extrudes the given polygon.
		bool						ExtrudePoly( CPolyRef poly, CVertRefArray &newVerts, bool bRemoveOriginal, bool bFlipAll, bool bNewBrush = false);

		// Clears the given selection categories.
		void						ClearSelections( BOOL bClearVerts=TRUE, BOOL bClearEdges=TRUE, BOOL bClearPolies=TRUE, BOOL bClearBrushes=TRUE );
		
		// Finishes each drawing operation.
		void						FinishDrawingPoly();
		void						SetupDrawingBrush( CEditBrush* pNewBrush );

		// Splits the polygon along the 'edge' defined by indices.
		void						SplitPolyWithEdge( CPolyRef poly, CEditVertArray &verts );

		// Import terrain map
		void						DoImportTerrainMap( CEditBrush *pBrush, CString &pathName );

		// Create primitives
		void						DoCreatePrimitiveBox( CVector &vCenter, CReal fSide );
		void						DoCreatePrimitiveCylinder( CVector &vCenter, int nNumSides, CReal fHeight, CReal fRadius );
		void						DoCreatePrimitivePyramid( CVector &vCenter, int nNumSides, CReal fHeight, CReal fRadius );
		void						DoCreatePrimitiveSphere(CVector &vCenter, int nSubdivisionsX, int nSubdivisionsY, CReal fRadius, BOOL bDome=FALSE);

		void						GetSelectedPolies(CMoArray<CEditPoly*> &polies);

		// This imports a file into the region and optionally positions it at
		// the green crosshair location.
		//
		// Returns: TRUE if the file import was a success.
		BOOL						ImportLTAFile(CString sFilename, BOOL bPositionAtCrosshair);
		BOOL						ImportOBJFile(CString sFilename, BOOL bPositionAtCrosshair);
		// talon: 2001 (0x4568fe; the name from ImportOBJFile's "calling
		// ImportWorldFile"): imports a .ed world.
		BOOL						ImportWorldFile(CString sFilename, BOOL bPositionAtCrosshair);
		bool						CreateBrushesFromScene(CEditRegion *pRegion, CObjInterface *pScene );

		//given a user input event name and a menu handler, it will set up the text for
		//the menu item

		// Does a recursive search through the node tree for nodes with a node flag, adding them to the actionList.  Used for undo.

	private:

	// Operations
	public:

		//these are callback functions for the trackers
		void						SetObjectEditMode();
		void						SetGeometryEditMode();
		void						SetBrushEditMode();
		void						NextTextureLayer();

		//handles replacing the texture specified by the immediate poly with the currently
		//active texture
		void						ReplaceTextures(bool bSelectionOnly);

	// Overrides
	
		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CRegionView)
	public:
		virtual void OnInitialUpdate();
		virtual void OnDraw(CDC* pDC);      // overridden to draw this view
		virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
		virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
		virtual void OnActivateFrame( UINT nState, CFrameWnd* pFrameWnd );
		//}}AFX_VIRTUAL

	#ifdef _DEBUG
		virtual void AssertValid() const;
		virtual void Dump(CDumpContext& dc) const;
	#endif

		// Generated message map functions
	public:
		//{{AFX_MSG(CRegionView)
		afx_msg void OnShrinkGridSpacing();
		afx_msg void OnUpdateShrinkGridSpacing( CCmdUI *pCmdUI );
		afx_msg void OnExpandGridSpacing();
		afx_msg void OnUpdateExpandGridSpacing( CCmdUI *pCmdUI );
		afx_msg void OnApplyColor();
		afx_msg void OnUpdateApplyColor( CCmdUI *pCmdUI );
		afx_msg void OnApplyTexture();
		afx_msg void OnUpdateApplyTexture( CCmdUI *pCmdUI );
		afx_msg void OnImportTerrainMap();
		afx_msg void OnCreatePrimitiveBox();
		afx_msg void OnCreatePrimitiveCylinder();
		afx_msg void OnCreatePrimitivePyramid();
		afx_msg void OnCreatePrimitiveSphere();
		afx_msg void OnCreatePrimitiveDome();
		afx_msg void OnUpdateSingleBrushOperation( CCmdUI *pCmdUI );
		afx_msg void OnSize(UINT nType, int cx, int cy);
		afx_msg BOOL OnEraseBkgnd(CDC* pDC);
		afx_msg void OnDestroy();
		afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnMouseMove(UINT nFlags, CPoint point);
		afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
		afx_msg void OnAlignPolyTextures();
		afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnContextMenu(CWnd*, CPoint point);
 		afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
		afx_msg void OnAutoTriangulate();
		afx_msg void OnKillFocus(CWnd* pNewWnd);
		afx_msg void OnSetFocus(CWnd* pOldWnd);
		afx_msg void OnRemoveExtraEdges();
		afx_msg void OnMapTextureCoords();
		afx_msg void OnRotateSelection();
		afx_msg void OnResetAllTextureCoords();
		afx_msg void OnEditUndo();
		afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
		afx_msg void OnEditRedo();
		afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
		afx_msg void OnJoinTaggedVertices();
		afx_msg void OnObjectHeight();
		afx_msg void OnBindToObject();
		afx_msg void OnAddObject();
		afx_msg void OnShowWireframe();
		afx_msg void OnShowObjects();
		afx_msg void OnObjectProperties();
		afx_msg void OnWorldInfo();
		afx_msg void OnShadeWireframe();
		afx_msg void OnShadeTextured();
		afx_msg void OnShadeFlat();
		afx_msg void OnShadeLightmapsOnly();
		afx_msg void OnShowNormals();
		afx_msg void OnBackfacingPolies();
		afx_msg void OnBackView();
		afx_msg void OnBottomView();
		afx_msg void OnFrontView();
		afx_msg void OnLeftView();
		afx_msg void OnPerspectiveView();
		afx_msg void OnRightView();
		afx_msg void OnTopView();
		afx_msg void OnProcessWorld();
		afx_msg void OnFileSave();
		afx_msg void OnUpdateFileSaveAs(CCmdUI* pCmdUI);
		afx_msg void OnCleanupGeometry();
		afx_msg void OnShowEditGrid();
		afx_msg void OnShowMarker();
		afx_msg void On4ViewConfiguration();
		afx_msg void OnUpdateStatus( CCmdUI* pCmdUI );
		afx_msg void OnUseDevice1();
		afx_msg void OnUseDevice2();
		afx_msg void OnUseDevice3();
		afx_msg void OnUseDevice4();
		afx_msg void OnRenderMode1();
		afx_msg void OnRenderMode2();
		afx_msg void OnRenderMode3();
		afx_msg void OnRenderMode4();
		afx_msg void OnReplaceTextures();
		afx_msg void OnSelectTexture();
		afx_msg void OnSelectBrushColor();
		afx_msg void OnResetDetailLevels();
		afx_msg void OnNavigatorStore();
		afx_msg void OnNavigatorOrganize();
		afx_msg void OnNavigatorGotoNext();
		afx_msg void OnNavigatorGotoPrevious();
		afx_msg void OnSelectionSelectAll();
		afx_msg void OnSelectionSelectNone();
		afx_msg void OnSelectionSelectInverse();
		afx_msg void OnUpdateSelectionSelectInverse(CCmdUI* pCmdUI);
		afx_msg void OnSelectionAdvanced();
		afx_msg void OnSelectionHideSelected();
		afx_msg void OnUpdateSelectionHideSelected(CCmdUI* pCmdUI);
		afx_msg void OnSelectionUnhideSelected();
		afx_msg void OnUpdateSelectionUnhideSelected(CCmdUI* pCmdUI);
		afx_msg void OnSelectionGroup();
		afx_msg void OnUpdateSelectionGroup(CCmdUI* pCmdUI);
		afx_msg void OnWorldImportObjects();
		afx_msg void OnBrushSplitBrush();
		afx_msg void OnUpdateBrushSplitBrush(CCmdUI* pCmdUI);
		afx_msg void OnBrushCarve();
		afx_msg void OnUpdateBrushCarve(CCmdUI* pCmdUI);
		afx_msg void OnBrushJoin();
		afx_msg void OnBrushUnJoin();
		afx_msg void OnBrushFlip();
		afx_msg void OnSelectionMirrorX();
		afx_msg void OnUpdateSelectionMirrorX(CCmdUI* pCmdUI);
		afx_msg void OnSelectionMirrorY();
		afx_msg void OnUpdateSelectionMirrorY(CCmdUI* pCmdUI);
		afx_msg void OnSelectionMirrorZ();
		afx_msg void OnUpdateSelectionMirrorZ(CCmdUI* pCmdUI);
		afx_msg void OnSelectionContainer();
		afx_msg void OnUpdateSelectionContainer(CCmdUI* pCmdUI);
		afx_msg void OnCenterMarkerOnSelection();
		afx_msg void OnUpdateCenterMarkerOnSelection(CCmdUI* pCmdUI);
		afx_msg void OnPlaceMarkerAtCamera();
		afx_msg void OnUpdatePlaceMarkerAtCamera(CCmdUI* pCmdUI);
		afx_msg void OnSelectionGenerateUniqueNames();
		afx_msg void OnUpdateSelectionGenerateUniqueNames(CCmdUI* pCmdUI);
		afx_msg void OnWorldObjectBrowser();
		afx_msg void OnPlaceMarkerAtVector();
		afx_msg void OnWorldDebugFindNamingConflicts();
		afx_msg void OnWorldScaleGeometry();
		afx_msg void OnMapTextureToView();
		afx_msg void OnResetTextureCoords();
		afx_msg void OnSelectionHideInverse();
		afx_msg void OnUpdateSelectionHideInverse(CCmdUI* pCmdUI);
		afx_msg void OnSelectionUnhideInverse();
		afx_msg void OnUpdateSelectionUnhideInverse(CCmdUI* pCmdUI);
		afx_msg void OnFileImportWorld();
		afx_msg void OnSelectionSavePrefab();
		afx_msg void OnUpdateSelectionSavePrefab(CCmdUI* pCmdUI);
		afx_msg void OnMatchTextureCoords();
		afx_msg void OnHollowBrush();
		// talon: 2001's (the message map 0x4dbb40).
		afx_msg void OnUpdateShrinkPixelScale(CCmdUI* pCmdUI);
		afx_msg void OnUpdateGrowPixelScale(CCmdUI* pCmdUI);
		afx_msg void OnShrinkPixelScale();
		afx_msg void OnGrowPixelScale();
		afx_msg void OnAltT();
		afx_msg void OnWalkthruMovement();
		afx_msg void OnOrbitMovement();
		afx_msg void OnUpdatePlanes();
		afx_msg void OnUseDirect3D();
		afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
		afx_msg void OnSelectDetailBrushes();
		afx_msg void OnWorldDebugClearLeakFinder();
		afx_msg void OnUpdateWorldDebugClearLeakFinder(CCmdUI* pCmdUI);
		afx_msg void OnWorldDebugGotoNextLeakVertice();
		afx_msg void OnUpdateWorldDebugGotoNextLeakVertice(CCmdUI* pCmdUI);
		afx_msg void OnWorldDebugGotoPreviousLeakVertice();
		afx_msg void OnUpdateWorldDebugGotoPreviousLeakVertice(CCmdUI* pCmdUI);
		afx_msg void OnLoadLightData();
		afx_msg void OnUnloadLightData();
		afx_msg void OnSaveLightData();
		afx_msg void OnUpdateLightData(CCmdUI* pCmdUI);
		//}}AFX_MSG

		DECLARE_MESSAGE_MAP()

		
		//-------------Member Variables-----------------//
		// talon: the 2001 layout (sizeof 0x8b8, the constructor 0x45292d and
		// destructor 0x452afe; CView 0x40, CViewRender 0x40..0x554). Offsets
		// are CRegionView's; members no function names are named by offset.
	public:

		int							m_nShadeMode;				// 0x554 (Get/SetShadeMode 0x46623c)
		BOOL						m_bWalkThroughMovement;		// 0x558 (0x466250; 0x457b10, 0x457b18)
		BOOL						m_bSelectBackfaces;			// 0x55c (0x466264)
		BOOL						m_bShowNormals;				// 0x560 (0x466278)

		CEditRay					m_rayMousePoint;			// 0x564 (OnContextMenu 0x457645)

		// THE region you're editing.
		CEditRegion					*m_pRegion;					// 0x57c

		int							m_nPixelScale;				// 0x580 (OnShrinkPixelScale 0x45976e)
		float						m_fPixelScale;				// 0x584
		BOOL						m_bShowWireframe;			// 0x588 (OnShowWireframe 0x457c06)
		BOOL						m_Unknown58c;				// 0x58c (= FALSE in the constructor)
		BOOL						m_bShowObjects;				// 0x590 (OnShowObjects 0x457bc8)
		BOOL						m_bShowGrid;				// 0x594 (OnShowEditGrid 0x458e87)
		BOOL						m_bShowMarker;				// 0x598 (OnShowMarker 0x458e9e)

		// When the right button goes down, if no editing actions are taken,
		// it sets this to TRUE so the popup menu comes up.
		BOOL						m_bDoMenu;					// 0x59c (0x455a6e)

		DWORD						m_dwGridSpacing;			// 0x5a0
		DWORD						m_dwMajorGridSpacing;		// 0x5a4

		CMoArray<CWorldNode*>		m_Unknown5a8;				// 0x5a8 (vtable 0x4d75fc; only constructed and destroyed)

		// Timing stuff.
		UINT						m_TimerID;					// 0x5bc

		DWORD						m_Unknown5c0[3];			// 0x5c0

		// Used to track the mouse.
		CPoint						m_LastMousePos;				// 0x5cc

		// Current state.
		EditState					m_EditState;				// 0x5d4

		CPolyRef					m_PolyToSplit;				// 0x5d8

		BOOL						m_Unknown5e0;				// 0x5e0 (= FALSE in the constructor)

		// Is the window in focus?
		BOOL						m_bInFocus;					// 0x5e4
		BOOL						m_Unknown5e8;				// 0x5e8 (= TRUE in the constructor)

		CUITrackerMgr				m_cTrackerMgr;				// 0x5ec

		DWORD						m_Unknown608[2];			// 0x608 (never touched)

		CRect						m_TagDrawRect;				// 0x610

		CBrushRef					m_Unknown620;				// 0x620 (UpdateImmediateSelection 0x467aa5)

		// Handles on the selection box.
		CVector						m_BoxHandles[NUM_BOX_HANDLES];			// 0x624
		CPoint						m_HandlePos[NUM_BOX_HANDLES];			// 0x6fc
		BOOL						m_HandlesInFrustum[NUM_BOX_HANDLES];	// 0x78c
		
		// More info on the box.
		CVector						m_BoxMiddle;				// 0x7d4
		CVector						m_BoxHalf;					// 0x7e0

		int							m_CurMouseOverHandle;		// 0x7ec

		// Different cursors for the app.
		HCURSOR						m_CursorArrow;				// 0x7f0
		HCURSOR						m_CursorNS;
		HCURSOR						m_CursorWE;
		HCURSOR						m_CursorNWSE;
		HCURSOR						m_CursorNESW; 
		HCURSOR						m_CursorSizeAll;			// 0x804
		
		// Is a size handle selected?  (If not, it won't try to size the brush selections.)
		BOOL						m_bSizeHandleSelected;		// 0x808 (0x467db6)

		DWORD						m_Unknown80c[10];			// 0x80c..0x834

		// Used for texture polygons with the "K" key
		int							m_nCurrentTextureVertIndex;	// 0x834 (OnInitTextureSpace 0x45d28d)

		// The leak vertex the view is at (OnWorldDebugGotoNextLeakVertice 0x45c4e8).
		int							m_nCurLeakVertex;			// 0x838

		// Distance to selected object.  Used to cycle selections.
		CReal						m_rSelectDist;				// 0x83c
		
		CPoint						m_MousePoint;				// 0x840 (OnMouseMove 0x45592a)

		// Only constructed and destroyed (vtables 0x4dcc28, 0x4dcbf8).
		CVertRefArray				m_Unknown848;				// 0x848
		CBrushRefArray				m_Unknown85c;				// 0x85c
		CPolyRef					m_Unknown870;				// 0x870
		DWORD						m_Unknown878[2];			// 0x878
		CVertRefArray				m_Unknown880;				// 0x880
		DWORD						m_Unknown894[9];			// 0x894..0x8b8
	};

	/////////////////////////////////////////////////////////////////////////////


#endif  // __REGIONVIEW_H__




