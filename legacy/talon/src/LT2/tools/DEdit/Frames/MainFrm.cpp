//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// MainFrm.cpp : implementation of the CMainFrame class
//

#include "bdefs.h"
#include "dedit.h"

#include "mainfrm.h"
#include "regiondoc.h"
#include "splash.h"

#include "projectbar.h"
#include "regionview.h"
#include "regionframe.h"
#include "regmgr.h"

#include "resource.h"
#include "optionssheet.h"

#include "advancedselect.h"
#include "advancedselectdlg.h"
#include "sysstreamsim.h"
#include "d_filemgr.h"
#include <math.h>
#include "paletteedit.h"
#include "preferencesdlg.h"
#include "rezbuilddlg.h"
#include "dedit_concommand.h"
#include "texture.h"
#include <dos.h>
#include <direct.h>
#include <io.h>

#include "optionsrun.h"
#include "propertiesdlg.h"
#include "classlist.h"
#include "texturedlg.h"
#include "modeldlg.h"
#include "worldsdlg.h"
#include "sounddlg.h"
#include "spritedlg.h"
#include "prefabdlg.h"
#include "optionswindows.h"
#include "texturedlg.h"

//for the Rez file creation

//for model rendering
#include "eventnames.h"

//for updating world objects



extern CMainFrame *g_pMainFrame;

// Defines
#define MODE_MENU_INSERT_POS	8	// The menu position to insert the mode menus at
#define MODE_MENU_BRUSH_INDEX	0	// The index to the brush menu in the IDR_MAINFRAME_MODE_MENUS menu

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMRCMDIFrameWndSizeDock)

// talon: the 2001 commands whose ids Build 69's resource.h gives other
// names or none (DEdit.rc's menus: "Group World Textures" 32954, "File
// preferences" 32811, "Light Editing Mode" 40087, "Center views on the
// marker" 40058).  The names are ours.
#define ID_MAINFRM_GROUP_WORLD_TEXTURES	32954
#define ID_MAINFRM_FILE_PREFERENCES		32811
#define ID_MAINFRM_LIGHT_EDITMODE		40087

// talon: 2001's message map (.rdata 0xd8430..0xd8970)
BEGIN_MESSAGE_MAP(CMainFrame, CMRCMDIFrameWndSizeDock)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_COMMAND_RANGE(ID_SELECT_TAB_WORLDSVIEW, ID_SELECT_TAB_CLASSVIEW, OnSelectTab)
	ON_COMMAND(ID_RUN_WORLD, OnRunWorld)
	ON_UPDATE_COMMAND_UI(ID_RUN_WORLD, OnUpdateRunWorld)
	ON_COMMAND(ID_SHOW_WORLD_TEXTURES, OnShowWorldTextures)
	ON_COMMAND(ID_SHOW_WORLD_CLASSES, OnShowWorldClasses)
	ON_COMMAND(ID_MAINFRM_GROUP_WORLD_TEXTURES, OnGroupWorldTextures)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DEBUGWINDOW, OnUpdateViewDebugWindow)
	ON_COMMAND(ID_VIEW_DEBUGWINDOW, OnViewDebugWindow)
	ON_UPDATE_COMMAND_UI(ID_SINGLE_NODE_SELECTION, OnUpdateSingleNodeSelection)
	ON_UPDATE_COMMAND_UI(ID_MULTI_NODE_SELECTION, OnUpdateMultiNodeSelection)
	ON_COMMAND(ID_SINGLE_NODE_SELECTION, OnSingleNodeSelection)
	ON_COMMAND(ID_MULTI_NODE_SELECTION, OnMultiNodeSelection)
	ON_COMMAND(ID_FILE_CREATEREZFILE, OnCreateRezFile)
	ON_COMMAND(ID_FILE_RELOADPALETTEDAT, OnReloadPalettes)
	ON_COMMAND(ID_FILE_CLOSE, OnClose)
	ON_UPDATE_COMMAND_UI(ID_FILE_CREATEREZFILE, OnUpdateProjectOpen)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_PROJECT_WINDOW, OnViewProjectWindow)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PROJECT_WINDOW, OnUpdateViewProjectWindow)
	ON_COMMAND(ID_MAINFRM_FILE_PREFERENCES, OnFilePreferences)
	ON_COMMAND(ID_VIEW_COLORSELECTION, OnViewColorSelection)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COLORSELECTION, OnUpdateViewColorSelection)
	ON_WM_DESTROY()
	ON_COMMAND(ID_GEOMETRY_EDITMODE, OnGeometryEditMode)
	ON_UPDATE_COMMAND_UI(ID_GEOMETRY_EDITMODE, OnUpdateGeometryEditMode)
	ON_COMMAND(ID_OBJECT_EDITMODE, OnObjectEditMode)
	ON_UPDATE_COMMAND_UI(ID_OBJECT_EDITMODE, OnUpdateObjectEditMode)
	ON_COMMAND(ID_BRUSH_EDITMODE, OnBrushEditMode)
	ON_UPDATE_COMMAND_UI(ID_BRUSH_EDITMODE, OnUpdateBrushEditMode)
	ON_UPDATE_COMMAND_UI(ID_MAINFRM_LIGHT_EDITMODE, OnUpdateLightEditMode)
	ON_COMMAND(ID_CENTERONSELECTION, OnCenterOnSelection)
	ON_UPDATE_COMMAND_UI(ID_CENTERONSELECTION, OnUpdateCenterOnSelection)
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_VIEW_TOOLTIPS, OnViewTooltips)
	ON_UPDATE_COMMAND_UI(ID_VIEW_TOOLTIPS, OnUpdateViewTooltips)
	ON_COMMAND(ID_EDIT_OPTIONS, OnOptions)
	ON_COMMAND(ID_EDIT_PALETTE, OnPaletteEdit )
	ON_UPDATE_COMMAND_UI(ID_WINDOW_NEXT, OnUpdateMDINext)
	ON_COMMAND(ID_TOGGLE_EDIT_MODE, OnToggleEditMode)
	ON_COMMAND(ID_CENTER_ON_MARKER, OnCenterOnMarker)
	ON_UPDATE_COMMAND_UI(ID_CENTER_ON_MARKER, OnUpdateCenterOnMarker)
	ON_UPDATE_COMMAND_UI(ID_FILE_RELOADPALETTEDAT, OnUpdateProjectOpen)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSE, OnUpdateProjectOpen)
	ON_COMMAND_EX(CG_ID_VIEW_PROJECTBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(CG_ID_VIEW_PROJECTBAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(CG_ID_VIEW_CREZBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(CG_ID_VIEW_CREZBAR, OnUpdateControlBarMenu)
	ON_UPDATE_COMMAND_UI(IDW_NODEVIEW_TOOLBAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(IDW_NODEVIEW_TOOLBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(IDW_WORLDEDIT_TOOLBAR, OnUpdateControlBarMenu)
	ON_COMMAND_EX(IDW_WORLDEDIT_TOOLBAR, OnBarCheck)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PALETTE, OnUpdateProjectOpen)
	ON_COMMAND(ID_WINDOW_NEXT,MDINext)
	ON_COMMAND(ID_VIEW_TOGGLE_FLOATING_TOOLBARS, OnViewToggleFloatingToolbars)
	ON_COMMAND(ID_MAINFRM_LIGHT_EDITMODE, OnLightEditMode)
	//}}AFX_MSG_MAP
	ON_WM_CLOSE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	// talon: 2001's seven (0x4f8964): no brush name or selection panes
	ID_INDICATOR_TEXTURE,
	ID_INDICATOR_VIEWTYPE,
	ID_INDICATOR_GRID,
	ID_INDICATOR_CAMERA,
	ID_INDICATOR_CURSOR,
	ID_INDICATOR_BOUNDING_BOX
};



/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	g_pMainFrame = this;

	m_pProjectBar = new CProjectBar;
	m_pPropertiesDlg = new CPropertiesDlg;
	m_pNodeView = new CNodeView;
	m_pClassListDlg = new ClassListDlg;
	m_pTextureDlg = new CTextureDlg;
	m_pModelDlg = new CModelDlg;
	m_pWorldsDlg = new CWorldsDlg;
	m_pSoundDlg = new CSoundDlg;
	m_pSpriteDlg = new CSpriteDlg;
	m_pPrefabDlg = new CPrefabDlg;

	m_WorldEditMode = BRUSH_EDITMODE;
	m_NodeSelectionMode = SINGLE_SELECTION;
	m_bShown = FALSE;
	m_bToolTips = TRUE;
	m_bClosing = FALSE;

	m_projectControls.SetSize(0);
}

CMainFrame::~CMainFrame()
{
}

//------------------------------------------------------------------------------------
//
//  CMainFrame::OnCreate
//
//  Purpose:	Creates Main Frame.
//
//------------------------------------------------------------------------------------
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CString			str;
	CFlatToolbar		*pNV = GetNodeViewToolbar();
	CFlatToolbar		*pWE = GetWorldEditToolbar();
	
	
	if (CMRCMDIFrameWndSizeDock::OnCreate(lpCreateStruct) == -1)
		return -1;

	//SCHLEGZ 12/31/97 New toolbars
	// Setup the standard toolbar...

	if (!m_wndMainBar.Create(this, /*WS_CHILD | WS_VISIBLE | CBRS_SIZE_DYNAMIC |*/
		CBRS_TOP, AFX_IDW_TOOLBAR ) ||
		!m_wndMainBar.LoadToolBar( IDR_MAINFRAME ))
	{
		TRACE0("Failed to create main toolbar\n");
		return -1;      // fail to create
	}
	// talon: 2001 loads string 61446 (0x42e4ab), "an unnamed file" in
	// DEdit.rc, not IDS_STANDARD_TOOLBAR's "Standard" (61448): retail.
	str.LoadString( 61446 );
	m_wndMainBar.SetWindowText( str );
	m_wndMainBar.EnableDocking( CBRS_ALIGN_ANY );


	// Setup the Node View toolbar.
	if( !pNV->Create( this, /*WS_CHILD | WS_VISIBLE | CBRS_SIZE_DYNAMIC |*/
		CBRS_TOP, IDW_NODEVIEW_TOOLBAR ) ||
		!pNV->LoadToolBar( IDR_NODEVIEW_TOOLBAR ))
	{
		TRACE0("Failed to create Node View toolbar\n");
		return -1;
	}

	str.LoadString( IDS_NODEVIEWTOOLBAR_TEXT );
	pNV->SetWindowText( str );
	pNV->EnableDocking( CBRS_ALIGN_ANY );

	pNV->SetButtonStyle( 0, TBBS_GROUP );
	pNV->SetButtonStyle( 1, TBBS_GROUP );
//	pNV->SetButtonStyle( 2, TBBS_GROUP );
//	pNV->SetButtonStyle( 3, TBBS_GROUP );


	// Setup the world editing toolbar.
	if( !pWE->Create(this, /*WS_CHILD | WS_VISIBLE | CBRS_SIZE_DYNAMIC |
			*/CBRS_TOP, IDW_WORLDEDIT_TOOLBAR ) ||
			!pWE->LoadToolBar( IDR_WORLDEDIT_TOOLBAR ))
	{
		TRACE0("Failed to create world edit toolbar\n");
		return -1;
	}

	str.LoadString( IDS_WORLDEDITTOOLBAR_TEXT );
	pWE->SetWindowText( str );
	pWE->EnableDocking( CBRS_ALIGN_ANY );

	pWE->SetButtonStyle( 0, TBBS_GROUP );
	pWE->SetButtonStyle( 1, TBBS_GROUP );
	pWE->SetButtonStyle( 2, TBBS_GROUP );


	// Setup status bar...

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	UINT nPaneID, nPaneStyle;
	int nPaneWidth, nPaneIndex;
	CString szStatusText;
	CSize szExtent;

	// Get a dc and a font...
	CClientDC dc( &m_wndStatusBar );
	CFont* pOldFont = dc.SelectObject( m_wndStatusBar.GetFont( ));

	// Setup the message pane...
    m_wndStatusBar.SetPaneInfo( 0, m_wndStatusBar.GetItemID( 0 ),
           SBPS_STRETCH, NULL );
	
	// Setup the current selected texture...
	nPaneIndex = m_wndStatusBar.CommandToIndex( ID_INDICATOR_TEXTURE );
	szStatusText.LoadString( IDS_NOSELTEXTURE );
	szExtent = dc.GetTextExtent( str );
	m_wndStatusBar.GetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, nPaneWidth );
	m_wndStatusBar.SetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, szExtent.cx );

	// Setup the view type pane...
	nPaneIndex = m_wndStatusBar.CommandToIndex( ID_INDICATOR_VIEWTYPE );
	szStatusText.LoadString( IDS_VIEW_PERSP );
	szExtent = dc.GetTextExtent( str );
	m_wndStatusBar.GetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, nPaneWidth );
	m_wndStatusBar.SetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, szExtent.cx );

	// Setup the grid pane...
	nPaneIndex = m_wndStatusBar.CommandToIndex( ID_INDICATOR_GRID );
	szStatusText.LoadString( IDS_GRID );
	szStatusText += "0000";
	szExtent = dc.GetTextExtent( szStatusText );
	m_wndStatusBar.GetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, nPaneWidth );
	m_wndStatusBar.SetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, szExtent.cx );

	// Setup the camera pane...
	nPaneIndex = m_wndStatusBar.CommandToIndex( ID_INDICATOR_CAMERA );
	szStatusText = _T( "(-1.0,-1.0,-1.0)@(-9999,-9999,-9999)" );
	szExtent = dc.GetTextExtent( szStatusText );
	m_wndStatusBar.GetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, nPaneWidth );
	m_wndStatusBar.SetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, szExtent.cx );

	// Setup the cursor pane...
	nPaneIndex = m_wndStatusBar.CommandToIndex( ID_INDICATOR_CURSOR );
	szStatusText = _T( "@(-9999,-9999,-9999)" );
	szExtent = dc.GetTextExtent( szStatusText );
	m_wndStatusBar.GetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, nPaneWidth );
	m_wndStatusBar.SetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, szExtent.cx );

	// Setup the bounding box pane
	nPaneIndex = m_wndStatusBar.CommandToIndex( ID_INDICATOR_BOUNDING_BOX );
	szStatusText = _T("(-9999, -9999, -9999)");
	szExtent = dc.GetTextExtent( szStatusText );
	m_wndStatusBar.GetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, nPaneWidth );
	m_wndStatusBar.SetPaneInfo( nPaneIndex, nPaneID, nPaneStyle, szExtent.cx );

	// Restore the dc's old font...
	dc.SelectObject(pOldFont);

	
	// Setup the color dialog...
	if( !GetColorSelectDlg()->Create(IDD_COLORSELECT_DLG, this) )
	{
		TRACE0("Failed to create the color selection dialog.");
		return -1;
	}
	GetColorSelectDlg()->ShowWindow( SW_HIDE );

	// Create Project Bar...
	InitProjectBar();

	// Restore registry settings for frame and components...
	RestoreRegistryInfo( );

	// Restore registry settings for project bar...
	GetProjectBar( )->RestoreRegistryInfo( _T( "General" ));

	// Dock everything...
	EnableDocking(CBRS_ALIGN_ANY, RUNTIME_CLASS(CDEDockFrameWnd));
	DockControlBar( &m_wndMainBar, AFX_IDW_DOCKBAR_TOP );
	DockControlBar( &m_NodeViewToolbar, AFX_IDW_DOCKBAR_TOP );
	DockControlBar( &m_WorldEditToolbar, AFX_IDW_DOCKBAR_TOP );
	DockControlBar( m_pProjectBar, AFX_IDW_DOCKBAR_LEFT );
	
	// Get the window options
	COptionsWindows *pOptions=GetApp()->GetOptions().GetWindowsOptions();

	// Create the project bars	
	int32 i;
	for (i=0; i < pOptions->GetNumMainControls(); i++)
	{
		CMRCSizeDialogBar *pControlBar=CreateProjectControlBar(pOptions->GetMainControl(i));

		// Make sure that the window gets properly sized
		pControlBar->RepositionControls();
	}	

	// Load toolbar states from registry...
	LoadBarState(_T("General"));
	LoadSizeBarState(_T("General"));

	// Make sure that all of our control bars are visible
	// (2001, 0x42e98b: after the bar states are loaded)
	for (i=0; i < m_projectControls.GetSize(); i++)
	{
		ShowControlBar( m_projectControls[i]->m_pWnd, TRUE, FALSE );
	}

	CSplashWnd::ShowSplashScreen(this);

	return 0;
}

