//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// RegionFrame.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "regionframe.h"
#include "regiondoc.h"
#include "regionview.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegionFrame

IMPLEMENT_DYNCREATE(CRegionFrame, CMDIChildWnd)

CRegionFrame::CRegionFrame()
{
	m_bValid = FALSE;
}

CRegionFrame::~CRegionFrame()
{
}


BEGIN_MESSAGE_MAP(CRegionFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CRegionFrame)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()







/////////////////////////////////////////////////////////////////////////////
// CRegionFrame message handlers

BOOL CRegionFrame::PreCreateWindow(CREATESTRUCT& cs) 
{
//	cs.style = WS_CHILD | WS_CAPTION
//		| WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE;
	
	return CMDIChildWnd::PreCreateWindow(cs);
}

BOOL CRegionFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CMDIFrameWnd* pParentWnd, CCreateContext* pContext) 
{
	// Create frame window
	if (!CMDIChildWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, pContext))
		return FALSE;

	// Recalculate layout
	RecalcLayout();

	return TRUE;
}

BOOL CRegionFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
	// create a splitter with 2 rows, 2 columns
	if (!m_wndSplitter.CreateStatic(this, 2, 2))
	{
		TRACE0("Failed to CreateStaticSplitter\n");
		return FALSE;
	}

	m_wndSplitter.CreateView( 0, 0, RUNTIME_CLASS(CRegionView), CSize(130, 50), pContext );
	m_wndSplitter.CreateView( 1, 0, RUNTIME_CLASS(CRegionView), CSize(130, 50), pContext );
	m_wndSplitter.CreateView( 0, 1, RUNTIME_CLASS(CRegionView), CSize(130, 50), pContext );
	m_wndSplitter.CreateView( 1, 1, RUNTIME_CLASS(CRegionView), CSize(130, 50), pContext );

	// activate the input view
	SetActiveView((CView*)m_wndSplitter.GetPane(0,0));

	m_bValid = TRUE;

	return TRUE;
}





void CRegionFrame::OnSize(UINT nType, int cx, int cy) 
{
	CRect rect;

	CMDIChildWnd::OnSize(nType, cx, cy);

	if( m_bValid )
	{
		GetClientRect( &rect );
		// talon: 0x452894..0x4528d4 stores each size as a double and reads
		// it back (fstpl/fldl -0x8(%ebp)) before the multiply; a volatile
		// double is the only form found that does (a plain double local, a
		// float cast, an inline helper and /2.0 all keep it on the FPU
		// stack). The 2001 spelling is not known.
		volatile double dSize;
		dSize = rect.Height();
		m_wndSplitter.SetRowInfo( 0, ( int )( dSize * 0.5 ), 10 );
		dSize = rect.Width();
		m_wndSplitter.SetColumnInfo( 0, ( int )( dSize * 0.5 ), 10 );
		m_wndSplitter.RecalcLayout();
	}
}
