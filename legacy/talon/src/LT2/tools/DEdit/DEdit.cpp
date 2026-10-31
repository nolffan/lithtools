//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// DEdit.cpp : Defines the class behaviors for the application.
//

#include "bdefs.h"
#include "butemgr.h"
#include "dedit.h"
#include "keymapdlg.h"

#include "optionsdisplay.h"
#include "mainfrm.h"

#include "edithelpers.h"

#include "regiondoc.h"
#include "regionview.h"
#include "regionframe.h"
#include "splash.h"

#include "editortransfer.h"
#include "regmgr.h"
#include "resourcemgr.h"

#include "projectbar.h"
#include <afxadv.h>
#include "stringdlg.h"
#include "dedit_concommand.h"

#include "newprojectdlg.h"
#include "deditoptions.h"
#include "version_resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


TCHAR szRegKeyCompany[] = _T( "LithTech Inc.\\Talon" );
TCHAR szRegKeyApp[] = _T( "DEdit" );
TCHAR szRegKeyVer[] = _T( "Settings" );


// talon: stand-in: 2001's Quick Help dialog (dialog 198, its constructor
// 0x44eeca), whose source (QuickHelpDlg.cpp by its place in the image)
// isn't written yet; its constructor is at the end of this file.
class CQuickHelpDlg : public CDialog
{
public:
	CQuickHelpDlg(CWnd* pParent = NULL);
};


/////////////////////////////////////////////////////////////////////////////
// CDEditApp

// 0x4d71b0
BEGIN_MESSAGE_MAP(CDEditApp, CWinApp)
	//{{AFX_MSG_MAP(CDEditApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_QUICKHELP, OnQuickHelp)
	ON_COMMAND(ID_KEYMAP, OnKeyMap)
	ON_COMMAND(ID_FILE_NEWPROJECT, OnFileNewProject)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_COMMAND(ID_FILE_CLOSEPROJECT, OnFileCloseProject)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEPROJECT, OnUpdateFileCloseProject)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_NEWWORLD, OnFileNewWorld)
	ON_UPDATE_COMMAND_UI(ID_FILE_NEWWORLD, OnUpdateFileNewWorld)
	ON_COMMAND(ID_FILE_SAVEWORLDAS, OnFileSaveWorldAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVEWORLDAS, OnUpdateFileSaveWorldAs)
	ON_COMMAND(ID_FILE_CLOSEWORLD, OnFileCloseWorld)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEWORLD, OnUpdateFileCloseWorld)
	ON_COMMAND_EX_RANGE(ID_FILE_MRU_FILE1, ID_FILE_MRU_FILE16, OnOpenRecentFile)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
	ON_COMMAND(ID_HELP_WWWDE, OnDEHomePage)
	ON_COMMAND(ID_HELP_ONLINEDOCUMENTATION, OnOnlineDoc)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDEditApp construction

// 0x41653e
CDEditApp::CDEditApp() : CWinApp("DEdit")
{
	m_pWorldTemplate		= NULL;
	m_TextureDisplaySize	= 64;
	m_bFullUpdate			= TRUE;

	// Init the global managers.
	dedit_InitConsoleCommands();
}


// 0x4165dc
CDEditApp::~CDEditApp()
{
	// Close the palette manager.
	dedit_TermConsoleCommands();

	// Terminate the class help a
	if (m_pClassHelpButeMgr)
	{
		m_pClassHelpButeMgr->Term();
		delete m_pClassHelpButeMgr;
		m_pClassHelpButeMgr=NULL;
	}
}

BOOL g_bGlobalUndoDisable=FALSE;

static char* FindArg( char *pName )
{
	int		i;
	char	withDash[256];

	sprintf( withDash, "%s%s", "-", pName );

	for( i=0; i < __argc; i++ )
	{
		if( CHelpers::UpperStrcmp(__argv[i], pName) || CHelpers::UpperStrcmp(__argv[i], withDash) )
		{
			if( i < (__argc-1) )
				return __argv[i+1];
			else
				return "";
		}
	}

	return NULL;
}

