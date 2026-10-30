// RegionView.cpp : implementation file
//

#include "bdefs.h"
#include "objectimporter.h"
#include "dedit.h"
#include "regiondoc.h"
#include "regionview.h"
#include "regionframe.h"
#include "editpoly.h"
#include "stringdlg.h"
#include "edithelpers.h"
#include "addobjectdlg.h"
#include "propertiesdlg.h"
#include "worldinfodlg.h"
#include "resource.h"
#include "classdlg.h"
#include "mainfrm.h"
#include "maptexturecoordsdlg.h"
#include "projectbar.h"
#include "geomroutines.h"
#include "rotateselection.h"
#include "texture.h"
#include "edit_actions.h"
#include "de_world.h"
#include "resource.h"
#include "node_ops.h"
#include "colorpicker.h"
#include "navigatordlg.h"
#include "navigatorstoredlg.h"
#include "advancedselect.h"
#include "advancedselectdlg.h"
#include "objectbrowserdlg.h"
#include "optionsobjectbrowser.h"
#include "vectoredit.h"
#include "generateuniquenamesdlg.h"
#include "namechangereportdlg.h"
#include "cobjinterface.h"
#include "optionsgenerateuniquenames.h"
#include "stdrvtrackers.h"
#include "ImportObjFileFormat.h"
#include "eventnames.h"
#include "ltamgr.h"
#include "polylightmap.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern BOOL g_bCheckNodes;

// MIKE 12/29/97 - Removed DoGroupNodes and merged it with OnSelectionGrouping because
//                 the undo stuff needed to be together.



//---------------------------------------------------------------------------
//
//  Statusbar strings
//
//  Purpose:	Global strings used by status bar.
//
//---------------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////////
// CRegionView

IMPLEMENT_DYNCREATE(CRegionView, CView)

CRegionView::CRegionView()
{
	m_fPixelScale			= 1.0f;
	m_Unknown5e8			= TRUE;
	m_nPixelScale			= 1;

	m_TagDrawRect.SetRect( -1, -1, -1, -1 );
	
	m_CurMouseOverHandle	= -1;
	m_Unknown5e0			= FALSE;

	m_bShowWireframe		= FALSE;
	m_Unknown58c			= FALSE;
	m_bShowObjects			= TRUE;
	m_bShowGrid				= TRUE;
	m_bShowMarker			= TRUE;

	// Setup all the defaults for the editing view.
	m_bInFocus				= FALSE;

	m_EditState				= EDIT_NOSTATE;
	m_dwGridSpacing			= 64;
	m_dwMajorGridSpacing	= 256;

	m_CursorArrow	= AfxGetApp()->LoadStandardCursor( IDC_ARROW );
	m_CursorNS		= AfxGetApp()->LoadStandardCursor( IDC_SIZENS );
	m_CursorWE		= AfxGetApp()->LoadStandardCursor( IDC_SIZEWE );
	m_CursorNWSE	= AfxGetApp()->LoadStandardCursor( IDC_SIZENWSE );
	m_CursorNESW	= AfxGetApp()->LoadStandardCursor( IDC_SIZENESW );
	m_CursorSizeAll = AfxGetApp()->LoadStandardCursor( IDC_SIZEALL );

	m_pRegion		= NULL;
	SetViewRenderRegionView(this);
	
	m_rSelectDist				= 0.0f;
	m_nCurrentTextureVertIndex	= 0;
	m_nCurLeakVertex			= 0;

	InitTracker();
}

CRegionView::~CRegionView()
{
// {MD 4/14/98} Apparently we don't need to free these since they're standard cursors.
//	::DeleteObject( m_CursorArrow );
//	::DeleteObject( m_CursorNS );
//	::DeleteObject( m_CursorWE );
}


// talon: the 2001 command ids that resource.h (Build 69's) has no name for,
// or names differently; values from the message map at 0x4dbb40.
#define ID_RV2001_SHRINK_PIXEL_SCALE		32843	// "Shrink pixel scale"
#define ID_RV2001_GROW_PIXEL_SCALE			32844	// "Grow pixel scale"
#define ID_RV2001_ALT_T						32945	// accelerator Alt+T
#define ID_RV2001_AUTO_TRIANGULATE			32875
#define ID_RV2001_NORMAL_TO_FORWARD			40007	// "As Forward Vector Rotation"
#define ID_RV2001_NORMAL_TO_UP				40008	// "As Up Vector Rotation"
#define ID_RV2001_NORMAL_TO_RIGHT			40009	// "As Right Vector Rotation"
#define ID_RV2001_SELECT_DETAIL_BRUSHES		32839	// "Select Detail Brushes"
#define ID_RV2001_INDICATOR_VIEWTYPE		59142
#define ID_RV2001_SAVE_LIGHT_DATA			40088	// "&Save Light Data"
#define ID_RV2001_PLACE_MARKER_AT_VECTOR	40067	// "Marker to Location"

// The 2001 message map (0x4dbb40), in its order.
BEGIN_MESSAGE_MAP(CRegionView, CView)
	ON_WM_CONTEXTMENU()
	ON_UPDATE_COMMAND_UI(ID_SHRINK_GRID_SPACING, OnUpdateShrinkGridSpacing)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_GRID_SPACING, OnUpdateExpandGridSpacing)
	ON_UPDATE_COMMAND_UI(ID_RV2001_SHRINK_PIXEL_SCALE, OnUpdateShrinkPixelScale)
	ON_UPDATE_COMMAND_UI(ID_RV2001_GROW_PIXEL_SCALE, OnUpdateGrowPixelScale)
	ON_UPDATE_COMMAND_UI(ID_APPLY_COLOR, OnUpdateApplyColor)
	ON_UPDATE_COMMAND_UI(ID_APPLY_TEXTURE, OnUpdateApplyTexture)
	ON_COMMAND(ID_SHRINK_GRID_SPACING, OnShrinkGridSpacing)
	ON_COMMAND(ID_EXPAND_GRID_SPACING, OnExpandGridSpacing)
	ON_COMMAND(ID_RV2001_SHRINK_PIXEL_SCALE, OnShrinkPixelScale)
	ON_COMMAND(ID_RV2001_GROW_PIXEL_SCALE, OnGrowPixelScale)
	ON_COMMAND(ID_APPLY_COLOR, OnApplyColor)
	ON_COMMAND(ID_RV2001_ALT_T, OnAltT)
	ON_COMMAND(ID_APPLY_TEXTURE, OnApplyTexture)
	ON_COMMAND(ID_HOLLOW_BRUSH, OnHollowBrush)
	ON_COMMAND(ID_IMPORT_TERRAIN_MAP, OnImportTerrainMap )
	ON_COMMAND(ID_ADDPRIM_BOX, OnCreatePrimitiveBox )
	ON_COMMAND(ID_ADDPRIM_CYLINDER, OnCreatePrimitiveCylinder )
	ON_COMMAND(ID_ADDPRIM_PYRAMID, OnCreatePrimitivePyramid )
	ON_UPDATE_COMMAND_UI(ID_HOLLOW_BRUSH, OnUpdateSingleBrushOperation)
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_DESTROY()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_COMMAND(ID_RV2001_AUTO_TRIANGULATE, OnAutoTriangulate)
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_POPUP_FUNCTIONS_REMOVEEXTRAEDGES, OnRemoveExtraEdges)
	ON_COMMAND(ID_POPUP_FUNCTIONS_MAPTEXTURECOORDS, OnMapTextureCoords)
	// talon: TODO (not written yet): ON_COMMAND(ID_RV2001_NORMAL_TO_FORWARD, OnNormalToForward)
	// talon: TODO (not written yet): ON_COMMAND(ID_RV2001_NORMAL_TO_UP, OnNormalToUp)
	// talon: TODO (not written yet): ON_COMMAND(ID_RV2001_NORMAL_TO_RIGHT, OnNormalToRight)
	// talon: TODO (not written yet): ON_UPDATE_COMMAND_UI(ID_RV2001_NORMAL_TO_FORWARD, OnUpdateNormalToVector)
	// talon: TODO (not written yet): ON_UPDATE_COMMAND_UI(ID_RV2001_NORMAL_TO_UP, OnUpdateNormalToVector)
	// talon: TODO (not written yet): ON_UPDATE_COMMAND_UI(ID_RV2001_NORMAL_TO_RIGHT, OnUpdateNormalToVector)
	ON_COMMAND(ID_POPUP_FUNCTIONS_ROTATESELECTION, OnRotateSelection)
	ON_COMMAND(ID_POPUP_FUNCTIONS_RESETALLTEXTURECOORDINATES, OnResetAllTextureCoords)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_POPUP_FUNCTIONS_JOINTAGGEDVERTICES, OnJoinTaggedVertices)
	ON_COMMAND(ID_OBJECT_HEIGHT, OnObjectHeight)
	ON_COMMAND(ID_POPUP_ADDOBJECT, OnAddObject)
	ON_COMMAND(ID_POPUP_BINDTOOBJECT, OnBindToObject)
	ON_COMMAND(ID_POPUP_OPTIONS_SHOWWIREFRAME, OnShowWireframe)
	ON_COMMAND(ID_POPUP_OBJECTPROPERTIES, OnObjectProperties)
	ON_COMMAND(ID_POPUP_WORLDINFO, OnWorldInfo)
	ON_COMMAND(ID_POPUP_SHADEMODE_WIREFRAMEPOLIES, OnShadeWireframe)
	ON_COMMAND(ID_POPUP_SHADEMODE_TEXTUREDPOLIES, OnShadeTextured)
	ON_COMMAND(ID_POPUP_SHADEMODE_FLATPOLIES, OnShadeFlat)
	ON_COMMAND(ID_POPUP_SHADEMODE_LIGHTMAPSONLY, OnShadeLightmapsOnly)
	ON_COMMAND(ID_POPUP_OPTIONS_SHOWOBJECTS, OnShowObjects)
	ON_COMMAND(ID_POPUP_OPTIONS_SHOWNORMALS, OnShowNormals)
	ON_COMMAND(ID_POPUP_OPTIONS_BACKFACINGPOLIES, OnBackfacingPolies)
	ON_COMMAND(ID_POPUP_MOVEMODE_WALKTHRUMOVEMENT, OnWalkthruMovement)
	ON_COMMAND(ID_POPUP_MOVEMODE_ORBITMOVEMENT, OnOrbitMovement)
	ON_COMMAND(ID_POPUP_VIEWMODE_BACKVIEW, OnBackView)
	ON_COMMAND(ID_POPUP_VIEWMODE_BOTTOMVIEW, OnBottomView)
	ON_COMMAND(ID_POPUP_VIEWMODE_FRONTVIEW, OnFrontView)
	ON_COMMAND(ID_POPUP_VIEWMODE_LEFTVIEW, OnLeftView)
	ON_COMMAND(ID_POPUP_VIEWMODE_PERSPECTIVEVIEW, OnPerspectiveView)
	ON_COMMAND(ID_POPUP_VIEWMODE_RIGHTVIEW, OnRightView)
	ON_COMMAND(ID_POPUP_VIEWMODE_TOPVIEW, OnTopView)
	ON_COMMAND(ID_POPUP_PROCESSWORLD, OnProcessWorld)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_COMMAND(ID_POPUP_FUNCTIONS_CLEANUPGEOMETRY, OnCleanupGeometry)
	ON_COMMAND(ID_POPUP_FUNCTIONS_UPDATEPLANES, OnUpdatePlanes)
	// talon: TODO (not written yet): ON_COMMAND(ID_POPUP_FUNCTIONS_FINDCONCAVEBRUSHES, OnFindConcaveBrushes)
	ON_COMMAND(ID_POPUP_OPTIONS_SHOWEDITGRID, OnShowEditGrid)
	ON_COMMAND(ID_POPUP_OPTIONS_SHOWMARKER, OnShowMarker)
	// talon: TODO (not written yet): ON_COMMAND(ID_POPUP_EXPORTQUAKEMAP, OnExportQuakeMap)
	ON_COMMAND(ID_POPUP_VIEWMODE_4VIEWCONFIGURATION, On4ViewConfiguration)
	ON_UPDATE_COMMAND_UI(ID_RV2001_INDICATOR_VIEWTYPE, OnUpdateStatus )
	ON_COMMAND(ID_USEDEVICE1, OnUseDevice1)
	ON_COMMAND(ID_USEDEVICE2, OnUseDevice2)
	ON_COMMAND(ID_USEDEVICE3, OnUseDevice3)
	ON_COMMAND(ID_USEDEVICE4, OnUseDevice4)
	ON_COMMAND(ID_RENDERMODE1, OnRenderMode1)
	ON_COMMAND(ID_RENDERMODE2, OnRenderMode2)
	ON_COMMAND(ID_RENDERMODE3, OnRenderMode3)
	ON_COMMAND(ID_RENDERMODE4, OnRenderMode4)
	ON_COMMAND(ID_USEDIRECT3D, OnUseDirect3D)
	ON_COMMAND(ID_POPUP_REPLACEALLTEXTURES,OnReplaceTextures)
	ON_COMMAND(ID_POPUP_SELECTTEXTURE,OnSelectTexture)
	// talon: TODO (not written yet): ON_COMMAND(ID_EDIT_SURFACE_FLAGS, OnEditSurfaceFlags)
	// talon: TODO (not written yet): ON_COMMAND(ID_EDIT_SURFACE_COLOR, OnEditSurfaceColor)
	ON_WM_CHAR()
	ON_COMMAND(ID_RESET_DETAIL_LEVELS, OnResetDetailLevels)
	ON_COMMAND(ID_NAVIGATOR_STORE, OnNavigatorStore)
	ON_COMMAND(ID_NAVIGATOR_ORGANIZE, OnNavigatorOrganize)
	ON_COMMAND(ID_NAVIGATOR_GOTONEXT, OnNavigatorGotoNext)
	ON_COMMAND(ID_NAVIGATOR_GOTOPREVIOUS, OnNavigatorGotoPrevious)
	ON_COMMAND(ID_SELECTION_SELECTALL, OnSelectionSelectAll)
	ON_COMMAND(ID_SELECTION_SELECTNONE, OnSelectionSelectNone)
	ON_COMMAND(ID_SELECTION_SELECTINVERSE, OnSelectionSelectInverse)
	ON_COMMAND(ID_SELECTION_ADVANCED, OnSelectionAdvanced)
	ON_COMMAND(ID_RV2001_SELECT_DETAIL_BRUSHES, OnSelectDetailBrushes)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_SELECTINVERSE, OnUpdateSelectionSelectInverse)
	ON_COMMAND(ID_SELECTION_HIDESELECTED, OnSelectionHideSelected)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_HIDESELECTED, OnUpdateSelectionHideSelected)
	ON_COMMAND(ID_SELECTION_UNHIDESELECTED, OnSelectionUnhideSelected)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_UNHIDESELECTED, OnUpdateSelectionUnhideSelected)
	ON_COMMAND(ID_SELECTION_GROUP, OnSelectionGroup)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_GROUP, OnUpdateSelectionGroup)
	ON_COMMAND(ID_WORLD_IMPORT_OBJECTS, OnWorldImportObjects)
	ON_COMMAND(ID_BRUSH_SPLITBRUSH, OnBrushSplitBrush)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_SPLITBRUSH, OnUpdateBrushSplitBrush)
	ON_COMMAND(ID_BRUSH_CARVE, OnBrushCarve)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_CARVE, OnUpdateBrushCarve)
	ON_COMMAND(ID_BRUSH_FLIP, OnBrushFlip)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_CARVE, OnUpdateBrushCarve)
	ON_COMMAND(ID_SELECTION_MIRROR_X, OnSelectionMirrorX)
	ON_COMMAND(ID_SELECTION_MIRROR_Y, OnSelectionMirrorY)
	ON_COMMAND(ID_SELECTION_MIRROR_Z, OnSelectionMirrorZ)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_MIRROR_X, OnUpdateSelectionMirrorX)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_MIRROR_Y, OnUpdateSelectionMirrorY)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_MIRROR_Z, OnUpdateSelectionMirrorZ)
	ON_COMMAND(ID_ADDPRIM_SPHERE, OnCreatePrimitiveSphere)
	ON_COMMAND(ID_ADDPRIM_DOME, OnCreatePrimitiveDome)
	ON_COMMAND(ID_SELECTION_CONTAINER, OnSelectionContainer)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_CONTAINER, OnUpdateSelectionContainer)
	ON_COMMAND(ID_CENTER_MARKER_ON_SELECTION, OnCenterMarkerOnSelection)
	ON_UPDATE_COMMAND_UI(ID_CENTER_MARKER_ON_SELECTION, OnUpdateCenterMarkerOnSelection)
	ON_COMMAND(ID_PLACE_MARKER_AT_CAMERA, OnPlaceMarkerAtCamera)
	ON_UPDATE_COMMAND_UI(ID_PLACE_MARKER_AT_CAMERA, OnUpdatePlaceMarkerAtCamera)
	ON_COMMAND(ID_SELECTION_GENERATE_UNIQUE_NAMES, OnSelectionGenerateUniqueNames)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_GENERATE_UNIQUE_NAMES, OnUpdateSelectionGenerateUniqueNames)
	ON_COMMAND(ID_WORLD_OBJECT_BROWSER, OnWorldObjectBrowser)
	// talon: TODO (not written yet): ON_COMMAND(ID_WORLD_DEBUG_LOADLEAKFILE, OnWorldDebugLoadLeakFile)
	ON_COMMAND(ID_WORLD_DEBUG_CLEARLEAKFINDER, OnWorldDebugClearLeakFinder)
	ON_COMMAND(ID_RV2001_PLACE_MARKER_AT_VECTOR, OnPlaceMarkerAtVector)
	ON_COMMAND(ID_WORLD_DEBUG_FIND_NAMING_CONFLICTS, OnWorldDebugFindNamingConflicts)
	ON_COMMAND(ID_WORLD_SCALE_GEOMETRY, OnWorldScaleGeometry)
	ON_COMMAND(ID_MAP_TEXTURE_TO_VIEW, OnMapTextureToView)
	ON_COMMAND(ID_RESET_TEXTURE_COORDS, OnResetTextureCoords)
	ON_COMMAND(ID_SELECTION_SCALE, OnMatchTextureCoords)
	// talon: TODO (not written yet): ON_COMMAND(0, OnCommandZero)
	ON_COMMAND(ID_SELECTION_HIDEINVERSE, OnSelectionHideInverse)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_HIDEINVERSE, OnUpdateSelectionHideInverse)
	ON_COMMAND(ID_SELECTION_UNHIDEINVERSE, OnSelectionUnhideInverse)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_UNHIDEINVERSE, OnUpdateSelectionUnhideInverse)
	ON_COMMAND(ID_FILE_IMPORT_WORLD, OnFileImportWorld)
	ON_COMMAND(ID_SELECTION_SAVEPREFAB, OnSelectionSavePrefab)
	ON_UPDATE_COMMAND_UI(ID_SELECTION_SAVEPREFAB, OnUpdateSelectionSavePrefab)
	ON_UPDATE_COMMAND_UI(ID_IMPORT_TERRAIN_MAP, OnUpdateSingleBrushOperation )
	ON_UPDATE_COMMAND_UI( ID_INDICATOR_CAMERA, OnUpdateStatus )
	ON_UPDATE_COMMAND_UI( ID_INDICATOR_GRID, OnUpdateStatus )
	ON_COMMAND(ID_WORLD_DEBUG_GOTO_NEXT_LEAK_VERTICE, OnWorldDebugGotoNextLeakVertice)
	ON_UPDATE_COMMAND_UI(ID_WORLD_DEBUG_GOTO_NEXT_LEAK_VERTICE, OnUpdateWorldDebugGotoNextLeakVertice)
	ON_COMMAND(ID_WORLD_DEBUG_GOTO_PREVIOUS_LEAK_VERTICE, OnWorldDebugGotoPreviousLeakVertice)
	ON_UPDATE_COMMAND_UI(ID_WORLD_DEBUG_GOTO_PREVIOUS_LEAK_VERTICE, OnUpdateWorldDebugGotoPreviousLeakVertice)
	ON_UPDATE_COMMAND_UI(ID_WORLD_DEBUG_CLEARLEAKFINDER, OnUpdateWorldDebugClearLeakFinder)
	ON_COMMAND(ID_WORLD_SPECIAL_ALIGNPOLYTEXTURES, OnAlignPolyTextures)
	ON_COMMAND(ID_LOAD_LIGHT_DATA, OnLoadLightData)
	ON_COMMAND(ID_UNLOAD_LIGHT_DATA, OnUnloadLightData)
	ON_UPDATE_COMMAND_UI(ID_UNLOAD_LIGHT_DATA, OnUpdateLightData)
	ON_COMMAND(ID_RV2001_SAVE_LIGHT_DATA, OnSaveLightData)
	ON_UPDATE_COMMAND_UI(ID_RV2001_SAVE_LIGHT_DATA, OnUpdateLightData)
END_MESSAGE_MAP()


// talon: the 2001 tracker set-up, written from the image (0x452bb7..
// 0x455025, one function: the function table's 0x453e95 row starts inside
// it). In the image it follows RegionView.cpp's constructor, destructor and
// message map (0x4528f2..0x452bb6) and comes before its other functions,
// as it does here. The 2001 trackers take their start and end events here
// (Build 69 reads them from the hotkey configuration): 53 trackers, names
// and keys as the image has them.

// The focus tracker's name is the one name the image reads from a variable
// (push 0x4f9aa8, a pointer to "Focus", RegionView.obj's first .data).
static LPCTSTR g_pszFocusTracker = "Focus";

