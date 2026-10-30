// talon: 0x0042f580..0x004306e0, after PrePoly: the MFC application (an
// AppWizard dialog app: CPreProcessorApp, its about box and its window,
// CPreProcessorDlg, which shows the Processing Options dialog and starts
// CPreProcessorThread to do the processing), the system-dependent
// functions Processing calls (status text, progress stages, questions),
// then, from 0x4306e0, PreSurface.cpp. The file name is ours and
// provisional: by the image's order (and Visual C++'s alphabetical project
// files) this is likely PreProcessor.cpp (the app, ..0x42f6b0) and
// PreProcessorDlg.cpp (..0x4306e0).

#include "bdefs.h"
#include <mmsystem.h>
#include "regmgr.h"
#include "processing.h"
#include "ProcessorOptions_decl.h"
#include "ProcessorApp_decl.h"
#include "presurface.h"
#include "preplane.h"
#include "prepoly.h"
#include "preworld.h"
#include "node.h"
#include "lightmap_planes.h"
#include "bspgen.h"


// ------------------------------------------------------------------------ //
// The globals (the names are ours).
// ------------------------------------------------------------------------ //

// The processor's options (0x4850d8), which the window fills from the
// command line and the options dialog and the thread hands DoProcessing.
CProcessorGlobs			g_AppGlobs;

// The window (0x4855f8) and the processing thread (0x4855fc).
CPreProcessorDlg		*g_pAppDlg;
CWinThread				*g_pAppThread;

// The progress bar of the stage in progress (0x485600).
CProgressCtrl			*g_pAppProgress;


// ------------------------------------------------------------------------ //
// CPreProcessorApp.
// ------------------------------------------------------------------------ //

// 0x474ef0 (GetMessageMap 0x42f580)
BEGIN_MESSAGE_MAP(CPreProcessorApp, CWinApp)
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// 0x42f590
CPreProcessorApp::CPreProcessorApp()
{
}


// 0x485018 (constructed at 0x42f600, destroyed at 0x42f620).
CPreProcessorApp theApp;


// 0x42f630
BOOL CPreProcessorApp::InitInstance()
{
	Enable3dControlsStatic();

	CPreProcessorDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	return FALSE;
}


// ------------------------------------------------------------------------ //
// CAboutDlg.
// ------------------------------------------------------------------------ //

// 0x42f6b0
CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

// 0x42f720
void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

// 0x474fc8 (GetMessageMap 0x42f730): no entries.
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// ------------------------------------------------------------------------ //
// CPreProcessorDlg.
// ------------------------------------------------------------------------ //

// 0x42f740
CPreProcessorDlg::CPreProcessorDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPreProcessorDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}


// 0x42f820: the processing thread is killed if it's still running.
CPreProcessorDlg::~CPreProcessorDlg()
{
	if(g_pAppThread)
	{
		DWORD exitCode;

		GetExitCodeThread(g_pAppThread->m_hThread, &exitCode);
		TerminateThread(g_pAppThread->m_hThread, exitCode);
		delete g_pAppThread;
	}
}


// 0x42f8d0
void CPreProcessorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PRIORITY, m_Priority);
}


// 0x474fe8 (GetMessageMap 0x42f8f0)
BEGIN_MESSAGE_MAP(CPreProcessorDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()


// 0x42f900: the world file is the command line's first argument ("-
// skipdialog" after it runs without questions and ends the dialog when
// done); the options dialog runs first, then the window starts the thread.
BOOL CPreProcessorDlg::OnInitDialog()
{
	CProcessingOptionsDlg optionsDlg;
	uint32 len;
	char *pExt;

	if(__argc < 2)
	{
		MessageBox("No world file specified.", "PreProcessor");
		PostQuitMessage(0);
		return TRUE;
	}

	memset(&g_AppGlobs, 0, sizeof(g_AppGlobs));
	strcpy(g_AppGlobs.m_InputFile, __argv[1]);

	len = strlen(g_AppGlobs.m_InputFile);
	if(len > 3)
	{
		pExt = &g_AppGlobs.m_InputFile[len - 3];
		if(stricmp(pExt, ".ed") == 0)
			*pExt = 0;
	}

	sprintf(g_AppGlobs.m_OutputFile, "%s.dat", g_AppGlobs.m_InputFile);

	// m_bSkipDialog: 0x4855b4.
	if(__argc > 2 && strlen(__argv[2]) > 1 && stricmp(&__argv[2][1], "skipdialog") == 0)
		g_AppGlobs.m_bSkipDialog = TRUE;

	optionsDlg.m_pGlobs = &g_AppGlobs;
	if(optionsDlg.DoModal() != IDOK)
	{
		PostQuitMessage(0);
		return TRUE;
	}

	CDialog::OnInitDialog();

	m_RegMgr.Init("Monolith Productions", "Lithtech", "2.0", "PreProcessor", HKEY_LOCAL_MACHINE, NULL);
	m_Priority.SetRange(0, 5);
	m_Priority.SetPos(m_RegMgr.Get("Priority", 3));

	// Add "About..." menu item to system menu (the wizard's NULL check
	// isn't there).
	CMenu* pSysMenu = GetSystemMenu(FALSE);
	CString strAboutMenu;
	strAboutMenu.LoadString(IDS_ABOUTBOX);
	if (!strAboutMenu.IsEmpty())
	{
		pSysMenu->AppendMenu(MF_SEPARATOR);
		pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	g_pAppDlg = this;
	g_pAppThread = AfxBeginThread(RUNTIME_CLASS(CPreProcessorThread));

	return TRUE;
}


// 0x42fff0
void CPreProcessorDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}


// 0x430070
void CPreProcessorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}