/************************************************************************/
// Call this to add a project control info structure based on the
// control type.
// Returns: pointer to the allocated control info structure.
CProjectControlBarInfo *CMainFrame::AddProjectControlInfo(CMoArray<CProjectControlBarInfo *> &controlArray,
														  CMainFrame::ProjectControl controlType)
{
	// The tab info structure
	CProjectControlBarInfo *pControlInfo=NULL;

	// Fill in the tab info structure with the appropriate data
	switch (controlType)
	{
	case CB_WORLDSVIEW:		
			pControlInfo = new CProjectControlBarInfo(GetWorldsDlg(), IDS_WORLDS, IDD_WORLDS_TABDLG, ID_SELECT_TAB_WORLDSVIEW, controlType);
			break;		
	case CB_TEXTUREVIEW:		
			pControlInfo = new CProjectControlBarInfo(GetTextureDlg(), IDS_TEXTURES, IDD_TEXTURE_TABDLG, ID_SELECT_TAB_TEXTUREVIEW, controlType);	
			break;		
	case CB_NODESVIEW:		
			pControlInfo = new CProjectControlBarInfo(GetNodeView(), IDS_NODES, IDD_NODEVIEW_TABDLG, ID_SELECT_TAB_NODESVIEW, controlType);	
			break;		
	case CB_PROPERTIESVIEW:		
			pControlInfo = new CProjectControlBarInfo(GetPropertiesDlg(), IDS_PROPERTIES, IDD_PROPERTIES, ID_SELECT_TAB_PROPERTIESVIEW, controlType);
			break;		
	case CB_MODELVIEW:		
			pControlInfo = new CProjectControlBarInfo(GetModelDlg(), IDS_MODELS, IDD_MODEL_TABDLG, ID_SELECT_TAB_MODELVIEW, controlType);
			break;		
	case CB_SOUNDVIEW:		
			pControlInfo = new CProjectControlBarInfo(GetSoundDlg(), IDS_SOUNDS, IDD_SOUND_TABDLG, ID_SELECT_TAB_SOUNDVIEW, controlType);
			break;	
	case CB_SPRITEVIEW:
			pControlInfo = new CProjectControlBarInfo(GetSpriteDlg(), IDS_SPRITES, IDD_SPRITE_TABDLG, ID_SELECT_TAB_SPRITEVIEW, controlType);	
			break;		
	case CB_CLASSVIEW:		
			pControlInfo = new CProjectControlBarInfo(GetClassListDlg(), IDS_CLASSLIST, IDD_CLASSES_TABDLG, ID_SELECT_TAB_CLASSVIEW, controlType);
			break;
	case CB_PREFABVIEW:
			// talon: 2001's prefab tab command is 33009 (0x42ebd5)
			pControlInfo = new CProjectControlBarInfo(GetPrefabDlg(), IDS_PREFABS, IDD_PREFAB_TABDLG, 33009, controlType);
			break;
	}

	ASSERT(pControlInfo);

	// Add the tab info
	if (pControlInfo)
	{
		controlArray.Append(pControlInfo);
	}

	// Return the tab info
	return pControlInfo;
}

/************************************************************************/
// This creates a project bar window (from the control type) and returns
// a pointer to the created window.  Note that it doesn't allocate a new
// CMRCSizeDialogBar, it is just returning a pointer to the window that
// is assocated with the control type.
CMRCSizeDialogBar *CMainFrame::CreateProjectControlBar(CMainFrame::ProjectControl controlType)
{
	// Make sure that this control type hasn't already been created
	int i;
#ifdef _DEBUG
	for (i=0; i < m_projectControls.GetSize(); i++)
	{
		ASSERT(m_projectControls[i]->m_ControlType != controlType);
	}
#endif

	// Add the project control info
	CProjectControlBarInfo *pInfo=AddProjectControlInfo(m_projectControls, controlType);

	// Get the window
	CMRCSizeDialogBar *pBar=pInfo->m_pWnd;

	// Create a new control bar window and attach it to the mainframe
	pBar->Create(this, pInfo->m_DialogID, CBRS_RIGHT, pInfo->m_SelectID);
	pBar->ShowWindow(SW_SHOW);
	pBar->EnableDocking( CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT );	

	// Set the title for the control bar
	CString sTitle;
	sTitle.LoadString(pInfo->m_TitleStringID);
	pBar->SetWindowText(sTitle);

	return pBar;
}

