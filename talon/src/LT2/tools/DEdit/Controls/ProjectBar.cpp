#include "bdefs.h"
#include "dedit.h"
#include "projectbar.h"
#include "resource.h"
#include "mainfrm.h"
#include "stringdlg.h"
#include "spriteeditdlg.h"
#include "regionview.h"
#include "regionframe.h"
#include "texture.h"
#include "sysstreamsim.h"
#include "resnewdir.h"
#include "model.h"
#include "mainfrm.h"
#include "optionswindows.h"
#include "prefabdlg.h"
#include "ltamgr.h"
#include "optionsrun.h"
#include "loadmodeldims.h"

#include <direct.h>		//for _mkdir

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




/////////////////////////////////////////////////////////////////////////////
// CProjectBar

// 2001's map (0x4d4d78): no thumbnail, icon, prefab-reference, cube/bump/normal
// map, rename, 32P, batch-reload or scale-coordinates commands.
BEGIN_MESSAGE_MAP(CProjectBar, CMRCSizeDialogBar)
	//{{AFX_MSG_MAP(CProjectBar)
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, IDC_PROJECTVIEW_TABS, OnTabsSelChange)
	ON_COMMAND(ID_POPUP_IMPORTTEXTURES, OnImportTextures)
	ON_COMMAND(ID_POPUP_IMPORTPCX, OnImportPcxFiles)
	ON_COMMAND(ID_POPUP_IMPORTMIPPCX, OnImportMipPcxFiles)
	ON_COMMAND(ID_IMPORT_TGA_FILES, OnImportTgaFiles)
	ON_COMMAND(ID_POPUP_IMPORTALPHAMASK, OnImportAlphaMask)
	ON_COMMAND(ID_IMPORT_8BIT_ALPHA, OnImport8BitAlphaMask)
	ON_COMMAND(ID_POPUP_CREATEALPHAFROMCOLOR, OnCreateAlphaFromColor)
	ON_COMMAND(ID_POPUP_CREATESOLIDALPHA, OnCreateSolidAlpha)
	ON_COMMAND(ID_POPUP_EXPORTPCX, OnExportPcxFile)
	ON_COMMAND(ID_POPUP_TEXTUREPROP, OnTextureProperties)
	ON_COMMAND(ID_POPUP_VIEWALLTEXTURES, OnViewAllTextures)
	ON_COMMAND(ID_POPUP_IMPORTSOUNDS, OnImportSounds)
	ON_COMMAND(ID_POPUP_IMPORTMODELS, OnImportModels)
	ON_COMMAND(ID_POPUP_CREATENEWSPRITE, OnPopupCreateNewSprite)
	ON_COMMAND(ID_POPUP_UPDATE, OnPopupUpdate)
	ON_COMMAND(ID_WORLD_ADDDIR, OnAddDir)
	ON_COMMAND(ID_PREFAB_OPEN, OnPrefabOpen)
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_POPUP_ADDTOPALETTE, OnAddToPalette)
	ON_COMMAND(ID_POPUP_EXPORTDTX8_24, OnExport8BitDTXFile)
	ON_COMMAND(ID_POPUP_EXPORTDTX_ALL_AS_BPP32P, OnExportAllAsBPP_32P)
	ON_COMMAND(ID_POPUP_FINDTEXINWORLD, OnFindTextureInWorld)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



CProjectBar::CProjectBar()
	: CMRCSizeDialogBar(SZBARF_STDMOUSECLICKS)
{
	m_pCurTextureSel		= NULL;
	m_dwUnknown290			= 0;
	m_ProjectFileName[0]	= 0;
	m_TabInfo.SetSize(0);
}


CProjectBar::~CProjectBar()
{
	DeleteAndClearArray(m_TabInfo);
}

