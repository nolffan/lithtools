// ProjectTabCtrl.cpp : implementation file
//

#include "bdefs.h"
#include "..\dedit.h"
#include "projecttabctrl.h"
#include "projectbar.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CProjectTabCtrl

CProjectTabCtrl::CProjectTabCtrl()
{
	m_pProjectBar=NULL;
}

CProjectTabCtrl::~CProjectTabCtrl()
{
}


BEGIN_MESSAGE_MAP(CProjectTabCtrl, CTabCtrl)
	//{{AFX_MSG_MAP(CProjectTabCtrl)
	ON_NOTIFY_REFLECT(TCN_SELCHANGING, OnSelchanging)
	ON_WM_LBUTTONDBLCLK()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CProjectTabCtrl message handlers

void CProjectTabCtrl::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult) 
{	
	*pResult = 0;
}

void CProjectTabCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{	
	// Get the currently selected tab
	int nCurTab=GetCurSel();

	CTabCtrl::OnLButtonDblClk(nFlags, point);

	// If the currently selected tab changed, then don't
	// allow the tab to be undocked from this click.
	if (nCurTab != GetCurSel())
	{
		return;
	}
	
	// Get the current item rectangle
	CRect rcItem;
	GetItemRect(nCurTab, rcItem);

	// See if the user clicked on this tab
	if (rcItem.PtInRect(point))
	{
		// Undock the tab
		m_pProjectBar->UndockTab(nCurTab, point);
	}	
}
