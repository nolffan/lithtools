#if !defined(AFX_PREFERENCESDLG_H__INCLUDED_)
#define AFX_PREFERENCESDLG_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PreferencesDlg.h : header file
//
// 2001's "Script Editor Preference" dialog (IDD 183), opened by
// CMainFrame::OnFilePreferences (0x42f912). The class and file names are
// ours: the module sits between PrefabDlg and PreLeaf in the image
// (0x443ab5..0x443c2e), so its file name sorts there.

/////////////////////////////////////////////////////////////////////////////
// CPreferencesDlg dialog

class CPreferencesDlg : public CDialog
{
// Construction
public:
	CPreferencesDlg(CWnd* pParent = NULL);   // standard constructor (0x443ab5)

// Dialog Data
	//{{AFX_DATA(CPreferencesDlg)
	enum { IDD = 183 };
	CEdit	m_editScriptEditor;
	//}}AFX_DATA

	// The script editor's path (in and out)
	char	m_szScriptEditor[512];

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPreferencesDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPreferencesDlg)
	afx_msg void OnBrowse();
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PREFERENCESDLG_H__INCLUDED_)
