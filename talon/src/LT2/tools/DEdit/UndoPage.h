//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
#if !defined(AFX_UNDOPAGE_H__731D64A3_43FD_11D1_A408_006097098780__INCLUDED_)
#define AFX_UNDOPAGE_H__731D64A3_43FD_11D1_A408_006097098780__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// UndoPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CUndoPage dialog

class CUndoPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CUndoPage)

// Construction
public:
	CUndoPage();
	~CUndoPage();

// Dialog Data
	//{{AFX_DATA(CUndoPage)
	enum { IDD = IDD_PROPPAGE_UNDO };
	int		m_dwAutoSaveTime;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CUndoPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CUndoPage)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UNDOPAGE_H__731D64A3_43FD_11D1_A408_006097098780__INCLUDED_)