// 0x430130
HCURSOR CPreProcessorDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


// 0x430140: the slider sets the thread's priority and saves it.
void CPreProcessorDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

	int priority = m_Priority.GetPos();
	m_RegMgr.Set("Priority", (DWORD)priority);

	if(g_pAppThread)
	{
		if(priority == 0)
			g_pAppThread->SetThreadPriority(THREAD_PRIORITY_IDLE);
		else if(priority == 1)
			g_pAppThread->SetThreadPriority(THREAD_PRIORITY_LOWEST);
		else if(priority == 2)
			g_pAppThread->SetThreadPriority(THREAD_PRIORITY_BELOW_NORMAL);
		else if(priority == 3)
			g_pAppThread->SetThreadPriority(THREAD_PRIORITY_NORMAL);
		else if(priority == 4)
			g_pAppThread->SetThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);
		else if(priority == 5)
			g_pAppThread->SetThreadPriority(THREAD_PRIORITY_HIGHEST);
	}
}


// ------------------------------------------------------------------------ //
// The functions Processing calls (Processing.h).
// ------------------------------------------------------------------------ //

// 0x430210: DoProcessing's clock (0x440980, 0x440eec). The name is ours.
DWORD ProcessorApp_GetTime()
{
	return timeGetTime();
}


// 0x430220. Retail: the caret goes to the edit's text limit
// (EM_GETLIMITTEXT), not its length: EM_SETSEL clamps it to the end, but
// once the text reaches the limit EM_REPLACESEL should add nothing more, so
// the window would stop showing status lines on a long run (inferred from
// the messages, not run; the log file has them all).
void DrawStatusTextToScreen(const char *pMsg)
{
	CEdit *pEdit;
	char str[500];
	int len;

	pEdit = (CEdit*)g_pAppDlg->GetDlgItem(IDC_STATUS);
	if(pEdit)
	{
		sprintf(str, "%s\r\n", pMsg);
		len = pEdit->GetLimitText();
		pEdit->SetSel(len, len, TRUE);
		pEdit->ReplaceSel(str);
	}
}


// Shows a stage's label and progress bar and makes the bar the current
// one (0x430330, the name is ours).
static void ProcessorApp_StartStage(int textID, int progressID)
{
	g_pAppProgress = (CProgressCtrl*)g_pAppDlg->GetDlgItem(progressID);
	if(g_pAppProgress)
	{
		g_pAppDlg->GetDlgItem(textID)->ShowWindow(SW_SHOW);
		g_pAppDlg->GetDlgItem(progressID)->ShowWindow(SW_SHOW);
		g_pAppProgress->SetRange(0, 1000);
	}
}


// 0x4302a0
void StartProgressStage(int iStage)
{
	if(iStage == PROGRESS_BSP)
	{
		ProcessorApp_StartStage(IDC_TEXT_BSP, IDC_PROGRESS_BSP);
	}
	else if(iStage == 1)
	{
		ProcessorApp_StartStage(IDC_TEXT_LIGHTING, IDC_PROGRESS_LIGHTING);
	}
	else if(iStage == PROGRESS_HULLS)
	{
		ProcessorApp_StartStage(IDC_TEXT_HULLS, IDC_PROGRESS_HULLS);
	}
	else if(iStage == PROGRESS_PVS)
	{
		ProcessorApp_StartStage(IDC_TEXT_PVS, IDC_PROGRESS_PVS);
		g_pAppDlg->SetDlgItemText(IDC_TEXT_PVS, "Approximation");
	}
	else if(iStage == PROGRESS_MERGEBRUSHES)
	{
		ProcessorApp_StartStage(IDC_TEXT_SETUP, IDC_PROGRESS_SETUP);
	}
}