void CRegionView::InitTracker()
{
	//the current tracker that is being added
	CRegionViewTracker *pRVTracker;

	//the tracker the drawing tracker lets override it
	CRegionViewTracker *pNavRotateTracker;

	pRVTracker = new CRVTrackerFocus(g_pszFocusTracker, this);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEMOVE));
	((CRVTrackerFocus *)pRVTracker)->SetLockEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Select.Depth", this, CRegionView::OnDepthSelect);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, 'Y'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'T', FALSE));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("ApplyTexture", this, CRegionView::OnApplyTextureClick);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, 'Y'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'A'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("SplitBrush", this, CRegionView::OnBrushSplitBrush);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'S'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'S'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("CarveBrush", this, CRegionView::OnBrushCarve);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'H'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'H'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Select.None", this, CRegionView::OnSelectNone);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'U'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'U'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("SetGridDir.Forward", this, CRegionView::OnSetGridOrientation);
	((CRVCallback *)pRVTracker)->SetData(GRID_FORWARD);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, '8'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, '8'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("SetGridDir.Right", this, CRegionView::OnSetGridOrientation);
	((CRVCallback *)pRVTracker)->SetData(GRID_RIGHT);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, '9'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, '9'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("SetGridDir.Up", this, CRegionView::OnSetGridOrientation);
	((CRVCallback *)pRVTracker)->SetData(GRID_UP);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, '0'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, '0'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("InitTextureSpace", this, CRegionView::OnInitTextureSpace);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'K'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'K'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Delete", this, CRegionView::OnDelete);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_DELETE));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_DELETE));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Nudge.Up", this, CRegionView::OnNudge);
	((CRVCallback *)pRVTracker)->SetData(VK_UP);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	((CRVCallback *)pRVTracker)->SetToggle(1, CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_UP));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_UP));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Nudge.Down", this, CRegionView::OnNudge);
	((CRVCallback *)pRVTracker)->SetData(VK_DOWN);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	((CRVCallback *)pRVTracker)->SetToggle(1, CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_DOWN));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_DOWN));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Nudge.Left", this, CRegionView::OnNudge);
	((CRVCallback *)pRVTracker)->SetData(VK_LEFT);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	((CRVCallback *)pRVTracker)->SetToggle(1, CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_LEFT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_LEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Nudge.Right", this, CRegionView::OnNudge);
	((CRVCallback *)pRVTracker)->SetData(VK_RIGHT);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	((CRVCallback *)pRVTracker)->SetToggle(1, CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_RIGHT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_RIGHT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("DeleteEdges.Remove", this, CRegionView::OnDeleteEdges);
	((CRVCallback *)pRVTracker)->SetData(TRUE);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_HOME));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_HOME));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("DeleteEdges", this, CRegionView::OnDeleteEdges);
	((CRVCallback *)pRVTracker)->SetData(FALSE);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_END));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_END));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("SplitEdge", this, CRegionView::OnSplitEdges);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_INSERT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_INSERT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("DeleteTaggedPolies", this, CRegionView::OnDeleteTaggedPolygons);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_NEXT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_NEXT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("GridToPoly", this, CRegionView::OnGridToPoly);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'G'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'G'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("TagPolyVerts", this, CRegionView::OnTagPolyVerts);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'Y'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'Y'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("FlipNormal", this, CRegionView::OnFlipNormal);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'F'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'F'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("OrbitVertex", this, CRegionView::OnOrbitVertex);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'P'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'P'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("OrbitVertex", this, CRegionView::OnSelectLightNode);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("EditLightNode.Up", this, CRegionView::OnEditLightNode);
	((CRVCallback *)pRVTracker)->SetData(TRUE);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'Q'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'Q'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("EditLightNode.Down", this, CRegionView::OnEditLightNode);
	((CRVCallback *)pRVTracker)->SetData(FALSE);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'W'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'W'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNodeRotate("NodeRotate", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'N'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'N'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerTextureMove("TextureMove", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'R'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'R'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerTextureWrap("TextureWrap", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'L'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'L'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerBrushSize("BrushSize", this);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerCurveEdit("CurveEdit", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'P'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	pRVTracker->SetPhantomEnd(FALSE);
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerFarDist("FarDist", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'C'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'C'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerGridSize("GridSize", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'Z'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSERIGHT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerMarkerMove("MarkerMove", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'X'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'X'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertMove("VertMove.Poly", this, CRVTrackerVertMove::esm_Poly);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSERIGHT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertMove("VertMove.SelVert", this, CRVTrackerVertMove::esm_SelVert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'S'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'S'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertMove("VertMove.ImmVert", this, CRVTrackerVertMove::esm_ImmVert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'M'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'M'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertSnap("VertSnap.Edge.SelVert", this, CRVTrackerVertMove::esm_SelVert, CRVTrackerVertSnap::est_Edge);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'D'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'D'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertSnap("VertSnap.Edge", this, CRVTrackerVertMove::esm_ImmVert, CRVTrackerVertSnap::est_Edge);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'E'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'E'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertSnap("VertSnap.Vert.SelVert", this, CRVTrackerVertMove::esm_SelVert, CRVTrackerVertSnap::est_Vert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'B'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'B'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertSnap("VertSnap.Vert", this, CRVTrackerVertMove::esm_ImmVert, CRVTrackerVertSnap::est_Vert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'V'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'V'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertScale("VertScale.Poly", this, CRVTrackerVertScale::esm_Poly);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'W'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'W'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertScale("VertScale.SelVert", this, CRVTrackerVertScale::esm_SelVert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'Q'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'Q'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNodeMove("NodeMove.Handle", this, CRVTrackerNodeMove::FLAG_HANDLE);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNodeMove("NodeMove.Snap", this, CRVTrackerNodeMove::FLAG_SNAP);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'M'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNodeMove("NodeMove.Perpendicular", this, CRVTrackerNodeMove::FLAG_PERP);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'M'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSERIGHT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNavMove("NavMove", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'I'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'I'));
	pRVTracker->SetPhantomEnd(FALSE);
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNavRotate("NavRotate", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'O'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'O'));
	pRVTracker->SetPhantomEnd(FALSE);
	m_cTrackerMgr.AddTracker(pRVTracker);

	pNavRotateTracker = pRVTracker;
	pRVTracker = new CRVTrackerDrawPoly("DrawPoly", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_SPACE));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_ESCAPE));
	pRVTracker->AddOverride(pNavRotateTracker);
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerTag("Tag.All", this, CRVTrackerTag::FLAG_ALL);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("UnJoinBrushes", this, CRegionView::OnBrushUnJoin);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'J'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'J'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("JoinBrushes", this, CRegionView::OnBrushJoin);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'J'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'J'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("TriangulateBrushes", this, CRegionView::OnAutoTriangulate);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'E'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'E'));
	m_cTrackerMgr.AddTracker(pRVTracker);
}





inline void CRegionView::CheckAMenuItem( CMenu *pMenu, UINT nItem, int testVal1, int testVal2 )
{
	pMenu->CheckMenuItem( nItem, (testVal1==testVal2) ? (MF_CHECKED|MF_BYCOMMAND) : (MF_UNCHECKED|MF_BYCOMMAND) );
}



inline void CRegionView::EnableAMenuItem( CMenu *pMenu, UINT nItem, int testVal1, int testVal2 )
{
	pMenu->EnableMenuItem( nItem, (testVal1 == testVal2) ? (MF_ENABLED | MF_BYCOMMAND) : (MF_GRAYED | MF_BYCOMMAND));
}


BOOL CRegionView::OnIdle( LONG lCount )
{
	BOOL bTrackerBusy;

	BEGIN_MEMORYEXCEPTION()
	{
		CPoint		point = GetCurMousePos();
		BOOL		bDraw = FALSE;
		CRect		rect;


		if( !m_bInFocus || !m_pRegion )
			return TRUE;

		bTrackerBusy = m_cTrackerMgr.ProcessEvent(CUIEvent(UIEVENT_NONE));
		// Update the immediate selection information  (Note : This should be moved into a tracker...)
		if ( (!bTrackerBusy) && (point != m_LastMousePos))
		{
			if( UpdateImmediateSelection() )
			{
				GetRegionDoc()->UpdateAllViews( this );
				GetClientRect( &rect );
				bDraw = TRUE;
			}
		}

		if( bDraw )
			DrawRect( &rect );

		if(!bTrackerBusy)
			UpdateCursor();
		
		m_LastMousePos = point;

		UpdateStatusBar( );
	}
	END_MEMORYEXCEPTION()

	return bTrackerBusy;
}







/////////////////////////////////////////////////////////////////////////////
// CRegionView drawing

void CRegionView::OnDraw(CDC* pDC)
{
	BEGIN_MEMORYEXCEPTION()
	{
		// talon: 2001 (0x45562e) gets the client rectangle and doesn't use it.
		CRect rect;
		GetClientRect( &rect );

		DrawRect( NULL );
	}
	END_MEMORYEXCEPTION()
}


void CRegionView::DrawRect(CRect *pRect)
{
	CViewRender::DrawRect(pRect);

	if (m_bInFocus)
	{
		CRect rcClient, cCorner;
		GetClientRect(&rcClient);

		CDC *pDC = GetDC();
		if (pDC)
		{
			cCorner = rcClient;
			cCorner.DeflateRect(1,1,1,1);
			pDC->DrawEdge(cCorner, BDR_SUNKENOUTER, BF_TOPLEFT | BF_BOTTOMRIGHT);
			pDC->DrawEdge(rcClient, BDR_RAISEDINNER, BF_TOPLEFT | BF_BOTTOMRIGHT);
			ReleaseDC(pDC);
		}
	}
}


void CRegionView::GetRenderRect(LPRECT pRect, BOOL bClient)
{
	if (bClient)
		GetClientRect(pRect);
	else
		GetWindowRect(pRect);
	if (GetInFocus())
	{
		pRect->top += 2;
		pRect->left += 2;
		pRect->bottom -= 2;
		pRect->right -= 2;
	}
}



/////////////////////////////////////////////////////////////////////////////
// CRegionView diagnostics

#ifdef _DEBUG
void CRegionView::AssertValid() const
{
	CView::AssertValid();
}

void CRegionView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG






/////////////////////////////////////////////////////////////////////////////
// CRegionView message handlers

void CRegionView::OnInitialUpdate() 
{
	CRect rcClient;

	CView::OnInitialUpdate();
	
	// Status message..
	GetClientRect(&rcClient);

	// Install the main timer.
	m_TimerID = SetTimer( 10, 1, NULL );

	// Get the region pointer and set it in various structures.
	m_pRegion = &GetRegionDoc()->m_Region;

	// talon: 2001 (0x455797): the view's own defaults.
	SetWalkThroughMovement( TRUE );
	SetShadeMode( SM_TEXTURED );
	SetSelectBackfaces( TRUE );
	SetShowNormals( FALSE );
	SetShowWireframe( FALSE );

	SetupInitialDrawMgr();		
}

void CRegionView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);

	BEGIN_MEMORYEXCEPTION()
	{
		if(cx && cy)
		{
			SetupNewSize( cx, cy );
		}	
	}
	END_MEMORYEXCEPTION()
}

BOOL CRegionView::OnEraseBkgnd(CDC* pDC) 
{
	return TRUE;
}

void CRegionView::OnDestroy() 
{
	CView::OnDestroy();
	
	KillTimer( m_TimerID );
}

//--------------------------------------------------------------------------------------
//
// CRegionView::OnLButtonDown
//
// Purpose:  Message handler for left button down.
//
//--------------------------------------------------------------------------------------
void CRegionView::OnLButtonDown( UINT nFlags, CPoint point ) 
{
	BEGIN_MEMORYEXCEPTION()
	{
		//SetCapture();

		m_cTrackerMgr.ProcessEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT, point));
	}
	END_MEMORYEXCEPTION()
	
	CView::OnLButtonDown(nFlags, point);
}






void CRegionView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if (!m_cTrackerMgr.ProcessEvent(CUIMouseEvent(UIEVENT_MOUSEMOVE, UI_MOUSENONE, point)))
	{

		// Calculate the distance moved from last mouse point...
		LONG dist;
		dist = ( point.x - m_MousePoint.x ) * ( point.x - m_MousePoint.x ) + 
			( point.y - m_MousePoint.y ) * ( point.y - m_MousePoint.y );

		m_MousePoint = point;
		// Reset selection distance if mouse moved enough...
		if( dist > 3 )
			m_rSelectDist = 0.0f;	
	}

	CView::OnMouseMove(nFlags, point);
}


void CRegionView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	BEGIN_MEMORYEXCEPTION()
	{
		BOOL		bDraw = FALSE;
		
		bDraw |= m_cTrackerMgr.ProcessEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT, point));

		if (!bDraw)
			bDraw |= UpdateImmediateSelection();

		if( bDraw )
			DrawRect();
	}
	END_MEMORYEXCEPTION()

	CView::OnLButtonUp(nFlags, point);
}


void CRegionView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	BEGIN_MEMORYEXCEPTION()
	{
		m_bDoMenu = !m_cTrackerMgr.ProcessEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT, point));
	}
	END_MEMORYEXCEPTION()
	
	// SetCapture();
	CView::OnRButtonDown(nFlags, point);
}


void CRegionView::OnRButtonUp(UINT nFlags, CPoint point) 
{
	BEGIN_MEMORYEXCEPTION()
	{
		BOOL		bDraw = FALSE;

		if (!m_cTrackerMgr.ProcessEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSERIGHT, point)))
			UpdateImmediateSelection();

		if( bDraw )
			DrawRect();

		// ReleaseCapture();	
		CView::OnRButtonUp(nFlags, point);
	}
	END_MEMORYEXCEPTION()
}


void CRegionView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if( m_bInFocus )
	{
		BEGIN_MEMORYEXCEPTION()
		{
			m_cTrackerMgr.ProcessEvent(CUIKeyEvent(UIEVENT_KEYDOWN, nChar, (nFlags & (1 << 14)) == 0));
		}
		END_MEMORYEXCEPTION()
	}
	
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CRegionView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	BEGIN_MEMORYEXCEPTION()
	{	
		m_cTrackerMgr.ProcessEvent(CUIKeyEvent(UIEVENT_KEYUP, nChar));
	}
	END_MEMORYEXCEPTION()
	
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}



static void TriangulateBrushPolies(CEditBrush *pBrush)
{
	DWORD			i, j, k;
	CEditPoly		*pPoly, *pNewPoly;
	CReal			dot;

	CMoArray<CEditPoly*>	toAdd;
	CMoArray<CBasePoly*>	tris;
	
	
	// Go thru all polies.
	for(i=0; i < pBrush->m_Polies; i++)
	{
		pPoly = pBrush->m_Polies[i];

		for(j=0; j < pPoly->NumVerts(); j++)
		{
			dot = pPoly->m_Plane.m_Normal.Dot(pPoly->Pt(j)) - pPoly->m_Plane.m_Dist;
			
			if(dot > 0.1f || dot < -0.1f)
			{
				// Ok, needs to be triangulated.  Attempt to do so.
				if(pPoly->GetTriangles(tris))
				{
					for(k=0; k < tris; k++)
					{
						pNewPoly = new CEditPoly;
						
						pNewPoly->m_pBrush = pBrush;
						pNewPoly->CopyAttributes(pPoly);
						pNewPoly->m_Indices.CopyArray(tris[k]->m_Indices);

						g_GenerateNormal(pNewPoly);

						toAdd.Append(pNewPoly);
					}

					// Remove the poly from the brush.
					delete pPoly;
					pBrush->m_Polies.Remove(i);
					--i;
				}

				DeleteAndClearArray(tris);
				break;
			}
		}
	}

	for(i=0; i < toAdd; i++)
		pBrush->m_Polies.Append(toAdd[i]);

	pBrush->UpdatePlanes();
}


void CRegionView::OnAutoTriangulate()
{
	DWORD i;
	CEditRegion *pRegion = GetRegion();
	CWorldNode *pNode;
	PreActionList actionList;
	GPOS pos;


	// Build a list of mods.
	for(i=0; i < pRegion->m_Selections; i++)
	{
		pNode = pRegion->m_Selections[i];
		
		if(pNode->GetType() == Node_Brush)
		{
			actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, pNode));
		}
	}
	
	// Setup the undo.
	GetRegionDoc()->Modify(&actionList, FALSE);

	// Carry out the action.	
	for(pos=actionList; pos; )
	{
		TriangulateBrushPolies(actionList.GetNext(pos)->m_pNode->AsBrush());
	}

	if(actionList.GetSize() > 0)
	{
		GetRegionDoc()->RedrawAllViews();
		GDeleteAndRemoveElements(actionList);
	}
}

// talon: 2001 (0x455ef1, accelerator Alt+T; the name ours): toggles the
// immediate poly's flag at 0x70.
void CRegionView::OnAltT()
{
	if( GetEditMode() == GEOMETRY_EDITMODE )
	{
		GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_MODIFYNODE, IPoly().m_pBrush), TRUE);
		IPoly()()->m_Unknown_70 = !IPoly()()->m_Unknown_70;
	}
}



void CRegionView::OnKillFocus(CWnd* pNewWnd) 
{
	m_bInFocus = FALSE;

	// Get rid of the focus rectangle 
	// (Note : Can't DrawRect here because focus changes before the window is completely created)
	Invalidate();


	// Send an empty message to the trackers so they can pick up on the focus change
	m_cTrackerMgr.ProcessEvent(CUIEvent(UIEVENT_NONE));

	CView::OnKillFocus(pNewWnd);
	
}


void CRegionView::OnSetFocus(CWnd* pOldWnd) 
{
	m_bInFocus = TRUE;	
	
	// Make sure the focus rectangle shows up
	// (Note : Can't DrawRect here because focus changes before the window is completely created)
	Invalidate();

	


	// Send an empty message to the trackers so they can pick up on the focus change
	m_cTrackerMgr.ProcessEvent(CUIEvent(UIEVENT_NONE));

	CView::OnSetFocus(pOldWnd);

}


void CRegionView::OnEditUndo() 
{
	BEGIN_MEMORYEXCEPTION()
	{
		CRect		rect;
		
		BeginWaitCursor();

		// Turn off the drawing of the node view
		if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
		{
			GetNodeView()->SetRedraw(FALSE);
		}

		// Get the node ID of the selected node so that it can be reselected after the Undo
		DWORD dwID=0;		
		CWorldNode *pSelectedNode=GetNodeView()->GetSelectedNode();
		if (pSelectedNode)
		{
			dwID=pSelectedNode->GetUniqueID();
		}

		GetPropertiesDlg()->Term();
		GetNodeView( )->Term( );
		ClearSelections( TRUE, TRUE, TRUE );
		m_EditState = EDIT_NOSTATE;
		
		GetRegionDoc()->m_UndoMgr.Undo();

		DrawRect();
		SetFocus();
		GetNodeView()->Init(GetRegionDoc());
		GetDocument()->UpdateAllViews( this );

		// Determine if the selected node is still in the world and reselect the node if possible
		if (pSelectedNode)
		{
			pSelectedNode=GetRegion()->FindNodeByID(dwID);
			if (pSelectedNode)
			{
				GetNodeView()->HighlightNode(pSelectedNode);
			}
		}

		// Turn on the drawing of the node view
		if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
		{
			GetNodeView()->SetRedraw(TRUE);
			GetNodeView()->Invalidate();	
		}

		EndWaitCursor();
	}
	END_MEMORYEXCEPTION()
}

void CRegionView::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetRegionDoc()->m_UndoMgr.m_nUndos > 0 );	
}


void CRegionView::OnEditRedo() 
{
	BEGIN_MEMORYEXCEPTION()
	{
		CRect		rect;
	
		// Turn off the drawing of the node view
		if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
		{
			GetNodeView()->SetRedraw(FALSE);
		}

		// Get the node ID of the selected node so that it can be reselected after the Undo
		DWORD dwID=0;		
		CWorldNode *pSelectedNode=GetNodeView()->GetSelectedNode();
		if (pSelectedNode)
		{
			dwID=pSelectedNode->GetUniqueID();
		}

		GetPropertiesDlg()->Term();
		GetNodeView( )->Term( );
		ClearSelections( TRUE, TRUE, TRUE );
		m_EditState = EDIT_NOSTATE;
												   
		GetRegionDoc()->m_UndoMgr.Redo();

		DrawRect();
		SetFocus();
		GetNodeView()->Init(GetRegionDoc());
		GetDocument()->UpdateAllViews( this );

		// Determine if the selected node is still in the world and reselect the node if possible
		if (pSelectedNode)
		{
			pSelectedNode=GetRegion()->FindNodeByID(dwID);
			if (pSelectedNode)
			{
				GetNodeView()->HighlightNode(pSelectedNode);
			}
		}

		// Turn on the drawing of the node view
		if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
		{
			GetNodeView()->SetRedraw(TRUE);
			GetNodeView()->Invalidate();		
		}
	}
	END_MEMORYEXCEPTION()
}

void CRegionView::OnUpdateEditRedo(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetRegionDoc()->m_UndoMgr.m_nRedos > 0 );
}



void CRegionView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if( lHint == REGIONVIEWUPDATE_LIGHTMAPS )
	{
		// talon: 2001 (0x4562f1): only the Direct3D views draw lightmaps.
		if( m_DrawType == 1 )
			DrawRect();
	}
	else if( lHint == REGIONVIEWUPDATE_REDRAW )
	{
		DrawRect();
	}
	else if( lHint == REGIONVIEWUPDATE_EDITMODE )
	{
		UpdateImmediateSelection();
		DrawRect();
	}
	else
	{
		if( (pSender != this) && (pSender != NULL) )
			DrawRect();	
	}
}


void CRegionView::OnAddObject() 
{
	BEGIN_MEMORYEXCEPTION()
	{	
		// See what type of object, if any, is selected in the code view.
		CClassDlg dlg;
		
		CString sTitle;
		sTitle.LoadString(IDS_CHOOSE_OBJECT_TYPE);

		dlg.SetProject(GetProject());
		dlg.SetTitle(sTitle);

		if( dlg.DoModal() == IDOK )
		{
			// Get the document
			CRegionDoc *pDoc=GetRegionDoc();
			if (!pDoc)
			{
				return;
			}

			// Get the region
			CEditRegion *pRegion=pDoc->GetRegion();

			// Add the object
			CWorldNode *pObject=pDoc->AddObject(dlg.GetSelectedClass(), GetRegionDoc()->GetRegion()->GetMarker());

			// Bind the node to the active parent
			pDoc->BindNode(pObject, pRegion->GetActiveParentNode());

			// Select the object
			pDoc->SelectNode(pObject);
		}	
	}
	END_MEMORYEXCEPTION()
}

/************************************************************************/
// Import objects from a .txt file
void CRegionView::OnWorldImportObjects() 
{
	CString sFileName;
	sFileName = GetProject()->m_BaseProjectDir + "\\*.txt";
	// Let the user chose the file to import from
	CFileDialog fileDlg(TRUE, "txt", (LPCTSTR)sFileName, OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT, "Text (*.txt)|*.txt|All Files (*.*)|*.*||");

	// Display the dialog
	if (fileDlg.DoModal() == IDOK)
	{
		//clear our selections so that the newly added objects will be selected
		m_pRegion->ClearSelections();

		//open up each and every file
		POSITION Pos = fileDlg.GetStartPosition();

		while(Pos)
		{
			CString sFilename = fileDlg.GetNextPathName(Pos);

			// Create the object importer class
			CObjectImporter importer;
			
			// Import the objects from the file
			importer.LoadObjectFile(sFilename);
			importer.ImportObjects();
		}
	}	
}

/************************************************************************/
// Imports a world into this world
void CRegionView::OnFileImportWorld() 
{
	// talon: 2001 (0x456566): .ed worlds, .obj files and LTA geometry.
	CString sFileName;
	sFileName = GetProject()->m_BaseProjectDir + "\\*.ed";
	CString sFilter;
	sFilter = "DEdit World (*.ed)|*.ed|Obj File (*.obj)|*.obj|LTA Geometry (*.lta)|*.lta||";
	// Let the user choose the file to import from
	CFileDialog fileDlg(TRUE, "ed", (LPCTSTR)sFileName, 0, sFilter, NULL);

	// Display the dialog
	if (fileDlg.DoModal() != IDOK)
	{
		return;
	}

	// Import the file, leaving its original location in tact without
	// pasting it at the green crosshair.
	CString fileName = fileDlg.GetPathName();
	fileName.MakeLower();
	if( strstr( fileName, ".ed" ) )
	{
		ImportWorldFile( fileDlg.GetPathName(), FALSE /* Don't place at crosshair */ );
	}
	else if( strstr( fileName, ".lta" ) )
	{
		ImportLTAFile( fileDlg.GetPathName(), FALSE /* Don't place at crosshair */ );
	}
	else if( strstr( fileName, ".obj" ) )
	{
		ImportOBJFile( fileDlg.GetPathName(), FALSE /* Don't place at crosshair */ );
	}
	else
	{
		MessageBox("Error: Unknown extension on selected file");
	}
}