BOOL CProjectBar::Init(CMainFrame *pFrame)
{
	CRect			rect;
	DWORD			i;
	CString			str;

	// Set the project bar for the tab control
	m_Tabs.SetProjectBar(this);

	// Get the window options
	COptionsWindows *pOptions=GetApp()->GetOptions().GetWindowsOptions();

	// Setup our tab window list.
	DeleteAndClearArray(m_TabInfo);
	for (i=0; i < pOptions->GetNumBarControls(); i++)
	{
		pFrame->AddProjectControlInfo(m_TabInfo, pOptions->GetBarControl(i));
	}

	static CString strWndClass;
	if (strWndClass.IsEmpty())
	{
		strWndClass = AfxRegisterWndClass(CS_DBLCLKS);
	}

	if(!CMRCSizeDialogBar::Create(pFrame,IDD_PROJECTBAR_DLG,
	   /*WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | */CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_HIDE_INPLACE | CBRS_SIZE_DYNAMIC,
									   ID_VIEW_PROJECT_WINDOW))
		return FALSE;

	EnableDocking( CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT );

	str.LoadString( IDS_PROJECTBAR_TITLE );
	SetWindowText( str );

	// Setup all the tabs and stuff.
	GetClientRect( &rect );

	m_Tabs.Create(WS_CHILD|WS_VISIBLE|TCS_TOOLTIPS|TCS_MULTILINE, rect,
		this, IDC_PROJECTVIEW_TABS );

	// Load tab images
	m_TabImages.Create(IDB_PROJECT_ICONS, 20, 2, RGB(0,255,0));

	m_Tabs.SetImageList( &m_TabImages );
	m_Tabs.SetPadding( CSize(4,4) );
	m_Tabs.SetItemSize( CSize(22,22) );

	// Use the status bar's font
	m_Tabs.SetFont(pFrame->GetStatusBar()->GetFont());

	m_ToolTips.Create( this, TTS_ALWAYSTIP );


	TC_ITEM item;
	memset( &item, 0, sizeof(item) );
	item.mask = TCIF_TEXT | TCIF_IMAGE;
	CString csTemp;

	// Setup the tabs.
	for(i=0; i < m_TabInfo; i++)
	{
		csTemp.LoadString(m_TabInfo[i]->m_TitleStringID);
		item.pszText = csTemp.GetBuffer(0);
		item.cchTextMax = csTemp.GetLength();
		item.iImage = 0;
		m_Tabs.InsertItem( i, &item );

		// Create the dialog.
		CMRCSizeDialogBar *pDlgBar=(CMRCSizeDialogBar *)m_TabInfo[i]->m_pWnd;
		pDlgBar->Create(&m_Tabs, m_TabInfo[i]->m_DialogID, CBRS_NOALIGN, m_TabInfo[i]->m_SelectID);
		pDlgBar->ShowWindow(SW_HIDE);
	}

	// Select the first tab
	m_Tabs.SetCurSel(0);

	// Create the debug window
	m_DebugDlg.Create( IDD_DEBUGDLG, pFrame );

	// Show the first window
	if (m_TabInfo.GetSize() > 0)
	{
		m_pVisibleDlg = m_TabInfo[0]->m_pWnd;
		m_pVisibleDlg->ShowWindow( SW_SHOW );
	}
	else
	{
		m_pVisibleDlg=NULL;
	}

	ResizeElements();
	UpdateAll();

	Invalidate();
	UpdateWindow();

	return TRUE;
}

void CProjectBar::Term()
{
	m_pCurTextureSel = NULL;
	ClearAll();
	GetProject()->Term();
	CloseAllRegionDocs( );
}

/************************************************************************/
// This destroys the control windows that the project bar owns
void CProjectBar::DestroyControlWindows()
{
	int i;
	for (i=0; i < m_TabInfo.GetSize(); i++)
	{
		m_TabInfo[i]->m_pWnd->DestroyWindow();
	}
}

BOOL CProjectBar::Open( const char *pFilename )
{
	CMoFileIO file;
	char pathBuffer[256], fileBuffer[256];

	BeginWaitCursor( );

	Term( );

	CHelpers::ExtractPathAndFileName( pFilename, pathBuffer, fileBuffer );

	// Really load the project.
	if( file.Open(pFilename, "rb") )
	{
		m_Project.LoadProjectFile( file );
		file.Close();

		// Init the directory names in the project.
		m_Project.OnOpenProject(pathBuffer);

		UpdateAll();

		// Load the layout file.
		LoadLayoutFile( pFilename );

		strcpy( m_ProjectFileName, pFilename );

		// Finish up
		UpdateAll();

		EndWaitCursor( );

		return TRUE;
	}

	AppMessageBox( IDS_ERR_OPENFILE, MB_OK );
	return FALSE;
}



BOOL CProjectBar::Save()
{
	CMoFileIO		file;

	if( strlen(m_ProjectFileName) > 0 )
	{
		// Save the project file.
		if( file.Open(m_ProjectFileName, "wb") )
		{
			GetProject()->SaveProjectFile( file );
			file.Close();
		}

		SaveLayoutFile( m_ProjectFileName );
	}

	return TRUE;
}

BOOL CProjectBar::Close()
{
	if( !Save( ))
		return FALSE;

	if( !CloseAllRegionDocs( ))
		return FALSE;

	Term();
	memset(m_ProjectFileName, 0, sizeof(m_ProjectFileName));

	return TRUE;
}


BOOL CProjectBar::CloseAllRegionDocs( )
{
	// Close ALL region documents.
	while( m_RegionDocs > 0 )
	{
		CRegionDoc *pDoc = m_RegionDocs[0];

		if( !pDoc->SaveModified())
			return FALSE;
		GetApp()->m_pWorldTemplate->RemoveDocument( pDoc );
		pDoc->OnCloseDocument();
	}

	return TRUE;
}

BOOL CProjectBar::IsProjectOpen()
{
	return strlen(m_ProjectFileName) > 0;
}


BOOL CProjectBar::VerifyProjectIsOpen()
{
	if( IsProjectOpen() )
	{
		return TRUE;
	}
	else
	{
		AppMessageBox( IDS_NOPROJECTOPEN, MB_OK );
		return FALSE;
	}
}


