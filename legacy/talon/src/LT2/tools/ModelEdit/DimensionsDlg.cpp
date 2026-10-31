// DimensionsDlg.cpp : implementation file
//

#include "precompile.h"
#include "modeledit.h"
#include "dimensionsdlg.h"
#include "model_ops.h"
#include "gl_modelrender.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDimensionsDlg dialog


// 0x402165
CDimensionsDlg::CDimensionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDimensionsDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDimensionsDlg)
	//}}AFX_DATA_INIT
}


// 0x40224b
void CDimensionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDimensionsDlg)
	DDX_Control(pDX, IDC_ZDIM, m_ZDim);
	DDX_Control(pDX, IDC_YDIM, m_YDim);
	DDX_Control(pDX, IDC_XDIM, m_XDim);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDimensionsDlg, CDialog)
	//{{AFX_MSG_MAP(CDimensionsDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDimensionsDlg message handlers

// 0x402291
void CDimensionsDlg::OnOK() 
{
	CString str;
	int i, iAnim;
	AnimInfo *pAnim;

	if( !m_pModel )
		return;

	for(i=0; i < m_nAnims; i++)
	{
		iAnim = m_Anims[i];
		
		if(iAnim >= (int)m_pModel->m_Anims.GetSize())
			continue;

		pAnim = &m_pModel->m_Anims[iAnim];

		m_XDim.GetWindowText( str );
		pAnim->m_vDims.x = ( float )atof(( LPCTSTR )str );
		m_YDim.GetWindowText( str );
		pAnim->m_vDims.y = ( float )atof(( LPCTSTR )str );
		m_ZDim.GetWindowText( str );
		pAnim->m_vDims.z = ( float )atof(( LPCTSTR )str );
	}

	CDialog::OnOK();
}

// 0x40236c
BOOL CDimensionsDlg::OnInitDialog() 
{
	CString str;
	AnimInfo *pAnim;

	CDialog::OnInitDialog();

	if( m_pModel && m_Anims[0] < (int)m_pModel->m_Anims.GetSize() )
	{
		pAnim = &m_pModel->m_Anims[m_Anims[0]];

		str.Format( "%f", pAnim->m_vDims.x );
		m_XDim.SetWindowText( str );
		str.Format( "%f", pAnim->m_vDims.y );
		m_YDim.SetWindowText( str );
		str.Format( "%f", pAnim->m_vDims.z );
		m_ZDim.SetWindowText( str );
	}
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
