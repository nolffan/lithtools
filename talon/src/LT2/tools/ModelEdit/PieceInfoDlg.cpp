// PieceInfoDlg.cpp : implementation file
//
// 2001's Piece Info dialog, written from the image (0x424903..0x424cfe).

#include "precompile.h"
#include "modeledit.h"
#include "pieceinfodlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPieceInfoDlg dialog


CPieceInfoDlg::CPieceInfoDlg(ModelPiece *pPiece, CWnd* pParent /*=NULL*/)
	: CDialog(CPieceInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPieceInfoDlg)
	m_SpecularPower = 0.0f;
	m_SpecularScale = 0.0f;
	m_TextureIndex = 0;
	//}}AFX_DATA_INIT
	m_pPiece = pPiece;
}


void CPieceInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPieceInfoDlg)
	DDX_Text(pDX, IDC_SPECULARPOWER, m_SpecularPower);
	DDV_MinMaxFloat(pDX, m_SpecularPower, 0.f, 100.f);
	DDX_Text(pDX, IDC_SPECULARSCALE, m_SpecularScale);
	DDV_MinMaxFloat(pDX, m_SpecularScale, -10.f, 10.f);
	DDX_Text(pDX, IDC_TEXTUREINDEX0, m_TextureIndex);
	DDV_MinMaxInt(pDX, m_TextureIndex, 0, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CPieceInfoDlg, CDialog)
	//{{AFX_MSG_MAP(CPieceInfoDlg)
	ON_EN_CHANGE(IDC_SPECULARPOWER, OnChangeSpecularPower)
	ON_EN_CHANGE(IDC_SPECULARSCALE, OnChangeSpecularScale)
	ON_EN_CHANGE(IDC_TEXTUREINDEX0, OnChangeTextureIndex)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPieceInfoDlg message handlers

void CPieceInfoDlg::OnOK() 
{
	UpdateData(TRUE);

	m_pPiece->m_SpecularPower = m_SpecularPower;
	m_pPiece->m_SpecularScale = m_SpecularScale;
	m_pPiece->m_iTexture = m_TextureIndex;

	CDialog::OnOK();
}

BOOL CPieceInfoDlg::OnInitDialog() 
{
	m_SpecularPower = m_pPiece->m_SpecularPower;
	m_SpecularScale = m_pPiece->m_SpecularScale;
	m_TextureIndex = m_pPiece->m_iTexture;
	m_ChangedFlags = 0;

	CDialog::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPieceInfoDlg::MarkChanged(UINT nLabelID, DWORD flag)
{
	CWnd *pWnd;
	char text[256], newText[256];

	if(m_ChangedFlags & flag)
		return;

	m_ChangedFlags |= flag;
	pWnd = GetDlgItem(nLabelID);
	if(pWnd)
	{
		pWnd->GetWindowText(text, sizeof(text));
		sprintf(newText, "* %s", text);
		pWnd->SetWindowText(newText);
	}
}

void CPieceInfoDlg::OnChangeSpecularPower() 
{
	MarkChanged(IDC_SPECULARPOWER_LABEL, 1);
}

void CPieceInfoDlg::OnChangeSpecularScale() 
{
	MarkChanged(IDC_SPECULARSCALE_LABEL, 2);
}

void CPieceInfoDlg::OnChangeTextureIndex() 
{
	MarkChanged(IDC_TEXTUREINDEX_LABEL, 4);
}