void CProjectBar::ResizeElements()
{
	CRect rcWnd,rect, rcItem;
	DWORD i;


	if( ::IsWindow(m_Tabs.m_hWnd) && ::IsWindow(m_hWnd) )
	{
		// the rect of the dialog bar
		GetClientRect(&rcWnd);

		rcWnd.DeflateRect( 6, 6 );
		m_Tabs.MoveWindow( &rcWnd, FALSE );

		// move them
		m_Tabs.GetItemRect(m_Tabs.GetCurSel(),&rect);
		m_Tabs.GetClientRect(&rcWnd);
		rcWnd.top = rect.top;

		m_Tabs.GetItemRect( 0, &rcItem );
		rcWnd.top += rcItem.Height();
		rcWnd.DeflateRect( 3, 3 );

		for(i=0; i < m_TabInfo; i++)
		{
			if( ::IsWindow(m_TabInfo[i]->m_pWnd->m_hWnd) )
				m_TabInfo[i]->m_pWnd->MoveWindow( &rcWnd, TRUE );
		}
	}

}


void CProjectBar::UpdateAll()
{
	if(IsProjectOpen())
	{
		UpdateTextureDlg();
		UpdatePropertiesDlg();
		UpdateTextureIDs();
		UpdateWorldsDlg( );
		UpdateModelDlg();
		UpdateSoundDlg();
		UpdateSpriteDlg();
		UpdatePrefabDlg();
		GetClassListDlg()->Update();
	}
}

void CProjectBar::ClearAll()
{
	GetTextureDlg()->ClearAll();
	UpdatePropertiesDlg();
	UpdateTextureIDs();
	GetWorldsDlg()->ClearAll();
	GetModelDlg()->ClearAll();
	GetSoundDlg()->ClearAll();
	GetSpriteDlg()->ClearAll();
	GetPrefabDlg()->ClearAll();
}

void CProjectBar::UpdateTextureDlg()
{
	GetTextureDlg()->UpdateDirectories();
}

void CProjectBar::UpdatePropertiesDlg( )
{
	GetPropertiesDlg()->Redraw();
}


void CProjectBar::UpdateWorldsDlg()
{
	GetWorldsDlg()->UpdateDirectories( );
}

void CProjectBar::UpdateModelDlg()
{
	GetModelDlg()->UpdateDirectories( );
}

void CProjectBar::UpdateSoundDlg()
{
	GetSoundDlg()->UpdateDirectories( );
}

void CProjectBar::UpdateSpriteDlg()
{
	GetSpriteDlg()->UpdateDirectories( );
}

void CProjectBar::UpdatePrefabDlg()
{
	GetPrefabDlg()->UpdateDirectories( );
}

void CProjectBar::OnSize(UINT nType, int cx, int cy)
{
	CMRCSizeDialogBar::OnSize(nType, cx, cy);

	ResizeElements();
}


void CProjectBar::OnTabsSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	CMainFrame::ProjectControl controlType = m_TabInfo[m_Tabs.GetCurSel()]->m_ControlType;

	SetTab( controlType );

	*pResult = 0;
	ResizeElements();
}

void CProjectBar::SetTab(CMainFrame::ProjectControl controlType)
{
	if (m_pVisibleDlg)
	{
		m_pVisibleDlg->ShowWindow( SW_HIDE );
	}

	// Set the visible dialog
	BOOL bFound=FALSE;
	int nTabIndex=0;

	int i;
	for (i=0; i < m_TabInfo.GetSize(); i++)
	{
		if (controlType == m_TabInfo[i]->m_ControlType)
		{
			m_pVisibleDlg = m_TabInfo[i]->m_pWnd;
			nTabIndex=i;
			bFound=TRUE;
			break;
		}
	}
	ASSERT(bFound);

	if (m_pVisibleDlg)
	{
		m_pVisibleDlg->ShowWindow( SW_SHOW );
	}

	m_Tabs.SetCurSel(nTabIndex);
}

/************************************************************************/
// This undocks a tab and docks it to the mainframe
void CProjectBar::UndockTab(int nIndex, CPoint position)
{
	// The info for thie index
	CProjectControlBarInfo *pInfo=m_TabInfo[nIndex];

	// Get the control bar
	CMRCSizeDialogBar *pControlBar=pInfo->m_pWnd;

	// Terminate the tab
	SpecialCaseTabTerm(pControlBar, pInfo->m_ControlType);

	// Destroy the control bar window
	pControlBar->DestroyWindow();

	// Get the main frame
	CMainFrame *pFrame=GetMainFrame();

	// Create the control bar
	pFrame->CreateProjectControlBar(pInfo->m_ControlType);

	// Dock the window to the mainframe
	pFrame->FloatControlBar(pControlBar, position);

	// Make sure that the window gets properly sized
	pControlBar->RepositionControls();

	// Do special case initialization for this tab
	SpecialCaseTabInit(pControlBar, pInfo->m_ControlType);

	// Delete the tab info for this window
	delete m_TabInfo[nIndex];
	m_TabInfo.Remove(nIndex);

	// Remove the tab for this window
	m_Tabs.DeleteItem(nIndex);

	// There is not a visible dialog anymore (until a new tab is selected)
	m_pVisibleDlg=NULL;

	// Set the new tab
	int nTabs=m_TabInfo.GetSize();
	if (nTabs > 0)
	{
		if (nTabs <= nIndex)
		{
			// Go to the last tab
			SetTab(m_TabInfo[nTabs-1]->m_ControlType);
		}
		else
		{
			// Go the next tab in line
			SetTab(m_TabInfo[nIndex]->m_ControlType);
		}
	}

	// Redraw
	Invalidate();
	UpdateWindow();
}

