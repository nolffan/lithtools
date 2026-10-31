//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// AdvancedSelectDlg.cpp : implementation file
//

#include "bdefs.h"
#include "..\dedit.h"
#include "advancedselectdlg.h"
#include "regiondoc.h"
#include "editprojectmgr.h"
#include "edithelpers.h"
#include "optionsadvancedselect.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// AdvancedSelectDlg dialog


AdvancedSelectDlg::AdvancedSelectDlg(CWnd* pParent /*=NULL*/)
	: CDialog(AdvancedSelectDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(AdvancedSelectDlg)
	m_bNodesOfClass = false;
	m_ObjectName = _T("");
	m_ClassName = _T("");
	m_bObjectsWithName = false;
	m_nSelect = 0;
	m_bMatchWholeWord = false;
	m_bShowResults = false;
	//}}AFX_DATA_INIT
}

// Loads the settings from the registry
void AdvancedSelectDlg::LoadRegistrySettings()
{
	// Get the options class
	COptionsAdvancedSelect *pOptions=GetApp()->GetOptions().GetAdvancedSelectOptions();
	if (!pOptions)
	{
		return;
	}

	// Set the member variables to their corresponding option value
	m_bNodesOfClass=pOptions->GetUseClassField();
	m_ClassName=pOptions->GetClassField();
	m_bMatchWholeWord=pOptions->GetMatchWholeName();

	m_bObjectsWithName=pOptions->GetUseNameField();
	m_ObjectName=pOptions->GetNameField();	

	m_nSelect=pOptions->GetSelectionOperation();	

	m_bShowResults=pOptions->GetShowResults();
}

// Saves the settings from the registry
void AdvancedSelectDlg::SaveRegistrySettings()
{
	// Get the options class
	COptionsAdvancedSelect *pOptions=GetApp()->GetOptions().GetAdvancedSelectOptions();
	if (!pOptions)
	{
		return;
	}

	// Set the member variables to their corresponding option value
	pOptions->SetUseClassField(m_bNodesOfClass);
	pOptions->SetClassField(m_ClassName);
	pOptions->SetMatchWholeName(m_bMatchWholeWord);

	pOptions->SetUseNameField(m_bObjectsWithName);
	pOptions->SetNameField(m_ObjectName);	

	pOptions->SetSelectionOperation(m_nSelect);	

	pOptions->SetShowResults(m_bShowResults);
}

/************************************************************************/
// Updates the enabled/disabled status of the controls
void AdvancedSelectDlg::UpdateEnabledStatus()
{
	UpdateData();

	BOOL bEnable;

	// Update the name controls
	bEnable=m_bObjectsWithName;
	GetDlgItem(IDC_OBJECTNAME)->EnableWindow(bEnable);
	GetDlgItem(IDC_CHECK_MATCH_WHOLE_WORD)->EnableWindow(bEnable);

	// Update the class controls	
	GetDlgItem(IDC_CLASSCOMBO)->EnableWindow(m_bNodesOfClass);

	// Update the select button and the operation options
	if (m_bObjectsWithName || m_bNodesOfClass)
	{
		bEnable=true;		
	}
	else
	{
		bEnable=false;		
	}

	GetDlgItem(IDOK)->EnableWindow(bEnable);
	GetDlgItem(IDC_RADIO_SELECT)->EnableWindow(bEnable);
	GetDlgItem(IDC_RADIO_DESELECT)->EnableWindow(bEnable);
	GetDlgItem(IDC_STATIC_OPERATION)->EnableWindow(bEnable);
}

void AdvancedSelectDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(AdvancedSelectDlg)
	DDX_Control(pDX, IDC_CLASSCOMBO, m_comboClass);
	DDX_Check(pDX, IDC_NODESOFCLASS, m_bNodesOfClass);
	DDX_Text(pDX, IDC_OBJECTNAME, m_ObjectName);
	DDX_CBString(pDX, IDC_CLASSCOMBO, m_ClassName);
	DDX_Check(pDX, IDC_OBJECTSWITHNAME, m_bObjectsWithName);
	DDX_Radio(pDX, IDC_RADIO_SELECT, m_nSelect);
	DDX_Check(pDX, IDC_CHECK_MATCH_WHOLE_WORD, m_bMatchWholeWord);
	DDX_Check(pDX, IDC_SHOWRESULTS, m_bShowResults);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(AdvancedSelectDlg, CDialog)
	//{{AFX_MSG_MAP(AdvancedSelectDlg)
	ON_BN_CLICKED(IDC_NODESOFCLASS, OnNodesOfClass)
	ON_BN_CLICKED(IDC_OBJECTSWITHNAME, OnObjectsWithName)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// AdvancedSelectDlg message handlers

BOOL AdvancedSelectDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: Add extra initialization here

	//SCHLEGZ 12/31/97 Changed the classes edit to a combo box and filled it up with classes
	CProjectClass		*pRootClass;

	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_CLASSCOMBO);
	
	for(int i=0; i < GetProject()->m_Classes; i++)
	{
		pRootClass = GetProject()->m_Classes[i];
		if(pRootClass)
		{
			pBox->AddString(pRootClass->m_pClass->m_ClassName);
			AddItemsToBox( pRootClass->m_Children );
		}
	}

	// Select the proper string in the combobox
	m_comboClass.SelectString(-1, m_ClassName);

	// Update the enabled status
	UpdateEnabledStatus();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void AdvancedSelectDlg::AddItemsToBox( CMoArray<CProjectClass*> &classes )
{
	ClassDef *pClass;
	DWORD i;
	
	CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_CLASSCOMBO);

	for(i=0; i < classes; i++ )
	{
		pClass = classes[i]->m_pClass;

		if(pClass->m_ClassFlags & CF_HIDDEN)
		{
			AddItemsToBox(classes[i]->m_Children);
		}
		else
		{
			pBox->AddString(pClass->m_ClassName);		
			AddItemsToBox(classes[i]->m_Children);
		}
	}
}

/************************************************************************/
// The "nodes" checkbox was clicked
void AdvancedSelectDlg::OnNodesOfClass() 
{
	// Update the enabled status of the controls
	UpdateEnabledStatus();		
}

/************************************************************************/
// The "name" checkbox was clicked
void AdvancedSelectDlg::OnObjectsWithName() 
{
	// Update the enabled status of the controls
	UpdateEnabledStatus();		
}