/************************************************************************/
// This destroys a project bar window and returns a pointer to the
// destroyed window.
CMRCSizeDialogBar *CMainFrame::DestroyProjectControlBar(CMainFrame::ProjectControl controlType)
{
	// Find the window
	CMRCSizeDialogBar *pWnd=NULL;

	int i;
	for (i=0; i < m_projectControls.GetSize(); i++)
	{
		if (m_projectControls[i]->m_ControlType == controlType)
		{
			// Store the window
			pWnd=m_projectControls[i]->m_pWnd;

			// Delete the control from the array
			delete m_projectControls[i];
			m_projectControls.Remove(i);

			break;
		}
	}

	ASSERT(pWnd);
	if (pWnd)
	{
		pWnd->DestroyWindow();
	}

	// Save the settings in the registry
	GetApp()->GetOptions().GetWindowsOptions()->SetMainControlArray(m_projectControls);

	return pWnd;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	UpdateStatusText
//
//	PURPOSE:	Central function for updating status bar text strings
//
// ----------------------------------------------------------------------- //

void CMainFrame::UpdateStatusText(int nIndex, const char* szText)
{
	UINT nID=0,nStyle=0;
	int nWidth=0;
	CSize size;

	int nPane = m_wndStatusBar.CommandToIndex(nIndex);

	m_wndStatusBar.GetPaneInfo(nPane,nID,nStyle,nWidth);
	{//compute size of status string
		HGDIOBJ hOldFont = NULL;
		HFONT hFont = (HFONT)m_wndStatusBar.SendMessage(WM_GETFONT);
		CClientDC dc(NULL);
		if (hFont != NULL) 
			hOldFont = dc.SelectObject(hFont);
		size = dc.GetTextExtent(szText,strlen(szText));
		if (hOldFont != NULL) 
			dc.SelectObject(hOldFont);
	}
	m_wndStatusBar.SetPaneInfo(nPane,nID,nStyle,size.cx);
	m_wndStatusBar.SetPaneText(nPane,szText,TRUE);

	return;
}

//--------------------------------------------------------------------------------
//
//  CMainFrame::DockControlBarRightOf
//
//  Purpose:	Puts Bar to the right of RightOf.
//
//--------------------------------------------------------------------------------


//-----------------------------------------------------------------------------
//
//  Registry data
//
//-----------------------------------------------------------------------------
static TCHAR szWindowFormat[] = _T("%u,%u,%d,%d,%d,%d,%d,%d,%d,%d");
static TCHAR szColorDlgFormat[] = _T( "%d %d %d %d %d" );
extern TCHAR szRegKeyCompany[];
extern TCHAR szRegKeyApp[];
extern TCHAR szRegKeyVer[];
static TCHAR szMainWindow[] = _T( "MainWindow" );
static TCHAR szColorSelectDlg[] = _T( "ColorSelectDlg" );
static TCHAR szToolTips[] = _T( "ToolTips" );
static TCHAR szDScriptEditor[] = _T( "DScriptEditor" );		// 2001
static TCHAR szUndoPreferences[] = _T("Undo Preferences");
static TCHAR szUndoPreferencesFormat[] = _T("%d %d %d %d");

//-----------------------------------------------------------------------------
//
//  CMainFrame::RestoreRegistryInfo()
//
//  Purpose:	Reads registry for window placement and toolbar info.
//
//-----------------------------------------------------------------------------
void CMainFrame::RestoreRegistryInfo()
{
	CRegMgr			regMgr;
	UINT32			bufSize;
	char			str[501];
	TCHAR			temp[256];

	if( regMgr.Init( szRegKeyCompany, szRegKeyApp, szRegKeyVer, NULL, HKEY_CURRENT_USER ))
	{
		// Place the main window.
		bufSize=500;
		if( regMgr.Get( szMainWindow, str, bufSize) )
		{
			WINDOWPLACEMENT wp;
			int nRead = _stscanf(str, szWindowFormat,
				&wp.flags, &wp.showCmd,
				&wp.ptMinPosition.x, &wp.ptMinPosition.y,
				&wp.ptMaxPosition.x, &wp.ptMaxPosition.y,
				&wp.rcNormalPosition.left, &wp.rcNormalPosition.top,
				&wp.rcNormalPosition.right, &wp.rcNormalPosition.bottom);

			if( nRead == 10 )
			{
				wp.length = sizeof wp;
				GetMainFrame( )->SetWindowPlacement( &wp );
			}
		}

		// Get tooltips flag...
		if( regMgr.Get( szToolTips, str, bufSize ))
		{
			_stscanf( str, "%u", &m_bToolTips );
		}

		// Set the tooltips flags...
		SetToolTips( );

		// Place the color dialog.
		bufSize=500;
		if( regMgr.Get( szColorSelectDlg, str, bufSize) )
		{
			BOOL bVisible;
			int left, top, width, height;

			sscanf( str, szColorDlgFormat, &bVisible, &left, &top, &width, &height );

			GetColorSelectDlg()->MoveWindow( left, top, width, height );
			GetColorSelectDlg()->ShowWindow( bVisible ? SW_SHOW : SW_HIDE );
		}

		// 2001 (0x42ef86): the DScript editor, or the default.
		// talon: string 32797 isn't in DEdit.rc's string table, so the
		// default is empty (retail).
		bufSize=256;
		if( regMgr.Get( szDScriptEditor, temp, bufSize ) )
			m_DScriptEditor = temp;
		else
			m_DScriptEditor.LoadString( 32797 );
	
		// Get the undo preferences...
		bufSize=256;
		SetUndoPreferencesDefault( m_iUndoGeometry, m_iUndoTextures, m_iUndoProperties, m_iUndoGeneral );
		SetUndoPreferencesDefault( m_iCurUndoGeometry, m_iCurUndoTextures, m_iCurUndoProperties, m_iCurUndoGeneral );
		if( regMgr.Get( szUndoPreferences, str, bufSize) )
		{
			sscanf( str, szUndoPreferencesFormat, &m_iUndoGeometry, &m_iUndoTextures, 
				&m_iUndoProperties, &m_iUndoGeneral );
		}
	
	}	
}

//-----------------------------------------------------------------------------
//
//  CMainFrame::SaveRegistryInfo()
//
//  Purpose:	Saves window placement and toolbar info to registry.
//
//-----------------------------------------------------------------------------
void CMainFrame::SaveRegistryInfo()
{
	CRegMgr				regMgr;
	char				keyVal[501];
	CRect				rect;


	if( regMgr.Init( szRegKeyCompany, szRegKeyApp, szRegKeyVer, NULL, HKEY_CURRENT_USER ))
	{
		// Window position.
		WINDOWPLACEMENT wp;
		wp.length = sizeof wp;
		if (GetMainFrame( )->GetWindowPlacement( &wp ))
		{
			wp.flags = 0;
			if( GetMainFrame( )->IsZoomed( ))
				wp.flags |= WPF_RESTORETOMAXIMIZED;
			// and write it to the .INI file
			wsprintf(keyVal, szWindowFormat,
				wp.flags, wp.showCmd,
				wp.ptMinPosition.x, wp.ptMinPosition.y,
				wp.ptMaxPosition.x, wp.ptMaxPosition.y,
				wp.rcNormalPosition.left, wp.rcNormalPosition.top,
				wp.rcNormalPosition.right, wp.rcNormalPosition.bottom);
				regMgr.Set( szMainWindow, keyVal);
		}

		// Save tooltips flag...
		wsprintf( keyVal, "%u", m_bToolTips );
		regMgr.Set( szToolTips, keyVal );

		// Color select dialog position.
		GetColorSelectDlg()->GetWindowRect( &rect );
		sprintf( keyVal, szColorDlgFormat, GetColorSelectDlg()->IsWindowVisible(), rect.left, rect.top, rect.Width(), rect.Height() );
		regMgr.Set( szColorSelectDlg, keyVal);

		// 2001 (0x42f1d2)
		regMgr.Set( szDScriptEditor, m_DScriptEditor );
	
		sprintf( keyVal, szUndoPreferencesFormat, m_iUndoGeometry, m_iUndoTextures, m_iUndoProperties, m_iUndoGeneral );
		regMgr.Set( szUndoPreferences, keyVal );
	}

	// Save the settings in the registry
	GetApp()->GetOptions().GetWindowsOptions()->SetMainControlArray(m_projectControls);
}

void CMainFrame::InitProjectBar()
{
	m_pProjectBar->Init(this);
}


BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.style |= WS_VISIBLE;
	return CMRCMDIFrameWndSizeDock::PreCreateWindow(cs);
}


/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMRCMDIFrameWndSizeDock::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMRCMDIFrameWndSizeDock::Dump(dc);
}

#endif //_DEBUG





// ---------------------------------------------------- //
// Helpers
// ---------------------------------------------------- //

int CMainFrame::GetNumRegionDocs()
{
	return (int)m_pProjectBar->m_RegionDocs.GetSize();
}


void CMainFrame::UpdateAllRegions( LPARAM hint )
{
	CDocTemplate		*pTemp = GetApp()->m_pWorldTemplate;
	POSITION			pos;

	for( pos=pTemp->GetFirstDocPosition(); pos != NULL; )
		pTemp->GetNextDoc(pos)->UpdateAllViews( NULL, hint );
}




/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

static BOOL _FindNameInList(char *pTestName, CLinkedList<CString*> &names);
static void _InsertStringIntoList(char *pString, CLinkedList<CString*> &theList);


// talon: 2001's "Show World Textures" (0x42f331) and its size table
// (0x42f2cf), written from the image; the names are ours.
#define NUM_TEXTURE_SIZES	9

static uint32 _GetTextureSizeIndex(uint32 size)
{
	switch(size)
	{
		case 16:	return 0;
		case 32:	return 1;
		case 64:	return 2;
		case 128:	return 3;
		case 256:	return 4;
		case 512:	return 5;
		case 1024:	return 6;
		case 2048:	return 7;
		case 4096:	return 8;
		default:	return 0;
	}
}

void CMainFrame::OnShowWorldTextures()
{
	CDocTemplate *pTemp = GetApp()->m_pWorldTemplate;
	POSITION pos;
	CRegionDoc *pDoc;
	CString *pName;
	uint32 iPoly, x, y, nBrushes;
	LPOS brushPos, namePos;
	CEditBrush *pBrush;
	CEditPoly *pPoly;
	char *pTextureName;
	CTexture *pTexture;
	DStream *pStream;
	CLinkedList<CString*> textureNames;
	CMoArray<CTexture*> textures;
	uint32 sizeCounts[NUM_TEXTURE_SIZES][NUM_TEXTURE_SIZES];

	memset(sizeCounts, 0, sizeof(sizeCounts));
	textures.SetCacheSize(500);

	// For each region doc.
	for( pos=pTemp->GetFirstDocPosition(); pos != NULL; )
	{
		pDoc = (CRegionDoc*)pTemp->GetNextDoc(pos);

		if(pDoc->IsKindOf(RUNTIME_CLASS(CRegionDoc)))
		{	
			// Build the list of texture names, and count the sizes.
			for(brushPos=pDoc->m_Region.m_Brushes; brushPos; )
			{
				pBrush = pDoc->m_Region.m_Brushes.GetNext(brushPos);

				for(iPoly=0; iPoly < pBrush->m_Polies.GetSize(); iPoly++)
				{
					pPoly = pBrush->m_Polies[iPoly];

					pTextureName = pPoly->GetTexture(0).m_pTextureName;
					if(!_FindNameInList(pTextureName, textureNames))
					{
						_InsertStringIntoList(pTextureName, textureNames);
					}

					pTexture = dib_GetDibTexture(pPoly->GetTexture(0).m_pTextureFile);
					if(pTexture && textures.FindElement(pTexture) == BAD_INDEX)
					{
						sizeCounts[_GetTextureSizeIndex(pTexture->m_pDib->GetWidth())][_GetTextureSizeIndex(pTexture->m_pDib->GetHeight())]++;
						textures.Append(pTexture);
					}
				}
			}

			// Print it to the debug window.
			AddDebugMessage("");
			AddDebugMessage("Textures for world %s", pDoc->m_FileName);
			for(namePos=textureNames; namePos; )
			{
				pName = textureNames.GetNext(namePos);

				CString fullName = dfm_GetFullFilename(GetFileMgr(), *pName);
				pStream = streamsim_Open(fullName, "rb");
				if(pStream)
					pStream->Release();

				// How many brushes use it.
				nBrushes = 0;
				for(brushPos=pDoc->m_Region.m_Brushes; brushPos; )
				{
					pBrush = pDoc->m_Region.m_Brushes.GetNext(brushPos);

					for(iPoly=0; iPoly < pBrush->m_Polies.GetSize(); iPoly++)
					{
						if(stricmp(pBrush->m_Polies[iPoly]->GetTexture(0).m_pTextureName, *pName) == 0)
						{
							nBrushes++;
							break;
						}
					}
				}

				AddDebugMessage("  %s (%d) %s", (LPCTSTR)(*pName), nBrushes, pStream ? "" : "(Invalid)");
			}

			AddDebugMessage("");
			for(y=0; y < NUM_TEXTURE_SIZES; y++)
			{
				for(x=0; x < NUM_TEXTURE_SIZES; x++)
				{
					if(sizeCounts[y][x])
					{
						AddDebugMessage("%dx%d count: %d", (int)pow(2.0, (double)(y+4)), (int)pow(2.0, (double)(x+4)), sizeCounts[y][x]);
					}
				}
			}

			DeleteAndRemoveElements(textureNames);
		}
	}
}