/************************************************************************/
// This takes an undocked control bar and inserts it into the tab dialog
void CProjectBar::DockTab(CWnd *pWnd, CMainFrame::ProjectControl controlType)
{
	// Get the control bar
	CMRCSizeDialogBar *pControlBar=(CMRCSizeDialogBar *)pWnd;

	// Terminate the window
	SpecialCaseTabTerm(pControlBar, controlType);

	// Destroy the control bar window
	GetMainFrame()->DestroyProjectControlBar(controlType);

	// Add the tab info
	CProjectControlBarInfo *pTabInfo=GetMainFrame()->AddProjectControlInfo(m_TabInfo, controlType);

	// Create a new control bar window and attach it to the tab dialog
	pControlBar->Create(&m_Tabs, pTabInfo->m_DialogID, CBRS_NOALIGN, pTabInfo->m_SelectID);
	pControlBar->ShowWindow(SW_HIDE);

	// Do special case initialization for this tab
	SpecialCaseTabInit(pControlBar, controlType);

	// Add the tab to the tab control
	int nTabIndex=m_TabInfo.GetSize()-1;

	TC_ITEM item;
	memset( &item, 0, sizeof(item) );
	item.mask = TCIF_TEXT | TCIF_IMAGE;

	CString sTemp;
	sTemp.LoadString(pTabInfo->m_TitleStringID);
	item.pszText = sTemp.GetBuffer(0);
	item.cchTextMax = sTemp.GetLength();
	item.iImage = 0;

	m_Tabs.InsertItem(nTabIndex, &item );

	// Resize the items
	ResizeElements();

	// Redraw
	Invalidate();

	// Save the settings in the registry
	GetApp()->GetOptions().GetWindowsOptions()->SetBarControlArray(m_TabInfo);
}

/************************************************************************/
// Do special case initialization (for docking and undocking)
void CProjectBar::SpecialCaseTabInit(CMRCSizeDialogBar *pControlBar, CMainFrame::ProjectControl controlType)
{
	// Get the region doc
	CRegionDoc *pDoc=GetActiveRegionDoc();

	// Switch on the tab type
	switch (controlType)
	{
	case CMainFrame::CB_WORLDSVIEW:		// The world view
		{
			UpdateWorldsDlg();
			break;
		}
	case CMainFrame::CB_TEXTUREVIEW:	// The texture view
		{
			UpdateTextureDlg();
			break;
		}
	case CMainFrame::CB_NODESVIEW:
		{
			// The node view does not require any special case initialization
			break;
		}
	case CMainFrame::CB_PROPERTIESVIEW:	// The properties view
		{
			if (pDoc)
			{
				pDoc->SetupPropertiesDlg(FALSE);
			}
			break;
		}
	case CMainFrame::CB_MODELVIEW:		// The model view
		{
			UpdateModelDlg();
			break;
		}
	case CMainFrame::CB_SOUNDVIEW:		// The sound view
		{
			UpdateSoundDlg();
			break;
		}
	case CMainFrame::CB_SPRITEVIEW:		// The sprite view
		{
			UpdateSpriteDlg();
			break;
		}
	case CMainFrame::CB_PREFABVIEW:
		{
			UpdatePrefabDlg();
			break;
		}
	case CMainFrame::CB_CLASSVIEW:		// The class view
		{
			GetClassListDlg()->Update();
			break;
		}
	default:
		{
			ASSERT(FALSE);
		}
	}
}

/************************************************************************/
// Do special case termination (for docking and undocking)
void CProjectBar::SpecialCaseTabTerm(CMRCSizeDialogBar *pControlBar, CMainFrame::ProjectControl controlType)
{
	// Switch on the tab type
	switch (controlType)
	{
	case CMainFrame::CB_PROPERTIESVIEW:
		{
			GetPropertiesDlg()->Term();
			break;
		}
	}
}

int CProjectBar::FindTabBySelectID(DWORD id)
{
	DWORD i;

	for(i=0; i < m_TabInfo; i++)
		if(m_TabInfo[i]->m_SelectID == id)
			return (int)i;

	return -1;
}

void CProjectBar::OnImportSounds()
{
	if( VerifyProjectIsOpen() )
	{
		GetSoundDlg()->DoImportOperation();
	}
}


