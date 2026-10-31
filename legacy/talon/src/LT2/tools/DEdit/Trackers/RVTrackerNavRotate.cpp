//////////////////////////////////////////////////////////////////////
// RVTrackerNavRotate.h - Implementation for the Navigation movement tracker 

#include "bdefs.h"
#include "rvtrackernavrotate.h"

CRVTrackerNavRotate::CRVTrackerNavRotate(LPCTSTR pName, CRegionView *pView) :
	CRegionViewTracker(pName, pView)
{
	m_bAutoCenter		= FALSE;
	m_bAutoHide			= TRUE;

	m_bForward = FALSE;
	m_pForwardEvent = new CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT);

	m_bBackward = FALSE;
	m_pBackwardEvent = new CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT);

	m_bIdleOnly			= TRUE;
}

CRVTrackerNavRotate::~CRVTrackerNavRotate()
{
	delete m_pForwardEvent;
	delete m_pBackwardEvent;
}

// Watch for the mouse buttons
void CRVTrackerNavRotate::WatchEvent(const CUIEvent &cEvent)
{
	if (cEvent == *m_pForwardEvent)
		m_bForward = TRUE;
	else if (cEvent.IsInverse(*m_pForwardEvent))
		m_bForward = FALSE;

	if (cEvent == *m_pBackwardEvent)
		m_bBackward = TRUE;
	else if (cEvent.IsInverse(*m_pBackwardEvent))
		m_bBackward = FALSE;

	// Call the base class watch event
	CRegionViewTracker::WatchEvent(cEvent);
}

BOOL CRVTrackerNavRotate::OnStart()
{
	m_dwIdleDelta = 0;

	return TRUE;
}

BOOL CRVTrackerNavRotate::OnUpdate(const CUIEvent &cEvent)
{
	// Keep track of time between idles
	m_dwIdleDelta += GetTimeDelta();

	// Only move during idle
	if (m_bIdleOnly && (cEvent.GetType() != UIEVENT_NONE))
		return TRUE;

	CReal		xAngle, yAngle;
	CNavigator *pNav = &m_pView->Nav();

	// Magnify in parallel views
	if( m_pView->IsParallelViewType() )
	{		
		m_pView->ViewDef()->m_Magnify += ((CReal)(m_cCurPt.y - m_cLastPt.y) * m_pView->ViewDef()->m_Magnify) / 300.0f;
		if( m_pView->ViewDef()->m_Magnify < 0.0001f )
			m_pView->ViewDef()->m_Magnify = 0.0001f;
	}
	else
	{
	
		xAngle = (CReal)(m_cCurPt.y - m_cLastPt.y) * 0.005f;
		yAngle = (CReal)(m_cCurPt.x - m_cLastPt.x) * 0.005f;

		pNav->ORotY( yAngle );
		pNav->ORotX( xAngle );
		
		// Restrict their rotation so the camera doesn't roll.
		pNav->Right().y = 0;
		pNav->Right().Norm();

		pNav->MakeForward();
		pNav->Forward().Norm();

		pNav->MakeUp();
		pNav->Up().Norm();

		// The direction to move
		float fDirection=0.0f;
		if (m_bForward)
		{
			fDirection=1.0f;
		}
		else if (m_bBackward)
		{
			fDirection=-1.0f;
		}
		
		// The number of units to move per second
		float fUnitsPerSecond=225.0f;

		// The number of units to move based on time
		float fUnits=((float)m_dwIdleDelta/1000.0f)*fUnitsPerSecond;

		// Move the camera	
		pNav->Pos() += pNav->Forward()*fDirection*fUnits;
	}

	// Reset the idle delta accumulator
	m_dwIdleDelta = 0;

	// Reset the cursor position
	CPoint cScreenPt = GetStartPt();
	GetView()->ClientToScreen(&cScreenPt);
	SetCursorPos(cScreenPt.x, cScreenPt.y);

	m_pView->Invalidate(FALSE);

	return TRUE;
}

BOOL CRVTrackerNavRotate::OnEnd()
{
	return TRUE;
}