/************************************************************************/
// This imports a file into the region and optionally positions it at
// the green crosshair location.
//
// Returns: TRUE if the file import was a success.
BOOL CRegionView::ImportLTAFile(CString sFilename, BOOL bPositionAtCrosshair)
{
	// talon: 2001 (0x45670c): LTA geometry, through the 2001 LoadLTA (no
	// file version, no prefabs).

	// The string holder for the region
	CStringHolder stringHolder;

	// The region class
	CEditRegion region;

	// Set the string holder for the region
	region.m_pStringHolder = GetRegion()->m_pStringHolder;

	// Load the region
	BeginWaitCursor();
	RegionLoadStatus status=region.LoadLTA(sFilename, GetProject());
	EndWaitCursor();

	// Handle the errors
	switch (status)
	{
	case REGIONLOAD_INVALIDFILE:
		{
			MessageBox("Error: Invalid file", "Error", MB_OK);
			return FALSE;
			break;
		}
	case REGIONLOAD_OUTOFMEMORY:
		{
			MessageBox("Error: Out of memory", "Error", MB_OK);
			return FALSE;
			break;
		}
	case REGIONLOAD_INVALIDFILEVERSION:
		{
			MessageBox("Error: Invalid file version", "Error", MB_OK);
			return FALSE;
			break;
		}
	case REGIONLOAD_OK:
		{
			// No errors
			break;
		}
	default:
		{
			MessageBox("Unknown error", "Error", MB_OK);
			return FALSE;
		}
	}

	// Update all the texture IDs and polygon pieces.
	region.UpdateTextureIDs(GetProject());
	region.UpdatePolyTriangles();

	// This stores the nodes that are going to be pasted
	CMoArray<CWorldNode*> nodeArray;

	// Set the cache size to the number of brushes plus the number of objects
	nodeArray.SetCacheSize(region.m_Brushes.GetSize()+region.m_Objects.GetSize());

	// Build the flattened array so that it can be pasted via the clipboard
	region.FlattenTreeToArray(&region.m_RootNode, nodeArray);

	// Remove the root node
	if (nodeArray.GetSize() > 0)
	{
		nodeArray.Remove(0);

		// Paste the nodes
		if (nodeArray.GetSize() > 0)
		{
			// Don't check nodes in debug because it takes a very long time
			BOOL bCheckNodes=g_bCheckNodes;
			g_bCheckNodes=FALSE;

			PreActionList actionList;
			((CMainFrame *)AfxGetMainWnd( ))->GetClipboard( )->PasteNodes( GetRegion(), nodeArray, actionList, bPositionAtCrosshair);

			// Setup the undo for the new nodes it added.
			GetRegionDoc()->Modify(&actionList, TRUE);
			GetRegionDoc()->NotifySelectionChange();

			// Reset this parameter
			g_bCheckNodes=bCheckNodes;
		}
	}

	return TRUE;
}

/************************************************************************/
// This imports a file into the region and optionally positions it at
// the green crosshair location.
//
// Returns: TRUE if the file import was a success.
BOOL CRegionView::ImportWorldFile(CString sFilename, BOOL bPositionAtCrosshair)
{
	// talon: 2001 (0x4568fe; the name from ImportOBJFile's "calling
	// ImportWorldFile"): a .ed world.

	// The string holder for the region
	CStringHolder stringHolder;

	// Open the file
	CMoFileIO file;
	if (!file.Open(sFilename, "rb"))
	{
		MessageBox("Error: Unable to open file", "Error", MB_OK);
		return FALSE;
	}

	// The region class
	CEditRegion region;

	// Set the string holder for the region
	region.m_pStringHolder = GetRegion()->m_pStringHolder;

	// Load the region
	BeginWaitCursor();
	RegionLoadStatus status=region.LoadFile(file, NULL, GetProject());
	EndWaitCursor();

	// Close the file
	file.Close();

	// Handle the errors
	switch (status)
	{
	case REGIONLOAD_INVALIDFILE:
		{
			MessageBox("Error: Invalid file", "Error", MB_OK);
			return FALSE;
			break;
		}
	case REGIONLOAD_OUTOFMEMORY:
		{
			MessageBox("Error: Out of memory", "Error", MB_OK);
			return FALSE;
			break;
		}
	case REGIONLOAD_INVALIDFILEVERSION:
		{
			MessageBox("Error: Invalid file version", "Error", MB_OK);
			return FALSE;
			break;
		}
	case REGIONLOAD_OK:
		{
			// No errors
			break;
		}
	default:
		{
			MessageBox("Unknown error", "Error", MB_OK);
			return FALSE;
		}
	}

	// Update all the texture IDs and polygon pieces.
	region.UpdateTextureIDs(GetProject());
	region.UpdatePolyTriangles();

	// This stores the nodes that are going to be pasted
	CMoArray<CWorldNode*> nodeArray;

	// Set the cache size to the number of brushes plus the number of objects
	nodeArray.SetCacheSize(region.m_Brushes.GetSize()+region.m_Objects.GetSize());

	// Build the flattened array so that it can be pasted via the clipboard
	region.FlattenTreeToArray(&region.m_RootNode, nodeArray);

	// Remove the root node
	if (nodeArray.GetSize() > 0)
	{
		nodeArray.Remove(0);

		// Paste the nodes
		if (nodeArray.GetSize() > 0)
		{
			// Don't check nodes in debug because it takes a very long time
			BOOL bCheckNodes=g_bCheckNodes;
			g_bCheckNodes=FALSE;

			PreActionList actionList;
			((CMainFrame *)AfxGetMainWnd( ))->GetClipboard( )->PasteNodes( GetRegion(), nodeArray, actionList, bPositionAtCrosshair);

			// Setup the undo for the new nodes it added.
			GetRegionDoc()->Modify(&actionList, TRUE);
			GetRegionDoc()->NotifySelectionChange();

			// Reset this parameter
			g_bCheckNodes=bCheckNodes;
		}
	}

	return TRUE;
}

/************************************************************************/
// This imports a file into the region and optionally positions it at
// the green crosshair location.
//
// Returns: TRUE if the file import was a success.

// -------------------------------------------------------------
bool CRegionView::CreateBrushesFromScene(CEditRegion *pRegion, CObjInterface *pScene )
{
	ClassDef		*pClassDef;
	ClassDef		*pTerrainClassDef;
	CEditBrush		*pTemplateBrush;
	CBaseEditObj	*pTerrainObj;
	CEditBrush		*pBrush;
	CEditPoly		*pPoly;
	CEditVert		vert;
	CBoolProp		*pNoSnap;

	pClassDef			= GetProject()->FindClassDef("Brush");	
	pTerrainClassDef	= GetProject()->FindClassDef("Terrain"); 

	pTemplateBrush = no_CreateNewBrush(pRegion);
	SetupWorldNodeFromClass(pClassDef, NULL, pTemplateBrush, pRegion);

	// Create a Terrain object to hold the brushes.
	pTerrainObj = new CBaseEditObj;
	no_InitializeNewNode(pRegion, pTerrainObj, pRegion->GetRootNode());
	SetupWorldNodeFromClass(pTerrainClassDef, NULL, pTerrainObj, pRegion);

	OutputDebugString("\ncreating brushes");

	if( pScene->m_faceSetList.GetSize()>0 )
	{
		for(int i=0; i < pScene->m_faceSetList.GetSize(); i++)
		{
			bool foundUVs = false;
			float tvert[8];
			float vvert[12];

			pBrush = no_CreateNewBrush(pRegion);
			no_AttachNode(pRegion, pBrush, pTerrainObj);
			pPoly = new CEditPoly(pBrush);
			
			assert(pPoly);

			pBrush->m_Polies.Append(pPoly);
			
			// Copy the values from the template brush.
			pBrush->m_PropList.CopyValues(&pTemplateBrush->m_PropList);
		
			pBrush->SetClassName(pTemplateBrush->GetClassName());			

			int faceSize = pScene->m_faceSetList[i].GetSize();
		
			if( pScene->m_flag == CObjInterface::RAW )
			{
				if( 3 == faceSize  || 4 == faceSize || 12 < faceSize )			// v v v [v]
				{
					for( int vertNum = 0; vertNum < faceSize; vertNum++ )
					{
						int idx = pScene->m_faceSetList[i][faceSize-vertNum-1];
						vert.x = pScene->m_vertexList[idx-1][0]; 
						vert.y = pScene->m_vertexList[idx-1][1]; 
						vert.z = pScene->m_vertexList[idx-1][2]; 
						pBrush->m_Points.Append(vert);
						pPoly->m_Indices.Append(vertNum);
					}
				}
				else if( 6 == faceSize  || 8 == faceSize )		// v/t v/t v/t [v/t]
				{
					foundUVs = true;	// face has texture info, don't reset later
					int uu, vv;
					uu = vv = 0;

					for(int vertNum = 0, indexCount = 0; 
						vertNum < faceSize; 
						vertNum+=2, indexCount++ )
					{
						int idx = pScene->m_faceSetList[i][faceSize-vertNum-2];
						vert.x = pScene->m_vertexList[idx-1][0]; 
						vert.y = pScene->m_vertexList[idx-1][1]; 
						vert.z = pScene->m_vertexList[idx-1][2];
						idx = pScene->m_faceSetList[i][faceSize-vertNum-1];
						tvert[uu++] = pScene->m_texCoordList[idx-1][0];
						tvert[uu++] = pScene->m_texCoordList[idx-1][1];
						vvert[vv++] = vert.x;
						vvert[vv++] = vert.y;
						vvert[vv++] = vert.z;
						pBrush->m_Points.Append(vert);
						pPoly->m_Indices.Append(indexCount);
					}
				}
				else 
				{
					for(int vertNum = 0, indexCount = 0;		// v/t/n v/t/n v/t/n [v/t/n]
						vertNum < faceSize; 
						vertNum+=3, indexCount++ )
					{
						int idx = pScene->m_faceSetList[i][faceSize-vertNum-3];
						vert.x = pScene->m_vertexList[idx-1][0]; 
						vert.y = pScene->m_vertexList[idx-1][1]; 
						vert.z = pScene->m_vertexList[idx-1][2]; 
						pBrush->m_Points.Append(vert);
						pPoly->m_Indices.Append(indexCount);
					}
				}
			} 
			else
			{
				if( 3 == faceSize  || 4 == faceSize || 12 < faceSize )			// v v v [v]
				{
					for( int vertNum = 0; vertNum < faceSize; vertNum++ )
					{
						int idx = pScene->m_faceSetList[i][vertNum];
						vert.x = pScene->m_vertexList[idx-1][0]; 
						vert.y = pScene->m_vertexList[idx-1][1]; 
						vert.z = pScene->m_vertexList[idx-1][2]; 
						pBrush->m_Points.Append(vert);
						pPoly->m_Indices.Append(vertNum);
					}
				}
				else if( 6 == faceSize  || 8 == faceSize )		// v/t v/t v/t [v/t]
				{
					for(int vertNum = 0, indexCount = 0; 
						vertNum < faceSize; 
						vertNum+=2, indexCount++ )
					{
						int idx = pScene->m_faceSetList[i][vertNum];
						vert.x = pScene->m_vertexList[idx-1][0]; 
						vert.y = pScene->m_vertexList[idx-1][1]; 
						vert.z = pScene->m_vertexList[idx-1][2]; 
						pBrush->m_Points.Append(vert);
						pPoly->m_Indices.Append(indexCount);
					}
				}
				else 
				{
					for(int vertNum = 0, indexCount = 0;		// v/t/n v/t/n v/t/n [v/t/n]
						vertNum < faceSize; 
						vertNum+=3, indexCount++ )
					{
						int idx = pScene->m_faceSetList[i][vertNum];
						vert.x = pScene->m_vertexList[idx-1][0]; 
						vert.y = pScene->m_vertexList[idx-1][1]; 
						vert.z = pScene->m_vertexList[idx-1][2]; 
						pBrush->m_Points.Append(vert);
						pPoly->m_Indices.Append(indexCount);
					}
				}

			}

			// Set NoSnap to TRUE.
			if((pNoSnap = (CBoolProp*)pBrush->m_PropList.GetProp("NoSnap")) && 
				pNoSnap->m_Type == PT_BOOL)
			{
				pNoSnap->m_Value = TRUE;
			}

			// Set the lighting to gouraud
			CStringProp *pStringProp = (CStringProp*)pBrush->m_PropList.GetProp("Lighting");
			if(pStringProp && pStringProp->m_Type == PT_STRING)
			{
				pStringProp->SetString("Gouraud");
			}

			if((pNoSnap = (CBoolProp*)pBrush->m_PropList.GetProp("Subdivide")) && 
				pNoSnap->m_Type == PT_BOOL)
			{
				pNoSnap->m_Value = FALSE;
			}

			pPoly->UpdatePlane();
			
			// talon: 2001 (0x45704d) makes no texture space from the file's
			// coordinates: Build 69's SetUVTextureSpace (UVtoOPQ) isn't there.
			pPoly->SetupBaseTextureSpace();
		}
	}
	no_DestroyNode(pRegion, pTemplateBrush, FALSE);

	return true;
}


/************************************************************************/
BOOL CRegionView::ImportOBJFile(CString sFilename, BOOL bPositionAtCrosshair)
{
	// open the obj file
	// load the ed file into the scene
	// save the scene as an ed file and name it sFilename+".ed"
	// ImportWorldFile(CString sFilename+".ed", BOOL bPositionAtCrosshair)
	// talon: 2001 (0x457092).
	BOOL retVal = FALSE;
	
	CObjInterface objInterface;
	
	FILE* pObjFile = fopen(LPCTSTR(sFilename), "rb");
	if( pObjFile )
	{
		CImportObjFileFormat impObjDlg;
		int isOK = impObjDlg.DoModal();

		OutputDebugString("\ncalling objInterface.ImportObj()");

		if( isOK == IDOK )
		{
			CEditRegion		region;
			CMoFileIO		file;

			BeginWaitCursor( ); // Put the wait cursor on the screen

			if( impObjDlg.m_from3DSMax )
				objInterface.ImportObj(pObjFile,impObjDlg.m_reverse_normals, CObjInterface::F3DSMAX );
			else if( impObjDlg.m_fromMaya )
				objInterface.ImportObj(pObjFile,impObjDlg.m_reverse_normals, CObjInterface::FMAYA );
			else if( impObjDlg.m_fromSoftImage )
				objInterface.ImportObj(pObjFile,impObjDlg.m_reverse_normals, CObjInterface::FSOFT );

			OutputDebugString("\ncalling CreateBrushesFromScene()");
			CreateBrushesFromScene(&region, &objInterface );

			CString filename = sFilename+".ed";
			if( file.Open(filename, "wb") )
			{
				OutputDebugString("\ncalling region.SaveFile(file);");
				region.SaveFile(file, FALSE);
				file.Close();
				OutputDebugString("\ncalling ImportWorldFile");
				ImportWorldFile(filename, bPositionAtCrosshair);
				unlink(LPCTSTR(filename));
				retVal = TRUE;
			}

			EndWaitCursor( ); // End the wait cursor
		}
		fclose(pObjFile);
	}
	return retVal;
}

/************************************************************************/
void CRegionView::OnBindToObject() 
{
	BEGIN_MEMORYEXCEPTION()
	{	
		// See what type of object, if any, is selected in the code view.
		CBaseEditObj	*pObj;
		CClassDlg		dlg;
		
		// Get the document and region
		CRegionDoc *pDoc=GetRegionDoc();
		CEditRegion *pRegion=pDoc->GetRegion();

		CString sTitle;
		sTitle.LoadString(IDS_CHOOSE_OBJECT_TYPE);

		// Enable individual binding if more than one node is selected
		if (pRegion->GetNumSelections() > 1)
		{
			dlg.SetEnableBindIndividually(TRUE);
		}

		dlg.SetProject(GetProject());
		dlg.SetTitle(sTitle);
		
		if( dlg.DoModal() != IDOK )
		{
			return;
		}

		// Check if no class was selected
		if (dlg.GetSelectedClass().IsEmpty())  return;

		PreActionList actionList;

		// Determine if we should bind individual objects to each brush
		if (dlg.GetBindIndividually() && pRegion->GetNumSelections() > 1)
		{
			// An array of nodes to select
			CArray<CWorldNode *, CWorldNode *>	nodesToSelect;

			int i;
			for (i=0; i < pRegion->GetNumSelections(); i++)
			{
				// The current selection
				CWorldNode *pSelectedNode=pRegion->GetSelection(i);

				// Skip null nodes
				if (pSelectedNode->GetType() == Node_Null)
				{
					continue;
				}

				// Add the object
				CWorldNode *pObject=pDoc->AddObject(dlg.GetSelectedClass(), pSelectedNode->GetUpperLeftCornerPos());

				// Bind the object to its parent
				pDoc->BindNode(pObject, pSelectedNode->FindParentContainer(), &actionList);

				// Bind the selected node to the object
				pDoc->BindNode(pSelectedNode, pObject, &actionList);

				// Add the node to the selection array
				nodesToSelect.Add(pObject);
			}

			// Select the nodes
			for (i=0; i < nodesToSelect.GetSize(); i++)
			{
				pDoc->SelectNode(nodesToSelect[i], FALSE);
			}
		}
		else
		{
			// Add the object
			CWorldNode *pObject=pDoc->AddObject(dlg.GetSelectedClass(), GetRegionDoc()->GetRegion()->GetMarker());

			// Set the initial parent node
			CWorldNode *pParentNode=pRegion->GetRootNode();

			// Get the first node that is selected.  This is used to determine the container node
			// to place the binded nodes under.  If the first node is a container node, then use that
			// one.
			if (pRegion->GetNumSelections() > 0)
			{
				// Get the first selected node
				CWorldNode *pSelection=pRegion->GetSelection(0);

				// If it is a NULL node then use this one
				if (pSelection->GetType() == Node_Null)
				{
					pParentNode=pSelection;
				}
				else
				{
					// Search for the parent container node
					CWorldNode *pParentContainer=pSelection->FindParentContainer();
					if (pParentContainer)
					{
						pParentNode=pParentContainer;
					}
				}
			}

			// Bind the object to its parent
			pDoc->BindNode(pObject, pParentNode, &actionList);

			// Bind the selected nodes to the object
			pDoc->BindNodeToSelected(pObject, &actionList);

			// Select the object
			pDoc->SelectNode(pObject, FALSE);
		}	
		// For some reason, doing this modify makes it happen twice..
		//pDoc->Modify(&actionList, FALSE);
	}
	END_MEMORYEXCEPTION()
}






void CRegionView::OnWorldInfo() 
{
	CWorldInfoDlg dlg;

	if(m_pRegion)
	{
		dlg.m_pRegion = m_pRegion;
		dlg.m_InputWorldName = GetDocument()->GetTitle();

		if(dlg.DoModal() == IDOK)
		{
			m_pRegion->m_pInfoString = m_pRegion->m_pStringHolder->AddString(dlg.m_WorldInfoString);
			GetRegionDoc()->Modify();
		}
	}
}

void CRegionView::OnContextMenu(CWnd* pWnd, CPoint point)
{
	//don't bring up the context menu if this view does not have focus currently
	if(GetFocus() != pWnd)
		return;

	char *deviceNames[50];
	int nDeviceNames, i;
	UINT itemID;

	if( m_bDoMenu )
	{
		m_rayMousePoint = ViewDef()->MakeRayFromScreenPoint( GetCurMousePos( ));

		CMenu menu;
		VERIFY(menu.LoadMenu(CG_IDR_POPUP_REGION_VIEW));

		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);

		CWnd* pWndPopupOwner = this;
		while (pWndPopupOwner->GetStyle() & WS_CHILD)
			pWndPopupOwner = pWndPopupOwner->GetParent();

		
		// Fill in the Direct3D options.

		//First get the D3D popup menu
		CMenu *pDirect3dOptions = pPopup->GetSubMenu(9);
		ASSERT( pDirect3dOptions );		// someone changed the menu layout, fix the index

		pDirect3dOptions = pDirect3dOptions->GetSubMenu(7);
		ASSERT( pDirect3dOptions );		// someone changed the menu layout, fix the index

		if(pDirect3dOptions)
		{
			CheckAMenuItem(pDirect3dOptions, ID_USEDEVICE1, m_DeviceNum, 0);
			CheckAMenuItem(pDirect3dOptions, ID_USEDEVICE2, m_DeviceNum, 1);
			CheckAMenuItem(pDirect3dOptions, ID_USEDEVICE3, m_DeviceNum, 2);
			CheckAMenuItem(pDirect3dOptions, ID_USEDEVICE4, m_DeviceNum, 3);
			
			CheckAMenuItem(pDirect3dOptions, ID_RENDERMODE1, m_RenderMode, 0);
			CheckAMenuItem(pDirect3dOptions, ID_RENDERMODE2, m_RenderMode, 1);
			CheckAMenuItem(pDirect3dOptions, ID_RENDERMODE3, m_RenderMode, 2);
			CheckAMenuItem(pDirect3dOptions, ID_RENDERMODE4, m_RenderMode, 3);

			// Set device names.
			nDeviceNames = dm_GetDirectDrawDeviceNames(deviceNames, 4);

			for(i=0; i < nDeviceNames; i++)
			{
				itemID = pDirect3dOptions->GetMenuItemID(i);
				pDirect3dOptions->ModifyMenu(i, MF_BYPOSITION | MF_STRING, itemID, deviceNames[i]);
				free(deviceNames[i]);
			}

			// Disable ones they can't choose.
			for( ; i < 4; i++)
				pDirect3dOptions->RemoveMenu(nDeviceNames, MF_BYPOSITION);
		}		


		// Enable stuff.
		CheckAMenuItem( pPopup, ID_POPUP_SHADEMODE_WIREFRAMEPOLIES, GetShadeMode(), SM_WIREFRAME );
		CheckAMenuItem( pPopup, ID_POPUP_SHADEMODE_FLATPOLIES, GetShadeMode(), SM_FLAT );
		CheckAMenuItem( pPopup, ID_POPUP_SHADEMODE_TEXTUREDPOLIES, GetShadeMode(), SM_TEXTURED );
		CheckAMenuItem( pPopup, ID_POPUP_SHADEMODE_LIGHTMAPSONLY, GetShadeMode(), SM_LIGHTMAPSONLY);

		CheckAMenuItem( pPopup, ID_POPUP_VIEWMODE_PERSPECTIVEVIEW, GetViewMode(), VM_PERSPECTIVE );
		CheckAMenuItem( pPopup, ID_POPUP_VIEWMODE_TOPVIEW, GetViewMode(), VM_TOP );
		CheckAMenuItem( pPopup, ID_POPUP_VIEWMODE_BOTTOMVIEW, GetViewMode(), VM_BOTTOM );
		CheckAMenuItem( pPopup, ID_POPUP_VIEWMODE_FRONTVIEW, GetViewMode(), VM_FRONT );
		CheckAMenuItem( pPopup, ID_POPUP_VIEWMODE_BACKVIEW, GetViewMode(), VM_BACK );
		CheckAMenuItem( pPopup, ID_POPUP_VIEWMODE_LEFTVIEW, GetViewMode(), VM_LEFT );
		CheckAMenuItem( pPopup, ID_POPUP_VIEWMODE_RIGHTVIEW, GetViewMode(), VM_RIGHT );

		CheckAMenuItem( pPopup, ID_POPUP_OPTIONS_BACKFACINGPOLIES, IsSelectBackfaces(), TRUE );
		CheckAMenuItem( pPopup, ID_POPUP_OPTIONS_SHOWWIREFRAME, IsShowWireframe(), TRUE );
		CheckAMenuItem( pPopup, ID_POPUP_OPTIONS_SHOWNORMALS, IsShowNormals(), TRUE );
		CheckAMenuItem( pPopup, ID_POPUP_OPTIONS_SHOWOBJECTS, IsShowObjects(), TRUE );
		CheckAMenuItem( pPopup, ID_POPUP_OPTIONS_SHOWEDITGRID, IsShowGrid(), TRUE );
 		CheckAMenuItem( pPopup, ID_POPUP_OPTIONS_SHOWMARKER, IsShowMarker(), TRUE );

		
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
			pWndPopupOwner);
	}
}