void CProjectBar::OnImportPcxFiles()
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoImportOperation(PCX_TO_TEXTURE);
	}
}


void CProjectBar::OnImportTgaFiles()
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoImportOperation(TGA_TO_TEXTURE);
	}
}


void CProjectBar::OnImportMipPcxFiles()
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoImportMipPcxOperation();
	}
}


void CProjectBar::OnCreateSolidAlpha()
{
	if(VerifyProjectIsOpen())
	{
		GetTextureDlg()->DoCreateAlphaMask(CAM_Solid);
	}
}


void CProjectBar::OnCreateAlphaFromColor()
{
	if(VerifyProjectIsOpen())
	{
		GetTextureDlg()->DoCreateAlphaMask(CAM_FromColor);
	}
}


void CProjectBar::OnImportAlphaMask()
{
	if(VerifyProjectIsOpen())
	{
		GetTextureDlg()->DoCreateAlphaMask(CAM_From4BitPCX);
	}
}


void CProjectBar::OnImport8BitAlphaMask()
{
	if(VerifyProjectIsOpen())
	{
		GetTextureDlg()->DoCreateAlphaMask(CAM_From8BitPCX);
	}
}


void CProjectBar::OnExportPcxFile()
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoExportPcxOperation();
	}
}


void CProjectBar::OnExport8BitDTXFile()
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoExport8BitDTXOperation();
	}
}

void CProjectBar::OnExportAllAsBPP_32P()
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoExportAllAsBPP_32POperation();
	}
}

void CProjectBar::OnTextureProperties()
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoTextureProperties();
	}
}

void CProjectBar::OnViewAllTextures()
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoViewAllTextures();
	}
}


void CProjectBar::OnImportTextures()
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoImportOperation(0);
	}
}

void CProjectBar::OnImportModels()
{
	if( VerifyProjectIsOpen() )
	{
		GetModelDlg()->DoImportOperation();
	}
}

void CProjectBar::OnPopupCreateNewSprite()
{
	CString				fileName, extension, str, fullName;
	CStringDlg			dlg;
	FILE				*fp;
	DFileIdent *pIdent;
	CMoFileIO file;
	int status;
	BOOL bCreate;		// RETAIL BUG: never set TRUE (0x403c9c tests a stale slot)


	if( VerifyProjectIsOpen() )
	{
		dlg.m_bAllowNumbers = TRUE;
		dlg.m_bAllowLetters = TRUE;
		dlg.m_MaxStringLen = 70;

		if( dlg.DoModal(IDS_NEWSPRITE, IDS_ENTERSPRITENAME) == IDOK )
		{
			extension.LoadString( IDS_SPRITE_EXTENSION );
			fileName = dfm_BuildName(GetSpriteDlg()->m_csCurrentDir, dlg.m_EnteredText + extension);
			fullName = dfm_GetFullFilename( GetFileMgr( ), fileName );

			// If the file already exist, ask if they want to overwrite...
			if( dfm_OpenFileRelative( GetFileMgr(), fileName, file ))
			{
				file.Close( );

				str.FormatMessage( IDS_SPRITE_ALREADY_EXISTS, dlg.m_EnteredText );
				status = AfxMessageBox( str, MB_YESNOCANCEL | MB_ICONQUESTION );

				if( status == IDNO )
				{
					bCreate = FALSE;
				}
				else if( status == IDCANCEL )
				{
					return;
				}
			}

			if( bCreate )
			{
				fp = fopen( fullName, "wb" );
				if( !fp )
				{
					str.FormatMessage( IDS_ERRORCREATING_SPRITE, dlg.m_EnteredText );
					AppMessageBox( str, MB_OK );
					return;
				}
				fclose( fp );
			}


			// Create the dialog and hook it to the document's stuff.
			dfm_GetFileIdentifier(GetFileMgr(), fileName, &pIdent);
			if(pIdent)
			{
				EditSpriteFile(pIdent);
			}

			UpdateAll();
		}
	}
}


void CProjectBar::OnPopupUpdate()
{
	UpdateAll();
}


void CProjectBar::GetLayoutFilename( const char *pGetFrom, char *pOutput )
{
	char		pathName[256], fileName[256];
	char		baseFilename[256], extension[256];
	CString		layoutFileName;
	CString		layoutExtension;

	CHelpers::ExtractPathAndFileName( pGetFrom, pathName, fileName );
	CHelpers::ExtractFileNameAndExtension( fileName, baseFilename, extension );

	layoutExtension.LoadString( IDS_LAYOUT_EXTENSION );
	layoutFileName = pathName;
	layoutFileName = dfm_BuildName( layoutFileName, baseFilename + layoutExtension );

	strcpy( pOutput, layoutFileName );
}

