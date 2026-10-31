//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
#ifndef __PREFABDLG_H__
#define __PREFABDLG_H__

// PrefabDlg.h : header file
//
// talon: 2001's prefab tab (0x4434a5..0x443ab5): a copy of the worlds tab
// listing the prefab worlds (.ed); a double click imports the prefab into
// the active world, "Open" opens it as a world. Build 69's (prefab
// references, thumbnails) is a different class.

#include "resourcemgr.h"
#include "baserezdlg.h"

/////////////////////////////////////////////////////////////////////////////
// CPrefabDlg dialog

class CPrefabDlg : public CBaseRezDlg
{
// Construction
public:
	CPrefabDlg();   // standard constructor
	virtual ~CPrefabDlg();	// destructor

// Dialog Data
	//{{AFX_DATA(CPrefabDlg)
	enum { IDD = IDD_PREFAB_TABDLG };
	CTreeCtrl	m_PrefabTree;
	CListCtrl	m_PrefabList;
	//}}AFX_DATA

	CImageList	*m_pIconList;
	CImageList	*m_pIconList2;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrefabDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL OnInitDialogBar();

	// Generated message map functions
	//{{AFX_MSG(CPrefabDlg)
	afx_msg void OnSize(UINT nType, int cx, int cy);	
	afx_msg void OnSelchangedDirectory(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkPrefabList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	// Opens the selected prefab as a world (CProjectBar::OnPrefabOpen calls it)
	afx_msg void OnPrefabOpen();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif
