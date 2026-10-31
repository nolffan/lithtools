//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// PropPageDisplay.cpp : implementation file
//

#include "bdefs.h"
#include "..\dedit.h"
#include "proppagedisplay.h"
#include "optionsdisplay.h"
#include "edithelpers.h"
#include "colorpicker.h"
#include "regiondoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropPageDisplay property page

IMPLEMENT_DYNCREATE(CPropPageDisplay, CPropertyPage)

CPropPageDisplay::CPropPageDisplay() : CPropertyPage(CPropPageDisplay::IDD)
{
	//{{AFX_DATA_INIT(CPropPageDisplay)
	m_bShadePolygons		= TRUE;
	m_bShowSurfaceColor		= FALSE;
	m_nHandleSize			= 3;
	m_nVertexSize			= 2;
	m_nVertexDrawRule		= 0;
	//}}AFX_DATA_INIT
	m_bInit=FALSE;
}

CPropPageDisplay::~CPropPageDisplay()
{
}

void CPropPageDisplay::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPropPageDisplay)
	DDX_Control(pDX, IDC_SPIN_VERTEX_SIZE, m_spinVertexSize);
	DDX_Control(pDX, IDC_SPIN_HANDLE_SIZE, m_spinHandleSize);
	DDX_Control(pDX, IDC_LIST_COLOR_CHOICES, m_listColorChoices);
	DDX_Check(pDX, IDC_CHECK_SHADE_POLYGONS, m_bShadePolygons);
	DDX_Check(pDX, IDC_CHECK_SHOW_SURFACE_COLOR, m_bShowSurfaceColor);
	DDX_Text(pDX, IDC_EDIT_HANDLE_SIZE, m_nHandleSize);
	DDV_MinMaxInt(pDX, m_nHandleSize, 1, 25);
	DDX_Text(pDX, IDC_EDIT_VERTEX_SIZE, m_nVertexSize);
	DDV_MinMaxInt(pDX, m_nVertexSize, 1, 25);
	DDX_Radio(pDX, IDC_RADIO_VERTEX_RULE, m_nVertexDrawRule);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPropPageDisplay, CPropertyPage)
	//{{AFX_MSG_MAP(CPropPageDisplay)
	ON_BN_CLICKED(IDC_BUTTON_CHANGE_COLOR, OnButtonChangeColor)
	ON_WM_PAINT()
	ON_LBN_SELCHANGE(IDC_LIST_COLOR_CHOICES, OnSelchangeListColorChoices)
	ON_LBN_DBLCLK(IDC_LIST_COLOR_CHOICES, OnDblclkListColorChoices)
	ON_BN_CLICKED(IDC_STATIC_COLOR_BOX, OnStaticColorBox)
	ON_BN_CLICKED(IDC_CHECK_SHADE_POLYGONS, UpdateDataHandlerAndDraw)
	ON_BN_CLICKED(IDC_CHECK_SHOW_SURFACE_COLOR, UpdateDataHandlerAndDraw)
	ON_EN_CHANGE(IDC_EDIT_HANDLE_SIZE, UpdateDataHandlerAndDraw)
	ON_EN_CHANGE(IDC_EDIT_VERTEX_SIZE, UpdateDataHandlerAndDraw)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_RADIO_SELECTED_BRUSHES, UpdateDataHandlerAndDraw)
	ON_BN_CLICKED(IDC_RADIO_VERTEX_RULE, UpdateDataHandlerAndDraw)
	ON_BN_CLICKED(IDC_BUTTON_RESET, OnButtonReset)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropPageDisplay message handlers

/************************************************************************/
// Dialog initialization
BOOL CPropPageDisplay::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// The display options class
	COptionsDisplay *pDisplayOptions=GetApp()->GetOptions().GetDisplayOptions();
	if (!pDisplayOptions)
	{
		ASSERT(FALSE);
		return TRUE;
	}

	// Fill the listbox with the color choices
	int i;
	for (i=0; i < COptionsDisplay::kNumColorOptions; i++)
	{
		m_listColorChoices.AddString(pDisplayOptions->GetColorItem(i)->GetDisplayName());
	}

	// Select the first item in the list
	m_listColorChoices.SetCurSel(0);
	
	// Get the other display options.  Retail bug: m_bShowSurfaceColor
	// is not read from the options (OnInitDialog, 0x44e111).
	m_bShadePolygons		= pDisplayOptions->IsShadePolygons();
	m_nVertexSize			= pDisplayOptions->GetVertexSize();
	m_nHandleSize			= pDisplayOptions->GetHandleSize();
	m_nVertexDrawRule		= pDisplayOptions->GetVertexDrawRule();

	// Setup the spin controls
	m_spinVertexSize.SetRange(1, 25);
	m_spinHandleSize.SetRange(1, 25);

	UpdateData(FALSE);

	m_bInit=TRUE;

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/************************************************************************/
// Handle the paint message
void CPropPageDisplay::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	// The display options class
	COptionsDisplay *pDisplayOptions=GetApp()->GetOptions().GetDisplayOptions();
	if (pDisplayOptions)
	{
		// Make sure that an item is selected
		int nSelectedItem=m_listColorChoices.GetCurSel();
		if (nSelectedItem != LB_ERR)
		{
			// Get the client rectangle for the color box
			CRect rcColor;
			GetDlgItem(IDC_STATIC_COLOR_BOX)->GetWindowRect(rcColor);
			ScreenToClient(rcColor);
			
			// Draw the color box
			dc.FillRect(rcColor, &CBrush(pDisplayOptions->GetColor(nSelectedItem)));
		}
	}
}

