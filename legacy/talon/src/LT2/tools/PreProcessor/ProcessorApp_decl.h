#ifndef __PROCESSORAPP_DECL_H__
#define __PROCESSORAPP_DECL_H__

// talon: the MFC application of the shipped Processor.exe (AvP2 v1.0),
// 0x0042f580..0x004306e0: an AppWizard dialog application. The class
// names CPreProcessorApp and CPreProcessorDlg are ours; the thread's,
// CPreProcessorThread, is its CRuntimeClass's (0x475208, "CPreProcessor-
// Thread", 0x47ee7c). The resource IDs are the resources' (dialogs 100 and
// 102, icon 128, string 101), the names ours.

#define IDD_ABOUTBOX					100		// "About PreProcessor"
#define IDS_ABOUTBOX					101		// "&About PreProcessor..."
#define IDD_PREPROCESSOR_DIALOG			102		// "LithTech World Processor"
#define IDR_MAINFRAME					128		// the icon
#define IDM_ABOUTBOX					0x0010	// the system menu's About item

// Dialog 102's controls (hidden but the status edit, the slider and its
// label; each progress stage shows its label and bar).
#define IDC_PRIORITY					1000	// msctls_trackbar32 "Slider1", after "Process Priority"
#define IDC_PROGRESS_BSP				1001
#define IDC_PROGRESS_HULLS				1002
#define IDC_PROGRESS_PVS				1003
#define IDC_PROGRESS_SETUP				1004
#define IDC_TEXT_BSP					1005	// "Generating BSP"
#define IDC_TEXT_HULLS					1006	// "Generating Hulls"
#define IDC_TEXT_PVS					1007	// "Full Optimization"
#define IDC_TEXT_SETUP					1008	// "Setting Up"
#define IDC_TEXT_LIGHTING				1009	// "Lighting"
#define IDC_PROGRESS_LIGHTING			1010
#define IDC_STATUS						1011	// the status edit

// The application (no members of its own; vtable 0x474f28, message map
// 0x474ef0). theApp is at 0x485018.
class CPreProcessorApp : public CWinApp
{
public:
						CPreProcessorApp();			// 0x42f590

	virtual BOOL		InitInstance();				// 0x42f630

	DECLARE_MESSAGE_MAP()							// GetMessageMap 0x42f580
};

// The about box (dialog 100; vtable 0x475068, message map 0x474fc8).
class CAboutDlg : public CDialog
{
public:
						CAboutDlg();				// 0x42f6b0

	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);	// 0x42f720

	DECLARE_MESSAGE_MAP()							// GetMessageMap 0x42f730
};

// The main window (dialog 102, 0x2b8 bytes; vtable 0x475138, message map
// 0x474fe8), laid out from its constructor (0x42f740) and destructor
// (0x42f820).
class CPreProcessorDlg : public CDialog
{
public:
						CPreProcessorDlg(CWnd* pParent = NULL);	// 0x42f740
						~CPreProcessorDlg();		// 0x42f820

	enum { IDD = IDD_PREPROCESSOR_DIALOG };

	CSliderCtrl			m_Priority;		// 0x5c: the thread's priority, 0..5 (OnHScroll)

protected:
	HICON				m_hIcon;		// 0x98
	CRegMgr				m_RegMgr;		// 0x9c: HKLM\...\Monolith Productions\Lithtech\2.0\PreProcessor

	virtual void		DoDataExchange(CDataExchange* pDX);	// 0x42f8d0
	virtual BOOL		OnInitDialog();				// 0x42f900

	afx_msg void		OnSysCommand(UINT nID, LPARAM lParam);	// 0x42fff0
	afx_msg void		OnPaint();					// 0x430070
	afx_msg HCURSOR		OnQueryDragIcon();			// 0x430130
	afx_msg void		OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);	// 0x430140
	DECLARE_MESSAGE_MAP()							// GetMessageMap 0x42f8f0
};

// The processing thread (0x68 bytes, no members of its own; vtable
// 0x475244, message map 0x475220): its InitInstance does the processing.
class CPreProcessorThread : public CWinThread
{
	DECLARE_DYNCREATE(CPreProcessorThread)			// CreateObject 0x430570, GetRuntimeClass 0x4305e0
protected:
						CPreProcessorThread();		// 0x4305f0

public:
	virtual BOOL		InitInstance();				// 0x430640
	virtual int			ExitInstance();				// 0x4306c0

protected:
	virtual				~CPreProcessorThread();		// 0x430630

	DECLARE_MESSAGE_MAP()							// GetMessageMap 0x4306d0
};

#endif  // __PROCESSORAPP_DECL_H__
