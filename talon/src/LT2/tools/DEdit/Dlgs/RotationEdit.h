//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
#if !defined(AFX_ROTATIONEDIT_H__5999A6E2_4297_11D1_B4A7_00A024805738__INCLUDED_)
#define AFX_ROTATIONEDIT_H__5999A6E2_4297_11D1_B4A7_00A024805738__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// RotationEdit.h : header file
//


#include "resource.h"

// talon: the 2001 dialog (resource 189) has one edit box, "Rotation
// (y/p/r):", id 1020, which the 2001 resource.h names nothing we have.
#ifndef IDC_ROTATIONTEXT
#define IDC_ROTATIONTEXT	1020
#endif


/////////////////////////////////////////////////////////////////////////////
// CRotationEdit dialog

// talon: 2001's CRotationEdit (0x45f76a..0x4600b5): three scroll bars, the
// angles' titles and a "y p r" edit box, no copy/paste, no auto-apply, no
// callback.  The layout is the constructor's (0x45f76a): the bars at 0x5c,
// 0x98, 0xd4, the strings at 0x110..0x11c, the angles at 0x120.
class CRotationEdit : public CDialog
{
// Construction
public:
	CRotationEdit(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRotationEdit)
	enum { IDD = IDD_ROTATIONEDIT };
	CScrollBar	m_YawBar;
	CScrollBar	m_RollBar;
	CScrollBar	m_PitchBar;
	CString	m_PitchText;
	CString	m_RollText;
	CString	m_YawText;
	CString	m_RotationText;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRotationEdit)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	LTVector	m_EulerAngles;

	void UpdateScrollTexts();
	void UpdateRotationText();
	void DrawGraphical();
	void DrawAngleToWnd(CWnd *pWnd, float radians);
	void TrimZeros(CString &str);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRotationEdit)
	virtual BOOL OnInitDialog();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	virtual void OnOK();
	afx_msg void OnPaint();
	afx_msg void OnChangeRotationText();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ROTATIONEDIT_H__5999A6E2_4297_11D1_B4A7_00A024805738__INCLUDED_)
