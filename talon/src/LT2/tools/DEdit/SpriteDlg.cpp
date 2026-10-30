//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// SpriteDlg.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "spritedlg.h"
#include "editprojectmgr.h"
#include "edithelpers.h"
#include "mainfrm.h"
#include "projectbar.h"
#include "resnewdir.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSpriteDlg dialog


CSpriteDlg::CSpriteDlg()
{
	//{{AFX_DATA_INIT(CSpriteDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pIconList=NULL;
	m_pIconList2=NULL;

	InitBaseRezDlg("spr", &m_SpriteTree, &m_SpriteList, RESTYPE_SPRITE);
}

CSpriteDlg::~CSpriteDlg()
{
	if (m_pIconList)
	{
		delete m_pIconList;
	}
	if (m_pIconList2)
	{
		delete m_pIconList2;
	}
}

void CSpriteDlg::DoDataExchange(CDataExchange* pDX)
{
	CMRCSizeDialogBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSpriteDlg)
	DDX_Control(pDX, IDC_BASEREZ_TREE, m_SpriteTree);
	DDX_Control(pDX, IDC_BASEREZ_LIST, m_SpriteList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSpriteDlg, CBaseRezDlg)
	//{{AFX_MSG_MAP(CSpriteDlg)
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	ON_NOTIFY(TVN_SELCHANGED, IDC_BASEREZ_TREE, OnSelchangedDirectory)
	ON_NOTIFY( NM_DBLCLK, IDC_BASEREZ_LIST, OnDblClkSprite )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSpriteDlg message handlers

BOOL CSpriteDlg::OnInitDialogBar() 
{
	CMRCSizeDialogBar::OnInitDialogBar();
	
	// TODO: Add extra initialization here
	char szSysPath[MAX_PATH];
	char szIconDLL[MAX_PATH];

	GetSystemDirectory(szSysPath,MAX_PATH);
	sprintf(szIconDLL,"%s\\shell32.dll",szSysPath);

	if (m_pIconList)
	{
		delete m_pIconList;
	}
	if (m_pIconList2)
	{
		delete m_pIconList2;
	}
	m_pIconList=new CImageList;
	m_pIconList2=new CImageList;

	//tree control image list
	m_pIconList->Create(16,16,ILC_COLOR16,10,5);
	m_pIconList->SetBkColor(RGB(255,255,255));
	m_pIconList->Add(ExtractIcon(AfxGetInstanceHandle(),szIconDLL,3));	//closed folder
	m_pIconList->Add(ExtractIcon(AfxGetInstanceHandle(),szIconDLL,4));	//open folder
	m_SpriteTree.SetImageList(m_pIconList,TVSIL_NORMAL);

	//list control image list
	m_pIconList2->Create(16,16,ILC_COLOR4,10,5);
	m_pIconList2->SetBkColor(RGB(255,255,255));
	m_pIconList2->Add(AfxGetApp()->LoadIcon(IDI_SPRITES_TAB_ICON));
	m_SpriteList.SetImageList(m_pIconList2,LVSIL_SMALL);
	
	CRect rect;
	m_SpriteList.GetClientRect( &rect );

	m_SpriteList.InsertColumn(0,"Name",LVCFMT_LEFT,(rect.Width()-35)/3,-1);
	m_SpriteList.InsertColumn(1,"Size",LVCFMT_RIGHT,(rect.Width()-35)/3,-1);
	m_SpriteList.InsertColumn(2,"Modified",LVCFMT_LEFT,(rect.Width()-35)/3,-1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSpriteDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// TODO: Add your message handler code here
	CMenu menu;
	
	if(!GetProjectBar()->IsProjectOpen())
		return;

	if(pWnd->m_hWnd == m_SpriteTree.m_hWnd)
	{
		VERIFY(menu.LoadMenu(CG_IDR_POPUP_WORLDTREE));
	}
	else if(pWnd->m_hWnd == m_SpriteList.m_hWnd)
	{
		// Nothing active unless a directory is selected.
		if(!IsDirectorySelected())
			return;

		VERIFY(menu.LoadMenu(CG_IDR_POPUP_SPRITE_DLG));
	}
	// 2001: no 'else return' (0x408465; retail bug: a right-click elsewhere
	// tracks a NULL popup)

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	CWnd* pWndPopupOwner = GetProjectBar();

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);				
}

void CSpriteDlg::OnSize(UINT nType, int cx, int cy) 
{
	CMRCSizeDialogBar::OnSize(nType, cx, cy);
	
	// Reposition the controls
	RepositionControls();
}

void CSpriteDlg::OnSelchangedDirectory(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	DDirIdent *pIdent;

	HTREEITEM hItem=pNMTreeView->itemNew.hItem;

	if(hItem)
	{
		pIdent = (DDirIdent*)m_pTree->GetItemData(hItem);
		if(pIdent)
		{
			m_csCurrentDir = pIdent->m_Filename;
			m_hCurrentItem = hItem;

			PopulateList();
		}
	}

	*pResult = 0;
}

void CSpriteDlg::OnDblClkSprite( NMHDR * pNMHDR, LRESULT * pResult )
{
	POINT point;
	UINT nFlags;
	DFileIdent *pIdent;
	int nItem;

	GetCursorPos(&point);
	m_SpriteList.ScreenToClient(&point);

	nItem = m_SpriteList.HitTest(point,&nFlags);

	if(nItem >= 0)
	{
		pIdent = (DFileIdent*)m_SpriteList.GetItemData(nItem);
		if(pIdent)
		{
			GetProjectBar()->EditSpriteFile(pIdent);
		}
	}

	*pResult = 0;
}
