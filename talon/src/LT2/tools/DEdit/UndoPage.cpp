//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// UndoPage.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "undopage.h"
#include "mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CUndoPage property page

IMPLEMENT_DYNCREATE(CUndoPage, CPropertyPage)

// 2001 (0x4011f4 .. 0x4012a1): the auto-save time (dialog 106's only edit) and
// nothing else
CUndoPage::CUndoPage() : CPropertyPage(CUndoPage::IDD)
{
	//{{AFX_DATA_INIT(CUndoPage)
	m_dwAutoSaveTime = 0;
	//}}AFX_DATA_INIT

	m_psp.dwFlags |= PSP_USEHICON;
	m_psp.hIcon = AfxGetApp()->LoadIcon(IDI_UNDO_ICON);
}

CUndoPage::~CUndoPage()
{
}

void CUndoPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CUndoPage)
	DDX_Text(pDX, IDC_SAVETIME, m_dwAutoSaveTime);
	DDV_MinMaxInt(pDX, m_dwAutoSaveTime, 1, 32768);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CUndoPage, CPropertyPage)
	//{{AFX_MSG_MAP(CUndoPage)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