static void ProcessSpecialArgs()
{
	if(FindArg("noundo"))
		g_bGlobalUndoDisable = TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CDEditApp object

CDEditApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CDEditApp initialization

// 0x416687
BOOL CDEditApp::InitInstance()
{
	// Set the working directory and the EXE directory
	GetCurrentDirectory( sizeof(m_WorkingDir), m_WorkingDir );

	char szModuleFilename[MAX_PATH];	
	GetModuleFileName(m_hInstance, (char *)szModuleFilename, MAX_PATH);

	char szDrive[_MAX_DIR];
	char szPath[MAX_PATH];

	_splitpath ( (const char *)szModuleFilename, (char *)szDrive, (char *)szPath, NULL, NULL );	
	sprintf(m_ExeDirectory, "%s%s", szDrive, szPath);	

	RegisterEditorTransferFormat();
	SetRegistryKey( szRegKeyCompany );


	// CG: The following block was added by the Splash Screen component.
	{
		CCommandLineInfo cmdInfo;
		ParseCommandLine(cmdInfo);

		CSplashWnd::EnableSplashScreen(cmdInfo.m_bShowSplash);
	}

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	Enable3dControlsStatic();	// Call this when linking to MFC statically

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	m_pWorldTemplate = new CMultiDocTemplate(
		IDR_MAINFRAME,		
		RUNTIME_CLASS(CRegionDoc),
		RUNTIME_CLASS(CRegionFrame), // custom MDI child frame
		RUNTIME_CLASS(CRegionView));
	AddDocTemplate(m_pWorldTemplate);

   // enable file manager drag/drop and DDE Execute open
   EnableShellOpen();
   RegisterShellFileTypes();
   
   LoadRegistryStuff();

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Load the bute file for the class help	
	m_pClassHelpButeMgr=new CButeMgr;
	if (!m_pClassHelpButeMgr->Parse("ClassHlp.but"))
	{
		// Delete the ButeMgr file
		delete m_pClassHelpButeMgr;
		m_pClassHelpButeMgr=NULL;
	}
	else
	{
		// Don't display any ButeMgr errors
		m_pClassHelpButeMgr->SetDisplayFunc(NULL);
	}

	// The main window has been initialized, so show and update it.
	pMainFrame->UpdateWindow();

	ProcessSpecialArgs();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual BOOL OnInitDialog();
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CDEditApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// 0x416a82
void CDEditApp::OnQuickHelp()
{
	CQuickHelpDlg dlg;
	dlg.DoModal();
}

// 0x416abd
void CDEditApp::OnKeyMap()
{
	CKeyMapDlg dlg;
	dlg.DoModal();
}


/////////////////////////////////////////////////////////////////////////////
// CDEditApp commands

// 0x416b48
void CDEditApp::OnFileNewProject() 
{
	CString str, projFilename, fileExtension;
	FILE *fp;
	char dirName[500], dirNumString[10], title[200];
	int dirNum;
	DWORD dwAttribs;


	CNewProjectDlg dlg(GetMainWnd());

	// Fill in the defaults.
	for(dirNum=0; dirNum < 32000; dirNum++)
	{
		CHelpers::ExtractNames(m_pszExeName, dirName, NULL, NULL, NULL);
		sprintf(dirNumString, "%d", dirNum);
		
		dlg.m_ProjectDir = dirName;
		dlg.m_ProjectDir = dfm_BuildName(dlg.m_ProjectDir, "Project");
		dlg.m_ProjectDir += dirNumString;

		// If it doesn't exist, terminate the loop.. we've found a good name.
		dwAttribs = GetFileAttributes(dlg.m_ProjectDir);
		if(dwAttribs == 0xFFFFFFFF)
			break;
	}

	// Set the project dir to the full path.
	GetFullPathName(dlg.m_ProjectDir, 500, dirName, NULL);
	dlg.m_ProjectDir = dirName;

	CHelpers::ExtractNames(m_pszExeName, dirName, NULL, NULL, NULL);
	
	// Let them edit the stuff.
	if(dlg.DoModal() == IDOK)
	{
		// Save the current project...
		if(GetProjectBar()->IsProjectOpen())
			GetProjectBar( )->Save( );

		// Create project directory...
		if( !CreateDirectory(dlg.m_ProjectDir, NULL) )
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				str.FormatMessage(IDS_UNABLETOCREATEDIR, dlg.m_ProjectDir);
				AfxGetMainWnd()->MessageBox(str, AfxGetAppName(), MB_OK);
				return;
			}
		}

		fileExtension.LoadString(IDS_DEP_EXTENSION);
		CHelpers::ExtractNames(dlg.m_ProjectDir, NULL, title, NULL, NULL);
		projFilename = dfm_BuildName(dlg.m_ProjectDir, title + fileExtension);

		// Create project file...
		fp = fopen(projFilename, "wb");
		if(fp)
		{
			fclose(fp);
		}
		else
		{
			str.FormatMessage(IDS_ERRORCREATING_PROJECT, projFilename);
			AppMessageBox( str, MB_OK );
			return;
		}

		// Open the new project...
		if( GetProjectBar()->Open( projFilename ))
			AddToRecentFileList( projFilename );
		else
			return;
	}
	else
	{
		return;
	}

	// 2001's five resource directories (0x416d8f..0x416e2c)
	if( !CResourceMgr::CreateDir( "Worlds", RESTYPE_WORLD ) ||
		!CResourceMgr::CreateDir( "Sprites", RESTYPE_SPRITE ) ||
		!CResourceMgr::CreateDir( "Textures", RESTYPE_TEXTURE ) ||
		!CResourceMgr::CreateDir( "Sounds", RESTYPE_SOUND ) ||
		!CResourceMgr::CreateDir( "Models", RESTYPE_MODEL ))
	{
			str.FormatMessage( IDS_ERRORCREATING_PROJECT, title );
			AppMessageBox( str, MB_OK );
			return;
	}

	//prompt for new world
	if(AfxMessageBox(IDS_CREATENEWORLD,MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		CStringDlg			dlg;
		CString				worldpath,fileName, extension, fullName;
		CMoFileIO			file;
		BOOL				bCreate = TRUE;
		int				status;

		dlg.m_bAllowFile = TRUE;
		dlg.m_MaxStringLen = 70;
		dlg.m_bBeeping = TRUE;

		worldpath = "Worlds";

		if( dlg.DoModal(IDS_NEWWORLD, IDS_ENTERWORLDNAME) == IDOK )
		{
			extension.LoadString( IDS_WORLDEXTENSION );
			fileName = dfm_BuildName(worldpath, dlg.m_EnteredText + extension);
			fullName = dfm_GetFullFilename( GetFileMgr( ), fileName );

			// If the file already exist, ask if they want to overwrite...
			if( dfm_OpenFileRelative( GetFileMgr(), fileName, file ))
			{
				file.Close( );

				str.FormatMessage( IDS_WORLD_ALREADY_EXISTS, dlg.m_EnteredText );
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
					str.FormatMessage( IDS_ERRORCREATING_WORLD, dlg.m_EnteredText );
					AppMessageBox( str, MB_OK );
					return;
				}
				fclose( fp );
			}

			GetProjectBar( )->OpenRegionDoc( fullName );
		
			GetProjectBar( )->UpdateAll();
		}
	}
}

