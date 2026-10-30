//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// PrefabDlg.cpp : implementation file
//
// talon: 2001's prefab tab, written from the image (0x4434a5..0x443ab5)
// as the copy of CWorldsDlg it is. Its DoDataExchange, OnSize and
// OnSelchangedDirectory are byte for byte the worlds tab's, and the link
// keeps one copy of each (the message map, 0x4da0b8, points at 0x40d2e9
// and 0x412298).

#include "bdefs.h"
#include "dedit.h"
#include "prefabdlg.h"
#include "editprojectmgr.h"
#include "edithelpers.h"
#include "mainfrm.h"
#include "projectbar.h"
#include "regiondoc.h"
#include "regionview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPrefabDlg dialog


CPrefabDlg::CPrefabDlg() : CBaseRezDlg()
{
	//{{AFX_DATA_INIT(CPrefabDlg)
	//}}AFX_DATA_INIT

	m_pIconList=NULL;
	m_pIconList2=NULL;

	m_hCurrentItem = NULL;
	InitBaseRezDlg("ed", &m_PrefabTree, &m_PrefabList, RESTYPE_PREFAB);
}

CPrefabDlg::~CPrefabDlg()
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

void CPrefabDlg::DoDataExchange(CDataExchange* pDX)
{
	CMRCSizeDialogBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPrefabDlg)
	DDX_Control(pDX, IDC_BASEREZ_TREE, m_PrefabTree);
	DDX_Control(pDX, IDC_BASEREZ_LIST, m_PrefabList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPrefabDlg, CBaseRezDlg)
	//{{AFX_MSG_MAP(CPrefabDlg)
	ON_WM_SIZE()
	ON_NOTIFY(TVN_SELCHANGED, IDC_BASEREZ_TREE, OnSelchangedDirectory)
	ON_NOTIFY(NM_DBLCLK, IDC_BASEREZ_LIST, OnDblclkPrefabList)
	ON_WM_CONTEXTMENU()	
	ON_COMMAND(ID_PREFAB_OPEN, OnPrefabOpen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPrefabDlg message handlers

void CPrefabDlg::OnSize(UINT nType, int cx, int cy) 
{
	CMRCSizeDialogBar::OnSize(nType, cx, cy);
	
	// Reposition the controls
	RepositionControls();
}

BOOL CPrefabDlg::OnInitDialogBar() 
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
	m_PrefabTree.SetImageList(m_pIconList,TVSIL_NORMAL);

	//list control image list
	m_pIconList2->Create(16,16,ILC_COLOR4,10,5);
	m_pIconList2->SetBkColor(RGB(255,255,255));
	m_pIconList2->Add(AfxGetApp()->LoadIcon(IDI_WORLDS_TAB_ICON));
	m_PrefabList.SetImageList(m_pIconList2,LVSIL_SMALL);
	
	CRect rect;
	m_PrefabList.GetClientRect( &rect );

	// Clear the world list
	m_PrefabList.DeleteAllItems();

	while (m_PrefabList.DeleteColumn(0))
	{
	}
	
	m_PrefabList.InsertColumn(0,"Name",LVCFMT_LEFT,(rect.Width()-35)/3,-1);
	m_PrefabList.InsertColumn(1,"Size",LVCFMT_RIGHT,(rect.Width()-35)/3,-1);
	m_PrefabList.InsertColumn(2,"Modified",LVCFMT_LEFT,(rect.Width()-35)/3,-1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPrefabDlg::OnSelchangedDirectory(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
//	LPPOINT lpPoint=new POINT;
//	UINT* pnFlags=new UINT;
	DDirIdent *pIdent;

//	GetCursorPos(lpPoint);
//	m_PrefabTree.ScreenToClient(lpPoint);
//	HTREEITEM hItem=m_PrefabTree.HitTest(*lpPoint,pnFlags);
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

	//cleanup
//	delete lpPoint;
//	delete pnFlags;

	*pResult = 0;
}

// A double click imports the prefab into the active world (0x443852)
void CPrefabDlg::OnDblclkPrefabList(NMHDR* pNMHDR, LRESULT* pResult)
{
	int iItem = m_PrefabList.GetNextItem( -1, LVNI_SELECTED );
	DFileIdent *pIdent;
	CString fullName;

	if(iItem != -1)
	{
		pIdent = (DFileIdent*)m_PrefabList.GetItemData(iItem);
		if(pIdent)
		{
			fullName = dfm_GetFullFilename(GetFileMgr(), pIdent->m_Filename);

			CRegionDoc *pDoc = GetActiveRegionDoc();
			if(pDoc)
			{
				POSITION pos = pDoc->GetFirstViewPosition();
				CRegionView *pView = (CRegionView*)pDoc->GetNextView(pos);
				if(pView)
				{
					pView->ImportWorldFile(fullName, TRUE);
				}
			}
		}
	}	
}

void CPrefabDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	CMenu menu;
	
	if(!GetProjectBar()->IsProjectOpen())
		return;

	if(pWnd->m_hWnd == m_PrefabTree.m_hWnd)
	{
		VERIFY(menu.LoadMenu(CG_IDR_POPUP_WORLDTREE));
	}
	else if(pWnd->m_hWnd == m_PrefabList.m_hWnd)
	{
		// Nothing active unless a directory is selected.
		if(!IsDirectorySelected())
		{
			return;
		}

		VERIFY(menu.LoadMenu(CG_IDR_POPUP_PREFAB));
	}

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	CWnd* pWndPopupOwner = GetProjectBar();

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);	
}

// Opens the prefab as a world (0x4439f9)
void CPrefabDlg::OnPrefabOpen()
{
	int iItem = m_PrefabList.GetNextItem( -1, LVNI_SELECTED );
	DFileIdent *pIdent;
	CString fullName;

	if(iItem != -1)
	{
		pIdent = (DFileIdent*)m_PrefabList.GetItemData(iItem);
		if(pIdent)
		{
			fullName = dfm_GetFullFilename(GetFileMgr(), pIdent->m_Filename);
			GetProjectBar()->OpenRegionDoc(fullName);
		}
	}	
}
