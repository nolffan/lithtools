//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// WorldsDlg.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "worldsdlg.h"
#include "editprojectmgr.h"
#include "edithelpers.h"
#include "mainfrm.h"
#include "projectbar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CWorldsDlg dialog


CWorldsDlg::CWorldsDlg() : CBaseRezDlg()
{
	//{{AFX_DATA_INIT(CWorldsDlg)
	//}}AFX_DATA_INIT

	m_pIconList=NULL;
	m_pIconList2=NULL;

	m_hCurrentItem = NULL;
	InitBaseRezDlg("ed", &m_WorldTree, &m_WorldList, RESTYPE_WORLD);
}

CWorldsDlg::~CWorldsDlg()
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

void CWorldsDlg::DoDataExchange(CDataExchange* pDX)
{
	CMRCSizeDialogBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWorldsDlg)
	DDX_Control(pDX, IDC_BASEREZ_TREE, m_WorldTree);
	DDX_Control(pDX, IDC_BASEREZ_LIST, m_WorldList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWorldsDlg, CBaseRezDlg)
	//{{AFX_MSG_MAP(CWorldsDlg)
	ON_WM_SIZE()
	ON_NOTIFY(TVN_SELCHANGED, IDC_BASEREZ_TREE, OnSelchangedDirectory)
	ON_NOTIFY(NM_DBLCLK, IDC_BASEREZ_LIST, OnDblclkWorldsList)
	ON_WM_CONTEXTMENU()	
	ON_COMMAND(ID_WORLD_OPEN, OnWorldListOpen)
	ON_COMMAND(ID_FILE_NEWWORLD, OnFileNewWorld)
	ON_COMMAND(ID_WORLD_DELETE, OnWorldDelete)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorldsDlg message handlers

void CWorldsDlg::OnSize(UINT nType, int cx, int cy) 
{
	CMRCSizeDialogBar::OnSize(nType, cx, cy);
	
	// Reposition the controls
	RepositionControls();
}

BOOL CWorldsDlg::OnInitDialogBar() 
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
	m_WorldTree.SetImageList(m_pIconList,TVSIL_NORMAL);

	//list control image list
	m_pIconList2->Create(16,16,ILC_COLOR4,10,5);
	m_pIconList2->SetBkColor(RGB(255,255,255));
	m_pIconList2->Add(AfxGetApp()->LoadIcon(IDI_WORLDS_TAB_ICON));
	m_WorldList.SetImageList(m_pIconList2,LVSIL_SMALL);
	
	CRect rect;
	m_WorldList.GetClientRect( &rect );

	// Clear the world list
	m_WorldList.DeleteAllItems();

	while (m_WorldList.DeleteColumn(0))
	{
	}
	
	m_WorldList.InsertColumn(0,"Name",LVCFMT_LEFT,(rect.Width()-35)/3,-1);
	m_WorldList.InsertColumn(1,"Size",LVCFMT_RIGHT,(rect.Width()-35)/3,-1);
	m_WorldList.InsertColumn(2,"Modified",LVCFMT_LEFT,(rect.Width()-35)/3,-1);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWorldsDlg::OnSelchangedDirectory(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
//	LPPOINT lpPoint=new POINT;
//	UINT* pnFlags=new UINT;
	DDirIdent *pIdent;

//	GetCursorPos(lpPoint);
//	m_WorldTree.ScreenToClient(lpPoint);
//	HTREEITEM hItem=m_WorldTree.HitTest(*lpPoint,pnFlags);
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

void CWorldsDlg::OnDblclkWorldsList(NMHDR* pNMHDR, LRESULT* pResult)
{
	int iItem = m_WorldList.GetNextItem( -1, LVNI_SELECTED );
	DFileIdent *pIdent;
	CString fullName;

	if(iItem != -1)
	{
		pIdent = (DFileIdent*)m_WorldList.GetItemData(iItem);
		if(pIdent)
		{

			fullName = dfm_GetFullFilename(GetFileMgr(), pIdent->m_Filename);
			GetProjectBar()->OpenRegionDoc(fullName);
		}
	}	

	return;
}

void CWorldsDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// TODO: Add your message handler code here
	CMenu menu;
	
	if(!GetProjectBar()->IsProjectOpen())
		return;

	CWnd* pWndPopupOwner = GetProjectBar();

	if(pWnd->m_hWnd == m_WorldTree.m_hWnd)
	{
		VERIFY(menu.LoadMenu(CG_IDR_POPUP_WORLDTREE));
	}
	else if(pWnd->m_hWnd == m_WorldList.m_hWnd)
	{
		// Nothing active unless a directory is selected.
		if(!IsDirectorySelected())
		{
			return;
		}

		VERIFY(menu.LoadMenu(CG_IDR_POPUP_WORLDLIST));
		pWndPopupOwner = this;
	}
	// 2001: no 'else return' (0x40d3f0; retail bug: a right-click elsewhere
	// tracks a NULL popup)

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);	
}

void CWorldsDlg::OnWorldListOpen()
{
	OnDblclkWorldsList(NULL,NULL);

	return;
}

void CWorldsDlg::OnFileNewWorld()
{
	GetApp()->OnFileNewWorld();
}

void CWorldsDlg::OnWorldDelete() 
{
	// Figure out which one they want to delete
	int iIndex = m_WorldList.GetSelectionMark();
	if (iIndex < 0)
		return;

	// Display a confirmation dialog
	CString csWorldName = m_WorldList.GetItemText(iIndex, 0);
	CString csMessage = "Are you sure you want to delete the world " + csWorldName + "?";
	if (MessageBox(csMessage, "Confirmation", MB_YESNO | MB_ICONQUESTION) == IDNO)
		return;

	// Get the full name of the world
	DFileIdent *pIdent;
	pIdent = (DFileIdent*)m_WorldList.GetItemData(iIndex);
	if(!pIdent)
	{
		MessageBox("Internal error deleting world!", "Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	csWorldName = dfm_GetFullFilename(GetFileMgr(), pIdent->m_Filename);

	// Delete the world
	if (!DeleteFile(csWorldName))
	{
		MessageBox("Error deleting world", "Error", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	// Refresh the display list
	PopulateList();
}