BOOL CRegionView::PreTranslateMessage(MSG* pMsg)
{
	// CG: This block was added by the Pop-up Menu component
	{
		// Shift+F10: show pop-up menu.
		if ((((pMsg->message == WM_KEYDOWN || pMsg->message == WM_SYSKEYDOWN) && // If we hit a key and
			(pMsg->wParam == VK_F10) && (GetKeyState(VK_SHIFT) & ~1)) != 0) ||	// it's Shift+F10 OR
			(pMsg->message == WM_CONTEXTMENU))									// Natural keyboard key
		{
			CRect rect;
			GetClientRect(rect);
			ClientToScreen(rect);

			CPoint point = rect.TopLeft();
			point.Offset(5, 5);
			OnContextMenu(NULL, point);

			return TRUE;
		}
	}

	return CView::PreTranslateMessage(pMsg);
}




//////////////////////////////////////////////////////
// MENU HANDLERS

void CRegionView::OnShadeWireframe() 
{
	SetShadeMode( SM_WIREFRAME );
	DrawRect();
}


void CRegionView::OnShadeTextured() 
{
	SetShadeMode( SM_TEXTURED );
	DrawRect();
}


void CRegionView::OnShadeFlat() 
{
	SetShadeMode( SM_FLAT );
	DrawRect();
}



void CRegionView::OnShadeLightmapsOnly() 
{
	SetShadeMode( SM_LIGHTMAPSONLY );
	DrawRect();
}

// talon: 2001 (0x457b10, 0x457b18).
void CRegionView::OnWalkthruMovement()
{
	SetWalkThroughMovement(TRUE);
}

void CRegionView::OnOrbitMovement()
{
	SetWalkThroughMovement(FALSE);
}



void CRegionView::OnBackView() 
{
	SetViewMode( VM_BACK );
	DrawRect();
}



void CRegionView::OnBottomView() 
{
	SetViewMode( VM_BOTTOM );
	DrawRect();
}



void CRegionView::OnFrontView() 
{
	SetViewMode( VM_FRONT );
	DrawRect();
}



void CRegionView::OnLeftView() 
{
	SetViewMode( VM_LEFT );
	DrawRect();
}



void CRegionView::OnPerspectiveView() 
{
	SetViewMode( VM_PERSPECTIVE );
	DrawRect();
}



void CRegionView::OnRightView() 
{
	SetViewMode( VM_RIGHT );
	DrawRect();
}



void CRegionView::OnTopView() 
{
	SetViewMode( VM_TOP );
	DrawRect();
}



void CRegionView::OnShowObjects()
{
	SetShowObjects(!IsShowObjects());
	DrawRect();
}


void CRegionView::OnBackfacingPolies() 
{
	SetSelectBackfaces( !IsSelectBackfaces() );
	
	UpdateImmediateSelection();
	DrawRect();
}


void CRegionView::OnShowWireframe() 
{
	SetShowWireframe(!IsShowWireframe());
	DrawRect();
}


void CRegionView::OnShowNormals() 
{
	SetShowNormals( !IsShowNormals() );
	DrawRect();
}


void CRegionView::OnRemoveExtraEdges() 
{
	BEGIN_MEMORYEXCEPTION()
	{
		CReal normalThresh = 0.05f;
		CReal distThresh = 0.0001f;
		DWORD i;
		LPOS pos;

		// MIKE 12/29/97 - Switched from CountUndoTextures to just TRUE.
		GetRegionDoc()->Modify();

		for(pos=m_pRegion->m_Brushes; pos; )
		{
			CEditBrush* pBrush = m_pRegion->m_Brushes.GetNext(pos);
			pBrush->RemoveExtraEdges( normalThresh, distThresh );
			m_pRegion->UpdateBrushGeometry(pBrush);
		}

		DrawRect();
		SetFocus();
	}
	END_MEMORYEXCEPTION()
}


void CRegionView::OnResetAllTextureCoords() 
{
}


// Returns a list of polies from either the immediate poly or from the selected brushes.
void CRegionView::GetSelectedPolies(CMoArray<CEditPoly*> &polies)
{
	CEditRegion *pRegion;
	CEditBrush *pBrush;
	DWORD i, j;


	pRegion = GetRegion();

	polies.SetCacheSize(40);

	if(GetEditMode() == GEOMETRY_EDITMODE)
	{	
		if(TaggedPolies() == 1)
		{
			// Just add the immediate poly.
			if(IPoly().IsValid())
				polies.Append(IPoly()());
		}
		else
		{
			// Add all the tagged polies (exclude the immediate poly).
			for(i=1; i < TaggedPolies(); i++)
			{
				CPolyRef &ref = TaggedPolies()[i];

				if(polies.FindElement(ref()) == BAD_INDEX)
				{
					polies.Append(ref());
				}
			}
		}
	}
	else
	{
		for(i=0; i < pRegion->m_Selections; i++)
		{
			if(pRegion->m_Selections[i]->GetType() == Node_Brush)
			{
				pBrush = (CEditBrush*)pRegion->m_Selections[i];

				for(j=0; j < pBrush->m_Polies; j++)
				{
					polies.Append(pBrush->m_Polies[j]);
				}
			}
		}
	}
}


// talon: 2001 (0x457e32): the first selected poly's texture mapping into
// the 2001 dialog, which applies the changes and makes the undo itself.
void CRegionView::OnMapTextureCoords() 
{
	CMapTextureCoordsDlg dlg(this);
	CMoArray<CEditPoly*> polies;
	CEditPoly *pPoly;
	CTexture *pTexture;
	DVector normP, normQ;

	polies.SetCacheSize(200);
	GetSelectedPolies(polies);
	if(polies.GetSize() > 0)
	{
		pPoly = polies[0];
		if(pPoly->m_pTextureFile)
		{
			pTexture = dib_GetDibTexture(pPoly->m_pTextureFile);
			if(pTexture && pTexture->m_pDib)
			{
				// Setup parameters for the dialog.
				dlg.m_TextureWidth = pTexture->m_pDib->GetWidth() >> pTexture->m_UIMipmapOffset;
				dlg.m_TextureHeight = pTexture->m_pDib->GetHeight() >> pTexture->m_UIMipmapOffset;
				
				//get the normal versions of the vectors
				normP = pPoly->P;
				normP.Norm();
				normQ = pPoly->Q;
				normQ.Norm();

				//find the U offset ranging from [0...TexWidth)
				dlg.m_UOffset = normP.Dot(pPoly->O - pPoly->Pt(0));
				dlg.m_UOffset = (float)fmod(dlg.m_UOffset, (float)dlg.m_TextureWidth);
				if (fabs(dlg.m_UOffset) < 0.001)
					dlg.m_UOffset = 0.0f;

				//find the V offset ranging from [0...TexHeight)
				dlg.m_VOffset = normQ.Dot(pPoly->O - pPoly->Pt(0));
				dlg.m_VOffset = (float)fmod(dlg.m_VOffset, (float)dlg.m_TextureHeight);
				if (fabs(dlg.m_VOffset) < 0.001)
					dlg.m_VOffset = 0.0f;

				//find out the scale for this polygon
				dlg.m_UScale = (1.0f / pPoly->P.Mag()) * (float)dlg.m_TextureWidth;
				dlg.m_VScale = (1.0f / pPoly->Q.Mag()) * (float)dlg.m_TextureHeight;

				dlg.m_RefRotation = 0.0f;
				dlg.m_Rotation = 0.0f;

				dlg.m_bStick = pPoly->m_Unknown_70;
				dlg.m_bXZOnly = (pPoly->m_SurfaceFlags >> 16) & 1;

				if(dlg.DoModal() == IDOK)
					dlg.ApplyChanges();
			}
		}
	}
}



void CRegionView::OnRotateSelection() 
{
	CRotateSelection dlg;
	CReal rRotation;
	CEditRegion *pRegion;
	CMatrix matRot;
	CVector vRotationVector, vRotationCenter;
	DWORD i;
	PreActionList actionList;
	CLinkedList<CEditBrush*> brushList;

	pRegion = GetRegion( );
	ASSERT( pRegion );

	if( !pRegion->m_Selections.GetSize( ))
		return;

	// Get input...
	if( dlg.DoModal( ) == IDOK )
	{
		BeginWaitCursor( );

		rRotation = dlg.m_rRotation;

		// Make sure input is -2PI <= x <= 2PI...
		if( rRotation >= 0 )
			rRotation = (float)fmod( rRotation * MATH_PI / 180.0f, 2.0f * MATH_PI );
		else
			rRotation = -(float)fmod( -rRotation * MATH_PI / 180.0f, 2.0f * MATH_PI );

		// Get the mouse point at rmb click...
		vRotationVector = m_pViewDef->m_Nav.Forward( );
		vRotationVector.Norm( );
		vRotationCenter = GetRegion()->m_vMarker;

		//now setup the rotation matrix. Note the use of the negative angle, this is
		//to cause the rotation to be clockwise which the artists requested.
		gr_SetupRotationAroundVector(&matRot, vRotationVector, -rRotation);

		// Setup undos...
		for(i=0; i < pRegion->m_Selections; i++)
			actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, pRegion->m_Selections[i]));
		
		GetRegionDoc()->Modify(&actionList, TRUE);

		// Loop over all selected nodes...
		for( i = 0; i < pRegion->m_Selections.GetSize( ); i++ )
		{
			pRegion->m_Selections[i]->Rotate( matRot, vRotationCenter );
			
			if(pRegion->m_Selections[i]->GetType() == Node_Brush)
			{
				brushList.AddTail(pRegion->m_Selections[i]->AsBrush());
			}
		}

		// Update the geometry stuff...
		pRegion->UpdatePlanes(&brushList);
		pRegion->CleanupGeometry(&brushList);
		GetRegionDoc()->UpdateSelectionBox();

		EndWaitCursor( );
	}
}



void CRegionView::OnJoinTaggedVertices() 
{
	DWORD		nVert;
	CEditBrush	*pBrush;
	GenList<CVertRef> &vertList = TaggedVerts();

	if( vertList.GenGetSize() == 0 )
		return;

	// Make sure all the tagged vertices are on the same brush
	GenListPos pFinger = vertList.GenBegin();

	nVert = vertList.GenGetAt(pFinger).m_iVert;
	pBrush = vertList.GenGetNext(pFinger).m_pBrush;
	while (vertList.GenIsValid(pFinger))
	{
		if (vertList.GenGetNext(pFinger).m_pBrush != pBrush)
			return;
	}

	// Go back and change the vertices to the first in the list
	pFinger = vertList.GenBegin();
	vertList.GenGetNext(pFinger);
	while (vertList.GenIsValid(pFinger))
	{
		DWORD nJoinVert = vertList.GenGetNext(pFinger).m_iVert;
		pBrush->ChangeVertexReferences( nJoinVert, nVert );
	}

	// Remove the now un-used vertices in the brush
	pBrush->RemoveUnusedVerts();
	
	// Clear out the vertex list
	vertList.GenRemoveAll();

	GetDocument()->UpdateAllViews( this );
}


void CRegionView::OnObjectHeight()
{
	CMoArray<CBaseEditObj*>	objects;
	CBaseEditObj			*pObj;
	CVectorProp				*pProp;

	DWORD					i;
	CEditRegion				*pRegion;
	CWorldNode				*pNode;
	CStringDlg				dlg;
	float					fHeight;
	
	CEditRay				ray;
	CPolyRef				ref;
	CReal					dist;
	PreActionList actionList;



	// First build a list of selected objects.
	pRegion = GetRegion();
	for(i=0; i < pRegion->m_Selections; i++)
	{
		pNode = pRegion->m_Selections[i];

		if(pNode->GetType() == Node_Object)
		{
			objects.Append(pNode->AsObject());
		}
	}

	// If there's more than one, prompt for the height above the ground.
	if(objects > 0)
	{
		dlg.m_bAllowNumbers = TRUE;
		
		if(dlg.DoModal(IDS_OBJECT_HEIGHT, IDS_ENTER_OBJECT_HEIGHT) == IDOK)
		{
			fHeight = (float)atof((LPCTSTR)dlg.m_EnteredText);
			
			// Setup an undo.
			for(i=0; i < objects; i++)
			{
				actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, objects[i]));
			}

			GetRegionDoc()->Modify(&actionList, TRUE);

			for(i=0; i < objects; i++)
			{
				pObj = objects[i];

				if(pProp = (CVectorProp*)pObj->m_PropList.GetProp("m_Pos"))
				{
					ray.m_Pos = pProp->m_Vector;
					ray.m_Dir.Init(0.0f, -1.0f, 0.0f);

					ref = CastRayAtPolies(ray, &dist);
					if(ref.IsValid())
					{
						pProp->m_Vector.y = (ray.m_Pos.y + ray.m_Dir.y*dist) + fHeight;
					}
				}
			}

			GetRegionDoc()->RedrawAllViews();
		}
	}
}


void CRegionView::OnProcessWorld() 
{
	GetRegionDoc()->StartPreProcessor();	
}

void CRegionView::OnFileSave() 
{
	GetRegionDoc()->OnSaveDocument( NULL );
}


void CRegionView::OnUpdateFileSaveAs( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( FALSE );
}

// talon: 2001 (0x458c4d): updates every poly's plane.
void CRegionView::OnUpdatePlanes()
{
	LPOS pos;
	CEditBrush *pBrush;
	DWORD i;

	for(pos=m_pRegion->m_Brushes; pos; )
	{
		pBrush = m_pRegion->m_Brushes.GetNext(pos);

		for(i=0; i < pBrush->m_Polies; i++)
			pBrush->m_Polies[i]->UpdatePlane();
	}
}

void CRegionView::OnCleanupGeometry() 
{
	// MIKE 12/29/97 - removed this call.. full undos puff and CleanupGeometry is a hack anyway.
	// GetRegionDoc()->Modify((( CMainFrame * )AfxGetMainWnd( ))->CountUndoGeometry( ));
	
	m_pRegion->CleanupGeometry();
}


void CRegionView::OnShowEditGrid() 
{
	SetShowGrid(!IsShowGrid());
	DrawRect( );
}

void CRegionView::OnShowMarker() 
{
	SetShowMarker(!IsShowMarker());
	DrawRect( );
}
 
void CRegionView::On4ViewConfiguration() 
{
	CRegionFrame		*pFrame = (CRegionFrame*)GetParent()->GetParent();
	CSplitterWnd		*pSplitter = pFrame->GetSplitter();
	CRegionView			*pRegionView;
	CRect				rect;

	
	pFrame->GetClientRect( &rect );

	pSplitter->SetRowInfo( 0, rect.Height()/2, 10 );
	pSplitter->SetColumnInfo( 0, rect.Width()/2, 10 );
	pSplitter->RecalcLayout();

	// Perspective view.
	pRegionView = (CRegionView*)pFrame->GetSplitter()->GetPane(0,0);
	pRegionView->SetViewMode( VM_PERSPECTIVE );

	// Top view.
	pRegionView = (CRegionView*)pFrame->GetSplitter()->GetPane(0,1);
	pRegionView->SetViewMode( VM_TOP );

	// Front view.
	pRegionView = (CRegionView*)pFrame->GetSplitter()->GetPane(1,1);
	pRegionView->SetViewMode( VM_FRONT );

	// Left view.
	pRegionView = (CRegionView*)pFrame->GetSplitter()->GetPane(1,0);
	pRegionView->SetViewMode( VM_LEFT );


	GetRegionDoc()->RedrawAllViews();
}


void CRegionView::OnUpdateSingleBrushOperation( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( (GetRegion()->NumBrushSelections() == 1) );
}

void CRegionView::OnUpdateShrinkGridSpacing( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( GetGridSpacing() > 1 );
}

void CRegionView::OnUpdateExpandGridSpacing( CCmdUI *pCmdUI )
{
	// NOTE: If the grid needs to be spaced larger, it needs to be saved using
	// a DWORD in the .lyt file.
	pCmdUI->Enable( GetGridSpacing() < (1<<15) );
}

// talon: 2001 (0x4596cb, the name ours).
void CRegionView::OnUpdateShrinkPixelScale( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( m_nPixelScale > 1 );
}

// talon: 2001 (0x4596e7, the name ours).
void CRegionView::OnUpdateGrowPixelScale( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( m_nPixelScale < 8 );
}

void CRegionView::OnShrinkGridSpacing()
{
	if( GetGridSpacing() <= 1 )
		return;
	
	SetGridSpacing( GetGridSpacing() >> 1 );
	DrawRect();
	UpdateStatusBar( );
}

void CRegionView::OnExpandGridSpacing()
{
	SetGridSpacing( GetGridSpacing() << 1 );
	DrawRect();
	UpdateStatusBar( );
}

// talon: 2001 (0x45976e, the name ours): halves the size of a drawn pixel.
void CRegionView::OnShrinkPixelScale()
{
	CRect rect;

	if( m_nPixelScale > 1 )
	{
		GetClientRect( &rect );

		m_nPixelScale /= 2;
		m_fPixelScale = (float)m_nPixelScale;
		m_ViewDefInfo.m_fPixelScaleX = m_ViewDefInfo.m_fPixelScaleY = 1.0f / m_fPixelScale;

		SetupNewSize( rect.Width(), rect.Height() );
		DrawRect();
	}
}

// talon: 2001 (0x4597f5, the name ours): doubles the size of a drawn pixel.
void CRegionView::OnGrowPixelScale()
{
	CRect rect;

	if( m_nPixelScale < 8 )
	{
		GetClientRect( &rect );

		m_nPixelScale *= 2;
		m_fPixelScale = (float)m_nPixelScale;
		m_ViewDefInfo.m_fPixelScaleX = m_ViewDefInfo.m_fPixelScaleY = 1.0f / m_fPixelScale;

		SetupNewSize( rect.Width(), rect.Height() );
		DrawRect();
	}
}


void CRegionView::OnUpdateApplyColor( CCmdUI *pCmdUI )
{
	pCmdUI->Enable(GetRegion()->m_Selections.GetSize() > 0 && GetEditMode() == BRUSH_EDITMODE);
}


void CRegionView::OnApplyColor()
{
	COLORREF color;
	DWORD i;
	CEditBrush *pBrush;
	PreActionList actionList;
	CWorldNode *pNode;

	
	// Build an undo list.
	for(i=0; i < GetRegion()->m_Selections; i++)
	{
		pNode = GetRegion()->m_Selections[i];
		if(pNode->GetType() == Node_Brush)
		{
			actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, pNode));
		}
	}

	GetRegionDoc()->Modify(&actionList, TRUE);

	// Do the mods.
	color = GetColorSelectDlg()->GetCurColor();
	for( i=0; i < GetRegion()->m_Selections; i++ )
	{
		if(GetRegion()->m_Selections[i]->GetType() == Node_Brush)
		{
			pBrush = GetRegion()->m_Selections[i]->AsBrush();
			
			pBrush->r = GetRValue( color );
			pBrush->g = GetGValue( color );
			pBrush->b = GetBValue( color );
		}
	}

	GetRegionDoc()->RedrawAllViews();
}


// talon: 2001's is empty (folded with the other empty handlers at 0x44ef03).
void CRegionView::OnUpdateApplyTexture( CCmdUI *pCmdUI )
{
}

void CRegionView::OnApplyTexture()
{
	DWORD i, k;
	char *pCurTexName;
	CEditBrush *pBrush;
	CWorldNode *pNode;
	
	PreActionList actionList;
	CPreAction *pAction;
	GPOS pos;
	
	if( (GetEditMode() == BRUSH_EDITMODE) ||
		(GetEditMode() == OBJECT_EDITMODE))
	{
		// Put the current texture on the selected brushes.
		if( GetCurrentTextureName(pCurTexName, TRUE) )
		{
			// Build the list of the brushes we're going to modify..
			for(i=0; i < GetRegion()->m_Selections; i++)
			{
				pNode = GetRegion()->m_Selections[i];
				if(pNode->GetType() == Node_Brush)
				{
					actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, pNode));
				}
			}
			
			// Store the changes..
			GetRegionDoc()->Modify(&actionList, FALSE);
			
			// Make the changes.
			for(pos=actionList; pos; )
			{
				CWorldNode* pNode = actionList.GetNext(pos)->m_pNode;

				if(pNode->GetType() == Node_Brush)
				{
					pBrush = (CEditBrush*)pNode;
			
					for( k=0; k < pBrush->m_Polies; k++ )
					{
						pBrush->m_Polies[k]->GetTexture(GetCurrTexture()).m_pTextureName = pCurTexName;
						pBrush->m_Polies[k]->GetTexture(GetCurrTexture()).UpdateTextureID();
					}
				}
			}
		}
	}
	else if(GetEditMode() == GEOMETRY_EDITMODE)
	{
		// Put the selected texture onto the current poly.
		if( IPoly().IsValid() && GetCurrentTextureName(pCurTexName, TRUE) )
		{
			actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, IPoly().m_pBrush));
			GetRegionDoc()->Modify(&actionList, TRUE);
			
			IPoly()()->GetTexture(GetCurrTexture()).m_pTextureName = pCurTexName;
			IPoly()()->GetTexture(GetCurrTexture()).UpdateTextureID();
		}
	}

	GDeleteAndRemoveElements(actionList);
	GetRegionDoc()->RedrawAllViews();
}




void CRegionView::OnObjectProperties() 
{
	GetRegionDoc()->SetupPropertiesDlg(TRUE);
}


void CRegionView::OnUpdateStatus( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( TRUE );
}

