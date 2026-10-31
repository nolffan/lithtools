//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
#ifndef __MAINFRM_H__
#define __MAINFRM_H__

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

class CProjectBar;


#include "colorselectdlg.h"
#include "editprojectmgr.h"
#include "editclipboard.h"

#include "mrcext.h"	//schlegz for new project bar
#include "flattoolbar.h"	// talon: its constructor and destructor fold with MFC's CToolBar's (0x4c6f52, 0x4c708e)
#include "dedockframewnd.h"

class CPropertiesDlg;
class CNodeView;
class ClassListDlg;
class CTextureDlg;
class CPhysicsDlg;
class CWorldsDlg;
class CModelDlg;
class CSoundDlg;
class CSpriteDlg;
class CPrefabDlg;

class CProjectControlBarInfo;
class CMainFrame : public CMRCMDIFrameWndSizeDock	//SCHLEGZ changed to new frame for new project bar support
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

	enum ProjectControl
	{
		CB_WORLDSVIEW = 0,
		CB_TEXTUREVIEW,
		CB_NODESVIEW,
		CB_PROPERTIESVIEW,
		CB_MODELVIEW,
		CB_SOUNDVIEW,
		CB_SPRITEVIEW,
		CB_CLASSVIEW,
		CB_PREFABVIEW,
		CB_LAST_CONTROL_INDEX
	};

// Attributes
public:

	CStatusBar*		GetStatusBar()			{ return &m_wndStatusBar; }
	CFlatToolbar*	GetToolBar()			{ return &m_wndMainBar; }

	CFlatToolbar*	GetNodeViewToolbar()	{ return &m_NodeViewToolbar; }
	CFlatToolbar*	GetWorldEditToolbar()	{ return &m_WorldEditToolbar; }
	CProjectBar*	GetProjectBar()			{ return m_pProjectBar; }

	CNodeView*		GetNodeView()			{ return m_pNodeView; }
	CPropertiesDlg*	GetPropertiesDlg()		{ return m_pPropertiesDlg; }		
	ClassListDlg*	GetClassListDlg()		{ return m_pClassListDlg; }
	CTextureDlg*	GetTextureDlg()			{ return m_pTextureDlg; }
	CModelDlg*		GetModelDlg()			{ return m_pModelDlg; }
	CWorldsDlg*		GetWorldsDlg()			{ return m_pWorldsDlg; }
	CSoundDlg*		GetSoundDlg()			{ return m_pSoundDlg; }
	CSpriteDlg*		GetSpriteDlg()			{ return m_pSpriteDlg; }
	CPrefabDlg*		GetPrefabDlg()			{ return m_pPrefabDlg; }

	int				GetWorldEditMode()		{ return m_WorldEditMode; }

	int				GetNodeSelectionMode()	{ return m_NodeSelectionMode; }
	void			SetToolTips( );
	BOOL			IsClosing( )			{ return m_bClosing; }

	CEditClipboard *GetClipboard( )			{ return &m_Clipboard; }


	
	// Undo preferences

	
	void			SetUndoPreferencesDefault( UINT &iGeometry, UINT &iTextures, UINT &iProperties, UINT &iGeneral );

	UINT			m_iUndoGeometry;
	UINT			m_iCurUndoGeometry;
	UINT			m_iUndoTextures;
	UINT			m_iCurUndoTextures;
	UINT			m_iUndoProperties;
	UINT			m_iCurUndoProperties;
	UINT			m_iUndoGeneral;
	UINT			m_iCurUndoGeneral;


// Operations
public:
	void			UpdateStatusText(int nIndex, const char* szText);

	void			InitProjectBar();

	int				GetNumRegionDocs();
	void			UpdateAllRegions( LPARAM hint );
	
	// Updates the menu bar with the mode menus.  This basically adds
	// the correct menu and removes the incorrect menu.
	void			UpdateModeMenus();

	// Returns a pointer to the mode menu
	CMenu			*GetModeMenu()			{ return &m_modeMenu; }

	// Centers the views at the specific coordinate.
	// Pass in TRUE if you wish to center perspective views as well.		
	void			CenterViewsAtVector(CVector vCenter, BOOL bCenterPerspective=TRUE);
	
	// Call this to add a project control info structure based on the
	// control type.
	// Returns: pointer to the allocated control info structure.
	CProjectControlBarInfo	*AddProjectControlInfo(CMoArray<CProjectControlBarInfo *> &controlArray,
												   CMainFrame::ProjectControl controlType);

	// This creates a project bar window (from the control type) and returns
	// a pointer to the created window.  Note that it doesn't allocate a new
	// CMRCSizeDialogBar, it is just returning a pointer to the window that
	// is assocated with the control type.
	CMRCSizeDialogBar		*CreateProjectControlBar(CMainFrame::ProjectControl controlType);

	// This destroys a project bar window and returns a pointer to the
	// destroyed window.
	CMRCSizeDialogBar		*DestroyProjectControlBar(CMainFrame::ProjectControl controlType);