//----------------------------------------------------------------------------------------
//
//  CDEditApp::OnFileOpen() 
//
//  Purpose:	Opens project file.  This is done through a common open file dialog.  The
//				path is sent to the project bar object, which loads the layout file.  If
//				everything is successful, the file is added to the recent file list.
//
//----------------------------------------------------------------------------------------
// 0x4171dc
void CDEditApp::OnFileOpen() 
{
	CString			ext, filter;

	ext.LoadString( IDS_DEP_EXTENSION );
	filter.LoadString( IDS_FILEOPEN_FILTER );

	CFileDialog		dlg( TRUE, ext, NULL, 
							OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, 
							filter, AfxGetMainWnd( ));

	if( dlg.DoModal() == IDOK )
	{
		if( strlen( GetProjectBar( )->GetProjFilename( )))
			GetProjectBar( )->Save( );
		if( GetProjectBar()->Open( dlg.GetPathName( )))
			AddToRecentFileList( dlg.GetPathName( ));
	}
}


//----------------------------------------------------------------------------------------
//
//  CDEditApp::OnOpenRecentFile() 
//
//  Purpose:	Overrides default CWinApp::OnOpenRecentFile().  The override was needed
//				because the project file isn't a CDocument and doesn't follow the 
//				Doc/View framework.
//
//----------------------------------------------------------------------------------------
// 0x41731a
BOOL CDEditApp::OnOpenRecentFile(UINT nID)
{
	TCHAR szTemp[_MAX_PATH];

	ASSERT_VALID(this);
	ASSERT(m_pRecentFileList != NULL);

	ASSERT(nID >= ID_FILE_MRU_FILE1);
	ASSERT(nID < ID_FILE_MRU_FILE1 + (UINT)m_pRecentFileList->GetSize());
	int nIndex = nID - ID_FILE_MRU_FILE1;
	ASSERT((*m_pRecentFileList)[nIndex].GetLength() != 0);

	TRACE2("MRU: open file (%d) '%s'.\n", (nIndex) + 1,
			(LPCTSTR)(*m_pRecentFileList)[nIndex]);

	lstrcpy(szTemp, ( LPCTSTR )( *m_pRecentFileList )[ nIndex ]);
	if( strlen( GetProjectBar( )->GetProjFilename( )))
		GetProjectBar( )->Save( );
	if( GetProjectBar()->Open( szTemp ))
		AddToRecentFileList( szTemp );
	else
		m_pRecentFileList->Remove(nIndex);

	return TRUE;
}


BOOL CAboutDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the warning and build text
	CEdit *pEdit;
	CString str;

	pEdit = (CEdit *)GetDlgItem(IDC_ABOUTWARN);
	if( pEdit )
	{
		str.LoadString( IDS_ABOUTWARN );
		pEdit->SetWindowText( str );
	}

	pEdit = (CEdit *)GetDlgItem(IDC_BUILDSTRING);
	if (pEdit)
	{
		LTVersionInfo cVersion;
		char *pStr = str.GetBufferSetLength(256);
		if (GetLTExeVersion(GetApp()->m_hInstance, cVersion) == LT_OK)
			cVersion.GetString(pStr, 256);
		else
			str.LoadString( IDS_BUILDSTRING );
		pEdit->SetWindowText( str );
	}

	return TRUE;
}


// 0x417476: 2001's auto-save always runs, every GetAutoSaveTime() minutes,
// and saves the world itself.
BOOL CDEditApp::OnIdle( LONG lCount )
{
	CDocument			*pDoc;
	CRegionView			*pView;
	POSITION			docPos, viewPos;

	//SCHLEGZ: for Auto-Save
	static DWORD lastSaveTime = timeGetTime();
	
	switch( lCount )
	{
		case 0:
		{
			// Call all the views.
			for( docPos = m_pWorldTemplate->GetFirstDocPosition(); docPos != NULL; )
			{

				pDoc = m_pWorldTemplate->GetNextDoc( docPos );
			
				for( viewPos = pDoc->GetFirstViewPosition(); viewPos != NULL; )
				{
					pView = (CRegionView*)pDoc->GetNextView( viewPos );
					
					if( pView->IsKindOf( RUNTIME_CLASS(CRegionView) ) )
					{
						pView->OnIdle( lCount );
					}
				}

				// Get the autosave time
				DWORD dwAutoSaveTime = GetApp()->GetOptions().GetAutoSaveTime();
				if((timeGetTime() - lastSaveTime) > (dwAutoSaveTime * 60000))
				{
					CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();

					//update the status bar so that it will reflect the state
					CString csTemp = "Auto-saving: " + ((CRegionDoc*)pDoc)->m_FileName;
					pFrame->UpdateStatusText(0,( char * )LPCTSTR( csTemp ));

					//save the current time for the next time around
					lastSaveTime = timeGetTime();

					//save the backup file
					((CRegionDoc*)pDoc)->SaveRegion(FALSE);

					//return the status to ready
					pFrame->UpdateStatusText(0,"Ready");
				}
			}
			break;
		}
	}

	return CWinApp::OnIdle( lCount );
}


BOOL CDEditApp::PreTranslateMessage(MSG* pMsg)
{
	// CG: The following lines were added by the Splash Screen component.
	if (CSplashWnd::PreTranslateAppMessage(pMsg))
		return TRUE;

	return CWinApp::PreTranslateMessage(pMsg);
}

