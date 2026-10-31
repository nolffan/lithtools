//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// PropPageOptionsD3D.cpp : implementation file
//

#include "bdefs.h"
#include "..\dedit.h"
#include "proppageoptionsd3d.h"
#include "optionsdisplay.h"
#include "drawmgr.h"
#include "texture.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropPageOptionsD3D property page

IMPLEMENT_DYNCREATE(CPropPageOptionsD3D, CPropertyPage)

CPropPageOptionsD3D::CPropPageOptionsD3D() : CPropertyPage(CPropPageOptionsD3D::IDD)
{
	//{{AFX_DATA_INIT(CPropPageOptionsD3D)
	m_bBlurryTextures				= FALSE;
	m_bZBufferLines					= FALSE;
	m_bUseD3DAsDefault				= FALSE;
	//}}AFX_DATA_INIT
}

CPropPageOptionsD3D::~CPropPageOptionsD3D()
{
}

void CPropPageOptionsD3D::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropPageOptionsD3D)
	DDX_Control(pDX, IDC_COMBO_D3D_MODE, m_comboD3Dmode);
	DDX_Control(pDX, IDC_COMBO_D3DDEVICES, m_comboD3Ddevices);
	DDX_Check(pDX, IDC_BLURRY_TEXTURES, m_bBlurryTextures);
	DDX_Check(pDX, IDC_CHECK_ZBUFFER_LINES, m_bZBufferLines);
	DDX_Check(pDX, IDC_CHECK_USE_D3D_AS_DEFAULT, m_bUseD3DAsDefault);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropPageOptionsD3D, CPropertyPage)
	//{{AFX_MSG_MAP(CPropPageOptionsD3D)
	ON_BN_CLICKED(IDC_CHECK_ZBUFFER_LINES, OnCheckZBufferLines)
	ON_BN_CLICKED(IDC_CHECK_USE_D3D_AS_DEFAULT, OnCheckUseD3DAsDefault)
	ON_CBN_SELCHANGE(IDC_COMBO_D3DDEVICES, OnSelchangeCombo)
	ON_CBN_SELCHANGE(IDC_COMBO_D3D_MODE, OnSelchangeCombo)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropPageOptionsD3D message handlers

BOOL CPropPageOptionsD3D::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	// The display options class
	COptionsDisplay *pDisplayOptions=GetApp()->GetOptions().GetDisplayOptions();
	if (!pDisplayOptions)
	{
		ASSERT(FALSE);
		return FALSE;
	}
	
	// Fill the combo box with the device names
	char *deviceNames[4];
	memset(deviceNames, 0, sizeof(deviceNames));
	int nDevices=dm_GetDirectDrawDeviceNames(deviceNames, 4);

	int i;
	for (i=0; i < nDevices; i++)
	{
		m_comboD3Ddevices.AddString(deviceNames[i]);
		delete deviceNames[i];
	}

	// Add the different D3D modes
	m_comboD3Dmode.AddString("Hardware Acceleration");
	m_comboD3Dmode.AddString("Software MMX");
	m_comboD3Dmode.AddString("Software 8-bit Emulation");
	m_comboD3Dmode.AddString("Software RGB Emulation");

	// Get the D3D values
	m_bZBufferLines				= pDisplayOptions->IsZBufferLines();	
	m_bBlurryTextures			= (BOOL)pDisplayOptions->GetMipMapOffset();
	m_bUseD3DAsDefault			= pDisplayOptions->IsUseD3DAsDefault();

	m_comboD3Ddevices.SetCurSel(pDisplayOptions->GetDefaultD3DDevice());
	m_comboD3Dmode.SetCurSel(pDisplayOptions->GetDefaultD3DMode());

	// Update the enabled status of the controls
	UpdateEnabledStatus();

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/************************************************************************/
// Updates the display options with the current dialog data.  The blurry
// textures check (the mipmap offset) is read but never written back.
void CPropPageOptionsD3D::UpdateDisplayOptions()
{
	// Update the data
	UpdateData();

	// The display options class
	COptionsDisplay *pDisplayOptions=GetApp()->GetOptions().GetDisplayOptions();
	if (pDisplayOptions)
	{
		pDisplayOptions->SetZBufferLines(m_bZBufferLines);	
		pDisplayOptions->SetUseD3DAsDefault(m_bUseD3DAsDefault);

		// Set the default device
		int nSelection=m_comboD3Ddevices.GetCurSel();
		if (nSelection != CB_ERR)
		{
			pDisplayOptions->SetDefaultD3DDevice(nSelection);
		}

		// Set the default mode
		nSelection=m_comboD3Dmode.GetCurSel();
		if (nSelection != CB_ERR)
		{
			pDisplayOptions->SetDefaultD3DMode(nSelection);
		}
	}
}

/************************************************************************/
// Update the enabled status of the controls
void CPropPageOptionsD3D::UpdateEnabledStatus()
{
	GetDlgItem(IDC_STATIC_DEVICE_TEXT)->EnableWindow(m_bUseD3DAsDefault);
	GetDlgItem(IDC_STATIC_MODE_TEXT)->EnableWindow(m_bUseD3DAsDefault);
	GetDlgItem(IDC_COMBO_D3DDEVICES)->EnableWindow(m_bUseD3DAsDefault);
	GetDlgItem(IDC_COMBO_D3D_MODE)->EnableWindow(m_bUseD3DAsDefault);
}

/************************************************************************/
void CPropPageOptionsD3D::OnCheckZBufferLines() 
{
	UpdateDisplayOptions();

	// Redraw all of the documents
	RedrawAllDocuments();
}

/************************************************************************/
void CPropPageOptionsD3D::OnCheckUseD3DAsDefault() 
{
	UpdateData();
	UpdateDisplayOptions();
	UpdateEnabledStatus();
}

/************************************************************************/
void CPropPageOptionsD3D::OnSelchangeCombo() 
{
	UpdateData();
	UpdateDisplayOptions();
}