//------------------------------------------------------------------------------------
//
//  CRegionView::UpdateStatusBar()
//
//  Purpose:	Updates the text in the status bar that pertain to this view.
//
//------------------------------------------------------------------------------------
void CRegionView::UpdateStatusBar()
{
	CMainFrame *pFrame;
	CRect rRect;
	CPoint ptMousePos;
			
	if( !m_bInFocus )
		return;

	GetCursorPos( &ptMousePos );
	GetOwner()->GetWindowRect( &rRect );

	pFrame = ( CMainFrame * )AfxGetMainWnd();
	ASSERT( pFrame );
	if( pFrame )
	{
		CString sViewMode, sGrid, sCamera, sCursor, sTemp;
		int nViewMode;
		CVector vCamOrient, vCamPos, vCursorPos;
		CEditRay rayMousePoint;

		sGrid.LoadString( IDS_GRID );
		sTemp.Format( "%d", GetGridSpacing( ));
		sGrid += sTemp;
//		pStatusBar->SetPaneText( pStatusBar->CommandToIndex( ID_INDICATOR_GRID ), sGrid );
		pFrame->UpdateStatusText(ID_INDICATOR_GRID, sGrid.GetBuffer(0));
		
		// Only update the status bar when this view has focus...
		if( rRect.PtInRect( ptMousePos ))
		{
			rayMousePoint = ViewDef()->MakeRayFromScreenPoint( GetCurMousePos( ));
			vCamOrient = Nav( ).Forward( );
			vCamPos = Nav( ).Pos( );
			nViewMode = GetViewMode( );

			switch( nViewMode )
			{
				case VM_TOP:
					sViewMode.LoadString( IDS_VIEW_TOP );
					sCamera.Format( "@(%1.0f,n/a,%1.0f)", vCamPos.x, vCamPos.z );
					vCursorPos.Init( rayMousePoint.m_Pos.x, 0.0, rayMousePoint.m_Pos.z );
					sCursor.Format( "@(%1.0f,n/a,%1.0f)", vCursorPos.x, vCursorPos.z );
					break;
				case VM_BOTTOM:
					sViewMode.LoadString( IDS_VIEW_BOTTOM );
					sCamera.Format( "@(%1.0f,n/a,%1.0f)", vCamPos.x, vCamPos.z );
					vCursorPos.Init( rayMousePoint.m_Pos.x, 0.0, rayMousePoint.m_Pos.z );
					sCursor.Format( "@(%1.0f,n/a,%1.0f)", vCursorPos.x, vCursorPos.z );
					break;
				case VM_LEFT:
					sViewMode.LoadString( IDS_VIEW_LEFT );
					sCamera.Format( "@(n/a,%1.0f,%1.0f)", vCamPos.y, vCamPos.z );
					vCursorPos.Init( 0.0, rayMousePoint.m_Pos.y, rayMousePoint.m_Pos.z );
					sCursor.Format( "@(n/a,%1.0f,%1.0f)", vCursorPos.y, vCursorPos.z );
					break;
				case VM_RIGHT:
					sViewMode.LoadString( IDS_VIEW_RIGHT );
					sCamera.Format( "@(n/a,%1.0f,%1.0f)", vCamPos.y, vCamPos.z );
					vCursorPos.Init( 0.0, rayMousePoint.m_Pos.y, rayMousePoint.m_Pos.z );
					sCursor.Format( "@(n/a,%1.0f,%1.0f)", vCursorPos.y, vCursorPos.z );
					break;
				case VM_FRONT:
					sViewMode.LoadString( IDS_VIEW_FRONT );
					sCamera.Format( "@(%1.0f,%1.0f,n/a)", vCamPos.x, vCamPos.y );
					vCursorPos.Init( rayMousePoint.m_Pos.x, rayMousePoint.m_Pos.y, 0.0 );
					sCursor.Format( "@(%1.0f,%1.0f,n/a)", vCursorPos.x, vCursorPos.y );
					break;
				case VM_BACK:
					sViewMode.LoadString( IDS_VIEW_BACK );
					sCamera.Format( "@(%1.0f,%1.0f,n/a)", vCamPos.x, vCamPos.y );
					vCursorPos.Init( rayMousePoint.m_Pos.x, rayMousePoint.m_Pos.y, 0.0 );
					sCursor.Format( "@(%1.0f,%1.0f,n/a)", vCursorPos.x, vCursorPos.y );
					break;
				case VM_PERSPECTIVE:
				{
					CVector vDir;

					sViewMode.LoadString( IDS_VIEW_PERSP );
					sCamera.Format( "(%#3.1f,%#3.1f,%#3.1f)@(%1.0f,%1.0f,%1.0f)", 
						vCamOrient.x, vCamOrient.y, vCamOrient.z, 
						vCamPos.x, vCamPos.y, vCamPos.z );

					// Get the position and orientation the user selected with mouse...
					vCursorPos = rayMousePoint.m_Pos;
					vDir = rayMousePoint.m_Dir;
					vDir.Norm( );

					// Calculate a position along ray cast by mouse...
					vCursorPos = vCursorPos + vDir * 100;

					sCursor.Format( "@(%1.0f,%1.0f,%1.0f)", 
						vCursorPos.x, vCursorPos.y, vCursorPos.z );
					break;
				}
			}

			pFrame->UpdateStatusText(ID_INDICATOR_VIEWTYPE, sViewMode.GetBuffer(0));
			pFrame->UpdateStatusText(ID_INDICATOR_CAMERA, sCamera.GetBuffer(0));
			pFrame->UpdateStatusText(ID_INDICATOR_CURSOR, sCursor.GetBuffer(0));
		}
	}
}

void CRegionView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) 
{
	if( bActivate )
	{
		CRegionDoc *pDoc;
		pDoc = GetRegionDoc( );
		if( pDoc != NULL && !pDoc->m_WorldName.IsEmpty( ))
		{
			GetNodeView()->Init(pDoc);
		}
	}
	
	CView::OnActivateView(bActivate, pActivateView, pDeactiveView);	
}

void CRegionView::OnActivateFrame(UINT nState, CFrameWnd* pFrameWnd)
{
	if (!nState)
		m_cTrackerMgr.Cancel();

	CView::OnActivateFrame(nState, pFrameWnd);
}

void CRegionView::OnUseDevice1()
{
	RestartRender(FALSE, -1, 0, -1, TRUE);
}


void CRegionView::OnUseDevice2()
{
	RestartRender(FALSE, -1, 1, -1, TRUE);
}


void CRegionView::OnUseDevice3()
{
	RestartRender(FALSE, -1, 2, -1, TRUE);
}


void CRegionView::OnUseDevice4()
{
	RestartRender(FALSE, -1, 3, -1, TRUE);
}


void CRegionView::OnRenderMode1()
{
	RestartRender(FALSE, -1, -1, 0, TRUE);
}


void CRegionView::OnRenderMode2()
{
	RestartRender(FALSE, -1, -1, 1, TRUE);
}


void CRegionView::OnRenderMode3()
{
	RestartRender(FALSE, -1, -1, 2, TRUE);
}


void CRegionView::OnRenderMode4()
{
	RestartRender(FALSE, -1, -1, 3, TRUE);
}

// talon: 2001 (0x459ff0): toggles between Direct3D and the software renderer.
void CRegionView::OnUseDirect3D()
{
	RestartRender(FALSE, m_DrawType == 0, -1, -1, TRUE);
}


//
//  This is used for showing brush names when the mouse pointer is held over them
//








void CRegionView::ReplaceTextures(bool bSelectedOnly)
{
	LPOS lPos;
	DWORD i;
	CEditBrush *pBrush;
	CEditPoly *pPoly;
	int nOccurences;
	char szTextName[MAX_PATH], *pSrcTextureName;
	PreActionList actionList;
	CEditRegion *pRegion;
	CMoArray<CEditPoly *> cPolyList;

	if( !IPoly().IsValid())
		return;

	GetProjectBar()->GetCurrentTextureName(szTextName);
	pSrcTextureName = IPoly()()->GetTexture(GetCurrTexture()).m_pTextureName;
	pRegion = GetRegion();
	
	nOccurences = 0;	
	for(lPos=pRegion->m_Brushes; lPos; )
	{
		pBrush = pRegion->m_Brushes.GetNext(lPos);
	
		//see if we want to skip this one because it isn't selected
		if(bSelectedOnly && !pBrush->IsFlagSet(NODEFLAG_SELECTED))
			continue;
	
		for(i=0; i < pBrush->m_Polies; i++)
		{
			pPoly = pBrush->m_Polies[i];

			const char* pszTextureName = pPoly->GetTexture(GetCurrTexture()).m_pTextureName;

			if(pszTextureName)
			{
				if(CHelpers::UpperStrcmp(pszTextureName, pSrcTextureName))
				{
					++nOccurences;
					// Add an undo.
					AddToActionListIfNew(&actionList, new CPreAction(ACTION_MODIFYNODE, pBrush), TRUE);
					// Add it to the modify list
					cPolyList.Add(pPoly);
				}
			}
		}
	}

	// Save the undo list.
	GetRegionDoc()->Modify(&actionList, TRUE);

	// Actually modify the polies
	for(i = 0; i < cPolyList.GetSize(); i++)
	{
		pPoly = cPolyList[i];
		const char* pszTextureName = pPoly->GetTexture(GetCurrTexture()).m_pTextureName;

		if(pszTextureName)
		{
			if(CHelpers::UpperStrcmp(pszTextureName, pSrcTextureName))
			{
				pPoly->GetTexture(GetCurrTexture()).m_pTextureName = pRegion->m_pStringHolder->AddString(szTextName);
				pPoly->GetTexture(GetCurrTexture()).UpdateTextureID();
			}
		}
	}

	GetRegionDoc()->RedrawAllViews();
}

void CRegionView::OnReplaceTextures()
{
	ReplaceTextures(false);
}




void CRegionView::OnSelectTexture()
{
	//(YF 11-29-00)
	CEditPoly* pPoly = NULL;	

	//see if we are in geometry mode
	if( GetEditMode() == GEOMETRY_EDITMODE )		
	{	
		//we are in geometry mode. See if we are pointing at a face currently
		if( !IPoly( ).IsValid( ))

		{	
			// Not pointing at a face so select texture from first selected poly
			CMoArray<CEditPoly*> polies;
			polies.SetCacheSize(50);
			GetSelectedPolies( polies );
			if( polies > 0 )
			{
				pPoly = polies[ 0 ];
			}
		}
		else
		{
			//we are pointing at a face, so lets get the texture from that
			pPoly = IPoly()();
		}
	}
	else
	{
		//we are in brush editing mode, and should therefore 
		CEditRegion *pRegion = GetRegion();
		for( uint32 nCurrNode = 0; nCurrNode < pRegion->GetNumSelections(); ++nCurrNode )
		{
			CWorldNode* pNode = pRegion->GetSelection( nCurrNode );
			if( pNode->GetType() == Node_Brush )
			{
				CEditBrush* pBrush = pNode->AsBrush();
				ASSERT( pBrush );

				for( uint32 nCurrPoly = 0; nCurrPoly < pBrush->m_Polies; ++nCurrPoly )
				{
					//get this current polygon
					pPoly = pBrush->m_Polies[ nCurrPoly ];

					//see if this polygon has a valid texture
					if( pPoly && pPoly->GetTexture(GetCurrTexture()).m_pTextureName && 
						(strlen( pPoly->GetTexture(GetCurrTexture()).m_pTextureName ) > 0) && 
						stricmp( pPoly->GetTexture(GetCurrTexture()).m_pTextureName, "Default" ) != 0 )
					{
						// polygon with a texture found, so break out of the 2 for loops.
						nCurrNode = pRegion->GetNumSelections();	// this breaks out of the i loop.
						break;										// this breaks out of the j loop.
					}
				}
			}
		}
	}

	//if we have found a polygon, grab its texture
	if( pPoly )
	{	
		//actually select the texture
		GetProjectBar()->SetCurTextureSel( pPoly->GetTexture(GetCurrTexture()).m_pTextureFile );	
		GetProjectBar()->FindCurTextureSel();

		//set up the main frame status bar
		CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
		pFrame->UpdateStatusText( ID_INDICATOR_TEXTURE, pPoly->GetTexture(GetCurrTexture()).m_pTextureName );
		GetTextureDlg()->RenderLargeImage();
	}	

	return;
}




void CRegionView::OnSelectBrushColor()
{
	CEditPoly* pPoly = NULL;	

	//see if we are in geometry mode
	if( GetEditMode() == GEOMETRY_EDITMODE )		
	{	
		//we are in geometry mode. See if we are pointing at a face currently
		if( !IPoly( ).IsValid( ))

		{	
			// Not pointing at a face so select color from first selected poly
			CMoArray<CEditPoly*> polies;
			polies.SetCacheSize(50);
			GetSelectedPolies( polies );
			if( polies > 0 )
			{
				pPoly = polies[ 0 ];
			}
		}
		else
		{
			//we are pointing at a face, so lets get the color from that
			pPoly = IPoly()();
		}
	}
	else
	{
		//we are in brush editing mode, and should therefore 
		CEditRegion *pRegion = GetRegion();
		for( uint32 nCurrNode = 0; nCurrNode < pRegion->GetNumSelections(); ++nCurrNode )
		{
			CWorldNode* pNode = pRegion->GetSelection( nCurrNode );
			if( pNode->GetType() == Node_Brush )
			{
				CEditBrush* pBrush = pNode->AsBrush();
				ASSERT( pBrush );

				//get this current polygon
				if(pBrush->m_Polies.GetSize() > 0)
				{
					pPoly = pBrush->m_Polies[ 0 ];
				}
			}
		}
	}

	//if we have found a polygon, grab its color
	if( pPoly )
	{	
		//select this color in the picker
		CEditBrush* pBrush = pPoly->m_pBrush;
		GetColorSelectDlg()->SetCurColor(RGB(pBrush->r, pBrush->g, pBrush->b), TRUE);
	}	

	return;
}



// talon: 2001 (0x45a4a1): '-' and '=' darken and brighten the selected
// polies' ambient light.
void CRegionView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CMoArray<CEditPoly*> polies;
	DWORD i, j;

	if( nChar == '-' )
	{
		GetSelectedPolies( polies );
		for( i=0; i < polies; i++ )
		{
			for( j=0; j < 3; j++ )
			{
				if( polies[i]->m_Ambient[j] > 0 )
					polies[i]->m_Ambient[j] -= 3;
			}
		}
	}
	else if( nChar == '=' )
	{
		GetSelectedPolies( polies );
		for( i=0; i < polies; i++ )
		{
			for( j=0; j < 3; j++ )
			{
				if( polies[i]->m_Ambient[j] < 255 )
					polies[i]->m_Ambient[j] += 3;
			}
		}
	}

	if( polies > 0 )
		DrawRect();

	CView::OnChar(nChar, nRepCnt, nFlags);
}

void CRegionView::OnResetDetailLevels()
{
	CEditRegion *pRegion;
	CEditBrush *pBrush;
	LPOS pos;
	CRealProp *pProp;

	// talon: 2001 (0x45a56d): the brushes' "DetailLevel", a real.
	pRegion = GetRegion();
	for(pos=pRegion->m_Brushes; pos; )
	{
		pBrush = pRegion->m_Brushes.GetNext(pos);

		pProp = (CRealProp*)pBrush->m_PropList.GetProp("DetailLevel");
		if(pProp)
		{
			pProp->m_Value = 0.0f;
		}
	}
}


/******************************************************************/
// Store the current camera position in the list of navigator positions
void CRegionView::OnNavigatorStore() 
{
	// Create the default position string based off of this view
	CString sString;
	sString.Format("POS(%.1f, %.1f, %.1f)", Nav().m_Position.x, Nav().m_Position.y, Nav().m_Position.z);

	// Ask the user for the name that they want for this position
	CNavigatorStoreDlg storeDlg;
	storeDlg.m_sName=sString;

	if (storeDlg.DoModal() == IDOK)
	{		
		// Create the new navigation position
		CNavigatorPosItem *pNavigatorItem=new CNavigatorPosItem();

		// Set the name
		pNavigatorItem->SetDescription(storeDlg.m_sName.GetBuffer(0));

		// Add each view Nav position			
		CDocument* pDoc = GetDocument();
		POSITION pos = pDoc->GetFirstViewPosition();

		// Loop through each view
		while (pos != NULL)
		{
			CRegionView* pView =(CRegionView *)pDoc->GetNextView(pos);
			if(pView != NULL)
			{
				// Add the navigator
				CNavigator *pNav=new CNavigator;
				*pNav=pView->Nav();
				pNavigatorItem->AddNavigator(pNav);
			}	
		}

		// Get the edit region
		CEditRegion *pRegion = GetRegion();

		// Store the position in the array
		if (pRegion)
		{
			CNavigatorPosArray *pArray=pRegion->GetNavigatorPosArray();
			if (pArray)
			{
				pArray->Add(pNavigatorItem);
			}
		}

		// Set the modified flag		
		if (pDoc)
		{
			pDoc->SetModifiedFlag(TRUE);
		}
	}
}


/******************************************************************/
// Open the navigator organizer dialog
void CRegionView::OnNavigatorOrganize() 
{
	// Get the edit region
	CEditRegion *pRegion = GetRegion();
	if (!pRegion)
	{
		return;
	}

	// Create the navigator dialog
	CNavigatorDlg navigatorDlg;
	navigatorDlg.SetRegionView(this);

	navigatorDlg.SetNavigatorPosArray(pRegion->GetNavigatorPosArray());
	navigatorDlg.DoModal();	

	// Set the modified flag
	CDocument *pDoc=GetDocument();
	if (pDoc)
	{
		pDoc->SetModifiedFlag(TRUE);
	}
}


/******************************************************************/
// Go to the next navigator position
void CRegionView::OnNavigatorGotoNext() 
{
	// Get the edit region
	CEditRegion *pRegion = GetRegion();
	if (!pRegion)
	{
		return;
	}

	// Get the navigator array
	CNavigatorPosArray *pNavigatorArray=pRegion->GetNavigatorPosArray();
	if (!pNavigatorArray || pNavigatorArray->GetSize() <= 0)
	{
		return;
	}

	// Get the last viewed position and increment it
	int nIndex=pRegion->m_nLastNavigatorPos;
	nIndex++;	

	// Wrap the index if necessary
	if (nIndex > pNavigatorArray->GetSize()-1)
	{
		nIndex=0;
	}

	// View the position
	SwitchNavigator(nIndex);
}



/******************************************************************/
// Go to the previous navigator position
void CRegionView::OnNavigatorGotoPrevious() 
{
	// Get the edit region
	CEditRegion *pRegion = GetRegion();
	if (!pRegion)
	{
		return;
	}

	// Get the navigator array
	CNavigatorPosArray *pNavigatorArray=pRegion->GetNavigatorPosArray();
	if (!pNavigatorArray || pNavigatorArray->GetSize() <= 0)
	{
		return;
	}

	// Get the last viewed position and decrement it
	int nIndex=pRegion->m_nLastNavigatorPos;
	nIndex--;	

	// Wrap the index if necessary
	if (nIndex < 0)
	{
		nIndex=pNavigatorArray->GetSize()-1;
	}

	// View the position
	SwitchNavigator(nIndex);	
}



// Switch the navigator to one of the stored navigators for this level
void CRegionView::SwitchNavigator(int nNavIndex)
{	
	// Get the edit region
	CEditRegion *pRegion = GetRegion();	
	if (!pRegion)
	{
		return;
	}

	// Get the navigator array
	CNavigatorPosArray *pNavigatorArray=pRegion->GetNavigatorPosArray();
	if (!pNavigatorArray)
	{
		return;
	}

	// Get the document
	CDocument* pDoc = GetDocument();
	POSITION pos = pDoc->GetFirstViewPosition();

	// Loop through each view
	int nCurrentIndex=0;
	while (pos != NULL)
	{
		CRegionView* pView =(CRegionView *)pDoc->GetNextView(pos);
		if(pView != NULL)
		{
			// Set the navigator
			CNavigator *pNavigator=pNavigatorArray->GetAt(nNavIndex)->GetNavigator(nCurrentIndex);
			pView->Nav()=*pNavigator;
			nCurrentIndex++;
		}	
	}

	// Set the "last viewed position" variable
	pRegion->m_nLastNavigatorPos=nNavIndex;

	GetRegionDoc()->RedrawAllViews();
}

/************************************************************************/
// Handles the "Select All" command
void CRegionView::OnSelectionSelectAll() 
{
	// Get the edit region
	CEditRegion *pRegion=GetRegion();
	
	// Show the wait cursor
	BeginWaitCursor();

	// Select all of the nodes
	if (pRegion)
	{	
		pRegion->RecurseAndSelect(pRegion->GetRootNode(), FALSE);

		// Update the selection array
		pRegion->UpdateSelectionArray();

		// Make sure that the root node isn't selected
		pRegion->UnselectNode(pRegion->GetRootNode());
	}	

	// Redraw
	GetRegionDoc()->NotifySelectionChange();
	GetRegionDoc()->RedrawAllViews();

	// Stop showing the wait cursor
	EndWaitCursor();
}


/************************************************************************/
// Handles the "Select None" command
void CRegionView::OnSelectionSelectNone() 
{
	// Get the edit region
	CEditRegion *pRegion=GetRegion();
	
	// Select all of the nodes
	if (pRegion)
	{
		pRegion->RecurseAndUnselect(pRegion->GetRootNode(), FALSE);

		// Update the selection array
		pRegion->UpdateSelectionArray();
	}

	// Redraw
	GetRegionDoc()->NotifySelectionChange();
	GetRegionDoc()->RedrawAllViews();
}


/************************************************************************/
// Handles the "Select Inverse" command
void CRegionView::OnSelectionSelectInverse() 
{
	// Get the edit region
	CEditRegion *pRegion=GetRegion();
	
	// Select all of the nodes
	if (pRegion)
	{
		pRegion->RecurseAndInverseSelect(pRegion->GetRootNode());

		// Make sure that the root node isn't selected
		pRegion->UnselectNode(pRegion->GetRootNode());

		// Update the selection array
		pRegion->UpdateSelectionArray();
	}

	// Redraw
	GetRegionDoc()->RedrawAllViews();
}

/************************************************************************/
// The command UI update for the "Select Inverse" command
void CRegionView::OnUpdateSelectionSelectInverse(CCmdUI* pCmdUI) 
{
	CEditRegion *pRegion=GetRegion();
	if (pRegion)
	{
		if (pRegion->m_Selections.GetSize() > 0)
			pCmdUI->Enable(TRUE);
		else
			pCmdUI->Enable(FALSE);
	}
}

/************************************************************************/
// Handles the "Advanced Select" command
void CRegionView::OnSelectionAdvanced() 
{				
	// Show the advanced select dialog
	AdvancedSelectDlg dlg;
	dlg.LoadRegistrySettings();

	if(dlg.DoModal() == IDOK)
	{
		dlg.SaveRegistrySettings();

		// Set the select flag based on the radio button
		BOOL bSelect;
		if (dlg.m_nSelect == 0)
		{
			bSelect=TRUE;
		}
		else
		{
			bSelect=FALSE;
		}

		// Get the document
		CRegionDoc *pDoc=(CRegionDoc *)GetDocument();
		if(pDoc)
		{
			// Setup the advanced select class
			CAdvancedSelect advancedSelect;
			advancedSelect.Init(pDoc);

			// Add the criteria
			if (dlg.m_bNodesOfClass)
			{
				advancedSelect.AddClassCriteria(dlg.m_ClassName);
			}
			if (dlg.m_bObjectsWithName)
			{
				advancedSelect.AddNameCriteria(dlg.m_ObjectName, !dlg.m_bMatchWholeWord);
			}
			// Show the wait cursor
			BeginWaitCursor();

			// Turn off the drawing of the node view
			if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
			{
				GetNodeView()->SetRedraw(FALSE);
			}

			// Do the selection operation
			if (bSelect)
			{
				advancedSelect.Select(TRUE, dlg.m_bShowResults);
			}
			else
			{
				advancedSelect.Unselect(TRUE, dlg.m_bShowResults);
			}			

			// Turn on the drawing of the node view
			if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
			{
				GetNodeView()->SetRedraw(TRUE);
			}

			// Stop showing the wait cursor
			EndWaitCursor();
		}
	}	
}