void CDEditApp::LoadRegistryStuff()
{	
	// Load the options
	m_DEditOptions.Init(&m_genRegMgr, "HKEY_CURRENT_USER\\Software\\LithTech Inc.\\Talon\\DEdit");
	m_DEditOptions.Load();
}


void CDEditApp::SaveRegistryStuff()
{
	// Save the options
	m_DEditOptions.Save();
}

BOOL CDEditApp::SaveAllModified() 
{
	SaveRegistryStuff();

	return CWinApp::SaveAllModified();
}

void CDEditApp::OnDEHomePage()
{
	ShellExecute(AfxGetMainWnd()->GetSafeHwnd(),"open","http://www.lithtech.com",NULL,NULL,SW_SHOW);

	return;
}

void CDEditApp::OnOnlineDoc()
{
	char szPath[MAX_PATH];
	sprintf(szPath, "%s\\lithtech.hlp", GetApp()->m_ExeDirectory);

	ShellExecute(AfxGetMainWnd()->GetSafeHwnd(),"open",szPath,NULL,NULL,SW_SHOW);

	return;
}

// 0x417699: 2001's creates an empty world file (Build 69 writes an LTA one).
void CDEditApp::OnFileNewWorld()
{
	CStringDlg			dlg;
	FILE				*fp;
	CMoFileIO			file;
	CString				fileName, extension, fullName, str;
	BOOL				bCreate;
	int					status;
	DDirIdent *pIdent;
	CString msg;


	if( GetProjectBar( )->VerifyProjectIsOpen() )
	{
		//make sure we have already selected a world dir
		if(strcmp(GetProjectBar( )->m_Project.m_BaseProjectDir,GetWorldsDlg()->GetCurDirPath( )) == 0)
		{
			AfxMessageBox(IDS_BADWORLDDIR,MB_OK | MB_ICONINFORMATION);
			return;
		}

		dlg.m_bAllowFile = TRUE;
		dlg.m_MaxStringLen = 70;
		dlg.m_bBeeping = TRUE;

		if( dlg.DoModal(IDS_NEWWORLD, IDS_ENTERWORLDNAME) == IDOK )
		{
			extension.LoadString( IDS_WORLDEXTENSION );
			
			// Make sure they have a worlds directory selected.
			if(GetWorldsDlg()->IsDirectorySelected())
			{
				pIdent = GetWorldsDlg()->GetSelectedDirectory();
				if(!pIdent)
					return;
			}
			else
			{
				pIdent = GetWorldsDlg()->GetFirstDirectory();
				if(pIdent)
				{
					msg.FormatMessage(IDS_NOWORLDDIRSELECTED, pIdent->m_Filename);
					if(AppMessageBox(msg, MB_OKCANCEL) == IDOK)
					{
					}
					else
					{
						return;
					}
				}
				else
				{
					AppMessageBox(IDS_MUSTHAVEONEWORLDSDIRECTORY, MB_OK);
					return;
				}
			}

			fileName = dfm_BuildName(pIdent->m_Filename, dlg.m_EnteredText + extension);
			fullName = dfm_GetFullFilename(GetFileMgr(), fileName);

			// If the file already exist, ask if they want to overwrite...
			if( dfm_OpenFileRelative( GetFileMgr(), fileName, file ))
			{
				file.Close( );

				str.FormatMessage( IDS_WORLD_ALREADY_EXISTS, dlg.m_EnteredText );
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

			// talon: retail bug: bCreate is never set unless the file exists
			// and they answer No (0x4178b1 reads a dead temporary's slot).
			if( bCreate )
			{
				fp = fopen( fullName, "wb" );
				if( !fp )
				{
 					str.FormatMessage( IDS_ERRORCREATING_WORLD, dlg.m_EnteredText );
 					AppMessageBox( str, MB_OK );
 					return;
				}
				fclose( fp );
			}

			GetProjectBar( )->OpenRegionDoc(fullName);
			GetWorldsDlg()->UpdateDirectories( );
			GetWorldsDlg()->PopulateList( );
		}
	}
}

void CDEditApp::OnUpdateFileNewWorld( CCmdUI *pCmdUI )
{
	if( GetProjectBar( )->IsProjectOpen() )
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CDEditApp::OnFileSave()
{
	GetProjectBar()->Save();

	return;
}

void CDEditApp::OnUpdateFileSave( CCmdUI *pCmdUI )
{
	if( GetProjectBar( )->IsProjectOpen() )
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);

}

void CDEditApp::OnFileCloseProject()
{
	GetProjectBar()->Close();

	return;
}

void CDEditApp::OnUpdateFileCloseProject( CCmdUI *pCmdUI )
{
	if( GetProjectBar( )->IsProjectOpen() )
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);

}

