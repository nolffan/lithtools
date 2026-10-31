#if !defined(AFX_COMMANDSTRINGDLG_H__4431AA23_482A_11D1_B4AC_00A024805738__INCLUDED_)
#define AFX_COMMANDSTRINGDLG_H__4431AA23_482A_11D1_B4AC_00A024805738__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CommandStringDlg.h : header file
//

#include "model.h"


/////////////////////////////////////////////////////////////////////////////
// CCommandStringDlg dialog

class CCommandStringDlg : public CDialog
{
// Construction
public:
	CCommandStringDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCommandStringDlg)
	enum { IDD = IDD_COMMANDSTRINGEDIT };
	CEdit	m_CommandString;
	//}}AFX_DATA


	// Set this before running the dialog.
	Model *m_pModel;

	// talon: 2001's: the string the helper controls build (0x9c).
	char	m_szCommandString[2000];


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCommandStringDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCommandStringDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeHelperControl();
	afx_msg void OnChangeCommandString();
	//}}AFX_MSG

	// talon: 2001's command helper controls (0x401d68, 0x401e31).
	void	UpdateCommandString();
	void	UpdateControls();
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COMMANDSTRINGDLG_H__4431AA23_482A_11D1_B4AC_00A024805738__INCLUDED_)
