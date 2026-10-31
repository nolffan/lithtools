//////////////////////////////////////////////////////////////////////
// RVTrackerTextMove.h - Implementation for the texture movement tracker 

#include "bdefs.h"
#include "rvtrackertextmove.h"

#define MAX_TEXTURE_SIZE		512

CRVTrackerTextureMove::CRVTrackerTextureMove(LPCTSTR pName, CRegionView *pView) :
	CRegionViewTracker(pName, pView),
	m_pPoly(NULL),
	m_bRotate(FALSE),
	m_bScale(FALSE),
	m_bStep(FALSE),
	m_bSaveUndo(TRUE)
{
	m_bAutoCenter = FALSE;
	m_bAutoHide = TRUE;

	m_pRotateEvent = new CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT);
	m_pScaleEvent = new CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT);
	m_pStepEvent = new CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT);
}

CRVTrackerTextureMove::~CRVTrackerTextureMove()
{
	delete m_pRotateEvent;
	delete m_pScaleEvent;
	delete m_pStepEvent;
}

// Watch for the shift key to be pressed
void CRVTrackerTextureMove::WatchEvent(const CUIEvent &cEvent)
{
	if (cEvent == *m_pRotateEvent)
		m_bRotate = TRUE;
	else if (cEvent.IsInverse(*m_pRotateEvent))
		m_bRotate = FALSE;

	if (cEvent == *m_pScaleEvent)
		m_bScale = TRUE;
	else if (cEvent.IsInverse(*m_pScaleEvent))
		m_bScale = FALSE;

	if (cEvent == *m_pStepEvent)
		m_bStep = TRUE;
	else if (cEvent.IsInverse(*m_pStepEvent))
		m_bStep = FALSE;

	// Call the base class watch event
	CRegionViewTracker::WatchEvent(cEvent);
}

BOOL CRVTrackerTextureMove::OnStart()
{
	if (m_pView->GetEditMode() != GEOMETRY_EDITMODE)
		return FALSE;

	m_bSaveUndo = TRUE;

	m_pPoly = NULL;

	if (!m_pView->IPoly().IsValid())
		return FALSE;

	m_pPoly = m_pView->IPoly()();

	// Save the original texture space
	m_vOriginalO = m_pPoly->O;
	m_vOriginalP = m_pPoly->P;
	m_vOriginalQ = m_pPoly->Q;

	m_iTotalRotate = 0;
	m_iTotalScaleX = 0;
	m_iTotalScaleY = 0;
	m_iTotalOfsX = 0;
	m_iTotalOfsY = 0;

	return TRUE;
}

BOOL CRVTrackerTextureMove::OnUpdate(const CUIEvent &cEvent)
{
	// Only process on idle events
	if (cEvent.GetType() != UIEVENT_NONE)
		return TRUE;

	// Don't update the texture if it hasn't moved
	if (m_cCurPt == m_cLastPt)
		return TRUE;

	//see if we need to set up the undo
	if(m_bSaveUndo)
	{
		m_pView->GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_MODIFYNODE, m_pPoly->m_pBrush), TRUE);
		m_bSaveUndo = FALSE;
	}

	CVector cVectorO = m_pPoly->O;
	CVector cVectorP = m_pPoly->P;
	CVector cVectorQ = m_pPoly->Q;

	int32 iOfsX = m_cCurPt.x - m_cLastPt.x;
	int32 iOfsY = m_cCurPt.y - m_cLastPt.y;

	if (m_bRotate)
	{
		m_iTotalRotate += iOfsX;
		int32 iStepRotate = (m_bStep) ? (m_iTotalRotate / 15) * 15 : m_iTotalRotate;

		CMatrix matRot;
		matRot.SetupRot(m_pPoly->Normal(), (float)iStepRotate * MATH_PI / 180.0f);

		float fMag = cVectorP.Mag();
		cVectorP = m_vOriginalP;
		cVectorP.Norm();
		cVectorP = matRot * cVectorP * fMag;

		fMag = cVectorQ.Mag();
		cVectorQ = m_vOriginalQ;
		cVectorQ.Norm();
		cVectorQ = matRot * cVectorQ * fMag;
	}
	else if (m_bScale)
	{
		m_iTotalScaleX += iOfsX;
		int32 iStepScaleX = (m_bStep) ? (m_iTotalScaleX / 16) * 16 : m_iTotalScaleX;
		m_iTotalScaleY += iOfsY;
		int32 iStepScaleY = (m_bStep) ? (m_iTotalScaleY / 16) * 16 : m_iTotalScaleY;

		cVectorP = m_vOriginalP * (((float)iStepScaleX) / 64.0f + 1.0f);
		cVectorQ = m_vOriginalQ * (((float)iStepScaleY) / 64.0f + 1.0f);
	}
	else
	{
		m_iTotalOfsX += iOfsX;
		int32 iStepOfsX = (m_bStep) ? (m_iTotalOfsX / 16) * 16 : m_iTotalOfsX;
		m_iTotalOfsY += iOfsY;
		int32 iStepOfsY = (m_bStep) ? (m_iTotalOfsY / 16) * 16 : m_iTotalOfsY;

		cVectorO = m_vOriginalO + m_vOriginalP * ((float)iStepOfsX * 0.5f) + m_vOriginalQ * ((float)iStepOfsY * 0.5f);
	}

	m_pPoly->SetTextureSpace(cVectorO, cVectorP, cVectorQ);

	CenterCursor();

	m_pView->Invalidate(FALSE);

	return TRUE;
}

BOOL CRVTrackerTextureMove::OnEnd()
{
	return TRUE;
}
