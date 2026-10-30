//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
#if !defined(AFX_VECTOREDIT_H__BA288E82_0342_11D1_99E3_0060970987C3__INCLUDED_)
#define AFX_VECTOREDIT_H__BA288E82_0342_11D1_99E3_0060970987C3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// VectorEdit.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVectorEdit dialog

// talon: 2001's CVectorEdit (0x465289..0x4654e2): one "x y z" edit box
// (resource 182), the string at 0x5c and the vector at 0x60; no spin
// controls, increment, copy/paste, auto-apply or callback, and no
// destructor of its own.
class CVectorEdit : public CDialog
{
// Construction
public:
	CVectorEdit(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CVectorEdit)
	enum { IDD = IDD_VECTOR_EDIT };
	CString	m_sVector;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVectorEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CVectorEdit)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeVectorEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	// Call this before DoModal is called
	void	SetVector(CVector v)	{ m_vVector=v; }

	// Call this after DoModal
	CVector	GetVector()				{ return m_vVector; }

protected:
	// This is called to trim the trailing zeros from a floating point number
	void	TrimZeros(CString &sNumber);

protected:
	// The current vector
	CVector		m_vVector;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VECTOREDIT_H__BA288E82_0342_11D1_99E3_0060970987C3__INCLUDED_)
