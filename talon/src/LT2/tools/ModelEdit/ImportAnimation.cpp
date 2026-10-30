// ImportAnimation.cpp : implementation file
//

#include "precompile.h"
#include "modeledit.h"
#include "importanimation.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CImportAnimation

IMPLEMENT_DYNAMIC(CImportAnimation, CFileDialog)

CImportAnimation::CImportAnimation(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd) :
		CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	//{{AFX_DATA_INIT(CImportAnimation)
	m_bImportKeystrings = FALSE;
	m_bImportUserDims = FALSE;
	m_bImportTranslations = FALSE;
	m_bUseUVCoords = FALSE;
	m_bImportSockets = FALSE;
	m_bImportWeightSets = FALSE;
	m_bImportLODs = FALSE;
	//}}AFX_DATA_INIT
	m_ofn.Flags |= OFN_ENABLETEMPLATE;
	m_bImportAnimations = TRUE;
	m_ofn.lpTemplateName = MAKEINTRESOURCE( IDD_IMPORTANIM );
}

void CImportAnimation::DoDataExchange(CDataExchange* pDX)
{
	//{{AFX_DATA_MAP(CImportAnimation)
	DDX_Check(pDX, IDC_IMPORT_ANIMATIONS, m_bImportAnimations);
	DDX_Check(pDX, IDC_IMPORT_KEYSTRINGS, m_bImportKeystrings);
	DDX_Check(pDX, IDC_IMPORT_LODS, m_bImportLODs);
	DDX_Check(pDX, IDC_IMPORT_SOCKETS, m_bImportSockets);
	DDX_Check(pDX, IDC_IMPORT_TRANSLATIONS, m_bImportTranslations);
	DDX_Check(pDX, IDC_IMPORT_USERDIMS, m_bImportUserDims);
	DDX_Check(pDX, IDC_IMPORT_USEUVCOORDS, m_bUseUVCoords);
	DDX_Check(pDX, IDC_IMPORT_WEIGHTSETS, m_bImportWeightSets);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CImportAnimation, CFileDialog)
	//{{AFX_MSG_MAP(CImportAnimation)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


void CImportAnimation::OnInitDone( )
{
}

void CImportAnimation::OnDestroy() 
{
	UpdateData(TRUE);
	CFileDialog::OnDestroy();
}