/************************************************************************/
// The "Change Color" button was pressed
void CPropPageDisplay::OnButtonChangeColor() 
{
	// The display options class
	COptionsDisplay *pDisplayOptions=GetApp()->GetOptions().GetDisplayOptions();
	if (pDisplayOptions)
	{
		// Make sure that an item is selected
		int nSelectedItem=m_listColorChoices.GetCurSel();
		if (nSelectedItem != LB_ERR)
		{
			// Load the color picker
			CColorPicker colorPicker;
			colorPicker.Init();
			colorPicker.SetCurrentColor(pDisplayOptions->GetColor(nSelectedItem));

			if (colorPicker.DoModal() == IDOK)
			{
				pDisplayOptions->GetColorItem(nSelectedItem)->SetColor(colorPicker.GetColor());
				Invalidate();
			}
			
			// Redraw all of the documents
			RedrawAllDocuments();

			// Save the color options
			pDisplayOptions->Save();
		}
	}
}

/************************************************************************/
// The color choices dialog has been double clicked
void CPropPageDisplay::OnDblclkListColorChoices() 
{
	OnButtonChangeColor();	
}

/************************************************************************/
// The color box has been clicked
void CPropPageDisplay::OnStaticColorBox() 
{
	OnButtonChangeColor();	
}

/************************************************************************/
// The selected color item has changed
void CPropPageDisplay::OnSelchangeListColorChoices() 
{
	// Get the client rectangle for the color box
	CRect rcColor;
	GetDlgItem(IDC_STATIC_COLOR_BOX)->GetWindowRect(rcColor);	
	ScreenToClient(rcColor);

	InvalidateRect(rcColor);
}

/************************************************************************/
// Updates the display options class with the dialog data
void CPropPageDisplay::UpdateDisplayOptions()
{
	if (m_bInit)
	{
		UpdateData();
		
		// The display options class
		COptionsDisplay *pDisplayOptions=GetApp()->GetOptions().GetDisplayOptions();
		if (pDisplayOptions)
		{
			// Set the option in the display class options	
			pDisplayOptions->SetShadePolygons(m_bShadePolygons);
			pDisplayOptions->SetShowSurfaceColor(m_bShowSurfaceColor);
			pDisplayOptions->SetVertexSize(m_nVertexSize);
			pDisplayOptions->SetHandleSize(m_nHandleSize);

			// Set the vertex rule
			switch (m_nVertexDrawRule)
			{
			case 0:
				{
					pDisplayOptions->SetVertexDrawRule(COptionsDisplay::kVertexDrawAll);
					break;
				}
			case 1:
				{
					pDisplayOptions->SetVertexDrawRule(COptionsDisplay::kVertexDrawSelectedOnly);
					break;
				}
			}
		}
	}
}

//**********************************************************************/
// defualt handler that will update the data
void CPropPageDisplay::UpdateDataHandlerAndDraw()
{
	// Update the display options
	UpdateDisplayOptions();

	// Redraw all of the documents
	RedrawAllDocuments();
}


/************************************************************************/
// The window is being destroyed
void CPropPageDisplay::OnDestroy() 
{
	CPropertyPage::OnDestroy();
	
	m_bInit=FALSE;	
}

/************************************************************************/
// The reset button was pressed
void CPropPageDisplay::OnButtonReset() 
{
	// The display options class
	COptionsDisplay *pDisplayOptions=GetApp()->GetOptions().GetDisplayOptions();
	if (pDisplayOptions)
	{
		// Display the warning message box
		CString sMessage="Do you want to reset your colors to the default settings?";
		if (MessageBox(sMessage, "Reset", MB_YESNO) == IDYES)
		{
			// Reset the colors
			pDisplayOptions->ResetToDefaultColors();

			// Invalidate the dialog
			Invalidate();

			// Redraw all of the documents
			RedrawAllDocuments();

			// Save the color options
			pDisplayOptions->Save();
		}
	}
}