static BOOL _FindNameInList(char *pTestName, CLinkedList<CString*> &names)
{
	LPOS pos;
	CString *pName;

	for(pos=names; pos; )
	{
		pName = names.GetNext(pos);
	
		if(pName->CompareNoCase(pTestName) == 0)
			return TRUE;
	}

	return FALSE;
}


static void _InsertStringIntoList(char *pString, CLinkedList<CString*> &theList)
{
	LPOS pos;
	CString *pNewString, *pTest;

	pNewString = new CString;
	*pNewString = pString;

	for(pos=theList; pos; )
	{
		pTest = theList.GetAt(pos);

		if(pTest->CompareNoCase(pString) == 1)
		{
			theList.InsertBefore(pos, pNewString);
			return;
		}

		theList.GetNext(pos);
	}

	theList.Append(pNewString);
}

void CMainFrame::OnShowWorldClasses()
{
	CDocTemplate *pTemp = GetApp()->m_pWorldTemplate;
	POSITION pos;
	CRegionDoc *pDoc;
	CString *pString, *pName;
	uint32 iObject;
	LPOS classPos;
	CBaseEditObj *pObject;
	char *pClassName;
	CLinkedList<CString*> classNames;

	// For each region doc.
	for( pos=pTemp->GetFirstDocPosition(); pos != NULL; )
	{
		pDoc = (CRegionDoc*)pTemp->GetNextDoc(pos);

		if(pDoc->IsKindOf(RUNTIME_CLASS(CRegionDoc)))
		{	
			// Build the list of class names.
			for(iObject=0; iObject < pDoc->m_Region.m_Objects.GetSize(); ++iObject )
			{
				pObject = pDoc->m_Region.m_Objects[iObject];

				pClassName = (char*)pObject->GetClassName();
				if(!_FindNameInList(pClassName, classNames))
				{
					_InsertStringIntoList(pClassName, classNames);
				}
			}

			// Print it to the debug window.
			AddDebugMessage("");
			AddDebugMessage("Classes for world %s", pDoc->m_FileName);
			for(classPos=classNames; classPos; )
			{
				pName = classNames.GetNext(classPos);
				AddDebugMessage("  %s  %s", (LPCTSTR)(*pName), (GetProject()->FindClassDef(*pName) != NULL) ? "" : "(Invalid)");
			}
			
			AddDebugMessage("");

			DeleteAndRemoveElements(classNames);
		}
	}
}


// 2001's "File preferences" (0x42f912): the script editor's path, through
// the "Script Editor Preference" dialog (IDD 183, its constructor 0x443ab5).
void CMainFrame::OnFilePreferences()
{
	CPreferencesDlg dlg;

	strcpy(dlg.m_szScriptEditor, m_DScriptEditor);
	if(dlg.DoModal() == IDOK)
	{
		m_DScriptEditor = dlg.m_szScriptEditor;
	}
}

void CMainFrame::OnViewProjectWindow() 
{
	if( m_pProjectBar && ::IsWindow(m_pProjectBar->m_hWnd) )
	{
		if( m_pProjectBar->IsWindowVisible() )
			ShowControlBar( m_pProjectBar, FALSE, FALSE );
		else
			ShowControlBar( m_pProjectBar, TRUE, FALSE );

	}

	RecalcLayout();
}


void CMainFrame::OnUpdateViewProjectWindow(CCmdUI* pCmdUI) 
{
	if( m_pProjectBar && ::IsWindow(m_pProjectBar->m_hWnd) )
	{
		pCmdUI->SetCheck( m_pProjectBar->IsWindowVisible() );
	}
}











void CMainFrame::OnOptions() 
{
	// 2001 (0x42f9cd): "DEdit", the autosave time, the mipmap offset and
	// the run page; no backups, no misc or models pages
	COptionsSheet	OptDlg("DEdit",this,0);

	// Get the options
	CDEditOptions *pOptions=&GetApp()->GetOptions();

	// Set the autosave time
	OptDlg.m_undoPage.m_dwAutoSaveTime		= pOptions->GetAutoSaveTime();

	if(OptDlg.DoModal() == IDOK)
	{
		BOOL offset = !!OptDlg.m_d3dPage.m_bBlurryTextures;

		// Set the mipmap offset
		pOptions->GetDisplayOptions()->SetMipMapOffset(offset);
		
		// Set the autosave time
		pOptions->SetAutoSaveTime(OptDlg.m_undoPage.m_dwAutoSaveTime);

		// Update the run options
		OptDlg.m_runPage.SaveOptions();
	}
}

void CMainFrame::OnPaletteEdit() 
{
	CPaletteEdit dlgPaletteEdit( this );

	dlgPaletteEdit.DoModal( );
}

void CMainFrame::OnViewColorSelection() 
{
	if( GetColorSelectDlg()->IsWindowVisible() )
		GetColorSelectDlg()->ShowWindow( SW_HIDE );
	else
		GetColorSelectDlg()->ShowWindow( SW_SHOW );
}

void CMainFrame::OnUpdateViewColorSelection(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck( GetColorSelectDlg()->IsWindowVisible() );
}












void CMainFrame::OnDestroy() 
{
	GetColorSelectDlg( )->DestroyWindow( );
	CMRCMDIFrameWndSizeDock::OnDestroy();
}



BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	return CMRCMDIFrameWndSizeDock::OnCreateClient(lpcs, pContext);
}




// ---------------------------------------------------- //
// Main handlers
// ---------------------------------------------------- //

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	GetDirectorySize
//
//	PURPOSE:	Recursively find the total size of a directory
//
// ----------------------------------------------------------------------- //

DWORD GetDirectorySize(char* szPath)
{
	DWORD dwSize = 0;
	WIN32_FIND_DATA data;
	
	SetCurrentDirectory(szPath);

	HANDLE hFile = FindFirstFile("*.*",&data);
	FindNextFile(hFile,&data);
	
	while(FindNextFile(hFile,&data))
	{
		if(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			char szNewPath[MAX_PATH];
			sprintf(szNewPath, "%s\\%s",szPath,data.cFileName);

			dwSize += GetDirectorySize(szNewPath);
		}
		else
		{
			dwSize += (data.nFileSizeHigh * MAXDWORD) + data.nFileSizeLow;
		}
	}

	return dwSize;
}

