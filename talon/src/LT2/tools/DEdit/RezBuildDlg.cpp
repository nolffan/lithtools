//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// RezBuildDlg.cpp : the "Building Rez File" dialog (see RezBuildDlg.h).
//

#include "bdefs.h"
#include "dedit.h"
#include "rezbuilddlg.h"
#include "edithelpers.h"


// The dialog the callbacks report to (0x63c648).
static CRezBuildDlg	*g_pRezBuildDlg;


// 0x45f4a3: the file being packed, in the "Current File:" field.
static void RezBuildFileCallback(const char *pFilename)
{
	CWnd *pWnd = g_pRezBuildDlg->GetDlgItem(1024);
	if(pWnd)
		pWnd->SetWindowText(pFilename);
}


// 0x45f4c3: a message, appended to the log.
static void RezBuildMessageCallback(const char *pMsg)
{
	CEdit *pEdit = (CEdit*)g_pRezBuildDlg->GetDlgItem(1023);
	if(pEdit)
	{
		int nEnd = pEdit->GetLimitText();
		pEdit->SendMessage(EM_SETSEL, nEnd, nEnd);
		pEdit->ReplaceSel(pMsg);
	}
}


// 0x45f50e
CRezBuildDlg::CRezBuildDlg(CWnd* pParent)
	: CDialog(CRezBuildDlg::IDD, pParent)
{
}


BEGIN_MESSAGE_MAP(CRezBuildDlg, CDialog)
	ON_WM_TIMER()
	ON_WM_PAINT()
END_MESSAGE_MAP()


// 0x45f55a: OK stays disabled until the file is built.
BOOL CRezBuildDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CWnd *pWnd = GetDlgItem(IDOK);
	if(pWnd)
		pWnd->EnableWindow(FALSE);

	m_bDone = FALSE;
	m_bPainted = FALSE;

	for(UINT nID=1; nID < 32000; nID++)
	{
		m_nTimer = SetTimer(nID, 200, NULL);
		if(m_nTimer)
			break;
	}

	return TRUE;
}


// 0x45f5ab
void CRezBuildDlg::OnTimer(UINT nIDEvent)
{
	CString str;

	CDialog::OnTimer(nIDEvent);

	if(!m_bDone && m_bPainted)
	{
		RezBuildInfo info;

		g_pRezBuildDlg = this;

		info.m_pDir = m_sProjectDir;
		info.m_pRezFile = m_sRezFile;
		info.m_FileCB = RezBuildFileCallback;
		info.m_MessageCB = RezBuildMessageCallback;
		if(BuildRezFile(&info) == 1)
		{
			str.FormatMessage(IDS_CANTOPENOUTPUTREZFILE, (LPCTSTR)m_sRezFile);
			AppMessageBox(str, MB_OK);
		}

		CWnd *pWnd = GetDlgItem(1024);
		if(pWnd)
			pWnd->SetWindowText("");

		pWnd = GetDlgItem(IDOK);
		if(pWnd)
			pWnd->EnableWindow(TRUE);

		m_bDone = TRUE;
		KillTimer(m_nTimer);
	}
}


// 0x45f68b: the build starts on the first timer tick after a paint.
void CRezBuildDlg::OnPaint()
{
	m_bPainted = TRUE;
	CDialog::OnPaint();
}
