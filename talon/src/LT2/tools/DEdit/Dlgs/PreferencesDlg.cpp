// PreferencesDlg.cpp : implementation file
//
// 2001's "Script Editor Preference" dialog, written from the image
// (0x443ab5..0x443c2e).

#include "bdefs.h"
#include "..\dedit.h"
#include "preferencesdlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// The edit control (1053)
#define IDC_EDIT_SCRIPT_EDITOR		1053

// The browse filter's string (0x443b56). The image's string table has no
// string 32796, so the filter is empty.
#define IDS_SCRIPT_EDITOR_FILTER	32796

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDlg dialog


CPreferencesDlg::CPreferencesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPreferencesDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPreferencesDlg)
	//}}AFX_DATA_INIT
}


void CPreferencesDlg::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CPreferencesDlg)
	DDX_Control(pDX, IDC_EDIT_SCRIPT_EDITOR, m_editScriptEditor);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPreferencesDlg, CDialog)
	//{{AFX_MSG_MAP(CPreferencesDlg)
	ON_BN_CLICKED(IDABORT, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDlg message handlers

void CPreferencesDlg::OnBrowse() 
{
	CString sFilter;
	sFilter.LoadString(IDS_SCRIPT_EDITOR_FILTER);

	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, sFilter, this);
	if (dlg.DoModal() == IDOK)
	{
		m_editScriptEditor.SetWindowText(dlg.GetPathName());
	}
}

void CPreferencesDlg::OnOK() 
{
	// (0x443bf1: 256, half the buffer)
	m_editScriptEditor.GetWindowText(m_szScriptEditor, 256);
	
	CDialog::OnOK();
}

void CPreferencesDlg::OnCancel() 
{
	CDialog::OnCancel();
}

BOOL CPreferencesDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_editScriptEditor.SetWindowText(m_szScriptEditor);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
