//////////////////////////////////////////////////////////////////////
// RVTrackerNavMove.h - Implementation for the Navigation movement tracker 

#include "bdefs.h"
#include "rvtrackernavmove.h"

CRVTrackerNavMove::CRVTrackerNavMove(LPCTSTR pName, CRegionView *pView) :
	CRegionViewTracker(pName, pView)
{
	m_bAutoCenter		= FALSE;
	m_bAutoHide			= TRUE;

	m_bPerp = FALSE;
	m_pPerpEvent = new CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT);

	m_bFast = FALSE;
	m_pFastEvent = new CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT);

	m_bIdleOnly			= TRUE;
}

CRVTrackerNavMove::~CRVTrackerNavMove()
{
	delete m_pPerpEvent;
	delete m_pFastEvent;
}

// Watch for the mouse buttons
void CRVTrackerNavMove::WatchEvent(const CUIEvent &cEvent)
{
	if (cEvent == *m_pPerpEvent)
		m_bPerp = TRUE;
	else if (cEvent.IsInverse(*m_pPerpEvent))
		m_bPerp = FALSE;

	if (cEvent == *m_pFastEvent)
		m_bFast = TRUE;
	else if (cEvent.IsInverse(*m_pFastEvent))
		m_bFast = FALSE;

	// Call the base class watch event
	CRegionViewTracker::WatchEvent(cEvent);
}

BOOL CRVTrackerNavMove::OnStart()
{
	return TRUE;
}

BOOL CRVTrackerNavMove::OnUpdate(const CUIEvent &cEvent)
{
	// Only move during idle
	if (m_bIdleOnly && (cEvent.GetType() != UIEVENT_NONE))
		return TRUE;

	// Don't update if it hasn't moved
	if (m_cCurPt == m_cLastPt)
		return TRUE;

	CVector		forward, right;
	CNavigator *pNav = &m_pView->Nav();

	// Set the initial speed value
	float fSpeed = (m_bFast) ? 3.0f : 0.5f;

	// Move the navigator.
	if( m_pView->IsParallelViewType() )
	{
		// Scale the speed based on the current magnification
		fSpeed/=(m_pView->ViewDef()->m_Magnify);

		pNav->Pos() -= pNav->Up() * ((CReal)(m_cCurPt.y - m_cLastPt.y) * fSpeed);
		pNav->Pos() += pNav->Right() * ((CReal)(m_cCurPt.x - m_cLastPt.x) * fSpeed);
	}
	else if (!m_bPerp)
	{
		right = pNav->Right();
		right.y = 0.0f;
		right.Norm();

		forward = pNav->Forward();
		forward.y = 0.0f;
		forward.Norm();

		pNav->Pos() -= forward * ((CReal)(m_cCurPt.y - m_cLastPt.y) * fSpeed);
		pNav->Pos() += right   * ((CReal)(m_cCurPt.x - m_cLastPt.x) * fSpeed);
	}
	// Walk-through perpendicular
	else
	{
		pNav->Pos().y -= ((CReal)(m_cCurPt.y - m_cLastPt.y) * fSpeed);
	}

	CenterCursor();

	m_pView->Invalidate();

	return TRUE;
}

BOOL CRVTrackerNavMove::OnEnd()
{
	return TRUE;
}

