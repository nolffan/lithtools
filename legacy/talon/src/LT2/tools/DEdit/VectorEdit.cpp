//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// VectorEdit.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "vectoredit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// talon: 2001's (0x465289..0x4654e2), written from the image: Build 69's
// is a later dialog (an edit box per component, spin controls, increment,
// copy/paste, auto-apply).

/////////////////////////////////////////////////////////////////////////////
// CVectorEdit dialog


CVectorEdit::CVectorEdit(CWnd* pParent /*=NULL*/)
	: CDialog(CVectorEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CVectorEdit)
	m_sVector = _T("");
	//}}AFX_DATA_INIT

	m_vVector = CVector(0.0f, 0.0f, 0.0f);
}


void CVectorEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CVectorEdit)
	DDX_Text(pDX, IDC_VECTOR_EDIT, m_sVector);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CVectorEdit, CDialog)
	//{{AFX_MSG_MAP(CVectorEdit)
	ON_EN_CHANGE(IDC_VECTOR_EDIT, OnChangeVectorEdit)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CVectorEdit message handlers

BOOL CVectorEdit::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// Format the vector
	CString sX;
	sX.Format("%.6f", m_vVector.x);
	TrimZeros(sX);

	CString sY;
	sY.Format("%.6f", m_vVector.y);
	TrimZeros(sY);

	CString sZ;
	sZ.Format("%.6f", m_vVector.z);
	TrimZeros(sZ);

	m_sVector.Format("%s %s %s", sX, sY, sZ);

	// Update the data
	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/************************************************************************/
// This is called to trim the trailing zeros from a floating point number
void CVectorEdit::TrimZeros(CString &sNumber)
{
	// Start at the end of the string
	int i=sNumber.GetLength()-1;

	// Decrement the index while the character is a zero and
	// the index isn't at the start of the string
	while (sNumber[i] == '0' && i > 1)
	{
		i--;
	}

	// Clip off the right side of the string removing the zeros
	sNumber=sNumber.Left(i+1);
	
	// If the last character is a '.' then add a zero
	int nLength=sNumber.GetLength();
	if (nLength > 0 && sNumber[nLength-1] == '.')
	{
		sNumber+="0";
	}
}

void CVectorEdit::OnChangeVectorEdit() 
{
	UpdateData(TRUE);

	if(sscanf(m_sVector, "%f %f %f", &m_vVector.x, &m_vVector.y, &m_vVector.z) != 3)
		GetDlgItem(IDOK)->EnableWindow(FALSE);
	else
		GetDlgItem(IDOK)->EnableWindow(TRUE);
}