/************************************************************************/
// Handles the "Select Container" command
void CRegionView::OnSelectionContainer() 
{
	// Get the document
	CRegionDoc *pDoc=GetRegionDoc();
	if (!pDoc)		
	{
		return;
	}

	// Get the region
	CEditRegion *pRegion=pDoc->GetRegion();
	if (!pRegion)
	{
		return;
	}

	// Make sure that there are selections
	if (pRegion->GetNumSelections() <= 0)
	{
		return;
	}

	// Store the currently selected nodes	
	CMoArray<CWorldNode *>	m_selectedNodeArray;
	m_selectedNodeArray.SetCacheSize(50);

	int i;
	for (i=0; i < pRegion->GetNumSelections(); i++)
	{
		CWorldNode *pSelection=pRegion->GetSelection(i);

		// Skip over container nodes
//		if (pSelection->GetType() != Node_Null)
		{
			m_selectedNodeArray.Add(pSelection);
		}
	}

	// Make an array of the container nodes for the selected nodes
	CMoArray<CWorldNode *>	m_containerArray;
	m_containerArray.SetCacheSize(50);

	// Select the container node for each selected node
	for (i=0; i < m_selectedNodeArray.GetSize(); i++)
	{
		// Get the container
		CWorldNode *pContainer=m_selectedNodeArray[i]->GetParent();		
		
		// Make sure that the container hasn't already been added to the array
		BOOL bFound=FALSE;

		int n;
		for (n=0; n < m_containerArray.GetSize(); n++)
		{
			if (m_containerArray[n] == pContainer)
			{
				bFound=TRUE;
				break;
			}
		}

		// Add the container if it wasn't found in the array
		if (!bFound)
		{
			m_containerArray.Add(pContainer);
		}
	}

	// Select all of the container nodes
	for (i=0; i < m_containerArray.GetSize(); i++)
	{
		// Select the nodes
		pRegion->RecurseAndSelect(m_containerArray[i]);		
	}		

	// Make sure that the root node didn't get selected
	pRegion->UnselectNode(pRegion->GetRootNode());

	// Notify that the selection has changed
	pDoc->NotifySelectionChange();
}

/************************************************************************/
// Command update handler for the "Select Container" command
void CRegionView::OnUpdateSelectionContainer(CCmdUI* pCmdUI)
{
	// talon: 2001 (0x45af33: the group, container and hide-inverse updates
	// are one function in the link)
	CRegionDoc *pDoc=GetRegionDoc();
	if (pDoc)
	{
		CEditRegion *pRegion=&pDoc->m_Region;
		if (pRegion && pRegion->m_Selections.GetSize() <= 0)
			pCmdUI->Enable(FALSE);
	}
}

/************************************************************************/
// Handles the "Hide Selected" command
void CRegionView::OnSelectionHideSelected() 
{
	// Get the region
	CEditRegion *pRegion=GetRegion();
	if (pRegion)
	{
		// Put the wait cursor on the screen
		BeginWaitCursor( );

		for (DWORD i=0; i < pRegion->m_Selections; i++)
		{
			pRegion->m_Selections[i]->HideNode();
		}

		// Update the node images
		GetNodeView()->UpdateNodeImage(pRegion->GetRootNode());

		// Redraw the views
		GetRegionDoc()->RedrawAllViews();

		// End the wait cursor
		EndWaitCursor( );
	}
}

/************************************************************************/
// The command UI update for the "Hide Selected" command
void CRegionView::OnUpdateSelectionHideSelected(CCmdUI* pCmdUI) 
{
	// talon: 2001 (0x45acf5, shared with the unhide updates by the link)
	CEditRegion *pRegion=GetRegion();
	if (pRegion && pRegion->m_Selections.GetSize() <= 0)
		pCmdUI->Enable(FALSE);
}

/************************************************************************/
// Handles the "Unhide Selected" command
// talon: 2001 (0x45ad15): selects every brush with a DetailLevel above 0.
// No EndWaitCursor follows the BeginWaitCursor (RETAIL_BUGS).
void CRegionView::OnSelectDetailBrushes()
{
	LPOS pos;
	CEditBrush *pBrush;
	CRealProp *pProp;

	BeginWaitCursor();

	OnSelectionSelectNone();

	for(pos=m_pRegion->m_Brushes; pos; )
	{
		pBrush = m_pRegion->m_Brushes.GetNext(pos);

		pProp = (CRealProp*)pBrush->m_PropList.GetProp("DetailLevel");
		if( pProp && pProp->m_Value > 0.0f )
			m_pRegion->SelectNode( pBrush );
	}

	GetNodeView()->UpdateNodeImage( m_pRegion->GetRootNode() );
	GetRegionDoc()->RedrawAllViews();
}

void CRegionView::OnSelectionUnhideSelected() 
{
	// Get the region
	CEditRegion *pRegion=GetRegion();
	if (pRegion)
	{
		// Put the wait cursor on the screen
		BeginWaitCursor( );

		for (DWORD i=0; i < pRegion->m_Selections; i++)
		{
			pRegion->m_Selections[i]->ShowNode();
		}

		// Update the node images
		GetNodeView()->UpdateNodeImage(pRegion->GetRootNode());

		// Redraw the views
		GetRegionDoc()->RedrawAllViews();

		// End the wait cursor
		EndWaitCursor( );
	}
}

/************************************************************************/
// The command UI update for the "Unhide Selected" command
void CRegionView::OnUpdateSelectionUnhideSelected(CCmdUI* pCmdUI) 
{
	// talon: 2001 (0x45acf5, shared with the unhide updates by the link)
	CEditRegion *pRegion=GetRegion();
	if (pRegion && pRegion->m_Selections.GetSize() <= 0)
		pCmdUI->Enable(FALSE);
}

/************************************************************************/
// Handles the "Hide Inverse" command
void CRegionView::OnSelectionHideInverse() 
{
	// Get the edit region
	CEditRegion *pRegion=GetRegion();
	
	// Put the wait cursor on the screen
	BeginWaitCursor( );

	// Hide the inverse nodes
	if (pRegion)
	{
		pRegion->RecurseAndInverseHide(pRegion->GetRootNode());		

		// Make sure that the root is not hidden
		pRegion->GetRootNode()->ShowNode();
	}

	// talon: 2001 (0x45ae28) updates the node images whether or not there is a
	// region (with none it passes the root of NULL: RETAIL_BUGS).
	GetNodeView()->UpdateNodeImage(pRegion->GetRootNode());

	// Redraw the views
	GetRegionDoc()->RedrawAllViews();

	// End the wait cursor
	EndWaitCursor( );
}

/************************************************************************/
// The command UI update for the "Hide Inverse" command
void CRegionView::OnUpdateSelectionHideInverse(CCmdUI* pCmdUI)
{
	// talon: 2001 (0x45af33: the group, container and hide-inverse updates
	// are one function in the link)
	CRegionDoc *pDoc=GetRegionDoc();
	if (pDoc)
	{
		CEditRegion *pRegion=&pDoc->m_Region;
		if (pRegion && pRegion->m_Selections.GetSize() <= 0)
			pCmdUI->Enable(FALSE);
	}
}

/************************************************************************/
// Handles the "Unhide Inverse" command
void CRegionView::OnSelectionUnhideInverse() 
{
	// Get the edit region
	CEditRegion *pRegion=GetRegion();
	
	// Put the wait cursor on the screen
	BeginWaitCursor( );

	// Unhide the inverse nodes
	if (pRegion)
	{
		pRegion->RecurseAndInverseUnhide(pRegion->GetRootNode());
	}

	GetNodeView()->UpdateNodeImage(pRegion->GetRootNode());

	// Redraw the views
	GetRegionDoc()->RedrawAllViews();

	// End the wait cursor
	EndWaitCursor( );
}

/************************************************************************/
// The command UI update for the "Unhide Inverse" command
void CRegionView::OnUpdateSelectionUnhideInverse(CCmdUI* pCmdUI) 
{
	// talon: 2001 (0x45acf5, as the hide-selected update)
	CEditRegion *pRegion=GetRegion();
	if (pRegion && pRegion->m_Selections.GetSize() <= 0)
		pCmdUI->Enable(FALSE);
}

/************************************************************************/
// The group function was called
void CRegionView::OnSelectionGroup() 
{	
	// Get the document
	CRegionDoc *pDoc=GetRegionDoc();
	if (!pDoc)
	{
		return;
	}

	// Get the region
	CEditRegion *pRegion=GetRegion();
	if (!pRegion)
	{
		return;
	}	

	// Put the wait cursor on the screen
	BeginWaitCursor( );

	// Add the null node to be parent...
	CWorldNode *pContainer = GetRegion()->AddNullNode(GetRegion()->GetActiveParentNode());
	
	// Add to the undo list.
	PreActionList actionList;
	actionList.AddTail(new CPreAction(ACTION_ADDEDNODE, pContainer));
				
	// Move the nodes
	pDoc->MoveSelectedNodes(pContainer);

	// Select the new container node
	GetRegion( )->RecurseAndSelect(pContainer);

	// Stop displaying the wait cursor
	EndWaitCursor( );	
}

/************************************************************************/
// Command UI updater for the group function
void CRegionView::OnUpdateSelectionGroup(CCmdUI* pCmdUI)
{
	// talon: 2001 (0x45af33: the group, container and hide-inverse updates
	// are one function in the link)
	CRegionDoc *pDoc=GetRegionDoc();
	if (pDoc)
	{
		CEditRegion *pRegion=&pDoc->m_Region;
		if (pRegion && pRegion->m_Selections.GetSize() <= 0)
			pCmdUI->Enable(FALSE);
	}
}

/************************************************************************/
// The split brush option was selected from the menu
void CRegionView::OnBrushSplitBrush() 
{		
	if (GetEditMode() != BRUSH_EDITMODE)
		return;

	CEditRegion *pRegion=GetRegion();

	// Make sure that we have one brush selected
	if((pRegion->GetNumSelections() < 1) || (pRegion->GetSelection(0)->GetType() != Node_Brush))
		return;

	LTVector forward, other;
	CEditPlane edgePlane;
	CEditBrush *pToSplit;
	CEditBrush *sides[2];
	DWORD indSel, indPoly, indPoint, indSide,indBrush;
	PreActionList actionList;
	CMoArray<CEditBrush*> toDelete;
	CEditVert newPt;
	PolySide side;
	CEditPoly *pPoly;
	CReal dot;

	// Check to see if we can split and get the last selected brush index
	int nLastSelectedBrush=0;
	if (!CanSplitBrush(&nLastSelectedBrush))
	{
		return;
	}	
	
	// Get the drawing brush
	CEditBrush *pBrush=NULL;
	
	// Check to see if the splitting plane should be from the drawing brush
	BOOL bSplitFromView=FALSE;
	if (DrawingBrush().m_Points.GetSize() > 0)
	{
		bSplitFromView=TRUE;
		pBrush=&DrawingBrush();

		// Make a plane from its points and our view.
		forward = ViewDef()->m_Nav.Forward();
		other = pBrush->m_Points[1] - pBrush->m_Points[0];
		edgePlane.m_Normal = forward.Cross(other);
		edgePlane.m_Normal.Norm();
		edgePlane.m_Dist = edgePlane.m_Normal.Dot(pBrush->m_Points[0]);
	}
	else
	{
		// Get the last selected brush
		pBrush=pRegion->GetSelection(nLastSelectedBrush)->AsBrush();

		// Use the last selected brush
		edgePlane.m_Normal = pBrush->m_Polies[0]->Normal();
		edgePlane.m_Normal.Norm();
		edgePlane.m_Dist = pBrush->m_Polies[0]->Dist();
	}

	for(indSel=0; indSel < pRegion->m_Selections; indSel++)
	{
		if(pRegion->m_Selections[indSel]->GetType() == Node_Brush)
		{
			pToSplit = pRegion->m_Selections[indSel]->AsBrush();
			
			// Don't split the splitting brush
			if (pToSplit == pBrush)
			{
				continue;
			}

			if(pToSplit->m_Polies)
			{
				// whether any of the brush's polies are on the front or the back of the splitting plane
				bool onBack = false, onFront = false; 

				// Check to see which side of the splitting plane the brush is on
				for (indPoly=0; indPoly < pToSplit->m_Polies.GetSize(); indPoly++)
				{
					pPoly = pToSplit->m_Polies[indPoly];

					for(indPoint=0; indPoint < pPoly->m_Indices.GetSize(); indPoint++ )
					{
						dot = edgePlane.m_Normal.Dot(pPoly->Pt(indPoint)) - edgePlane.m_Dist;
						if( dot > POINT_SIDE_EPSILON )
						{
							onFront = true;
						}
						else if( dot < -POINT_SIDE_EPSILON )
						{
							onBack  = true;
						}
					}
				}

				// Only continue if a useful split
				if (onBack && onFront)
				{

					CEditPoly temp(pToSplit->m_Polies[0]);
					temp.m_Plane = edgePlane;
					temp.m_Indices.Term();
					
					temp.m_Indices.Append(pToSplit->AddVertOrGetClosest(pBrush->m_Points[0], 0.01f));
					temp.m_Indices.Append(pToSplit->AddVertOrGetClosest(pBrush->m_Points[1], 0.01f));

					if (bSplitFromView)
					{
						// Create the new point from the view vector
						newPt = pBrush->m_Points[0] + forward;
					}
					else
					{
						// Just grab the 3rd point in the polygon
						newPt = pBrush->m_Points[2];
					}

					temp.m_Indices.Append(pToSplit->AddVertOrGetClosest(newPt, 0.01f));
					
					SimpleSplitBrush(pRegion, &temp, pToSplit, sides);
					// Update the verts in the original brush for proper undo handling
					pToSplit->RemoveUnusedVerts();
					if(sides[0]->m_Polies >= 1 && sides[1]->m_Polies >= 1)
					{
						// Setup an undo.
						actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, pToSplit));
						actionList.AddTail(new CPreAction(ACTION_ADDEDNODE, sides[0]));
						actionList.AddTail(new CPreAction(ACTION_ADDEDNODE, sides[1]));

						for(indSide=0; indSide < 2; indSide++)
						{
							sides[indSide]->RemoveUnusedVerts();
							sides[indSide]->UpdateBoundingInfo();
						}

						no_AttachNode(pRegion, sides[0], pToSplit->GetParent());
						no_AttachNode(pRegion, sides[1], pToSplit->GetParent());
						
						// (delete later so we can make the undo)
						toDelete.Append(pToSplit);
					}
					else
					{
						no_DestroyNode(pRegion, sides[0], FALSE);
						no_DestroyNode(pRegion, sides[1], FALSE);
					}
				}
			}
		}
	}

	GetRegionDoc()->Modify(&actionList, TRUE);

	// Delete the brushes that were split.
	for(indBrush=0; indBrush < toDelete; indBrush++)
	{
		GetNodeView( )->DeleteNode(toDelete[indBrush]);
		no_DestroyNode(pRegion, toDelete[indBrush], FALSE);
	}

	pBrush->Term();
	m_EditState = EDIT_NOSTATE;

	GetRegionDoc()->RedrawAllViews();


}


/************************************************************************/
// The command update handler for the split brush command
void CRegionView::OnUpdateBrushSplitBrush(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable(CanSplitBrush(NULL));
}


/************************************************************************/
// Returns TRUE if we are in a situation in which we can split a brush.
// pnLastSelectedBrush is filled in with the last selected brush index.
BOOL CRegionView::CanSplitBrush(int *pnLastSelectedBrush)
{
	// Get the region
	CEditRegion *pRegion=GetRegion();

	// Make sure that there is at least something selected
	if (pRegion->GetNumSelections() == 0)
	{
		return FALSE;
	}
	
	// See how many brushes are selected and store the index of the last brush
	int nBrushesSelected=0;
	int nLastBrushIndex=0;
	
	int i;
	for(i=0; i < pRegion->GetNumSelections(); i++)
	{
		if(pRegion->GetSelection(i)->GetType() == Node_Brush)
		{
			nBrushesSelected++;
			nLastBrushIndex=i;			
		}
	}

	// Store the last selected brush
	if (pnLastSelectedBrush)
	{
		*pnLastSelectedBrush=nLastBrushIndex;
	}

	// We must have at least one brush selected
	if (nBrushesSelected == 0)
	{		
		return FALSE;
	}

	// Check to see if the user is using the drawing brush
	if (DrawingBrush().m_Points > 0)
	{
		// There must be only 2 points set
		if (DrawingBrush().m_Points != 3)
		{			
			return FALSE;
		}

		// Don't allow splitting in the perspective view
		if (IsPerspectiveViewType())
		{			
			return FALSE;
		}	
	}
	else
	{
		// There must be more than one brush selected
		if (nBrushesSelected < 2)
		{			
			return FALSE;
		}

		// Check to see if the last brush is a plane (one poly)
		CEditBrush *pLastBrush=pRegion->GetSelection(nLastBrushIndex)->AsBrush();

		if (pLastBrush->m_Polies.GetSize() != 1)
		{			
			return FALSE;
		}

		// Make sure that the brush has at least 3 points
		if (pLastBrush->m_Points.GetSize() < 3)
		{
			return FALSE;
		}
	}

	return TRUE;
}

/************************************************************************/
// Handler for the join command
void CRegionView::OnBrushJoin() 
{
	if (GetEditMode() != BRUSH_EDITMODE)
		return;

	CEditRegion *pRegion=GetRegion();

	// Make sure that we have at least 2 brushes selected
	if(pRegion->GetNumSelections() < 2)
		return;

	PreActionList actionList;

	// Build a list of mods.
	for(uint32 nUndoLoop =0; nUndoLoop < pRegion->m_Selections.GetSize(); ++nUndoLoop)
	{
		CWorldNode *pNode = pRegion->m_Selections[nUndoLoop ];
		
		if(pNode->GetType() == Node_Brush)
		{
			actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, pNode));
		}
	}
	
	// Setup the undo.
	GetRegionDoc()->Modify(&actionList, FALSE);

	// Start joinin'!
	CEditBrush *pNewBrush = LTNULL;

	// Add the other brushes to the first brush
	for (uint32 nJoinLoop = 0; nJoinLoop < pRegion->m_Selections.GetSize(); ++nJoinLoop)
	{
		CWorldNode *pJoinNode = pRegion->m_Selections[nJoinLoop];
		if (pJoinNode->GetType() != Node_Brush)
		{
			// If it's not a brush, un-select and try again..
			pRegion->UnselectNode(pJoinNode);
			--nJoinLoop;
			continue;
		}
		
		CEditBrush *pJoinBrush = pJoinNode->AsBrush();

		// Use the first brush we come to..
		if (!pNewBrush)
		{
			pNewBrush = pJoinBrush;
			pRegion->UnselectNode(pJoinNode);
			--nJoinLoop;
			continue;
		}

		uint32 nVertOffset = pNewBrush->m_Points.GetSize();

		// Adjust the vertex references (Note : Reverse order to prevent overwrites..)
		for (uint32 nAdjVertLoop = pJoinBrush->m_Points.GetSize(); nAdjVertLoop > 0; --nAdjVertLoop)
		{
			pJoinBrush->ChangeVertexReferences(nAdjVertLoop - 1, nAdjVertLoop + nVertOffset - 1);
		}

		// Suck 'em over..
		pNewBrush->m_Points.AppendArray(pJoinBrush->m_Points);
		pNewBrush->m_Polies.AppendArray(pJoinBrush->m_Polies);

		// Point the polys in the right place
		for (uint32 nClearLoop = 0; nClearLoop < pJoinBrush->m_Polies.GetSize(); ++nClearLoop)
			pJoinBrush->m_Polies[nClearLoop]->m_pBrush = pNewBrush;

		// Clear out the old arrays
		pJoinBrush->m_Points.RemoveAll();
		pJoinBrush->m_Polies.RemoveAll();
	}

	// Skip out if there weren't actually any brushes...
	if (pNewBrush == 0)
		return;

	pNewBrush->UpdateBoundingInfo();

	// Delete the selected brushes
	DeleteSelectedNodes();

	// Geometry clean-up..
	pRegion->CleanupGeometry();

	// Select the new brush
	GetRegion()->SelectNode( pNewBrush->AsNode() );
	GetRegionDoc()->NotifySelectionChange();

	// Re-draw
	GetRegionDoc()->RedrawAllViews();
}

/************************************************************************/
// Handler for the un-join command
void CRegionView::OnBrushUnJoin() 
{
	if (GetEditMode() != BRUSH_EDITMODE)
		return;

	// Get the brush list from the selection list
	CMoArray<CEditBrush *> inBrushes;

	inBrushes.SetCacheSize(GetRegion()->m_Selections.GetSize());
	for (uint32 nFillInLoop = 0; nFillInLoop < GetRegion()->m_Selections.GetSize(); ++nFillInLoop)
	{
		CWorldNode *pNode = GetRegion()->m_Selections[nFillInLoop];
		if (pNode->GetType() == Node_Brush)
		{
			inBrushes.Append(pNode->AsBrush());
		}
	}

	// Empty?  Guess they don't want anything un-joined...
	if (!inBrushes.GetSize())
		return;

	// This might take a while.. Put the wait cursor on the screen
	BeginWaitCursor( );

	// Create an undo list
	PreActionList actionList;

	// Start un-joining
	for(uint32 nInBrushLoop = 0; nInBrushLoop < inBrushes.GetSize(); ++nInBrushLoop)
	{
		// Get our current brush..
		CEditBrush *pInBrush = inBrushes[nInBrushLoop];

		// Remember that it's going to be modified
		actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, inBrushes[nInBrushLoop]));

		// Go through the polygons..
		for (uint32 nInPolyLoop = 0; nInPolyLoop < pInBrush->m_Polies.GetSize(); ++nInPolyLoop)
		{
			CEditPoly *pInPoly = pInBrush->m_Polies[nInPolyLoop];

			// Create a new brush for this polygon
			CEditBrush *pOutBrush = no_CreateNewBrush(GetRegion(), pInBrush->GetParent());
			pOutBrush->m_PropList.CopyValues(&pInBrush->m_PropList);
			pOutBrush->SetName(pInBrush->GetName());

			// Create a new poly for the brush
			CEditPoly *pOutPoly = new CEditPoly(pInPoly);
			pOutPoly->m_pBrush = pOutBrush;

			// Copy the points of the current polygon
			for (uint32 nInPointLoop = 0; nInPointLoop < pInPoly->m_Indices.GetSize(); ++nInPointLoop)
			{
				pOutBrush->m_Points.Append(pInPoly->Pt(nInPointLoop));
				pOutPoly->m_Indices[nInPointLoop] = nInPointLoop;
			}

			// Add the poly to the brush
			pOutBrush->m_Polies.Append(pOutPoly);

			// Update the brush
			pOutBrush->UpdateBoundingInfo();

			// Add it to the undo list
			actionList.AddTail(new CPreAction(ACTION_ADDEDNODE, pOutBrush));

			// Select it
			GetRegion()->SelectNode(pOutBrush);
		}

		// Update the node view's list of the new brushes
		GetNodeView()->RecurseUpdateLabels(pInBrush->GetParent());
	}

	// Remember the undo
	GetRegionDoc()->Modify(&actionList, TRUE);

	// Delete the old brushes
	for (uint32 nDeleteLoop = 0; nDeleteLoop < inBrushes.GetSize(); ++nDeleteLoop)
	{
		GetNodeView( )->DeleteNode(inBrushes[nDeleteLoop]);
		no_DestroyNode(GetRegion(), inBrushes[nDeleteLoop], FALSE);
	}

	// Geometry clean-up..
	GetRegion()->CleanupGeometry();

	// Tell everyone we've been playing with the selections
	GetRegionDoc()->NotifySelectionChange();

	// Re-draw
	GetRegionDoc()->RedrawAllViews();

	// Ok, you're done waiting..
	EndWaitCursor( );
}

/************************************************************************/
// Handler for the flip command
void CRegionView::OnBrushFlip() 
{
	if (GetEditMode() != BRUSH_EDITMODE)
		return;

	CEditRegion *pRegion=GetRegion();

	// Make sure that we have one brush selected
	if((pRegion->GetNumSelections() < 1) || (pRegion->GetSelection(0)->GetType() != Node_Brush))
		return;

	DoFlipOperation();	
}

/************************************************************************/


/************************************************************************/
// Performs a mirror along the X-Axis
void CRegionView::OnSelectionMirrorX() 
{
	MirrorSelectedNodes(CVector(1,0,0));
}