// 0x4179da: 2001's saves the world, without asking, before closing it.
void CDEditApp::OnFileCloseWorld()
{
	CMDIFrameWnd *pFrame = (CMDIFrameWnd*)AfxGetApp()->m_pMainWnd;
	CMDIChildWnd *pChild = (CMDIChildWnd *) pFrame->GetActiveFrame();
	CView *pView = (CView *) pChild->GetActiveView();
	CRegionDoc *pDoc = (CRegionDoc*)pView->GetDocument();

	pDoc->SaveRegion(TRUE);
	m_pWorldTemplate->RemoveDocument( pDoc );
	pDoc->OnCloseDocument();
}

void CDEditApp::OnUpdateFileCloseWorld( CCmdUI *pCmdUI )
{
	if( ((CMainFrame*)m_pMainWnd)->GetNumRegionDocs() > 0 )
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

// 0x417a36: 2001's backup name is a temporary file named from the world's
// first three letters; .ed and .obj saves.
void CDEditApp::OnFileSaveWorldAs()
{
	CString			ext, filter, sFileName;

	ext.LoadString( IDS_WORLDEXTENSION );
	filter.LoadString( IDS_FILESAVEAS_FILTER );
	sFileName = GetProject()->m_BaseProjectDir + "\\Worlds\\*" + ext;

	CFileDialog		dlg(FALSE, ext, (LPCTSTR)sFileName, OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT, filter, AfxGetMainWnd( ));

	if( dlg.DoModal() == IDOK )
	{
		CMDIFrameWnd *pFrame = (CMDIFrameWnd*)AfxGetApp()->m_pMainWnd;
		CMDIChildWnd *pChild = (CMDIChildWnd *) pFrame->GetActiveFrame();
		CView *pView = (CView *) pChild->GetActiveView();
		CRegionDoc *pDoc = (CRegionDoc*)pView->GetDocument();

		char	pathName[256], fileName[256];
		char	worldName[256], extension[256], backupName[256];

		CHelpers::ExtractPathAndFileName( dlg.GetPathName(), pathName, fileName );
		CHelpers::ExtractFileNameAndExtension( fileName, worldName, extension );

		pDoc->m_FileName = dlg.GetPathName();	
		pDoc->m_WorldName = worldName;
		// talon: CDocument's virtual SetTitle (0x417bb6); Build 69's
		// CRegionDoc::SetTitle(bool) hides it
		((CDocument*)pDoc)->SetTitle(pDoc->m_WorldName);
		pDoc->SetPathName(dlg.GetPathName(),FALSE);

		GetTempPath(256, pathName);
		fileName[3] = 0;
		GetTempFileName(pathName, fileName, 0, backupName);
		pDoc->m_BackupName = backupName;

		if( stricmp(extension,"ed") == 0 ) 
		{
			pDoc->SaveRegion(TRUE);
		}
		else if( stricmp(extension,"obj") == 0 ) 
		{
			pDoc->SaveOBJ(TRUE);
		}
		GetProjectBar( )->UpdateAll();
	}

	return;
}

void CDEditApp::OnUpdateFileSaveWorldAs( CCmdUI *pCmdUI )
{
	if( ((CMainFrame*)m_pMainWnd)->GetNumRegionDocs() > 0 )
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}


// ------------------------------------------------------------------------
// talon: stand-in: the Quick Help dialog (see the top of this file).
// ------------------------------------------------------------------------

CQuickHelpDlg::CQuickHelpDlg(CWnd* pParent)
	: CDialog(IDD_QUICKHELP, pParent)
{
}

// talon: no DefStdlithAlloc and DefStdlithFree here: the 2001 StdLith
// (its allocator-override build, as ModelEdit's) has them, in l_allocator
// (0x00489640, 0x00489660).