private:

	void			RestoreRegistryInfo( );
	void			SaveRegistryInfo( );


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void ActivateFrame(int nCmdShow = -1);
protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members

	// talon: 2001's CMainFrame (0x460 bytes; the constructor 0x42e0cb).
	// The DScript editor's path, from the registry (0x42efac); the name is
	// ours.
	CString				m_DScriptEditor;

	int					m_WorldEditMode;
	int					m_NodeSelectionMode;

	BOOL				m_bShown;

	CStatusBar			m_wndStatusBar;

	CFlatToolbar		m_wndMainBar;
	CFlatToolbar		m_NodeViewToolbar;
	CFlatToolbar		m_WorldEditToolbar;

	// The project bar
	CProjectBar			*m_pProjectBar;
	
	CNodeView			*m_pNodeView;		// The node view
	CPropertiesDlg		*m_pPropertiesDlg;	// The properties dialog		
	ClassListDlg		*m_pClassListDlg;	// The class list
	CTextureDlg			*m_pTextureDlg;		// The texture dialog
	CModelDlg			*m_pModelDlg;		// The model dialog
	CWorldsDlg			*m_pWorldsDlg;		// The world dialog
	CSoundDlg			*m_pSoundDlg;		// The sound dialog
	CSpriteDlg			*m_pSpriteDlg;		// The sprite dialog
	CPrefabDlg			*m_pPrefabDlg;		// The prefab dialog

	// These are the project control bars that the MainFrame owns.
	// The rest of the control bars should be owned by the project bar.
	CMoArray<CProjectControlBarInfo *>	m_projectControls;

	// talon: a project manager of the frame's own (0x394, built and
	// destroyed by the frame and used nowhere else); the name is ours.
	CEditProjectMgr		m_Project;

	BOOL				m_bToolTips;
	BOOL				m_bClosing;

	CEditClipboard		m_Clipboard;

	CMenu				m_modeMenu;			// The mode menu which is added dynamically

// Generated message map functions
public:
	//{{AFX_MSG(CMainFrame)
	afx_msg void OnSelectTab(UINT id);
	afx_msg void OnRunWorld();
	afx_msg void OnUpdateRunWorld(CCmdUI *pCmdUI);
	afx_msg void OnShowWorldTextures();
	afx_msg void OnShowWorldClasses();
	afx_msg void OnGroupWorldTextures();
	afx_msg void OnFilePreferences();
	afx_msg void OnLightEditMode();
	afx_msg void OnUpdateLightEditMode(CCmdUI* pCmdUI);
	afx_msg void OnUpdateViewDebugWindow(CCmdUI *pCmdUI);
	afx_msg void OnViewDebugWindow();
	afx_msg void OnUpdateSingleNodeSelection( CCmdUI *pCmdUI );
	afx_msg void OnUpdateMultiNodeSelection( CCmdUI *pCmdUI );
	afx_msg void OnSingleNodeSelection();
	afx_msg void OnMultiNodeSelection();
	afx_msg void OnCreateRezFile();
	afx_msg void OnReloadPalettes();
	afx_msg void OnClose();
	afx_msg void OnUpdateProjectOpen( CCmdUI *pCmdUI );
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewProjectWindow();
	afx_msg void OnUpdateViewProjectWindow(CCmdUI* pCmdUI);
	afx_msg void OnViewColorSelection();
	afx_msg void OnUpdateViewColorSelection(CCmdUI* pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnGeometryEditMode();
	afx_msg void OnUpdateGeometryEditMode(CCmdUI* pCmdUI);
	afx_msg void OnObjectEditMode();
	afx_msg void OnUpdateObjectEditMode(CCmdUI* pCmdUI);
	afx_msg void OnBrushEditMode();
	afx_msg void OnUpdateBrushEditMode(CCmdUI* pCmdUI);
	afx_msg void OnCenterOnSelection();
	afx_msg void OnUpdateCenterOnSelection( CCmdUI *pCmdUI );
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnViewTooltips();
	afx_msg void OnUpdateViewTooltips(CCmdUI* pCmdUI);
	afx_msg void OnOptions();
	afx_msg void OnPaletteEdit();
	afx_msg void OnUpdateMDINext( CCmdUI* pCmdUI);
	afx_msg void OnToggleEditMode();
	afx_msg void OnCenterOnMarker();
	afx_msg void OnUpdateCenterOnMarker(CCmdUI* pCmdUI);
	afx_msg void OnViewToggleFloatingToolbars();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CProjectControlBarInfo
{
public:
	CProjectControlBarInfo() {}
	
	CProjectControlBarInfo(CMRCSizeDialogBar *pWnd, DWORD title, DWORD dlg, DWORD selectID, CMainFrame::ProjectControl controlType)
	{
		m_pWnd = pWnd;					
		m_TitleStringID = title;
		m_DialogID = dlg;
		m_SelectID = selectID;
		m_ControlType=controlType;
	}

	CMRCSizeDialogBar			*m_pWnd;
	DWORD						m_TitleStringID;
	DWORD						m_DialogID;	
	DWORD						m_SelectID;
	CMainFrame::ProjectControl	m_ControlType;
};

typedef CMoArray<CProjectControlBarInfo *> CProjectControlBarInfoArray;

/////////////////////////////////////////////////////////////////////////////


#endif  // __MAINFRM_H__



