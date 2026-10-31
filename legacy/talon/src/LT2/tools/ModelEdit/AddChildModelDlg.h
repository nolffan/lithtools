#if !defined(AFX_ADDCHILDMODELDLG_H__FAF12B10_D371_4304_AF4D_57AC5058345B__INCLUDED_)
#define AFX_ADDCHILDMODELDLG_H__FAF12B10_D371_4304_AF4D_57AC5058345B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddChildModelDlg.h : header file
//

#include "stringdlg.h"

/////////////////////////////////////////////////////////////////////////////
// CAddChildModelDlg dialog
//
// talon: 2001's (0x401062..0x401298): a CStringDlg on dialog 181, "Select
// child model(s)", whose Browse button fills the edit field from a
// multiple-selection file dialog. Build 69's is a CFileDialog template.

class CAddChildModelDlg : public CStringDlg
{
public:
	CAddChildModelDlg(CWnd* pParent = NULL);

	BOOL	m_bScaleSkeleton;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);

	//{{AFX_MSG(CAddChildModelDlg)
	afx_msg void OnBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDCHILDMODELDLG_H__FAF12B10_D371_4304_AF4D_57AC5058345B__INCLUDED_)