//------------------------------------------------------------------------------
//
//  CProjectBar::LoadLayoutFile()
//
//  Purpose:	Loads layout file containing regions left open when project was
//				closed.
//
//------------------------------------------------------------------------------
void CProjectBar::LoadLayoutFile( const char *pPathName )
{
	char				layoutFilename[256], docName[256];
	CString				docFilename, ext;
	CMoFileIO			file;

	BYTE				statusByte;
	CRect				rect;

	DWORD				i, nRegionDocs;
	CRegionView			*pRegionView;
	CRegionFrame		*pRegionFrame;
	CSplitterWnd		*pSplitter;
	POSITION			pos;

	CRegionDoc			*pRegionDoc;
	CObject				*pObj;

	CMultiDocTemplate	*pRegionTemp = GetApp()->m_pWorldTemplate;


	// Create layout name based on project name...
	GetLayoutFilename( pPathName, layoutFilename );

	if( file.Open(layoutFilename, "rb") )
	{
		// Read in all the Region docs and views.
		file >> nRegionDocs;
		nRegionDocs = MIN(nRegionDocs, 20);

		for( i=0; i < nRegionDocs; i++ )
		{
   			// Read document name...
   			file.ReadString(docName, 256);

			// Open the region document...
			pRegionDoc = OpenRegionDoc( docName );
			if( pRegionDoc )
			{
				// Read all views...
				pos = pRegionDoc->GetFirstViewPosition();
				while(1)
				{
					// Read status byte...
					// This byte is '1' if there is another view for this document, or '0'
					// if the last view was already read...
					file >> statusByte;

					// Handle condition when finished reading views...
					if( statusByte == 0 )
					{
						// Get the frame for the document...
						pRegionFrame = ( CRegionFrame * )pRegionView->GetParent()->GetParent();
						ASSERT( pRegionFrame );

						// Read in the window placement...
						WINDOWPLACEMENT wp;
						DWORD temp;
						wp.length = sizeof( WINDOWPLACEMENT );
						file >> temp;  wp.flags = temp;
						file >> temp;  wp.showCmd = temp;
						file >> wp.ptMinPosition.x;
						file >> wp.ptMinPosition.y;
						file >> wp.ptMaxPosition.x;
						file >> wp.ptMaxPosition.y;
						file >> wp.rcNormalPosition.left;
						file >> wp.rcNormalPosition.top;
						file >> wp.rcNormalPosition.right;
						file >> wp.rcNormalPosition.bottom;

						pRegionFrame->SetWindowPlacement( &wp );

						// Split the window panes evenly...
						pRegionFrame->GetClientRect( &rect );
						pSplitter = pRegionFrame->GetSplitter( );
						ASSERT( pSplitter );
						pSplitter->SetRowInfo( 0, rect.Height()/2, 10 );
						pSplitter->SetColumnInfo( 0, rect.Width()/2, 10 );
						pSplitter->RecalcLayout();
						pRegionFrame->Invalidate( FALSE );
						break;
					}
					// Handle condition when another view needs to be read...
					else if( statusByte == 1 )
					{
						// Get pointer to view...
						if( pos )
						{
							pObj = (CRegionView*)pRegionDoc->GetNextView( pos );
						}
						else
						{
							pObj = pRegionDoc->CreateNewRegionView();
						}

						// Load info about the view...
						if( pObj->IsKindOf( RUNTIME_CLASS( CRegionView )))
						{
							pRegionView = (CRegionView*)pObj;

							pRegionView->LoadProjFile( file );
						}
					}
					else
						return;
				}
			}
		}
		file.Close();
	}
}


//------------------------------------------------------------------------------
//
//  CProjectBar::SaveLayoutFile()
//
//  Purpose:	Saves layout file containing regions left open when project was
//				closed.
//
//------------------------------------------------------------------------------
void CProjectBar::SaveLayoutFile( const char *pPathName )
{
	char			layoutFilename[256];
	CMoFileIO		file;
	DWORD			i;
	CRegionDoc		*pDoc;

	CRegionView		*pRegionView;
	POSITION		pos;
	CRect			rect;


	// Create layout file name based on project file name...
	GetLayoutFilename( pPathName, layoutFilename );

	if( file.Open(layoutFilename, "wb") )
	{
		// Write out the CRegionDoc and view descriptions.
		file << m_RegionDocs.GetSize();
		for( i=0; i < m_RegionDocs; i++ )
		{
			pDoc = m_RegionDocs[i];

			// Write the filename of the doc...
			file.WriteString( (char*)((LPCTSTR)pDoc->m_FileName) );

			// Write out info on views...
			pos = pDoc->GetFirstViewPosition();
			while( pos )
			{
				pRegionView = (CRegionView*)pDoc->GetNextView(pos);

				if( pRegionView->IsKindOf(RUNTIME_CLASS(CRegionView)) )
				{
					// Write status byte which says view info follows...
					file << (BYTE)1;

					pRegionView->SaveProjFile( file );
				}
			}

			// Write status byte which says finished writing view info...
			file << (BYTE)0;

			WINDOWPLACEMENT wp;
			DWORD temp;
			CRegionFrame *pRegionFrame;

			// Write out frame position info...
			wp.length = sizeof( WINDOWPLACEMENT );
			ASSERT( pRegionView->GetParent( ));
			pRegionFrame = ( CRegionFrame * )pRegionView->GetParent()->GetParent( );
			ASSERT( pRegionFrame );
			pRegionFrame->GetWindowPlacement( &wp );

			wp.flags = 0;
			if( GetMainFrame( )->IsZoomed( ))
				wp.flags |= WPF_RESTORETOMAXIMIZED;
			temp = wp.flags;  file << temp;
			temp = wp.showCmd;  file << temp;
			file << wp.ptMinPosition.x;
			file << wp.ptMinPosition.y;
			file << wp.ptMaxPosition.x;
			file << wp.ptMaxPosition.y;
			file << wp.rcNormalPosition.left;
			file << wp.rcNormalPosition.top;
			file << wp.rcNormalPosition.right;
			file << wp.rcNormalPosition.bottom;
		}


		file.Close();
	}
}



