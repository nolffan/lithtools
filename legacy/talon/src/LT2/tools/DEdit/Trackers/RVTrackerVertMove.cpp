//////////////////////////////////////////////////////////////////////
// RVTrackerVertMove.h - Implementation for the vertex movement tracker 

#include "bdefs.h"
#include "rvtrackervertmove.h"

CRVTrackerVertMove::CRVTrackerVertMove(LPCTSTR pName, CRegionView *pView, ESelectMode eSelMode) :
	CRegionViewTracker(pName, pView),
	m_eSelectMode(eSelMode)
{
	m_bAutoCenter = FALSE;
	m_bAutoHide = TRUE;

	m_bPerp = FALSE;
	m_pPerpEvent = new CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT);

	m_bSnap = TRUE;
	m_pSnapEvent = new CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT);
}

CRVTrackerVertMove::~CRVTrackerVertMove()
{
	delete m_pPerpEvent;
	delete m_pSnapEvent;
}

// Watch for the toggle events to happen
void CRVTrackerVertMove::WatchEvent(const CUIEvent &cEvent)
{
	if (cEvent == *m_pPerpEvent)
		m_bPerp = TRUE;
	else if (cEvent.IsInverse(*m_pPerpEvent))
		m_bPerp = FALSE;

	if (cEvent == *m_pSnapEvent)
		m_bSnap = FALSE;
	else if (cEvent.IsInverse(*m_pSnapEvent))
		m_bSnap = TRUE;

	// Call the base class watch event
	CRegionViewTracker::WatchEvent(cEvent);
}

BOOL CRVTrackerVertMove::OnStart()
{
	// Only start in geometry mode
	if (m_pView->GetEditMode() != GEOMETRY_EDITMODE)
		return FALSE;

	switch (m_eSelectMode)
	{
		case esm_Poly :
			m_pView->GetSelectedPoly(m_cMovingVerts);
			break;
		case esm_SelVert :
			m_pView->GetSelectedVerts(m_cMovingVerts);
			break;
		case esm_ImmVert :
			m_pView->GetImmediateVert(m_cMovingVerts);
			break;
	}
	
	// Jump out if the movement list is empty for some reason
	if (!m_cMovingVerts.GetSize())
		return FALSE;

	CPoint point = m_cCurPt;
	CVertRef closest;
	PreActionList actionList;
	DWORD i;
	CEditGrid *pGrid = &m_pView->EditGrid();
	
	BOOL bStart = TRUE;

	// Act like we started out not snapping
	m_bSnapState = FALSE;

	// Start off on the green point.
	closest = m_pView->GetClosestVert(m_cMovingVerts, point, NULL);

	if( closest.IsValid() )
	{
		m_vStartVec = closest();
		m_vStartVec -= pGrid->Forward() * (pGrid->Forward().Dot(m_vStartVec - pGrid->Pos()));
		// Remember our distance away from the grid.
		GetVertexFromPoint(m_cCurPt, m_vSnapDist);
		m_vSnapDist -= closest();
		m_vSnapDist -= pGrid->Forward() * (pGrid->Forward().Dot(m_vSnapDist));
	}
	else
	{
		bStart = GetVertexFromPoint(point, m_vStartVec );
		// Initialize the snap distance
		m_vSnapDist.Init();
	}

	if (!bStart)
		return FALSE;

	// Setup an undo.
	for(i=0; i < m_cMovingVerts; i++)
	{
		AddToActionListIfNew(&actionList, 
			new CPreAction(ACTION_MODIFYNODE, m_cMovingVerts[i].m_pBrush), TRUE);
	}

	m_pView->GetRegionDoc()->Modify(&actionList, TRUE);

	return TRUE;
}

BOOL CRVTrackerVertMove::OnUpdate(const CUIEvent &cEvent)
{
	// Only update on idle events
	if (cEvent.GetType() != UIEVENT_NONE)
		return TRUE;

	// Don't update if it hasn't moved
	if ((m_cCurPt == m_cLastPt) && (m_bSnap == m_bSnapState))
		return TRUE;

	CVector			newVert;
	DWORD			i;

	// Do the autoscroll of the window
	DoAutoScroll();

	if (m_bPerp)
	{
		CVector vNormal(0.0f,0.0f,0.0f);
		if (m_eSelectMode == esm_Poly)
			vNormal = -m_pView->IPoly()()->Normal();
		else
		{
			CVector vMarker = m_pView->GetRegion( )->m_vMarker;
			for (i = 0; i < m_cMovingVerts; i++)
				vNormal += m_cMovingVerts[i]() - vMarker;
			vNormal.Norm();
		}

		for( i=0; i < m_cMovingVerts; i++ )
		{
			m_cMovingVerts[i]() -= vNormal * (CReal)(m_cCurPt.y - m_cLastPt.y);

			//update the planes of the brushes
			m_pView->GetRegion()->UpdateBrushGeometry(m_cMovingVerts[i].m_pBrush);
		}
	}
	else if (GetVertexFromPoint(m_cCurPt, newVert))
	{
		newVert -= m_vStartVec;

		if (!m_bSnap)
			newVert -= m_vSnapDist;

		for( i=0; i < m_cMovingVerts; i++ )
		{
			m_cMovingVerts[i]() += newVert;

			//update the planes of the brushes
			m_pView->GetRegion()->UpdateBrushGeometry(m_cMovingVerts[i].m_pBrush);
		}

		m_vStartVec += newVert;
	}

	// Remember the snap state
	m_bSnapState = m_bSnap;

	if( GetApp()->m_bFullUpdate )
	{
		m_pView->GetDocument()->UpdateAllViews(m_pView);
		m_pView->DrawRect();
	}

	m_cLastPt = m_cCurPt;

	return TRUE;
}

BOOL CRVTrackerVertMove::OnEnd()
{
	DWORD i;

	m_pView->UpdatePlanesOfPolies( m_cMovingVerts );

	m_cMovingVerts.Term();

	m_pView->GetRegionDoc()->UpdateSelectionBox();
	GetPropertiesDlg()->ReadControlValues();

	return TRUE;
}