/************************************************************************/
// Command update handler for the mirror x-axis command
void CRegionView::OnUpdateSelectionMirrorX(CCmdUI* pCmdUI) 
{
	// talon: 2001 (0x45bad5, one function for the three in the link)
	if (GetRegion()->m_Selections.GetSize() > 0)
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

/************************************************************************/
// Performs a mirror along the Y-Axis
void CRegionView::OnSelectionMirrorY() 
{
	MirrorSelectedNodes(CVector(0,1,0));
}

/************************************************************************/
// Command update handler for the mirror y-axis command
void CRegionView::OnUpdateSelectionMirrorY(CCmdUI* pCmdUI) 
{
	// talon: 2001 (0x45bad5, one function for the three in the link)
	if (GetRegion()->m_Selections.GetSize() > 0)
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

/************************************************************************/
// Performs a mirror along the Z-Axis
void CRegionView::OnSelectionMirrorZ() 
{
	MirrorSelectedNodes(CVector(0,0,1));	
}

/************************************************************************/
// Command update handler for the mirror z-axis command
void CRegionView::OnUpdateSelectionMirrorZ(CCmdUI* pCmdUI) 
{
	// talon: 2001 (0x45bad5, one function for the three in the link)
	if (GetRegion()->m_Selections.GetSize() > 0)
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

/************************************************************************/
// Centers the seleciton about the currently placed marker



/************************************************************************/
// Centers the marker on the selected brushes or objects
void CRegionView::OnCenterMarkerOnSelection() 
{
	CRegionDoc *pDoc=GetRegionDoc();
	CEditRegion *pRegion=GetRegion();
	if (!pRegion || !pDoc || (pRegion->GetNumSelections() == 0))
	{
		return;
	}

	//make sure the selection box is up to date
	pDoc->UpdateSelectionBox();

	// Place the marker	
	pRegion->m_vMarker = pDoc->m_SelectionMin + ((pDoc->m_SelectionMax - pDoc->m_SelectionMin) / 2.0f);

	// Update the views
	GetRegionDoc()->RedrawAllViews();
}

/************************************************************************/
// Command update handler for the "center marker on selection" command
void CRegionView::OnUpdateCenterMarkerOnSelection(CCmdUI* pCmdUI) 
{
	// talon: 2001 (0x45bbfa)
	if (GetDocument() && m_pRegion && m_pRegion->m_Selections.GetSize() < 1)
		pCmdUI->Enable(FALSE);
}

/************************************************************************/
// Places the marker at the camera position in a perspective view
void CRegionView::OnPlaceMarkerAtCamera() 
{
	// This should only be called in a perspective view
	ASSERT(IsPerspectiveViewType());

	// Get the region
	CEditRegion *pRegion=GetRegion();
	if (!pRegion)
	{
		return;
	}

	// Move the marker location
	pRegion->m_vMarker=Nav().Pos();

	// Redraw the views
	GetRegionDoc()->RedrawAllViews();
}

/************************************************************************/
// Places the marker at a specific location
void CRegionView::OnPlaceMarkerAtVector() 
{
	// Get the region
	CEditRegion *pRegion=GetRegion();
	if (!pRegion)
	{
		return;
	}

	// The vector editing dialog
	CVectorEdit vectorEditDlg;
	
	// Set the dialog to the current marker position
	vectorEditDlg.SetVector(pRegion->m_vMarker);

	// Display the dialog
	if (vectorEditDlg.DoModal() == IDOK)
	{
		// Move the marker
		pRegion->m_vMarker=vectorEditDlg.GetVector();

		// Redraw the views
		GetRegionDoc()->RedrawAllViews();
	}
}

/************************************************************************/
// Command update handler for the "place marker at camera" command
void CRegionView::OnUpdatePlaceMarkerAtCamera(CCmdUI* pCmdUI) 
{
	// Enable this command if we are in a perspective view
	pCmdUI->Enable(IsPerspectiveViewType());
}

/************************************************************************/
// Generates unique names for the selected objects
//
// bUpdateRefProps		- Updates properties for objects that reference
//						  other objects that have had their names change
// bUpdateSelPropsOnly	- Only objects that are selected will have their
//						  properties updated
// bDisplayReport		- A dialog reporting the name changes is displayed
// bGenerateUndoInfo	- Undo information is generated and added to the document
void CRegionView::GenerateUniqueNamesForSelected(BOOL bUpdateRefProps, BOOL bUpdateSelPropsOnly, BOOL bDisplayReport, BOOL bGenerateUndoInfo)
{
	// Get the region doc
	CRegionDoc *pDoc=GetRegionDoc();
	ASSERT(pDoc);

	// Get the region
	CEditRegion *pRegion=GetRegion();
	if (!pRegion)
	{
		return;
	}
	
	// Show the wait cursor
	BeginWaitCursor();

	// Generate undo information if needed
	if (bGenerateUndoInfo)
	{
		// The undo list
		PreActionList objectNameUndoInfo;				

		// Generate the undo information
		pRegion->GenerateUniqueNamesForSelected(NULL, NULL, &objectNameUndoInfo, TRUE /* Store undo information only */);

		// Store the undo information
		pDoc->Modify(&objectNameUndoInfo, TRUE);
	}

	// Generate unique names for the objects
	CStringArray originalNameArray;
	CStringArray updatedNameArray;
	pRegion->GenerateUniqueNamesForSelected(&originalNameArray, &updatedNameArray);

	// Stores the property names that are changed and their original/updated values
	CStringArray propertyNameArray;
	CStringArray propertyOriginalValueArray;
	CStringArray propertyUpdatedValueArray;

	// Update the referencing properties for the objects
	if (bUpdateRefProps)
	{
		// Store the undo information
		if (bGenerateUndoInfo)
		{
			PreActionList propertyUndoInfo;

			// Start out by storing the undo information
			int i;
			for (i=0; i < originalNameArray.GetSize(); i++)
			{
				pRegion->UpdateObjectsReferenceProps(bUpdateSelPropsOnly, originalNameArray[i], updatedNameArray[i],
													 NULL, NULL, NULL, &propertyUndoInfo, TRUE /* Store undo information only */);
			}

			// Store the undo information
			pDoc->Modify(&propertyUndoInfo, TRUE);
		}

		// Update the object properties
		int i;
		for (i=0; i < originalNameArray.GetSize(); i++)
		{
			pRegion->UpdateObjectsReferenceProps(bUpdateSelPropsOnly, originalNameArray[i], updatedNameArray[i],
												 &propertyNameArray, &propertyOriginalValueArray, &propertyUpdatedValueArray);
		}
	}		

	// Stop showing the wait cursor
	EndWaitCursor();

	// Display a report if necessary
	if (bDisplayReport && originalNameArray.GetSize() > 0)
	{		
		CNameChangeReportDlg nameChangeReportDlg;
		nameChangeReportDlg.SetObjectNameArrays(&originalNameArray, &updatedNameArray);
		nameChangeReportDlg.SetPropertyNameArrays(&propertyNameArray, &propertyOriginalValueArray, &propertyUpdatedValueArray);

		// Display the dialog
		nameChangeReportDlg.DoModal();
	}
}

/************************************************************************/
// Generates unique names for the selected nodes
void CRegionView::OnSelectionGenerateUniqueNames() 
{	
	// Construct the dialog
	COptionsGenerateUniqueNames *pOptions=GetApp()->GetOptions().GetGenerateUniqueNamesOptions();

	CGenerateUniqueNamesDlg uniqueNamesDlg;
	uniqueNamesDlg.m_bUpdateRefProps=pOptions->GetUpdateRefProps();
	uniqueNamesDlg.m_bUpdateSelPropsOnly=pOptions->GetUpdateSelPropsOnly();
	uniqueNamesDlg.m_bDisplayReportOfChanges=pOptions->GetDisplayReportOfChanges();

	// Display the "Generate Unique Names" dialog
	if (uniqueNamesDlg.DoModal() == IDOK)
	{
		// Save the options
		pOptions->SetUpdateRefProps(uniqueNamesDlg.m_bUpdateRefProps);
		pOptions->SetUpdateSelPropsOnly(uniqueNamesDlg.m_bUpdateSelPropsOnly);
		pOptions->SetDisplayReportOfChanges(uniqueNamesDlg.m_bDisplayReportOfChanges);

		// Generate the unique names
		GenerateUniqueNamesForSelected(uniqueNamesDlg.m_bUpdateRefProps, uniqueNamesDlg.m_bUpdateSelPropsOnly, uniqueNamesDlg.m_bDisplayReportOfChanges);
				
		// Update the properties dialog
		GetRegionDoc()->SetupPropertiesDlg(FALSE);

		// Redraw all of the views
		GetRegionDoc()->RedrawAllViews();				
	}
}

/************************************************************************/
// Command update handler for the "generate unique names" command
void CRegionView::OnUpdateSelectionGenerateUniqueNames(CCmdUI* pCmdUI) 
{
	// talon: 2001 (0x45bfa4, the generate-names and save-prefab updates in one)
	if (!m_pRegion || m_pRegion->m_Selections.GetSize() < 1)
		pCmdUI->Enable(FALSE);
}

/************************************************************************/
// Save the selected nodes as a prefab file
void CRegionView::OnSelectionSavePrefab() 
{
	// talon: 2001 (0x45bfc4): a prefab is a .ed world.

	// Make sure that we have selected nodes
	if (!m_pRegion || m_pRegion->m_Selections.GetSize() < 1)
	{
		return;
	}

	CString sFileName;
	sFileName = GetProject()->m_BaseProjectDir + "\\Prefab\\*.ed";
	// Let the user choose the file to save to
	CFileDialog fileDlg(FALSE, "ed", (LPCTSTR)sFileName, OFN_OVERWRITEPROMPT, "DEdit World (*.ed)|*.ed||");

	// Display the dialog
	if (fileDlg.DoModal() != IDOK)
	{
		return;
	}

	// Create the region
	CEditRegion region;	

	// Don't check nodes in debug because it takes a very long time
	BOOL bCheckNodes=g_bCheckNodes;
	g_bCheckNodes=FALSE;

	// Paste the selected nodes into the new region
	PreActionList actionList;
	((CMainFrame *)AfxGetMainWnd( ))->GetClipboard( )->PasteNodes(&region, GetRegion()->m_Selections, actionList, TRUE);

	// Clear the action list
	GDeleteAndRemoveElements(actionList);

	// Clear the selections in the new region
	region.ClearSelections();

	// Reset this parameter
	g_bCheckNodes=bCheckNodes;

	// Open the file
	CMoFileIO OutFile;
	if (!OutFile.Open(fileDlg.GetPathName(), "wb"))
	{
		MessageBox("Error: Unable to open file", "Error", MB_OK);
		return;
	}

	region.SaveFile(OutFile, FALSE);
	OutFile.Close();

	// Update the project bar
	GetProjectBar()->UpdateAll();	
}

/************************************************************************/
// Command update handler for the "Save Prefab" option
void CRegionView::OnUpdateSelectionSavePrefab(CCmdUI* pCmdUI) 
{
	// talon: 2001 (0x45bfa4, the generate-names and save-prefab updates in one)
	if (!m_pRegion || m_pRegion->m_Selections.GetSize() < 1)
		pCmdUI->Enable(FALSE);
}


/************************************************************************/

/************************************************************************/
// This command opens up the object browser
void CRegionView::OnWorldObjectBrowser() 
{	
	// Get the region doc
	CRegionDoc *pDoc=GetRegionDoc();
	ASSERT(pDoc);

	// Get the region
	CEditRegion *pRegion=GetRegion();
	ASSERT(pRegion);

	// Create the object browser dialog
	CObjectBrowserDlg objectBrowserDlg;

	// Search the selected items and see if there is an object selected.
	// If there is, select it in the object browser
	int i;
	for (i=0; i < pRegion->GetNumSelections(); i++)
	{
		// Get the node
		CWorldNode *pNode=pRegion->GetSelection(i);

		// Check to see if the node is an object
		if (pNode->GetType() == Node_Object)
		{
			// Select the node in the browser
			objectBrowserDlg.SetSelectedObject(pNode->GetName());
			break;
		}
	}

	// Set whether or not to group the objects by type
	COptionsObjectBrowser *pOptions=GetApp()->GetOptions().GetObjectBrowserOptions();
	if (pOptions)
	{
		objectBrowserDlg.m_bGroupByType=pOptions->IsGroupByType();
	}

	// Display the browser dialog
	if (objectBrowserDlg.DoModal() == IDOK)
	{
		// Save whether or not to group the objects by type		
		if (pOptions)
		{
			pOptions->SetGroupByType(objectBrowserDlg.m_bGroupByType);
		}
	}
}

// talon: 2001 (0x45c49d).
void CRegionView::OnWorldDebugClearLeakFinder()
{
	m_pRegion->m_Unknown_b4.Term();
	GetRegionDoc()->RedrawAllViews();
}

// talon: 2001 (0x45c4c4).
void CRegionView::OnUpdateWorldDebugClearLeakFinder(CCmdUI* pCmdUI)
{
	if( m_pRegion && m_pRegion->m_Unknown_b4 > 0 )
		pCmdUI->Enable( TRUE );
	else
		pCmdUI->Enable( FALSE );
}

// talon: 2001 (0x45c4e8): puts the marker on the next leak vertex.
void CRegionView::OnWorldDebugGotoNextLeakVertice()
{
	if( m_pRegion && m_nCurLeakVertex+1 < m_pRegion->m_Unknown_b4 )
	{
		m_nCurLeakVertex++;
		CVector &vLeak = m_pRegion->m_Unknown_b4[m_nCurLeakVertex];
		m_pRegion->m_vMarker = vLeak;
		GetMainFrame()->CenterViewsAtVector( vLeak );
		GetRegionDoc()->RedrawAllViews();
	}
}

// talon: 2001 (0x45c547): puts the marker on the previous leak vertex.
void CRegionView::OnWorldDebugGotoPreviousLeakVertice()
{
	if( m_pRegion && m_nCurLeakVertex > 0 && m_pRegion->m_Unknown_b4 > 0 )
	{
		m_nCurLeakVertex--;
		CVector &vLeak = m_pRegion->m_Unknown_b4[m_nCurLeakVertex];
		m_pRegion->m_vMarker = vLeak;
		GetMainFrame()->CenterViewsAtVector( vLeak );
		GetRegionDoc()->RedrawAllViews();
	}
}

// talon: 2001's are empty (folded at 0x44ef03).
void CRegionView::OnUpdateWorldDebugGotoNextLeakVertice(CCmdUI* pCmdUI)
{
}

void CRegionView::OnUpdateWorldDebugGotoPreviousLeakVertice(CCmdUI* pCmdUI)
{
}

/************************************************************************/
// This function selects the nodes that have naming conflicts with
// other nodes in the world.
void CRegionView::OnWorldDebugFindNamingConflicts() 
{
	// Get the region
	CEditRegion *pRegion=GetRegion();
	if (!pRegion)
	{
		return;
	}

	// Display the wait cursor
	BeginWaitCursor();

	// Turn off the drawing of the node view
	if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
	{
		GetNodeView()->SetRedraw(FALSE);
	}

	// Clear the selections
	pRegion->ClearSelections();

	// Go through each object and see if they are unique
	int i;
	for (i=0; i < pRegion->m_Objects.GetSize(); i++)
	{
		// Get the object
		CBaseEditObj *pObject=pRegion->m_Objects[i];

		// See if the object is unique
		if (pRegion->FindObjectsByName(pObject->GetName(), NULL, 2) > 1)
		{
			// Select the object
			pRegion->SelectNode(pObject);

			// Make sure that the node is visible in the node view
			GetNodeView()->HighlightNode(pObject);
		}
	}

	// Turn off the drawing of the node view
	if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
	{
		GetNodeView()->SetRedraw(TRUE);
		GetNodeView()->Invalidate();
	}

	GetRegionDoc()->NotifySelectionChange();
	GetRegionDoc()->RedrawAllViews();

	// Stop displaying the wait cursor
	EndWaitCursor();
}


/************************************************************************/
// This function scales the world by an input amount
void CRegionView::OnWorldScaleGeometry()
{
	// Get the scaling factor from the user
	CStringDlg dlg;
	dlg.m_bAllowNumbers = TRUE;
	dlg.m_bAllowOthers = TRUE;
		
	float fScaleFactor = 0.0f;

	while (dlg.DoModal(IDS_SCALE_WORLD, IDS_SCALE_ENTER_VALUE) == IDOK)
	{
		fScaleFactor = (float)atof((LPCTSTR)dlg.m_EnteredText);
		if (!fScaleFactor)
			MessageBox("Please enter a non-zero floating point value.", "Invalid Value");
		else
			break;			
	}
	if (!fScaleFactor || (fScaleFactor == 1.0f))
		return;

	// Display the wait cursor
	BeginWaitCursor();

	// Flag the world as modified
	GetRegionDoc()->Modify();

	// Scale the world
	GetRegion()->ScaleBy(fScaleFactor);

	// Display the wait cursor
	EndWaitCursor();

	// Refresh the selection cube
	GetRegionDoc()->UpdateSelectionBox();
	// Update the properties dialog
	GetRegionDoc()->SetupPropertiesDlg( FALSE );

	// Redraw all of the views
	GetRegionDoc()->RedrawAllViews();
}


//given a list of polygons, it will back up all the nodes associated with them
//and set up the undo
void CRegionView::MakeUndoForPolygons(CMoArray<CEditPoly*>* pPolyList)
{
	ASSERT(pPolyList);

	// Setup the undo information 
	PreActionList actionList;	

	for(uint32 i=0; i < pPolyList->GetSize(); i++)
	{
		bool bMakeUndo = true;

		//make sure another poly doesn't already have this brush
		for(uint32 j = 0; j < i; j++)
		{
			if((*pPolyList)[j]->m_pBrush == (*pPolyList)[i]->m_pBrush)
			{
				bMakeUndo = false;
				break;
			}
		}

		if(bMakeUndo)
			actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, (*pPolyList)[i]->m_pBrush));

	}

	//register the undo
	GetRegionDoc()->Modify(&actionList, TRUE);
}


void CRegionView::OnMapTextureToView()
{
	DVector O, P, Q;
	DWORD i;
	CMoArray<CEditPoly*> polies;

	O.Init();
	P = ViewDef()->m_Nav.Right();
	Q = -ViewDef()->m_Nav.Up();

	polies.SetCacheSize(256);
	GetSelectedPolies(polies);

	MakeUndoForPolygons(&polies);


	//now that the undo is done, actually do the operation
	for(i = 0; i < polies.GetSize(); i++)
	{
		polies[i]->SetTextureSpace(GetCurrTexture(), O, P, Q);
	}

	GetRegionDoc()->RedrawAllViews();
}




void CRegionView::OnResetTextureCoords()
{
	// Setup the undo information 
	PreActionList actionList;	

	if((GetEditMode() == BRUSH_EDITMODE) || (GetEditMode() == GEOMETRY_EDITMODE))
	{
		DWORD i;
		CMoArray<CEditPoly*> polies;

		polies.SetCacheSize(256);
		GetSelectedPolies(polies);

		CEditPoly *pPoly;

		MakeUndoForPolygons(&polies);

		// If using brush mode
		if (GetEditMode() == BRUSH_EDITMODE)
		{
			// Use cubic mapping
			for(i=0; i < polies; i++)
			{
				polies[i]->SetupBaseTextureSpace(GetCurrTexture());
				polies[i]->GetTexture(GetCurrTexture()).SetO(LTVector(0.0f, 0.0f, 0.0f));
			}
		}
		else if (GetEditMode() == GEOMETRY_EDITMODE)
		{
			// If only one polygon is selected..
			if (polies.GetSize() < 2)
			{
				// Use the polygon orientations as the texture space
				pPoly = polies[0];
				CTexturedPlane& PolyTex = pPoly->GetTexture(GetCurrTexture());

				LTVector vP = pPoly->Pt(0) - pPoly->Pt(1);
				vP.Norm();

				LTVector vQ = vP.Cross(pPoly->Normal());
				vQ.Norm();

				PolyTex.SetO(pPoly->Pt(0));
				PolyTex.SetP(vP);
				PolyTex.SetQ(vQ);
			}
			else
			{
				// Otherwise use the texture space from the first polygon
				CVector cVectorO, cVectorP, cVectorQ;
				pPoly = polies[0];
				CTexturedPlane& PolyTex = pPoly->GetTexture(GetCurrTexture());

				cVectorO = PolyTex.GetO();
				cVectorP = PolyTex.GetP();
				cVectorQ = PolyTex.GetQ();
				for (i = 1; i < polies.GetSize(); i++)
				{
					pPoly = polies[i];
					pPoly->GetTexture(GetCurrTexture()).SetO(cVectorO);
					pPoly->GetTexture(GetCurrTexture()).SetP(cVectorP);
					pPoly->GetTexture(GetCurrTexture()).SetQ(cVectorQ);
				}
			}
		}
	}

	GetRegionDoc()->RedrawAllViews();
}


// talon: 2001 (0x45cddc..0x45ce12): the light animations kept with the
// world (CRegionDoc, 0x450c20..).
void CRegionView::OnLoadLightData()
{
	GetRegionDoc()->LoadLightAnims();
	GetRegionDoc()->RedrawAllViews();
}

void CRegionView::OnUnloadLightData()
{
	GetRegionDoc()->TermLightAnims();
	GetRegionDoc()->RedrawAllViews();
}

void CRegionView::OnSaveLightData()
{
	GetRegionDoc()->SaveLightAnims();
}

void CRegionView::OnUpdateLightData(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( GetRegionDoc()->m_pLightWorld != NULL );
}

BOOL CRegionView::OnDepthSelect(CUITracker *pTracker)
{
	CRVCallback *pRVCTracker = (CRVCallback *)pTracker;

	BOOL bResult = TRUE;

	if( GetEditMode() == GEOMETRY_EDITMODE )
	{
		// Toggle the poly tag.
		if(IPoly().IsValid())
		{
			BOOL bFound = FALSE;
			for(DWORD i=1; i < TaggedPolies(); i++)
			{
				if(TaggedPolies()[i] == IPoly())
				{
					TaggedPolies().Remove(i);
					bFound = TRUE;
					break;
				}
			}

			if(!bFound)
			{						
				TaggedPolies().Add(IPoly());
			}

			GetRegionDoc()->RedrawAllViews();
		}
	}
	else if(GetEditMode() == BRUSH_EDITMODE)
	{	
		CReal dist;

		// Subtree or single toggle the current brush.
		CBrushRef brushRef = GetMouseOverBrush( &dist, m_rSelectDist );
		if( !brushRef.IsValid() && m_rSelectDist > 0.0f )
		{
			// Start at beginning and try again...
			m_rSelectDist = 0.0f;
			brushRef = GetMouseOverBrush( &dist, m_rSelectDist );
		}
		  
		if( brushRef.IsValid( ))
		{
			m_rSelectDist = dist;
			GetNodeView( )->m_NodeViewTree.SelectTree( brushRef(), pRVCTracker->GetToggleState(0));
//					GetRegionDoc()->NotifySelectionChange();
		}
	}			
	else if( GetEditMode() == OBJECT_EDITMODE )
	{
		// Setup the closest object to be selected and in the property dialog.
		CObjectRef obj = GetClosestObject( GetCurMousePos(), FALSE );
		if( obj() )
		{
			GetNodeView( )->m_NodeViewTree.SelectTree( obj(), pRVCTracker->GetToggleState(0));
		}

		GetRegionDoc()->NotifySelectionChange();
	}
	else if( GetEditMode() == LIGHT_EDITMODE )
	{
		// talon: 2001 (0x45cfc4)
		PickLightPoly();
	}
	else
		bResult = FALSE;

	return bResult;
}

BOOL CRegionView::OnApplyTextureClick(CUITracker *pTracker)
{
	CRVCallback *pRVCTracker = (CRVCallback *)pTracker;

	BOOL bResult = TRUE;

	if( GetEditMode() == BRUSH_EDITMODE )
	{
		// Apply the currently selected texture to the brush that the user is clicking with the mouse
		CReal dist;
		
		// Get the brush that the user is clicking on
		CBrushRef brushRef = GetMouseOverBrush( &dist, m_rSelectDist );
		if( !brushRef.IsValid() && m_rSelectDist > 0.0f )
		{
			// Start at beginning and try again...
			m_rSelectDist = 0.0f;
			brushRef = GetMouseOverBrush( &dist, m_rSelectDist );
		}

		if( brushRef.IsValid( ))
		{
			// Select the brush
			m_rSelectDist = dist;
			GetNodeView( )->m_NodeViewTree.SelectTree( brushRef(), pRVCTracker->GetToggleState(0));

			// Apply the texture
			OnApplyTexture();
		}
	}
	else
		bResult = FALSE;

	return bResult;
}

BOOL CRegionView::OnSetGridOrientation(CUITracker *pTracker)
{
	if (!IsPerspectiveViewType())
		return FALSE;

	CRVCallback *pRVCTracker = (CRVCallback *)pTracker;

	switch (pRVCTracker->GetData())
	{
		case GRID_FORWARD : 
		{
			EditGrid().Pos().Init( 0.0f, 0.0f, 0.0f );
			EditGrid().Forward().Init( 0.0f, 1.0f, 0.0f );
			EditGrid().Up().Init( 0.0f, 0.0f, 1.0f );
			EditGrid().MakeRight();
			break;
		}

		case GRID_RIGHT :
		{
			EditGrid().Pos().Init( 0.0f, 0.0f, 0.0f );
			EditGrid().Forward().Init( 1.0f, 0.0f, 0.0f );
			EditGrid().Up().Init( 0.0f, 0.0f, 1.0f );
			EditGrid().MakeRight();
			break;
		}
	
		case GRID_UP :
		{
			EditGrid().Pos().Init( 0.0f, 0.0f, 0.0f );
			EditGrid().Forward().Init( 0.0f, 0.0f, 1.0f );
			EditGrid().Up().Init( 0.0f, 1.0f, 0.0f );
			EditGrid().MakeRight();
			break;
		}
	}

	//refresh the views
	GetRegionDoc()->RedrawAllViews();

	return TRUE;
}

BOOL CRegionView::OnInitTextureSpace(CUITracker *pTracker)
{
	BOOL bResult = TRUE;

	if ((GetEditMode() == BRUSH_EDITMODE) && (GetRegion()->m_Selections.GetSize()))
	{
		DWORD i, k;

		CWorldNode *pNode;
		CEditBrush *pBrush;

		//first setup an undo for all the brushes
		PreActionList actionList;

		for(i=0; i < GetRegion()->m_Selections; i++)
		{
			pNode = GetRegion()->m_Selections[i];
			if(pNode->GetType() == Node_Brush)
			{
				actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, pNode));
			}
		}

		GetRegionDoc()->Modify(&actionList, TRUE);

		// InitTextureSpace() on the selected brushes' polies.
		for( i=0; i < GetRegion()->m_Selections; i++ )
		{
			pNode = GetRegion()->m_Selections[i];

			if(pNode->GetType() == Node_Brush)
			{
				pBrush = pNode->AsBrush();
				
				for( k=0; k < pBrush->m_Polies; k++ )
					pBrush->m_Polies[k]->SetupBaseTextureSpace(GetCurrTexture());
			}
		}
		GetRegionDoc()->RedrawPerspectiveViews();
	}					
	else if ((GetEditMode() == GEOMETRY_EDITMODE) && (IPoly().IsValid()))
	{
		// InitTextureSpace() on the immediate polygon.
		GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_MODIFYNODE, IPoly().m_pBrush), TRUE);
		
		CEditPoly *pPoly = IPoly()();

		// Make sure that the current vertex index is within range
		if (m_nCurrentTextureVertIndex >= pPoly->m_Indices.GetSize())
		{
			m_nCurrentTextureVertIndex=0;
		}

		DWORD index=m_nCurrentTextureVertIndex;
		m_nCurrentTextureVertIndex++;					

		if(index != BAD_INDEX )
		{
			CTexturedPlane& PolyTex = pPoly->GetTexture(GetCurrTexture());

			CVector O = PolyTex.GetO();
			CVector P = PolyTex.GetP();
			CVector Q = PolyTex.GetQ();

			//determine if we need to flip the vectors in order to preserve
			//any mirroring on the texture. This is done by finding the expected
			//vector, and seeing if it matches up to the specified vector direction
			//wise. If it doesn't, we know we need to mirror it when we are done
			float fPDot = (Q.Cross(pPoly->Normal())).Dot(P);
			float fQDot = (P.Cross(pPoly->Normal())).Dot(Q);

			bool bMirrorP = (fPDot > 0.0f);
			bool bMirrorQ = bMirrorP ? (fQDot < 0.0f) : (fQDot > 0.0f);
			
			float pScale = (bMirrorP) ? -P.Mag() : P.Mag();
			float qScale = (bMirrorQ) ? -Q.Mag() : Q.Mag();

			O = pPoly->Pt(index);
			P = pPoly->NextPt(index) - O;
			Q = pPoly->Normal().Cross(P);

			P.Norm();
			P *= pScale;

			Q.Norm();
			Q *= qScale;

			pPoly->SetTextureSpace(GetCurrTexture(), O, P, Q);
		}
		
		GetRegionDoc()->RedrawPerspectiveViews();
	}					
	else
		bResult = FALSE;

	return bResult;
}

