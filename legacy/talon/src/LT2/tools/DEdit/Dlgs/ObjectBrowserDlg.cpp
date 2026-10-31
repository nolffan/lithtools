// ObjectBrowserDlg.cpp : implementation file
//

#include "bdefs.h"
#include "..\dedit.h"
#include "regiondoc.h"
#include "editregion.h"
#include "edithelpers.h"
#include "objectbrowserdlg.h"
#include "mainfrm.h"
#include "nodeview.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectBrowserDlg dialog


CObjectBrowserDlg::CObjectBrowserDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CObjectBrowserDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CObjectBrowserDlg)
	m_bGroupByType = TRUE;
	//}}AFX_DATA_INIT

	m_bIsDialogValid=FALSE;
	m_pSelectedObject=NULL;
}


void CObjectBrowserDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CObjectBrowserDlg)
	DDX_Control(pDX, IDC_TREE_OBJECTS, m_treeObjects);
	DDX_Control(pDX, IDC_LIST_OBJECTS, m_listObjects);
	DDX_Check(pDX, IDC_CHECK_GROUP_BY_TYPE, m_bGroupByType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CObjectBrowserDlg, CDialog)
	//{{AFX_MSG_MAP(CObjectBrowserDlg)
	ON_BN_CLICKED(IDC_CHECK_GROUP_BY_TYPE, OnCheckGroupByType)
	ON_LBN_SELCHANGE(IDC_LIST_OBJECTS, OnSelchangeListObjects)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_OBJECTS, OnSelchangedTreeObjects)
	ON_NOTIFY(TVN_SELCHANGING, IDC_TREE_OBJECTS, OnSelchangingTreeObjects)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_SELECT, OnButtonSelect)
	ON_BN_CLICKED(IDC_BUTTON_LOCATE, OnButtonLocate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectBrowserDlg message handlers


BOOL CObjectBrowserDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// The dialog is valid
	m_bIsDialogValid=TRUE;

	// Get the active region
	CEditRegion *pRegion=GetActiveRegionDoc()->GetRegion();

	// Add each object to the list
	DWORD i;
	for (i=0; i < pRegion->m_Objects.GetSize(); i++)
	{
		CBaseEditObj *pObject=pRegion->m_Objects[i];

		// Add the string and associate this index with the object
		int nIndex=m_listObjects.AddString(pObject->GetName());
		m_listObjects.SetItemData(nIndex, (DWORD)pObject);
	}

	// Fill the tree
	BuildObjectTree(&m_treeObjects);

	// Update the control states
	UpdateControlStates();

	// Select the selected object
	if (m_pSelectedObject)
	{
		SetSelectedObject(m_pSelectedObject);
	}

	// Hide the controls that are to be invisible
	int n;
	for (n=0; n < m_controlInvisibleArray.GetSize(); n++)
	{
		GetDlgItem(m_controlInvisibleArray[n])->ShowWindow(SW_HIDE);
	}

	// Disable these controls if there isn't a selected object
	if (!m_pSelectedObject)
	{
		GetDlgItem(IDC_BUTTON_LOCATE)->EnableWindow(FALSE);
		GetDlgItem(IDC_BUTTON_SELECT)->EnableWindow(FALSE);		
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/************************************************************************/
// Sets whether or not the specified control should be visible or not
void CObjectBrowserDlg::SetControlVisible(int nID, BOOL bVisible)
{
	// If the dialog is valid, then hide or show the control
	if (m_bIsDialogValid)
	{
		int nShowCmd;
		if (bVisible)
		{
			nShowCmd=SW_SHOW;
		}
		else
		{
			nShowCmd=SW_HIDE;
		}

		GetDlgItem(nID)->ShowWindow(nShowCmd);
	}
	else
	{
		// If we are hiding this control, add it to the list to be
		// hidden upon initializing the dialog
		if (!bVisible)
		{
			m_controlInvisibleArray.Add((DWORD)nID);
		}
	}
}

/************************************************************************/
// The window is about to be destroyed
void CObjectBrowserDlg::OnDestroy() 
{
	CDialog::OnDestroy();
	
	m_bIsDialogValid=FALSE;	
}

/************************************************************************/
// Selects an object
void CObjectBrowserDlg::SetSelectedObject(CBaseEditObj *pObject)
{
	m_pSelectedObject=pObject;

	if (m_bIsDialogValid)
	{	
		// Select the object in the tree
		RecurseSelectObject(&m_treeObjects, m_treeObjects.GetRootItem(), m_pSelectedObject);

		// Search the list for the object
		int i;
		for (i=0; i < m_listObjects.GetCount(); i++)
		{
			CBaseEditObj *pListObject=(CBaseEditObj *)m_listObjects.GetItemData(i);
			if ((CBaseEditObj *)m_listObjects.GetItemData(i) == m_pSelectedObject)
			{
				m_listObjects.SetCurSel(i);
				break;
			}
		}

		// Enable the view and select buttons
		GetDlgItem(IDC_BUTTON_LOCATE)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_SELECT)->EnableWindow(TRUE);		
	}
}

/************************************************************************/
// Selects an object based on its name
void CObjectBrowserDlg::SetSelectedObject(CString sObjectName)
{
	// Get the active region
	CEditRegion *pRegion=GetActiveRegionDoc()->GetRegion();

	// Search the objects for one with the specified name
	DWORD i;
	for (i=0; i < pRegion->m_Objects.GetSize(); i++)
	{
		CBaseEditObj *pObject=pRegion->m_Objects[i];
		if (sObjectName.CompareNoCase(pObject->GetName()) == 0)
		{
			SetSelectedObject(pObject);
			break;
		}
	}
}

/************************************************************************/
// Fills the tree control with the objects grouped by their class
void CObjectBrowserDlg::BuildObjectTree(CTreeCtrl *pTree)
{
	// Get the active region
	CEditRegion *pRegion=GetActiveRegionDoc()->GetRegion();

	pTree->DeleteAllItems();

	HTREEITEM hRootItem=pTree->GetRootItem();

	DWORD i;
	for (i=0; i < pRegion->m_Objects.GetSize(); i++)
	{
		CBaseEditObj *pObject=pRegion->m_Objects[i];

		// Find the parent node for this object
		HTREEITEM hParentItem=NULL;

		// Start with the first sibling of the root
		HTREEITEM hSibling=pTree->GetChildItem(hRootItem);
		while (hSibling)
		{
			// Compare the items label to the class name for this object			
			bool bFound=(pTree->GetItemText(hSibling).CompareNoCase(pObject->GetClassName()) == 0);
			if (bFound)
			{
				hParentItem=hSibling;
				break;
			}

			// Go to the next sibling
			hSibling=pTree->GetNextSiblingItem(hSibling);
		}

		// If a parent wasn't found, then add the parent item to the root
		if (hParentItem == NULL)
		{
			// Add the new item
			hParentItem=pTree->InsertItem(pObject->GetClassName(), hRootItem, TVI_SORT);

			// Make the parent item bold
			pTree->SetItemState(hParentItem, TVIS_BOLD, TVIS_BOLD );			
		}

		// Add this object underneith the parent
		HTREEITEM hItem=pTree->InsertItem(pObject->GetName(), hParentItem, TVI_SORT);		

		// Associate this item with the object
		pTree->SetItemData(hItem, (DWORD)pObject);
	}

	// Make sure that the root is expanded
	pTree->Expand(hRootItem, TVE_EXPAND);
}

/************************************************************************/
// Updates the control states (visible and enabled) based on the
// options that are selected.
void CObjectBrowserDlg::UpdateControlStates()
{
	// Update the object data
	UpdateData();
	
	// Determine if the objects should be grouped by their type in a tree
	if (m_bGroupByType)
	{
		// Display the appropriate controls
		m_treeObjects.ShowWindow(SW_SHOW);
		m_listObjects.ShowWindow(SW_HIDE);
	}
	else
	{
		// Display the appropriate controls
		m_treeObjects.ShowWindow(SW_HIDE);
		m_listObjects.ShowWindow(SW_SHOW);
	}
}

/************************************************************************/
// The "group by type" box has been checked
void CObjectBrowserDlg::OnCheckGroupByType() 
{
	UpdateControlStates();
}

/************************************************************************/
// The listbox selection has changed
void CObjectBrowserDlg::OnSelchangeListObjects() 
{
	// Get the currently selected index
	int nIndex=m_listObjects.GetCurSel();
	if (nIndex != LB_ERR)
	{
		// Select the object
		SetSelectedObject((CBaseEditObj *)m_listObjects.GetItemData(nIndex));
	}
}

/************************************************************************/
// Selects a specific object in the tree.  Returns TRUE if the object
// has been selected.  The tree is searched recursively.
BOOL CObjectBrowserDlg::RecurseSelectObject(CTreeCtrl *pTree, HTREEITEM hParentItem, CBaseEditObj *pObject)
{
	// Check this node
	if ((CBaseEditObj *)pTree->GetItemData(hParentItem) == pObject)
	{
		pTree->SelectItem(hParentItem);
		pTree->EnsureVisible(hParentItem);

		return TRUE;
	}

	// Check the sibling nodes
	HTREEITEM hSiblingNode=pTree->GetNextSiblingItem(hParentItem);
	while (hSiblingNode)
	{
		// Check this node
		if ((CBaseEditObj *)pTree->GetItemData(hSiblingNode) == pObject)
		{
			pTree->SelectItem(hSiblingNode);
			pTree->EnsureVisible(hSiblingNode);

			return TRUE;
		}
		
		// Check the children nodes
		if (pTree->ItemHasChildren(hSiblingNode))
		{
			// Recurse into the child node
			if (RecurseSelectObject(pTree, pTree->GetChildItem(hSiblingNode), pObject))
			{
				return TRUE;
			}
		}

		// Get the next sibling
		hSiblingNode=pTree->GetNextSiblingItem(hSiblingNode);
	}

	// Check the children nodes
	if (pTree->ItemHasChildren(hParentItem))
	{
		// Recurse into the child node
		if (RecurseSelectObject(pTree, pTree->GetChildItem(hParentItem), pObject))
		{
			return TRUE;
		}
	}

	// The node was not found
	return FALSE;
}

/************************************************************************/
// The selection has changed on the tree control
void CObjectBrowserDlg::OnSelchangedTreeObjects(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	
	// Select the object
	SetSelectedObject((CBaseEditObj *)m_treeObjects.GetItemData(pNMTreeView->itemNew.hItem));
	
	*pResult = 0;
}

/************************************************************************/
// The selection is about to change on the tree control
void CObjectBrowserDlg::OnSelchangingTreeObjects(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	
	// Get the new item
	HTREEITEM hNewItem=pNMTreeView->itemNew.hItem;

	// Check to see if it is under the root item.  If it is, then prevent the selection
	// from taking place.
	if (m_treeObjects.GetParentItem(hNewItem) == NULL)
	{
		// Prevent the selection from happening
		*pResult=TRUE;
	}
	else
	{		
		*pResult = 0;
	}
}

/************************************************************************/
void CObjectBrowserDlg::OnButtonSelect() 
{	
	// Get the active region doc
	CRegionDoc *pDoc=GetActiveRegionDoc();
	ASSERT(pDoc);

	// Get the active region
	CEditRegion *pRegion=pDoc->GetRegion();
	ASSERT(pRegion);

	CBaseEditObj *pSelectedObject = GetSelectedObject();

	// Check to see if this node is already selected
	DWORD i;
	for (i=0; i < pRegion->m_Selections.GetSize(); i++)
	{
		if (pSelectedObject == pRegion->m_Selections[i])
		{
			// This node is already selected so we don't need
			// to continue on with the selection operation.
			return;
		}
	}

	// Determine if multiselect is on
	BOOL bMultiSelect=FALSE;
	if (GetMainFrame()->GetNodeSelectionMode()==MULTI_SELECTION)
	{
		bMultiSelect=TRUE;
	}

	// Select the node
	pDoc->SelectNode(pSelectedObject, !bMultiSelect);

	// Highlight the node in the node view
	GetNodeView()->HighlightNode(pSelectedObject);
}

/************************************************************************/
// This locates the selected object by centering the green crosshair on
// it and centering the parallel views on the green crosshair.
void CObjectBrowserDlg::OnButtonLocate() 
{
	// Get the active region doc
	CRegionDoc *pDoc=GetActiveRegionDoc();
	ASSERT(pDoc);

	// Get the active region
	CEditRegion *pRegion=pDoc->GetRegion();
	ASSERT(pRegion);

	// Get the selected object
	CBaseEditObj *pObject=GetSelectedObject();
	if (pObject)
	{
		// Center the marker on the selected object
		pRegion->m_vMarker=pObject->GetPos();

		// Center the views on the marker
		GetMainFrame()->CenterViewsAtVector(pRegion->m_vMarker, TRUE);
	}
}
