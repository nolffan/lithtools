//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// RotationEdit.cpp : implementation file
//

#include "bdefs.h"
#include "..\dedit.h"
#include "rotationedit.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// talon: written from the 2001 image (0x45f76a..0x4600b5); Build 69's is a
// later dialog (edit boxes per angle, copy/paste, auto-apply).

/////////////////////////////////////////////////////////////////////////////
// CRotationEdit dialog


CRotationEdit::CRotationEdit(CWnd* pParent /*=NULL*/)
	: CDialog(CRotationEdit::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRotationEdit)
	m_PitchText = _T("");
	m_RollText = _T("");
	m_YawText = _T("");
	m_RotationText = _T("");
	//}}AFX_DATA_INIT
}


void CRotationEdit::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRotationEdit)
	DDX_Control(pDX, IDC_YAWSCROLLBAR, m_YawBar);
	DDX_Control(pDX, IDC_ROLLSCROLLBAR, m_RollBar);
	DDX_Control(pDX, IDC_PITCHSCROLLBAR, m_PitchBar);
	DDX_Text(pDX, IDC_PITCHTITLE, m_PitchText);
	DDX_Text(pDX, IDC_ROLLTITLE, m_RollText);
	DDX_Text(pDX, IDC_YAWTITLE, m_YawText);
	DDX_Text(pDX, IDC_ROTATIONTEXT, m_RotationText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRotationEdit, CDialog)
	//{{AFX_MSG_MAP(CRotationEdit)
	ON_WM_HSCROLL()
	ON_WM_PAINT()
	ON_EN_CHANGE(IDC_ROTATIONTEXT, OnChangeRotationText)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRotationEdit message handlers

BOOL CRotationEdit::OnInitDialog() 
{
	SCROLLINFO info;
	float fMul;

	CDialog::OnInitDialog();
	
	// Convert from radians to degrees on input..
	fMul = 360.0f / MATH_CIRCLE;
	m_EulerAngles *= fMul;

	info.cbSize = sizeof(info);
	info.fMask = SIF_ALL;
	info.nMin = 0;
	info.nMax = 3600;
	info.nPage = 10;

	info.nPos = info.nTrackPos = (int)(m_EulerAngles.y * 10.0f + 0.5);
	m_YawBar.SetScrollInfo(&info);

	info.nPos = info.nTrackPos = (int)(m_EulerAngles.x * 10.0f + 0.5);
	m_PitchBar.SetScrollInfo(&info);

	info.nPos = info.nTrackPos = (int)(m_EulerAngles.z * 10.0f + 0.5);
	m_RollBar.SetScrollInfo(&info);

	UpdateScrollTexts();
	UpdateRotationText();
	DrawGraphical();

	UpdateData(FALSE);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CRotationEdit::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	if(nSBCode != SB_ENDSCROLL)
	{
		if(nSBCode == SB_PAGELEFT)
		{
			nPos = CLAMP(pScrollBar->GetScrollPos() - 100, 0, 3600);
		}
		else if(nSBCode == SB_PAGERIGHT)
		{
			nPos = CLAMP(pScrollBar->GetScrollPos() + 100, 0, 3600);
		}
		else if(nSBCode == SB_LINELEFT)
		{
			nPos = CLAMP(pScrollBar->GetScrollPos() - 10, 0, 3600);
		}
		else if(nSBCode == SB_LINERIGHT)
		{
			nPos = CLAMP(pScrollBar->GetScrollPos() + 10, 0, 3600);
		}

		if(pScrollBar)
		{
			pScrollBar->SetScrollPos(nPos);

			m_EulerAngles.z = (float)(int)((float)m_RollBar.GetScrollPos() / 10.0f + 0.5f);
			m_EulerAngles.x = (float)(int)((float)m_PitchBar.GetScrollPos() / 10.0f + 0.5f);
			m_EulerAngles.y = (float)(int)((float)m_YawBar.GetScrollPos() / 10.0f + 0.5f);

			UpdateScrollTexts();
			UpdateRotationText();
			DrawGraphical();

			UpdateData(FALSE);
		}
	}
}