BOOL CRegionView::OnSelectNone(CUITracker *pTracker)
{
	BOOL bResult = TRUE;

	switch (GetEditMode())
	{
		case BRUSH_EDITMODE :
		case OBJECT_EDITMODE :
		{
			GetRegion()->ClearSelections();
			GetRegionDoc()->NotifySelectionChange();
			GetRegionDoc()->RedrawAllViews();
			break;
		}
		case GEOMETRY_EDITMODE :
		{
			// Untag all vertices.
			TaggedVerts().GenRemoveAll();
			ClearSelections();
			GetRegionDoc()->RedrawAllViews();
			break;
		}
		case LIGHT_EDITMODE :
		{
			// talon: 2001 (0x45d40b): unselects the lightmap poly.
			GetRegionDoc()->m_LightPolyWorld = (uint32)-1;
			GetRegionDoc()->m_LightPolyName = (uint32)-1;
			GetRegionDoc()->RedrawAllViews();
			break;
		}
		default :
			bResult = FALSE;
			break;
	}

	return bResult;
}

BOOL CRegionView::OnDelete(CUITracker *pTracker)
{
	BOOL bResult = TRUE;

	switch (GetEditMode())
	{
		case BRUSH_EDITMODE :
		case OBJECT_EDITMODE :
			DeleteSelectedNodes();
			break;
		case GEOMETRY_EDITMODE :
			DeleteTaggedVertices();
			break;
		default :
			bResult = FALSE;
	}

	return bResult;
}

BOOL CRegionView::OnNudge(CUITracker *pTracker)
{
	CRVCallback *pRVCTracker = (CRVCallback *)pTracker;

	BOOL bResult = TRUE;

	switch (GetEditMode())
	{
		case BRUSH_EDITMODE :
		case OBJECT_EDITMODE :
		{
			bResult = GetRegion()->m_Selections.GetSize() != 0;
			float fDistance = (pRVCTracker->GetToggleState(1)) ? 1.0f : (float)m_dwGridSpacing;
			switch (pRVCTracker->GetData())
			{
				case VK_LEFT:
					OffsetSelectedNodes(ViewDef()->m_Nav.m_Right*(float)fDistance*(-1));				
					break;
				case VK_RIGHT:
					OffsetSelectedNodes(ViewDef()->m_Nav.m_Right*(float)fDistance);
					break;
				case VK_UP:
					OffsetSelectedNodes(ViewDef()->m_Nav.m_Up*(float)fDistance);
					break;
				case VK_DOWN:
					OffsetSelectedNodes(ViewDef()->m_Nav.m_Up*(float)fDistance*(-1));
					break;
				default :
					bResult = FALSE;
					break;
			}
			// Update properties dialog...
			m_pView->GetRegionDoc()->SetupPropertiesDlg( FALSE );
			break;
		}
		case GEOMETRY_EDITMODE :
		{
			if (pRVCTracker->GetToggleState(0))
			{
				// Scale the texture
				switch (pRVCTracker->GetData())
				{
					case VK_LEFT:
						ScaleTextureForPoly(1 /* P direction */, 0 /* Q direction */);
						break;
					case VK_RIGHT:
						ScaleTextureForPoly(-1 /* P direction */, 0 /* Q direction */ );
						break;
					case VK_UP:
						ScaleTextureForPoly(0 /* P direction */, -1 /* Q direction */ );
						break;
					case VK_DOWN:
						ScaleTextureForPoly(0 /* P direction */,  1 /* Q direction */ );
						break;
					default :
						bResult = FALSE;
						break;
				}
			}
			else
			{
				BOOL bGrid = pRVCTracker->GetToggleState(1);
				// Move the texture
				switch (pRVCTracker->GetData())
				{
					case VK_LEFT:
						OffsetTextureForPoly(-1 /* P direction */, 0 /* Q direction */, bGrid );
						break;
					case VK_RIGHT:
						OffsetTextureForPoly(1 /* P direction */, 0 /* Q direction */, bGrid );
						break;
					case VK_UP:
						OffsetTextureForPoly(0 /* P direction */, -1 /* Q direction */, bGrid );
						break;
					case VK_DOWN:
						OffsetTextureForPoly(0 /* P direction */, 1 /* Q direction */, bGrid );
						break;
					default :
						bResult = FALSE;
						break;
				}
			}
			break;
		}
		default :
			bResult = FALSE;
			break;
	}

	return bResult;
}


BOOL CRegionView::OnSplitEdges(CUITracker *pTracker)
{
	if (GetEditMode() != GEOMETRY_EDITMODE)
		return FALSE;

	SplitEdges();

	return TRUE;
}

BOOL CRegionView::OnDeleteTaggedPolygons(CUITracker *pTracker)
{
	if (GetEditMode() != GEOMETRY_EDITMODE)
		return FALSE;

	DeleteTaggedPolygons();

	return TRUE;
}

BOOL CRegionView::OnDeleteEdges(CUITracker *pTracker)
{
	if (GetEditMode() != GEOMETRY_EDITMODE)
		return FALSE;

	DeleteTaggedEdges((BOOL)(((CRVCallback *)pTracker)->GetData()));

	return TRUE;
}

BOOL CRegionView::OnGridToPoly(CUITracker *pTracker)
{
	if (GetEditMode() != GEOMETRY_EDITMODE)
		return FALSE;

	if (!IPoly().IsValid())
		return FALSE;

	// Align the grid to the current polygon

	CEditPoly *pPoly = IPoly()();
					
	EditGrid().Right() = pPoly->Pt(1) - pPoly->Pt(0);
	EditGrid().Right().Norm();
	EditGrid().Forward() = pPoly->Normal();
	EditGrid().MakeUp();

	if( EditGrid().Forward().Dot(Nav().Pos() - EditGrid().Pos()) < 0.0f )
		EditGrid().Forward() = -EditGrid().Forward();

	EditGrid().Pos() = pPoly->Pt(0);
	
	GetRegionDoc()->RedrawAllViews();

	return TRUE;
}

BOOL CRegionView::OnTagPolyVerts(CUITracker *pTracker)
{
	GenListPos foundPos;

	if (GetEditMode() != GEOMETRY_EDITMODE)
		return FALSE;

	if (!IPoly().IsValid())
		return FALSE;

	CEditPoly *pPoly = IPoly()();
	DWORD i;

	CVertRef ref;

	// Tag the vertices of the immediate polygon
	for( i=0; i < pPoly->m_Indices; i++ )
	{
		ref.Init( IPoly().m_pBrush, pPoly->m_Indices[i] );
		
		if( !TaggedVerts().GenFindElement(ref, foundPos) )
			TaggedVerts().GenAppend( ref );
	}

	GetRegionDoc()->RedrawAllViews();

	return TRUE;
}

BOOL CRegionView::OnFlipNormal(CUITracker *pTracker)
{
	// Flip the brushes instead...
	if (GetEditMode() == BRUSH_EDITMODE)
	{
		OnBrushFlip();
		return TRUE;
	}

	if (GetEditMode() != GEOMETRY_EDITMODE)
		return FALSE;

	if (!IPoly().IsValid())
		return FALSE;

	// Flip the current polygon's normal.
	GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_MODIFYNODE, IPoly().m_pBrush), TRUE);

	IPoly()()->Flip();
	
	UpdateImmediateSelection();
	GetRegionDoc()->RedrawAllViews();

	return TRUE;
}

BOOL CRegionView::OnOrbitVertex(CUITracker *pTracker)
{
	if (GetEditMode() != GEOMETRY_EDITMODE)
		return FALSE;

	if( (!IVert().IsValid()) || (!IsPerspectiveViewType()) )
		return FALSE;

	// Center the orbit on the current vertex.
	// talon: 2001 (0x45db09): sets the look-at point and the distance.
	Nav().m_LookAt = IVert()();
	Nav().UpdateViewerDistance();

	return TRUE;
}

// talon: 2001 (0x45db67): picks the lightmap texel under the cursor.
BOOL CRegionView::OnSelectLightNode(CUITracker *pTracker)
{
	if (GetEditMode() != LIGHT_EDITMODE)
		return FALSE;

	SelectLightTexel();
	return TRUE;
}

// talon: 2001 (0x45db86): brightens (TRUE) or darkens the selected texels.
BOOL CRegionView::OnEditLightNode(CUITracker *pTracker)
{
	if (GetEditMode() != LIGHT_EDITMODE)
		return FALSE;

	GetRegionDoc()->BrightenTexels(((CRegionViewCommand*)pTracker)->GetData());
	return TRUE;
}



//***************************************************************************//
















//***************************************************************************//



/*************************************************************************/

































void CRegionView::OnMatchTextureCoords()
{
	//only allow this if we are in geometry mode and have more than one polygon
	//selected
	if((GetEditMode() == GEOMETRY_EDITMODE) && (TaggedPolies().GetSize() < 2))
	{
		return;
	}

	//we need to build up a list of polygons that we will be adjusting
	CMoArray<CEditPoly*> PolyList;

	PolyList.SetCacheSize(256);
	GetSelectedPolies(PolyList);

	//first setup an undo
	MakeUndoForPolygons(&PolyList);

	//we will be taking the settings from the first polygon
	CEditPoly* pSource = PolyList[0];

	//get the texture coordinates and size of this polygon
	LTVector vSrcO, vSrcP, vSrcQ;
	pSource->GetTexture(GetCurrTexture()).GetTextureSpace(vSrcO, vSrcP, vSrcQ);

	uint32 nSrcWid = pSource->GetTexture(GetCurrTexture()).GetBaseTextureWidth();
	uint32 nSrcHgt = pSource->GetTexture(GetCurrTexture()).GetBaseTextureHeight();

	//now for all the other polygons we want to set the texture space to match
	for(uint32 nCurrPoly = 1; nCurrPoly < PolyList.GetSize(); nCurrPoly++)
	{
		CEditPoly* pCurr = PolyList[nCurrPoly];

		//get the size of this texture
		uint32 nCurrWid = pCurr->GetTexture(GetCurrTexture()).GetBaseTextureWidth();
		uint32 nCurrHgt = pCurr->GetTexture(GetCurrTexture()).GetBaseTextureHeight();

		//find out what scale factor we are going to want to apply
		CReal fXScale = (CReal)nCurrWid / (CReal)nSrcWid;
		CReal fYScale = (CReal)nCurrHgt / (CReal)nSrcHgt;

		//setup the new texture space
		pCurr->SetTextureSpace(GetCurrTexture(), vSrcO, vSrcP * fXScale, vSrcQ * fYScale);
	}

	//redraw the textured views
	GetRegionDoc()->RedrawPerspectiveViews();
}




void CRegionView::OnHollowBrush()
{
	DWORD				i, j, k, l;

	CStringDlg			dlg;
	CWorldNode			*pParent;
	CReal				thickness;
	CPolyRef			polyRef;
	CVertRefArray		newVerts;

	CEditPoly			*pPoly;
	CEditBrush			*pBrush;
	CEditBrush			*pNewBrush;
	CEditPoly			*pNewPoly;
	CVector				brushPos;
	BOOL				bOk, bValidNumber;
	DMatrix mPlaneNormal;
	DVector vPlaneDist;
	DPlane tempPlane;


	// undo list
	PreActionList actionList;

	if( (GetRegion()->m_Selections == 1) && (GetRegion()->m_Selections[0]->GetType() == Node_Brush) )
	{
		dlg.m_bAllowNumbers = TRUE;

		bOk = TRUE;
		bValidNumber = FALSE;
		dlg.m_EnteredText.Format( "%d", 16 );
		while( bOk && !bValidNumber )
		{
			if( bOk = ( dlg.DoModal(IDS_HOLLOWBRUSHCAPTION, IDS_ENTERBRUSHTHICKNESS) == IDOK ))
			{
				thickness = (CReal)atoi( dlg.m_EnteredText );
				if( thickness != 0.0f )
				{
					bValidNumber = TRUE;
				}
				else
				{
					AppMessageBox( IDS_ERROR_BRUSH_THICKNESS, MB_OK );
				}
			}
		}

		if( bOk )
		{
			pParent = GetRegion()->AddNullNode(GetRegion()->m_Selections[0]->GetParent());
			pBrush = GetRegion()->m_Selections[0]->AsBrush();

			CEditVertArray cHullPoints;
			CMoArray<DPlane> cPlanes;

			// Calculate the points on the hull
			for ( i = 0; i < pBrush->m_Points.GetSize(); i++ )
			{
				CVector vHullPoint( pBrush->m_Points[i] );

				// Clear out the plane list
				cPlanes.RemoveAll();

				// Find three polygons which contain this index
				for ( j = 0; (j < pBrush->m_Polies.GetSize()) && (cPlanes.GetSize() < 3); j++ )
				{
					pPoly = pBrush->m_Polies[j];
					for ( k = 0; k < pPoly->m_Indices.GetSize(); k++ )
					{
						if ( pPoly->m_Indices[k] == i )
						{
							// Convert to a DPlane and extend the plane out.
							tempPlane.m_Normal = pPoly->m_Plane.m_Normal;
							tempPlane.m_Dist = pPoly->m_Plane.m_Dist + thickness;	// Move the plane out..
							cPlanes.Append(tempPlane);
							break;
						}
					}
				}

				// If enough planes were found, calculate the intersection
				if ( cPlanes.GetSize() >= 3 )
				{
					// Iterate through the plane combinations until a valid combination of 3 planes is found
					bOk = FALSE;
					for (j = 0; (j < cPlanes.GetSize() - 2) && (!bOk); j++)
					{
						for (k = j + 1; (k < cPlanes.GetSize() - 1) && (!bOk); k++)
						{
							for (l = k + 1; (l < cPlanes.GetSize()) && (!bOk); l++)
							{
								bOk = gr_IntersectPlanes(cPlanes[j], cPlanes[k], cPlanes[l], vHullPoint);
							}
						}
					}

					if (!bOk)
					{
						MessageBox("Please remove any neighboring coplanar polygons before performing this operation.");
						return;
					}
				}
				else if ( cPlanes.GetSize() != 0 )
				{
					// If this happens, the brush is invalid and shouldn't be processed.
					MessageBox("Invalid geometry has been detected on this brush.  Please clean up the geometry and try again.");
					return;
				}
				// If cPlanes.GetSize() == 0 then the vertex isn't on any of the polygons and
				//	can safely be ignored.

				// Append this point to the list
				cHullPoints.Append( vHullPoint );
			}

			for( i=0; i < pBrush->m_Polies; i++ )
			{
				pPoly = pBrush->m_Polies[i];

				// Setup a new brush and a new starting poly.
				pNewBrush = no_CreateNewBrush(GetRegion(), pParent);
				pNewPoly = new CEditPoly( pNewBrush );

				pNewBrush->m_Polies.Append( pNewPoly );
				for( j=0; j < pPoly->NumVerts(); j++ )
				{
					pNewPoly->m_Indices.Append( (WORD)j );
					pNewBrush->m_Points.Append( pPoly->Pt(j) );
				}

				pNewPoly->Normal() = pPoly->Normal();
				pNewPoly->Dist() = pPoly->Dist();

				// Copy texture attributes
				pNewPoly->CopyAttributes(pPoly);

				// Extrude.
				polyRef.Init( pNewBrush, 0 );
				if( ExtrudePoly( polyRef, newVerts, FALSE, FALSE /*flip all*/, TRUE /*perpendicular*/ ) )
				{
					// ExtrudePoly makes all the polies except the starting one
					// facing the correct direction.
					pNewPoly->Flip();

					for( j=0; j < newVerts; j++ )
					{
						newVerts[j]() = cHullPoints[pPoly->m_Indices[j]];
					}

					for( j=0; j < pNewBrush->m_Polies; j++ )
					{
						if ( thickness < 0.0f )						
						{
							pNewBrush->m_Polies[i]->Flip();
						}
						pNewBrush->m_Polies[j]->UpdatePlane();								
						
						// Setup the base texture space
						for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
						{
							pNewBrush->m_Polies[j]->SetupBaseTextureSpace(nCurrTex);
						}
					}
				}

				// Clean up this brush
				CReal normalThresh = 0.05f;
				CReal distThresh = 0.0001f;
				pNewBrush->RemoveExtraEdges( normalThresh, distThresh );
				m_pRegion->UpdateBrushGeometry(pNewBrush);

				// Add to the undo list
				actionList.AddTail(new CPreAction(ACTION_ADDEDNODE, pNewBrush));
			}

			// Add the final brushes to the undo list
			GetRegionDoc()->Modify(&actionList, TRUE);

			DeleteSelectedNodes( );

			GetNodeView( )->UpdateNodeImage( GetRegion( )->GetRootNode( ));
			GetRegionDoc()->NotifySelectionChange();
		}
	}
	else
	{
		AppMessageBox( IDS_INVALIDHOLLOW, MB_OK );
	}
}

/************************************************************************/
// Handler for the carve command
void CRegionView::OnBrushCarve() 
{
	if (GetEditMode() != BRUSH_EDITMODE)
		return;

	CEditRegion *pRegion=GetRegion();

	// Make sure that we have one brush selected
	if((pRegion->GetNumSelections() < 1) || (pRegion->GetSelection(0)->GetType() != Node_Brush))
		return;

	//TODO: re-implement carve
	//DoCarveOperation();	
}

/************************************************************************/
// The command update handler for the carve command
void CRegionView::OnUpdateBrushCarve(CCmdUI* pCmdUI) 
{	
	// Get the edit region
	CEditRegion *pRegion=GetRegion();

	// Make sure that we have one brush selected
	if(pRegion->GetNumSelections() == 1 && pRegion->GetSelection(0)->GetType() == Node_Brush)
	{
		// Enable the menu item
		pCmdUI->Enable(TRUE);		
	}
	else
	{
		// Disable the menu item
		pCmdUI->Enable(FALSE);
	}
}
