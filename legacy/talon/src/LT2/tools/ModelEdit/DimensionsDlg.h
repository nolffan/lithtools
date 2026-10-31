#if !defined(AFX_DIMENSIONSDLG_H__FDFC39C1_8E6D_11D1_99E4_0060970987C3__INCLUDED_)
#define AFX_DIMENSIONSDLG_H__FDFC39C1_8E6D_11D1_99E4_0060970987C3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DimensionsDlg.h : header file
//


#include "model.h"


/////////////////////////////////////////////////////////////////////////////
// CDimensionsDlg dialog

class CDimensionsDlg : public CDialog
{
// Construction
public:
	CDimensionsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDimensionsDlg)
	enum { IDD = IDD_DIMENSIONS };
	CEdit	m_ZDim;
	CEdit	m_YDim;
	CEdit	m_XDim;
	//}}AFX_DATA


	// Set this before running the dialog.
	// talon: 2001's is a modal dialog for the selected animations
	// (0x402165..0x40243f): m_Anims holds 1024 (0x114..0x1114).
	Model *m_pModel;
	int m_Anims[1024];
	int m_nAnims;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDimensionsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDimensionsDlg)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIMENSIONSDLG_H__FDFC39C1_8E6D_11D1_99E4_0060970987C3__INCLUDED_)