// 2001 (0x42fbe3): no confirmation, and the rez file is built by the
// "Building Rez File" dialog (RezBuildDlg.cpp) through BuildRezFile.
void CMainFrame::OnCreateRezFile()
{
	CString baseDir, filterStr, outputFilename;
	CRezBuildDlg rezDlg;
	char baseDirTitle[256];

	if(m_pProjectBar->VerifyProjectIsOpen())
	{
		// Get the name of the directory the resources are in.
		baseDir = dfm_GetBaseDir(GetFileMgr());
		CHelpers::ExtractNames(baseDir, NULL, NULL, baseDirTitle, NULL);

		filterStr.LoadString(IDS_REZFILEFILTER);
		
		strcat(baseDirTitle, ".rez");
		outputFilename = GetProject()->m_BaseProjectDir + ".rez";

		CFileDialog FileDlg(FALSE, "rez", (LPCTSTR)outputFilename, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,
			filterStr, GetMainFrame());

		while(FileDlg.DoModal() == IDOK)
		{
			//Calculate the size of the rez file
			char szOldPath[MAX_PATH];
			GetCurrentDirectory(MAX_PATH,szOldPath);
			DWORD dwSize = GetDirectorySize(baseDir.GetBuffer(0));
			dwSize = dwSize / (DWORD)1024L; //convert to KBs
			SetCurrentDirectory(szOldPath);

			//SCHLEGZ 1/23/98: check to see if we have enough space for a rez file
			struct _diskfree_t diskfree;
			int nDrive = _getdrive(); // use current default drive
			if(_getdiskfree(nDrive, &diskfree) == 0 &&
				dwSize < (DWORD)diskfree.avail_clusters * (DWORD)diskfree.sectors_per_cluster *
					(DWORD)diskfree.bytes_per_sector / (DWORD)1024L)
			{
				rezDlg.m_sRezFile = FileDlg.GetPathName();
				rezDlg.m_sProjectDir = baseDir;
				rezDlg.DoModal();
				break;
			}

			AfxMessageBox("There is not enough free disk space available on target drive.",MB_OK | MB_ICONSTOP);
		}
	}
}

void CMainFrame::OnUpdateProjectOpen( CCmdUI* pCmdUI )
{
	if( GetProjectBar() )
		pCmdUI->Enable( GetProjectBar()->IsProjectOpen() );
}


void CMainFrame::OnReloadPalettes()
{
}

// talon: "Group World Textures" (32954) does nothing in 2001: its entry is
// the shared empty function 0x42fe6f.  The name is ours.
void CMainFrame::OnGroupWorldTextures()
{
}


void CMainFrame::OnClose()
{
	m_bClosing = TRUE;

	// Save Registry information...
	SaveRegistryInfo();	
	
	// (2001, 0x42fe70: no last project saved)
	if( GetProjectBar() )
		if( !GetProjectBar()->Close())
			return;	
	
	// Destroy the windows that the project bar owns
	GetProjectBar()->DestroyControlWindows();	
	GetProjectBar()->SaveRegistryInfo(_T("General"));

	// Save toolbar states to registry...
	SaveBarState(_T("General"));
	SaveSizeBarState(_T("General"));		

	// Destroy the project windows that we own
	int i;
	for (i=0; i < m_projectControls.GetSize(); i++)
	{
		m_projectControls[i]->m_pWnd->DestroyWindow();
	}

	if ( m_pProjectBar )		delete m_pProjectBar;	
	if ( m_pPropertiesDlg )		delete m_pPropertiesDlg;	
	if ( m_pNodeView )			delete m_pNodeView;	
	if ( m_pClassListDlg )		delete m_pClassListDlg;	
	if ( m_pTextureDlg )		delete m_pTextureDlg;
	if ( m_pModelDlg )			delete m_pModelDlg;	
	if ( m_pWorldsDlg )			delete m_pWorldsDlg;	
	if ( m_pSoundDlg )			delete m_pSoundDlg;	
	if ( m_pSpriteDlg )			delete m_pSpriteDlg;	
	if ( m_pPrefabDlg )			delete m_pPrefabDlg;

	DeleteAndClearArray(m_projectControls);

	CMRCMDIFrameWndSizeDock::OnClose( );	
}


// ---------------------------------------------------- //
// Edit mode handlers
// ---------------------------------------------------- //

void CMainFrame::OnGeometryEditMode() 
{
	CRegionFrame *pFrame;
	CRegionDoc *pDoc;

	// Clear all brush/object selections...
	pFrame = ( CRegionFrame * )MDIGetActive( );
	ASSERT( pFrame );
	pDoc = ( CRegionDoc * )pFrame->GetActiveDocument( );
	ASSERT( pDoc );
	pDoc->m_TaggedVerts.GenRemoveAll();
	
	m_WorldEditMode = GEOMETRY_EDITMODE;
	UpdateAllRegions( REGIONVIEWUPDATE_EDITMODE );

	// Update the menubar with the mode menus
	UpdateModeMenus();
}

void CMainFrame::OnUpdateGeometryEditMode(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetNumRegionDocs() > 0 );
	pCmdUI->SetRadio( GetWorldEditMode() == GEOMETRY_EDITMODE );
}

void CMainFrame::OnObjectEditMode() 
{
	m_WorldEditMode = OBJECT_EDITMODE;
	UpdateAllRegions( REGIONVIEWUPDATE_EDITMODE );

	// Update the menubar with the mode menus
	UpdateModeMenus();
}

void CMainFrame::OnUpdateObjectEditMode(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetNumRegionDocs() > 0 );
	pCmdUI->SetRadio( GetWorldEditMode() == OBJECT_EDITMODE );
}

void CMainFrame::OnBrushEditMode() 
{
	CRegionFrame *pFrame;
	CRegionDoc *pDoc;

	m_WorldEditMode = BRUSH_EDITMODE;
	UpdateAllRegions( REGIONVIEWUPDATE_EDITMODE );

	// Update the menubar with the mode menus
	UpdateModeMenus();
}

void CMainFrame::OnUpdateBrushEditMode(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetNumRegionDocs() > 0 );
	pCmdUI->SetRadio( GetWorldEditMode() == BRUSH_EDITMODE );
}

// talon: 2001's light editing mode (0x4300b3, 0x4300cf; Ctrl+L)
#ifndef LIGHT_EDITMODE
#define LIGHT_EDITMODE		3
#endif

void CMainFrame::OnLightEditMode() 
{
	m_WorldEditMode = LIGHT_EDITMODE;
	UpdateAllRegions( REGIONVIEWUPDATE_EDITMODE );

	// Update the menubar with the mode menus
	UpdateModeMenus();
}

void CMainFrame::OnUpdateLightEditMode(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetNumRegionDocs() > 0 );
	pCmdUI->SetRadio( GetWorldEditMode() == LIGHT_EDITMODE );
}




/************************************************************************/
// Updates the menu bar with the mode menus.  This basically adds
// the correct menu and removes the incorrect menu.
void CMainFrame::UpdateModeMenus()
{
	// Get the menu
	CMenu *pMenu=GetMenu();

	// Destroy the current menu
	m_modeMenu.DestroyMenu();

	// Get the menu to add	
	m_modeMenu.LoadMenu(IDR_MAINFRAME_MODE_MENUS);

	// Get the menu string that is current at the mode menu's position
	CString sCurrentMenuString;
	pMenu->GetMenuString(MODE_MENU_INSERT_POS, sCurrentMenuString, MF_BYPOSITION);

	// Check to see if a mode menu already exists on the menubar.  If so, remove it!
	int i;
	for (i=0; i < m_modeMenu.GetMenuItemCount(); i++)
	{
		// Get the menu string
		CString sMenuString;
		m_modeMenu.GetMenuString(i, sMenuString, MF_BYPOSITION);

		if (sCurrentMenuString == sMenuString)
		{
			// Remove the menu
			pMenu->RemoveMenu(MODE_MENU_INSERT_POS, MF_BYPOSITION);
			break;
		}
	}	

	// The index to the mode menu which will be added
	int nModeMenuIndex=0;

	// Figure out the index to the mode menu to add
	switch (m_WorldEditMode)
	{
	case BRUSH_EDITMODE:
		{
			nModeMenuIndex=MODE_MENU_BRUSH_INDEX;
			break;
		}
	default:
		{
			// There isn't a mode menu for this editing mode
			m_modeMenu.DestroyMenu();

			// Redraw the menu bar
			DrawMenuBar();
			return;
		}
	}	

	// Get the popup menu
	CMenu *pPopupMenu=m_modeMenu.GetSubMenu(nModeMenuIndex);	

	// Get the menu string
	CString sMenuString;
	m_modeMenu.GetMenuString(nModeMenuIndex, sMenuString, MF_BYPOSITION);

	// Add the menu and detach it from the other menu
	pMenu->InsertMenu(MODE_MENU_INSERT_POS, MF_BYPOSITION | MF_POPUP, (UINT)pPopupMenu->Detach(), sMenuString);

	// Redraw the menu bar
	DrawMenuBar();
}

