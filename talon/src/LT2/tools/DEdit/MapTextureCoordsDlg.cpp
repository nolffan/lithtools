// MapTextureCoordsDlg.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "maptexturecoordsdlg.h"
#include "regiondoc.h"
#include "regionview.h"
#include "texture.h"
#include "de_world.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapTextureCoordsDlg dialog
// talon: 2001's, written from the image (0x431464..0x431bd2).

CMapTextureCoordsDlg::CMapTextureCoordsDlg(CRegionView *pView)
	: CDialog(CMapTextureCoordsDlg::IDD, NULL)
{
	//{{AFX_DATA_INIT(CMapTextureCoordsDlg)
	m_UOffset = 0.0f;
	m_VOffset = 0.0f;
	m_UScale = 0.0f;
	m_VScale = 0.0f;
	m_Rotation = 0.0f;
	m_bStick = FALSE;
	m_bXZOnly = FALSE;
	//}}AFX_DATA_INIT

	m_pView = pView;
	m_UOffsetDelta = 0.0f;
	m_VOffsetDelta = 0.0f;
}


void CMapTextureCoordsDlg::DoDataExchange(CDataExchange* pDX)
{
	float fOldU = m_UOffset, fOldV = m_VOffset;

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMapTextureCoordsDlg)
	DDX_Text(pDX, IDC_ET_UOFFSET, m_UOffset);
	DDV_MinMaxInt(pDX, (int)m_UOffset, -1000000, 1000000);
	DDX_Text(pDX, IDC_ET_VOFFSET, m_VOffset);
	DDV_MinMaxInt(pDX, (int)m_VOffset, -1000000, 1000000);
	DDX_Text(pDX, IDC_ET_USCALE, m_UScale);
	DDV_MinMaxFloat(pDX, m_UScale, -1000000.0f, 1000000.0f);
	DDX_Text(pDX, IDC_ET_VSCALE, m_VScale);
	DDV_MinMaxFloat(pDX, m_VScale, -1000000.0f, 1000000.0f);
	DDX_Text(pDX, IDC_ET_ROTATION, m_Rotation);
	DDV_MinMaxInt(pDX, (int)m_Rotation, -359, 359);
	DDX_Check(pDX, IDC_CH_STICK, m_bStick);
	DDX_Check(pDX, IDC_CH_XZONLY, m_bXZOnly);
	//}}AFX_DATA_MAP

	m_UOffsetDelta += m_UOffset - fOldU;
	m_VOffsetDelta += m_VOffset - fOldV;
}


BEGIN_MESSAGE_MAP(CMapTextureCoordsDlg, CDialog)
	//{{AFX_MSG_MAP(CMapTextureCoordsDlg)
	ON_COMMAND(IDC_APPLY, OnApply)
	ON_COMMAND(IDC_FLIPX, OnFlipX)
	ON_COMMAND(IDC_FLIPY, OnFlipY)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



// Mirrors the selected polies' mapping along U (0) or V (1).
void CMapTextureCoordsDlg::DoFlip(DWORD iAxis)
{
	CMoArray<CEditPoly*> polies;
	LTVector vPQO[3];	// P, Q and O: iAxis indexes it
	uint32 i;
	CEditPoly *pPoly;

	m_pView->GetSelectedPolies(polies);

	for(i=0; i < polies; i++)
	{
		pPoly = polies[i];

		vPQO[0] = pPoly->GetTexture(0).GetP();
		vPQO[1] = pPoly->GetTexture(0).GetQ();
		vPQO[2] = pPoly->GetTexture(0).GetO();

		vPQO[iAxis] = -vPQO[iAxis];
		pPoly->SetTextureSpace(vPQO[2], vPQO[0], vPQO[1]);
	}

	m_pView->GetRegionDoc()->RedrawAllViews();
}


void CMapTextureCoordsDlg::ApplyChanges()
{
	CMoArray<CEditPoly*> polies;
	PreActionList actionList;
	uint32 i;
	CEditPoly *pPoly;
	float uScale, vScale, rotOffset;
	LTVector rotAxis;
	LTMatrix mRot;
	LTVector O, P, Q;

	polies.SetCacheSize(50);
	m_pView->GetSelectedPolies(polies);

	uScale = m_UScale / (float)m_TextureWidth;
	vScale = m_VScale / (float)m_TextureHeight;
	
	if(fabs(uScale) < 0.0001f)
		uScale = 0.0001f;

	if(fabs(vScale) < 0.0001f)
		vScale = 0.0001f;

	// Setup undos.
	for(i=0; i < polies; i++)
	{
		AddToActionListIfNew(&actionList, new CPreAction(ACTION_MODIFYNODE, polies[i]->m_pBrush), TRUE);
	}

	m_pView->GetRegionDoc()->Modify(&actionList, TRUE);

	// Modify the polies.
	for(i=0; i < polies; i++)
	{
		pPoly = polies[i];

		O = pPoly->GetTexture(0).GetO();
		P = pPoly->GetTexture(0).GetP();
		Q = pPoly->GetTexture(0).GetQ();

		P.Norm();
		Q.Norm();

		// Apply the offset.
		O += P * m_UOffsetDelta;
		O += Q * m_VOffsetDelta;

		// Apply scale.
		P /= uScale;
		Q /= vScale;

		// Apply rotation (radians).
		rotOffset = m_Rotation - m_RefRotation;
		rotOffset *= (MATH_PI / 180.0f);
		rotAxis = P.Cross(Q);
		rotAxis.Norm();
		Mat_SetupRot(&mRot, &rotAxis, rotOffset);
		MatVMul_InPlace_H(&mRot, &P);
		MatVMul_InPlace_H(&mRot, &Q);

		pPoly->SetTextureSpace(O, P, Q);
		pPoly->m_Unknown_70 = m_bStick;
	}

	m_UOffsetDelta = 0.0f;
	m_VOffsetDelta = 0.0f;
	m_RefRotation = m_Rotation;
	m_pView->GetRegionDoc()->RedrawAllViews();
}


void CMapTextureCoordsDlg::OnApply()
{
	UpdateData();	//capture current data to member variables
	ApplyChanges();
}


void CMapTextureCoordsDlg::OnFlipX()
{
	DoFlip(0);
}


void CMapTextureCoordsDlg::OnFlipY()
{
	DoFlip(1);
}
