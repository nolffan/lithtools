//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
#ifndef __SPRITEDLG_H__
#define __SPRITEDLG_H__


#include "resourcemgr.h"
#include "baserezdlg.h"

/////////////////////////////////////////////////////////////////////////////
// CSpriteDlg dialog

// 2001: a CBaseRezDlg with its own icon lists, as CModelDlg (no CBaseImgDlg,
// no thumbnails: CSpriteDlg::OnInitDialogBar, 0x408209)
class CSpriteDlg : public CBaseRezDlg
{
// Construction
public:
	CSpriteDlg();   // standard constructor
	virtual ~CSpriteDlg();	// destructor

// Dialog Data
	//{{AFX_DATA(CSpriteDlg)
	enum { IDD = IDD_SPRITE_TABDLG };
	CTreeCtrl	m_SpriteTree;
	CListCtrl	m_SpriteList;
	//}}AFX_DATA

	CImageList	*m_pIconList;
	CImageList	*m_pIconList2;

// Overrides
	//{{AFX_VIRTUAL(CSpriteDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL OnInitDialogBar();

	//{{AFX_MSG(CSpriteDlg)	
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangedDirectory(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClkSprite( NMHDR * pNMHDR, LRESULT * pResult );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}


#endif