/************************************************************************/
// Centers the views on the current selection
// 2001 (0x4302d4): the views look at the selection's centre (or the
// marker), a perspective view from as far as the selection's largest half
// dimension.
void CMainFrame::OnCenterOnSelection() 
{
	//run through all currently open documents
	for(uint32 i=0; i < m_pProjectBar->m_RegionDocs; i++ )
	{
		CRegionDoc *pRegionDoc = m_pProjectBar->m_RegionDocs[i];

		CVector vCenter;
		float fDist;

		if( pRegionDoc->GetRegion()->m_Selections > 0 )
		{
			CVector vHalfDims;
			vHalfDims = (pRegionDoc->m_SelectionMax - pRegionDoc->m_SelectionMin) * 0.5f;
			if( vHalfDims.Mag() < 0.001 )
			{
				vCenter = pRegionDoc->m_SelectionMin;
				fDist = 100.0f;
			}
			else
			{
				vCenter = pRegionDoc->m_SelectionMin + vHalfDims;
				fDist = LTMAX(vHalfDims.x, LTMAX(vHalfDims.y, vHalfDims.z));
			}
		}
		else
		{
			vCenter = pRegionDoc->GetRegion( )->m_vMarker;
			fDist = 100.0f;
		}

		//alright, now we need to reposition each and every view		
		for(POSITION pos = pRegionDoc->GetFirstViewPosition(); pos; )
		{
			//make sure that this is one of our views
			CView *pView = pRegionDoc->GetNextView(pos);
		
			if( pView->IsKindOf(RUNTIME_CLASS(CRegionView)) )
			{
				CRegionView *pRegionView = (CRegionView*)pView;

				if( pRegionView->IsPerspectiveViewType() )
				{
					if( !pRegionView->IsWalkThroughMovement() )
					{
						pRegionView->Nav().m_LookAt = vCenter;
						pRegionView->Nav().UpdateLooking();
					}
					else
					{
						pRegionView->Nav().Pos() = vCenter - pRegionView->Nav().Forward() * fDist;
					}
				}
				else
				{
					// Remove the displacement from centerPoint along normal and add in current
					// displacement...
					CVector vCamPos = vCenter;
					vCamPos += pRegionView->Nav().Forward( ) * 
						( pRegionView->Nav().Forward( ).Dot( pRegionView->Nav().Pos( ) - vCamPos ));
					pRegionView->Nav().Pos() = vCamPos;
				}
			}
		}

		pRegionDoc->RedrawAllViews();
	}
}

void CMainFrame::OnUpdateCenterOnSelection(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable( GetNumRegionDocs() > 0 );
}

/************************************************************************/
// Centers the views at the specific coordinate.
// Pass in TRUE if you wish to center perspective views as well.		
void CMainFrame::CenterViewsAtVector(CVector vCenter, BOOL bCenterPerspective)
{
	// Get the frame
	CRegionFrame *pFrame = (CRegionFrame *)MDIGetActive();	
	if (!pFrame)
	{
		return;
	}

	// Get the active document
	CRegionDoc *pRegionDoc = (CRegionDoc *)pFrame->GetActiveDocument();
	if (!pRegionDoc)
	{
		return;
	}	
	
	// Go through each view in the document
	POSITION pos;
	for( pos = pRegionDoc->GetFirstViewPosition(); pos; )
	{
		// Get the view
		CView *pView = pRegionDoc->GetNextView(pos);
	
		// Check to see if the view is a CRegionView
		if( pView->IsKindOf(RUNTIME_CLASS(CRegionView)) )
		{
			// Get the region view
			CRegionView *pRegionView = (CRegionView*)pView;

			// Check to see if the view is a perspective view
			if( pRegionView->IsPerspectiveViewType() )
			{
				if (bCenterPerspective)
				{
					// 2001 (0x430615): an orbiting camera looks at it
					if (!pRegionView->IsWalkThroughMovement())
					{
						pRegionView->Nav().m_LookAt = vCenter;
						pRegionView->Nav().UpdateLooking();
					}
					else
					{
						// Move the camera to the marker
						pRegionView->Nav().Pos() = vCenter;
					}
				}
			}
			else
			{					
				// Remove the displacement from centerPoint along normal and add in current
				// displacement...
				CVector vCamPos=vCenter;
				vCamPos += pRegionView->Nav().Forward( ) * 
					( pRegionView->Nav().Forward( ).Dot( pRegionView->Nav().Pos( ) - vCamPos ));
				pRegionView->Nav().Pos() = vCamPos;
			}
		}
	}

	// Redraw all of the views in this document
	pRegionDoc->RedrawAllViews();	
}

/************************************************************************/
// Centers the views on the marker
void CMainFrame::OnCenterOnMarker() 
{				
	// Get the frame
	CRegionFrame *pFrame = (CRegionFrame *)MDIGetActive();	
	if (!pFrame)
	{
		return;
	}

	// Get the active document
	CRegionDoc *pRegionDoc = (CRegionDoc *)pFrame->GetActiveDocument();
	if (!pRegionDoc)
	{
		return;
	}

	// Center the views
	CenterViewsAtVector(pRegionDoc->GetRegion( )->m_vMarker, TRUE);	
}

/************************************************************************/
void CMainFrame::OnUpdateCenterOnMarker(CCmdUI* pCmdUI) 
{
	// Get the frame
	CRegionFrame *pFrame = (CRegionFrame *)MDIGetActive();	
	if (!pFrame)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	// Get the active document
	CRegionDoc *pRegionDoc = (CRegionDoc *)pFrame->GetActiveDocument();
	if (!pRegionDoc)
	{
		pCmdUI->Enable(FALSE);
		return;
	}
}

//SCHLEGZ 1/22/98:  Added
void CMainFrame::OnUpdateMDINext(CCmdUI* pCmdUI)
{
	pCmdUI->Enable( GetNumRegionDocs() > 1 );
}

//Given a world name, this will use the options and launch a process to run the 
//specified world name
// 2001 (0x430765): one function, the world's own .dat (its name less its
// three-letter extension), the arguments' %WorldName% replaced, and the
// frame not minimized.
void CMainFrame::OnRunWorld()
{
	// The run options
	COptionsRun *pOptions=GetApp()->GetOptions().GetRunOptions();
	if (!pOptions)
	{
		return;
	}

	// Make sure a project is open.
	if(!GetProjectBar()->VerifyProjectIsOpen())
	{
		return;
	}		

	// Get the frame
	CRegionFrame *pFrame = (CRegionFrame *)MDIGetActive();	
	if (!pFrame)
	{
		return;
	}

	// Get the active document
	CRegionDoc *pDoc = (CRegionDoc *)pFrame->GetActiveDocument();
	if (!pDoc)	
	{
		return;
	}	

	// The world filename
	CString sFilename = pDoc->m_FileName;
	if (sFilename.GetLength() < 4)
	{
		return;
	}

	// The .dat file
	CString sDatFilename = sFilename.Left(sFilename.GetLength()-3);

	// Verify that the .dat file exists
	if (_access(sDatFilename + ".dat", 00) != 0)
	{
		MessageBox("Error: Cannot find the .dat file for your world.  Please run the processor.",
				   "Cannot find file", MB_OK | MB_ICONERROR);
		return;
	}

	// Get the project base directory
	CString sBaseProjectDir=GetProject()->m_BaseProjectDir;
	sBaseProjectDir.MakeUpper();
	sDatFilename.MakeUpper();
	
	// Make sure that the file is within the base project
	if (sBaseProjectDir.GetLength() > sBaseProjectDir.GetLength() ||
		sBaseProjectDir != sDatFilename.Left(sBaseProjectDir.GetLength()))
	{
		MessageBox("Error: Your .dat file must be in your project path.",
				   "Invalid file path", MB_OK | MB_ICONERROR);
		return;
	}

	// Get the relative path to the file but subtracting the base project directory from it
	CString sRelativePath=sDatFilename.Right(sDatFilename.GetLength()-sBaseProjectDir.GetLength()-1);
		
	// Create the program arguments string by replacing the "%WorldName%" string with the world name
	CString sArguments=pOptions->GetProgramArguments();
	sArguments.Replace("%WorldName%", sRelativePath);

	// Create the command line
	CString sCommandLine=pOptions->GetExecutable()+" "+sArguments;

	// Get the current working directory
	char szCurrentDir[MAX_PATH];
	_getcwd((char *)szCurrentDir, MAX_PATH);

	// Change the current working directory
	if (_chdir(pOptions->GetWorkingDirectory()) != 0)
	{
		MessageBox("Error: Cannot change working directory to: \""+pOptions->GetWorkingDirectory()+"\" "+
				   "Please verify your settings in the DEdit options dialog.",
				   "Cannot change directory", MB_OK | MB_ICONERROR);
		return;
	}

	// The create process structures
	PROCESS_INFORMATION processInfo;
	STARTUPINFO			startInfo;

	memset(&startInfo, 0, sizeof(STARTUPINFO));
	startInfo.cb = sizeof(STARTUPINFO);		
	BOOL bRet = CreateProcess(NULL, sCommandLine.GetBuffer(0), NULL, NULL, FALSE, 0, NULL, NULL, &startInfo, &processInfo);

	// Check to see if the process launched succesfully
	if (!bRet)
	{
		MessageBox("Unable to start process. Please verify your settings in the DEdit options dialog.", "Error", MB_OK | MB_ICONERROR);
	}

	// Change the working directory back
	_chdir(szCurrentDir);
}


void CMainFrame::OnUpdateRunWorld(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetNumRegionDocs() > 0);
}


void CMainFrame::OnUpdateViewDebugWindow(CCmdUI *pCmdUI)
{
	//pCmdUI->Enable(GetDebugDlg()->IsWindowVisible());
	pCmdUI->SetCheck(GetDebugDlg()->IsWindowVisible());
}


void CMainFrame::OnViewDebugWindow()
{
	if(GetDebugDlg()->IsWindowVisible())
		GetDebugDlg()->ShowWindow(SW_HIDE);
	else
		GetDebugDlg()->ShowWindow(SW_SHOW);
}






void CMainFrame::OnUpdateSingleNodeSelection( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( GetNumRegionDocs() > 0 );
	pCmdUI->SetCheck( m_NodeSelectionMode == SINGLE_SELECTION );
}

// 2001 (0x430ba1, 0x430c40): both clear every region's selections.
void CMainFrame::OnSingleNodeSelection()
{
	DWORD			i;

	m_NodeSelectionMode = SINGLE_SELECTION;

	for( i=0; i < m_pProjectBar->m_RegionDocs; i++ )
	{
		m_pProjectBar->m_RegionDocs[i]->GetRegion()->ClearSelections();
		m_pProjectBar->m_RegionDocs[i]->NotifySelectionChange();
		m_pProjectBar->m_RegionDocs[i]->RedrawAllViews();
	}
}

void CMainFrame::OnUpdateMultiNodeSelection( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( (GetNumRegionDocs() > 0) );
	pCmdUI->SetCheck( m_NodeSelectionMode == MULTI_SELECTION );
}

void CMainFrame::OnMultiNodeSelection()
{
	DWORD			i;

	m_NodeSelectionMode = MULTI_SELECTION;
	
	for( i=0; i < m_pProjectBar->m_RegionDocs; i++ )
	{
		m_pProjectBar->m_RegionDocs[i]->GetRegion()->ClearSelections();
		m_pProjectBar->m_RegionDocs[i]->NotifySelectionChange();
		m_pProjectBar->m_RegionDocs[i]->RedrawAllViews();
	}
}



void CMainFrame::ActivateFrame(int nCmdShow) 
{
	CMRCMDIFrameWndSizeDock::ActivateFrame(nCmdShow);
}

void CMainFrame::OnSetFocus(CWnd* pOldWnd) 
{
	CMRCMDIFrameWndSizeDock::OnSetFocus(pOldWnd);
	
}


//-----------------------------------------------------------------------------
//
//  CMainFrame::SetTooltips()
//
//  Purpose:	Sets tooltips flags in toolbars.
//
//-----------------------------------------------------------------------------
void CMainFrame::SetToolTips() 
{	
	// Toggle tooltips...
	if (m_bToolTips)
	{
		m_wndMainBar.SetBarStyle(m_wndMainBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_NodeViewToolbar.SetBarStyle(m_NodeViewToolbar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_WorldEditToolbar.SetBarStyle(m_WorldEditToolbar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	}
	else
	{
		m_wndMainBar.SetBarStyle(m_wndMainBar.GetBarStyle() & ~(CBRS_TOOLTIPS | CBRS_FLYBY));
		m_NodeViewToolbar.SetBarStyle(m_NodeViewToolbar.GetBarStyle() & ~( CBRS_TOOLTIPS | CBRS_FLYBY));
		m_WorldEditToolbar.SetBarStyle(m_WorldEditToolbar.GetBarStyle() & ~( CBRS_TOOLTIPS | CBRS_FLYBY));
	}
}

//-----------------------------------------------------------------------------
//
//  CMainFrame::OnViewTooltips()
//
//  Purpose:	Handles toggle of tool tips.
//
//-----------------------------------------------------------------------------
void CMainFrame::OnViewTooltips() 
{
	m_bToolTips = !m_bToolTips;
	SetToolTips( );
}

//-----------------------------------------------------------------------------
//
//  CMainFrame::OnUpdateViewTooltips()
//
//  Purpose:	Handles updating menu for tooltips check.
//
//-----------------------------------------------------------------------------
void CMainFrame::OnUpdateViewTooltips( CCmdUI* pCmdUI ) 
{
	pCmdUI->SetCheck( m_bToolTips );
}


//-----------------------------------------------------------------------------
//
//  CMainFrame::CountUndo
//
//  Purpose:	Decrements an undo counter until zero, then resets value.
//				Returns TRUE when hits zero.
//
//-----------------------------------------------------------------------------





//-----------------------------------------------------------------------------
//
//  CMainFrame::SetUndoPreferencesX
//
//  Purpose:	Sets undo preferences.
//
//-----------------------------------------------------------------------------
void CMainFrame::SetUndoPreferencesDefault( UINT &iGeometry, UINT &iTextures, UINT &iProperties, UINT &iGeneral )
{
	iGeometry = 1;
	iTextures = 1;
	iProperties = 1;
	iGeneral = 1;
}


void CMainFrame::OnSelectTab(UINT id)
{
	int iTab;
	CProjectBar *pBar;


	pBar = GetProjectBar();

	// This handles the accelerators that select project tabs (Ctrl+1 - Ctrl+9).
	iTab = pBar->FindTabBySelectID(id);
	if(iTab != -1)
	{
		pBar->SetTab(pBar->m_TabInfo[iTab]->m_ControlType);
	}
}


void CMainFrame::OnToggleEditMode() 
{
	// Make sure that it is okay to change the mode by calling
	// the message handler directly
	if (GetNumRegionDocs() <= 0)
	{
		return;
	}

	switch (m_WorldEditMode)
	{
	case BRUSH_EDITMODE:
		{
			OnGeometryEditMode();
			break;
		}
	case GEOMETRY_EDITMODE:
		{
			OnObjectEditMode();
			break;
		}
	case OBJECT_EDITMODE:
		{
			OnBrushEditMode();
			break;
		}
	default:
		{
			ASSERT(FALSE);
		}
	}
}

/************************************************************************/
// This toggles the display of the floating control bar windows
void CMainFrame::OnViewToggleFloatingToolbars() 
{
//	POSITION pos = m_listControlBars.GetHeadPosition();
//	while (pos != NULL)
//	{
//		CControlBar* pControlBar = (CControlBar*)m_listControlBars.GetNext(pos);
//		ASSERT_VALID(pControlBar);		

	int i;
	for (i=0; i < m_projectControls.GetSize(); i++)
	{
		CControlBar *pControlBar=m_projectControls[i]->m_pWnd;

		// Check to see if it is floating
		if (pControlBar->IsFloating())
		{
			// Check to see if it is visible
			if(pControlBar->IsWindowVisible())
			{
				// Hide the control bar
				ShowControlBar( pControlBar, FALSE, FALSE );
			}
			else
			{
				// Show the control bar
				ShowControlBar( pControlBar, TRUE, FALSE );
			}
		}
	}

	// Check the project bar
	if (m_pProjectBar->IsFloating())
	{
		if (m_pProjectBar->IsWindowVisible())
		{
			ShowControlBar(m_pProjectBar, FALSE, FALSE);
		}
		else
		{
			ShowControlBar(m_pProjectBar, TRUE, FALSE);
		}
	}
}