// Fills the stage's bar; with bShow its label and bar are shown (again:
// they already are). 0x430440, the name is ours.
static void ProcessorApp_EndStage(int textID, int progressID, BOOL bShow)
{
	if(g_pAppProgress)
	{
		((CProgressCtrl*)g_pAppDlg->GetDlgItem(progressID))->SetPos(1000);

		if(bShow)
		{
			g_pAppDlg->GetDlgItem(textID)->ShowWindow(SW_SHOW);
			g_pAppDlg->GetDlgItem(progressID)->ShowWindow(SW_SHOW);
		}

		g_pAppProgress = NULL;
	}
}


// 0x4303a0
void EndProgressStage(int iStage, BOOL bUnknown)
{
	if(iStage == PROGRESS_BSP)
	{
		ProcessorApp_EndStage(IDC_TEXT_BSP, IDC_PROGRESS_BSP, bUnknown);
	}
	else if(iStage == 1)
	{
		ProcessorApp_EndStage(IDC_TEXT_LIGHTING, IDC_PROGRESS_LIGHTING, bUnknown);
	}
	else if(iStage == PROGRESS_HULLS)
	{
		ProcessorApp_EndStage(IDC_TEXT_HULLS, IDC_PROGRESS_HULLS, bUnknown);
	}
	else if(iStage == PROGRESS_PVS)
	{
		ProcessorApp_EndStage(IDC_TEXT_PVS, IDC_PROGRESS_PVS, bUnknown);
	}
	else if(iStage == PROGRESS_MERGEBRUSHES)
	{
		ProcessorApp_EndStage(IDC_TEXT_SETUP, IDC_PROGRESS_SETUP, bUnknown);
	}
}


// 0x4304c0
void SetProgressBar(float fPercent)
{
	if(g_pAppProgress)
	{
		g_pAppProgress->SetPos((int)(fPercent * 1000.0f));
	}
}


// 0x4304f0
void SetFullOptimization()
{
	g_pAppDlg->SetDlgItemText(IDC_TEXT_PVS, "Full Optimization");
}


// 0x430510: with -skipdialog every question is answered no.
uint32 AskQuestion(const char *pQuestion, uint32 type)
{
	UINT mbType = MB_YESNO;
	int ret;

	if(g_AppGlobs.m_bSkipDialog)
		return QUES_NO;

	if((type & QUES_YES) && (type & QUES_NO) && (type & QUES_CANCEL))
		mbType = MB_YESNOCANCEL;

	ret = g_pAppDlg->MessageBox(pQuestion, "PreProcessor", mbType);
	if(ret == IDYES)
		return QUES_YES;
	else if(ret == IDNO)
		return QUES_NO;
	else if(ret == IDCANCEL)
		return QUES_CANCEL;

	// Retail: no return here (0x43056f returns MessageBox's own code in
	// eax; with MB_YESNO and MB_YESNOCANCEL it can't get here).
#pragma warning(disable : 4715)
}
#pragma warning(default : 4715)


// ------------------------------------------------------------------------ //
// CPreProcessorThread.
// ------------------------------------------------------------------------ //

// 0x475208 (CreateObject 0x430570, GetRuntimeClass 0x4305e0)
IMPLEMENT_DYNCREATE(CPreProcessorThread, CWinThread)

// 0x4305f0
CPreProcessorThread::CPreProcessorThread()
{
}

// 0x430630
CPreProcessorThread::~CPreProcessorThread()
{
}


// 0x430640: does the processing, then shows the OK button (or with
// -skipdialog ends the window) and plays the default sound. FALSE ends the
// thread.
BOOL CPreProcessorThread::InitInstance()
{
	CWnd *pOK;

	DoProcessing(&g_AppGlobs);
	g_pAppThread = NULL;

	if(!g_AppGlobs.m_bSkipDialog && (pOK = g_pAppDlg->GetDlgItem(IDOK)) != NULL)
	{
		pOK->ShowWindow(SW_SHOW);
		pOK->SetFocus();
	}
	else
	{
		if(g_pAppDlg)
			g_pAppDlg->EndDialog(IDOK);
	}

	PlaySound("Default", NULL, 0);
	return FALSE;
}


// 0x4306c0
int CPreProcessorThread::ExitInstance()
{
	return CWinThread::ExitInstance();
}


// 0x475220 (GetMessageMap 0x4306d0): no entries.
BEGIN_MESSAGE_MAP(CPreProcessorThread, CWinThread)
END_MESSAGE_MAP()