void CProjectBar::UpdateTextureIDs()
{
	for( int i = 0; i < m_RegionDocs; i++ )
	{
		m_RegionDocs[i]->m_Region.UpdateTextureIDs( &m_Project );
	}
}


CRegionDoc* CProjectBar::OpenRegionDoc( CString fullName )
{
	CRegionDoc			*pDoc;
	CMultiDocTemplate	*pDocTemp = GetApp()->m_pWorldTemplate;
	POSITION			docPos;


	// This whole block replaces this line .. I had to prevent MFC from adding it to the MRU list.
	// pDoc = (CRegionDoc*)pDocTemp->OpenDocumentFile( pName, TRUE );
	{
		BOOL				bAutoDelete;
		CFrameWnd			*pFrame;


		for( docPos = pDocTemp->GetFirstDocPosition(); docPos != NULL; )
		{
			pDoc = ( CRegionDoc * )pDocTemp->GetNextDoc( docPos );
			if( !stricmp(pDoc->m_FileName, fullName))
			{
				POSITION pos;
				CView *pView;

				pos = pDoc->GetFirstViewPosition( );
				if( pos == NULL )
					return NULL;

				pView = pDoc->GetNextView( pos );
				pView->GetParentFrame( )->BringWindowToTop( );
				return NULL;
			}
		}

		pDoc = (CRegionDoc*)pDocTemp->CreateNewDocument();

		ASSERT_VALID(pDoc);

		bAutoDelete = pDoc->m_bAutoDelete;
		pDoc->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong

		pFrame = pDocTemp->CreateNewFrame(pDoc, NULL);
		ASSERT_VALID(pFrame);

		pDoc->m_bAutoDelete = bAutoDelete;

		// open an existing document
		CWaitCursor wait;
		if (!pDoc->OnOpenDocument(fullName))
		{
			// user has be alerted to what failed in OnOpenDocument
			TRACE0("CDocument::OnOpenDocument returned FALSE.\n");
			pFrame->DestroyWindow();
			return NULL;
		}

		pDoc->SetPathName(fullName, FALSE);
		pDoc->m_Region.UpdatePolyTriangles();
		pDocTemp->InitialUpdateFrame( pFrame, pDoc, TRUE );
	}

	pDoc->SetModifiedFlag(FALSE);
	// talon: CRegionDoc::SetTitle(bool) hides CDocument's; 2001 calls the
	// virtual one (0x4044fd). See the report to part 7.
	((CDocument*)pDoc)->SetTitle(pDoc->m_WorldName);

	m_RegionDocs.Append( pDoc );

	// Set view info...
	POSITION pos;
	CRegionView *pView;
	CRegionFrame *pRegionFrame;
	pos = pDoc->GetFirstViewPosition();
	ASSERT( pos );

	pView = ( CRegionView * )pDoc->GetNextView( pos );
	ASSERT( pView );
	pRegionFrame = ( CRegionFrame * )pView->GetParent()->GetParent();
	ASSERT( pRegionFrame );

	CRect rect;
	WINDOWPLACEMENT wPlace;
	AfxGetMainWnd( )->GetClientRect( &rect );
	AfxGetMainWnd( )->ClientToScreen( &rect );

	wPlace.length = sizeof( WINDOWPLACEMENT );
	pRegionFrame->GetWindowPlacement( &wPlace );
	wPlace.showCmd = SW_SHOWMAXIMIZED;
	pRegionFrame->SetWindowPlacement( &wPlace );

	pView->On4ViewConfiguration( );

	GetNodeView()->Init(pDoc);
	pDoc->NotifySelectionChange();
	return pDoc;
}


void CProjectBar::RemoveRegionDoc( CRegionDoc *pDoc )
{
	DWORD		index = m_RegionDocs.FindElement(pDoc);

	if( index != BAD_INDEX )
		m_RegionDocs.Remove( index );
}


