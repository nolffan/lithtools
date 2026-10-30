//////////////////////////////////////////////////////////////////////
// TrackerMgr.cpp - Implementation of the tracker manager

#include "bdefs.h"
#include "trackermgr.h"

CUITrackerMgr::CUITrackerMgr() :
	m_pActiveTracker(NULL)
{
}

CUITrackerMgr::~CUITrackerMgr()
{
	ClearTrackerList();
}

void CUITrackerMgr::ClearTrackerList()
{
	for (DWORD uLoop = 0; uLoop < m_cTrackerList; uLoop++)
		delete m_cTrackerList[uLoop];
	m_cTrackerList.RemoveAll();
}

void CUITrackerMgr::AddTracker(CUITracker *cTracker)
{
	// Don't allow NULL trackers
	if (!cTracker)
		return;

	// Add the tracker to the list
	m_cTrackerList.Add(cTracker);
}

BOOL CUITrackerMgr::RemoveTracker(CUITracker *cTracker)
{
	// Don't try to remove NULL trackers
	if (!cTracker)
		return FALSE;

	// Remove the tracker from the list
	BOOL bResult = FALSE;
	DWORD uLoop = 0;
	while (uLoop < m_cTrackerList)
	{
		if (m_cTrackerList[uLoop] == cTracker)
		{
			m_cTrackerList.Remove(uLoop);
			// Save the result
			bResult = TRUE;
		}
		else
			uLoop++;
	}
	// Handle the active tracker getting removed
	if (GetActiveTracker() == cTracker)
		SetActiveTracker(NULL);

	return bResult;
}

CUITracker *CUITrackerMgr::FindTracker(LPCTSTR pName) const
{
	for (DWORD uLoop = 0; uLoop < m_cTrackerList; uLoop++)
		if (stricmp(m_cTrackerList[uLoop]->GetName(), pName) == 0)
			return m_cTrackerList[uLoop];
	return NULL;
}

BOOL CUITrackerMgr::ProcessEvent(const CUIEvent &cEvent)
{
	BOOL bResult = FALSE;

	// Send the event to the active tracker first
	if (GetActiveTracker())
	{
		bResult = GetActiveTracker()->ProcessEvent(cEvent);
		if (bResult)
			OnTrackerProcess(GetActiveTracker(), cEvent);
	}

	for (DWORD uLoop = 0; uLoop < m_cTrackerList; uLoop++)
	{
		CUITracker *pTracker = m_cTrackerList[uLoop];

		// Don't process the active tracker in the main loop
		if (pTracker == GetActiveTracker())
			continue;

		// Send the event to watching trackers
		if (bResult)
			pTracker->WatchEvent(cEvent);
		else
		{
			// If nobody's active yet, process the event
			bResult = pTracker->ProcessEvent(cEvent);
			// If somebody decided to take over, keep them as the active tracker
			if (bResult)
			{
				SetActiveTracker(m_cTrackerList[uLoop]);
				OnTrackerActivate(GetActiveTracker(), cEvent);
			}
		}
	}

	// Nobody's active
	if (!bResult)
		SetActiveTracker(NULL);

	return bResult;
}

void CUITrackerMgr::Cancel()
{
	DWORD uLoop;
	
	// Send the cancel notification to all trackers
	for (uLoop = 0; uLoop < m_cTrackerList.GetSize(); uLoop++)
		m_cTrackerList[uLoop]->Cancel();

	// Turn off the active tracker
	SetActiveTracker(NULL);
}
