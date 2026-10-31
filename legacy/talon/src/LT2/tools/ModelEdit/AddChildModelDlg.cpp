// AddChildModelDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "AddChildModelDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddChildModelDlg

// 0x401062
CAddChildModelDlg::CAddChildModelDlg(CWnd* pParent) :
	CStringDlg(IDD_ADDCHILDMODELDLG, pParent)
{
	m_bScaleSkeleton = FALSE;
}

// 0x4010eb
void CAddChildModelDlg::DoDataExchange(CDataExchange* pDX)
{
	CStringDlg::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_SCALESKELETON, m_bScaleSkeleton);
}


BEGIN_MESSAGE_MAP(CAddChildModelDlg, CStringDlg)
	//{{AFX_MSG_MAP(CAddChildModelDlg)
	ON_BN_CLICKED(IDC_ADDCHILD_BROWSE, OnBrowse)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// 0x401116
void CAddChildModelDlg::OnBrowse()
{
	char szFilter[] = "ABC Files (*.abc)|*.abc|LTA Files (*.lta)|*.lta|All Files (*.*)|*.*||";

	CFileDialog dlg(TRUE, "*.abc", NULL, OFN_ALLOWMULTISELECT, szFilter, NULL);

	char szFiles[2048];
	char szNames[2048];

	memset(szFiles, 0, sizeof(szFiles));
	memset(szNames, 0, sizeof(szNames));

	dlg.m_ofn.lpstrFile = szFiles;
	dlg.m_ofn.nMaxFile = sizeof(szFiles) - 1;

	if (dlg.DoModal() == IDOK)
	{
		// The directory, then each file name, double-null terminated.
		char *pName = szFiles;

		for (;;)
		{
			while (*pName)
				pName++;
			pName++;

			if (!*pName)
				break;

			if (szNames[0])
				sprintf(szNames, "%s; %s", szNames, pName);
			else
				sprintf(szNames, "%s", pName);
		}
	}

	if (szNames[0])
	{
		CWnd *pEdit = GetDlgItem(IDC_ENTEREDTEXT);
		pEdit->SetWindowText(szNames);
	}
	else if (dlg.m_ofn.lpstrFileTitle)
	{
		CWnd *pEdit = GetDlgItem(IDC_ENTEREDTEXT);
		pEdit->SetWindowText(dlg.m_ofn.lpstrFileTitle);
	}
}