void CRotationEdit::OnOK() 
{
	float fMul;

	// Convert from degrees to radians...
	fMul = MATH_CIRCLE / 360.0f;
	m_EulerAngles *= fMul;
	
	CDialog::OnOK();
}


void CRotationEdit::UpdateScrollTexts()
{
	m_RollText.Format(IDS_ROLL_TEXT, (int)m_EulerAngles.z);
	m_PitchText.Format(IDS_PITCH_TEXT, (int)m_EulerAngles.x);
	m_YawText.Format(IDS_YAW_TEXT, (int)m_EulerAngles.y);
}


void CRotationEdit::UpdateRotationText()
{
	CString sPitch, sYaw, sRoll;

	sPitch.Format("%.0f", m_EulerAngles.x);
	TrimZeros(sPitch);

	sYaw.Format("%.0f", m_EulerAngles.y);
	TrimZeros(sYaw);

	sRoll.Format("%.0f", m_EulerAngles.z);
	TrimZeros(sRoll);

	m_RotationText.Format("%s %s %s", sYaw, sPitch, sRoll);
}


void CRotationEdit::DrawGraphical()
{
	CWnd *pWnd;
	DVector radianRotation;
	float fMul;


	fMul = MATH_CIRCLE / 360.0f;
	radianRotation = m_EulerAngles * fMul;

	pWnd = GetDlgItem(IDC_YAWVIEW);
	DrawAngleToWnd(pWnd, MATH_HALFPI - radianRotation.y);

	pWnd = GetDlgItem(IDC_PITCHVIEW);
	DrawAngleToWnd(pWnd, -radianRotation.x);
}


void CRotationEdit::DrawAngleToWnd(CWnd *pWnd, float radians)
{
	CRect rect;
	POINT center, outward;
	CBrush theBrush(RGB(0,0,0));
	CBrush *pOldBrush;
	CPen pen(PS_SOLID, 1, RGB(255,255,255));
	CPen *pOldPen;
	CDC *pDC;

	if(pWnd)
	{
		pWnd->GetClientRect(&rect);
		center.x = rect.Width() / 2;
		center.y = rect.Height() / 2;
		outward.x = center.x + (int)(cos(radians) * (float)(rect.Width() / 2));
		outward.y = center.y - (int)(sin(radians) * (float)(rect.Height() / 2));
		
		pDC = pWnd->GetDC();
		if(pDC)
		{
			pOldBrush = pDC->SelectObject(&theBrush);
			pDC->Rectangle(&rect);
			pDC->SelectObject(pOldBrush);

			pOldPen = pDC->SelectObject(&pen);
			pDC->MoveTo(center);
			pDC->LineTo(outward);
			pDC->SelectObject(pOldPen);
			
			pWnd->ReleaseDC(pDC);
		}
	}
}


void CRotationEdit::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	DrawGraphical();	
	// Do not call CDialog::OnPaint() for painting messages
}


void CRotationEdit::OnChangeRotationText() 
{
	UpdateData(TRUE);

	sscanf(m_RotationText, "%f %f %f", &m_EulerAngles.y, &m_EulerAngles.x, &m_EulerAngles.z);

	m_RollBar.SetScrollPos((int)(m_EulerAngles.z * 10.0f + 0.5f));
	m_YawBar.SetScrollPos((int)(m_EulerAngles.y * 10.0f + 0.5f));
	m_PitchBar.SetScrollPos((int)(m_EulerAngles.x * 10.0f + 0.5f));

	UpdateScrollTexts();
	DrawGraphical();

	UpdateData(FALSE);
}


// Takes the zeros off the end of a number's fraction, and the point if
// nothing's left after it.
void CRotationEdit::TrimZeros(CString &str)
{
	int nDot, nEnd;

	for(nDot=0; nDot < str.GetLength(); nDot++)
	{
		if(str[nDot] == '.')
			break;
	}

	nEnd = str.GetLength() - 1;
	while(nEnd > nDot && str[nEnd] == '0')
		nEnd--;

	str = str.Left(nEnd + 1);

	nEnd = str.GetLength();
	if(nEnd > 0 && str[nEnd - 1] == '.')
		str = str.Left(nEnd - 1);
}