void CProjectBar::EditSpriteFile(DFileIdent *pIdent)
{
	if( IsWindow( m_SpriteEditDlg.m_hWnd ))
		return;

	m_SpriteEditDlg.m_pFile = pIdent;
	m_SpriteEditDlg.Create( this );
	m_SpriteEditDlg.ShowWindow( SW_SHOW );
}


void CProjectBar::GetCurrentTextureName( char *szTextureFilename, BOOL bUseSprites )
{
	int nIndex;
	DFileIdent *pFile;

	if(bUseSprites)
	{
		if(GetSpriteDlg()->IsWindowVisible())
		{
			nIndex = GetSpriteDlg()->m_SpriteList.GetNextItem(-1,LVNI_SELECTED);
			if(nIndex != -1)
			{
				pFile = (DFileIdent*)GetSpriteDlg()->m_SpriteList.GetItemData(nIndex);
				if(pFile)
					strcpy(szTextureFilename, pFile->m_Filename);
				else
					szTextureFilename[0] = 0;

				return;
			}
		}
	}

	if(m_pCurTextureSel)
	{
		strcpy(szTextureFilename, m_pCurTextureSel->m_Filename);
	}
	else
	{
		szTextureFilename[0] = '\0';
	}
}

void CProjectBar::SetCurTextureSel(DFileIdent *pIdent)
{
	m_pCurTextureSel = pIdent;
}

void CProjectBar::FindCurTextureSel()
{
	GetTextureDlg()->FindTexture(m_pCurTextureSel);
}

static const TCHAR szProjectBarSection[] = _T("%s-ProjectBar");

void CProjectBar::RestoreRegistryInfo( LPCTSTR lpszProfileName )
{
	CWinApp* pApp = AfxGetApp();

	TCHAR szSection[256];
	wsprintf( szSection, szProjectBarSection, lpszProfileName);
}
 
void CProjectBar::SaveRegistryInfo( LPCTSTR lpszProfileName )
{
	CWinApp* pApp = AfxGetApp();

	TCHAR szSection[256];
	wsprintf( szSection, szProjectBarSection, lpszProfileName);

	// Save the settings in the registry
	GetApp()->GetOptions().GetWindowsOptions()->SetBarControlArray(m_TabInfo);
}

void CProjectBar::OnPrefabOpen() 
{
	GetPrefabDlg()->OnPrefabOpen();	
}

// NOTE: This function uses ID_WORLD_ADDDIR but is actually where the directory-adding code goes.
void CProjectBar::OnAddDir()
{
	CString path;
	CResNewDir dlg;
	CBaseRezDlg *pVisibleDlg;

	if(m_pVisibleDlg)
	{
		if(m_pVisibleDlg->IsKindOf(RUNTIME_CLASS(CBaseRezDlg)))
		{
			pVisibleDlg = (CBaseRezDlg*)m_pVisibleDlg;

			if(dlg.DoModal() == IDOK)
			{
				path = dfm_BuildName(pVisibleDlg->m_csCurrentDir, dlg.m_szNewDir);
				
				CResourceMgr::CreateDir(path, pVisibleDlg->m_ResourceType);
				pVisibleDlg->UpdateDirectories();
				pVisibleDlg->Invalidate(FALSE);
			}
		}
	}
}


// NEAR (194 of 194 bytes, funclets 0x404972 and 0x404998 too): the shipped
// frame is four bytes bigger (sub $0xe0, ours $0xdc), every offset the same:
// a dword below tempStr that no instruction touches. Tried: a BOOL result
// local, DVector for vDims.
BOOL CProjectBar::GetModelDims( char *szModelFile, CVector *pDims )
{
	DStream *pStream;
	CVector vDims;
	char tempStr[200];

	if( !pDims || !szModelFile )
		return FALSE;

	// {MD 1/18/98: editor was treating sprite files like model files and hanging..}
	strncpy(tempStr, szModelFile, sizeof(tempStr)-1);
	strupr(tempStr);
	if(strstr(tempStr, ".ABC"))
	{
		pStream = streamsim_Open(szModelFile, "rb");
		if(pStream)
		{
			LithTry
			{
				if(!GetModelDimsFromFile(pStream, &vDims))
				{
					pStream->Release();
					return FALSE;
				}
			}
			LithCatch(...)
			{
				pStream->Release();
				return FALSE;
			}

			pStream->Release();
			pDims->x = vDims.x;
			pDims->y = vDims.y;
			pDims->z = vDims.z;
			return TRUE;
		}
	}

	return FALSE;
}

void CProjectBar::OnAddToPalette() 
{
	if( VerifyProjectIsOpen() )
	{
		GetTextureDlg()->DoAddToPalette();
	}
}
 

void CProjectBar::OnFindTextureInWorld() 
{
	// Get the active document
	CRegionDoc *pDoc = GetActiveRegionDoc();
	if (!pDoc)
		return;

	// Do we even have anything selected?
	if (!m_pCurTextureSel)
		return;

	// Select the desired brushes
	pDoc->SelectBrushesByTexture(m_pCurTextureSel->m_Filename);
}

