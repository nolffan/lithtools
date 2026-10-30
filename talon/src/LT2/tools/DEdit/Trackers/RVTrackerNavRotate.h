//////////////////////////////////////////////////////////////////////
// RVTrackerNavRotate.h - Header for the navigation movement tracker 

#ifndef __RVTRACKERNAVROTATE_H__
#define __RVTRACKERNAVROTATE_H__

#include "rvtracker.h"

class CRVTrackerNavRotate : public CRegionViewTracker
{
public:
	CRVTrackerNavRotate(LPCTSTR pName = "", CRegionView *pView = NULL);
	virtual ~CRVTrackerNavRotate();

	// talon: a member the 2001 class has before its own (0xb8), never
	// initialised; see the .cpp
	int m_nUnknownB8;

	// Track whether or not the mouse buttons are down: 2001's are events (the
	// constructor), where Build 69 has child trackers from the hotkeys
	BOOL m_bForward;
	CUIEvent *m_pForwardEvent;

	BOOL m_bBackward;
	CUIEvent *m_pBackwardEvent;

	// Only update on idle processing
	BOOL m_bIdleOnly;

	// Time accumulated since the last idle
	DWORD m_dwIdleDelta;

	virtual BOOL OnStart();
	virtual BOOL OnUpdate(const CUIEvent &cEvent);
	virtual BOOL OnEnd();

	virtual void WatchEvent(const CUIEvent &cEvent);

	// talon: vtable slots 0xe8 (0x477248), 0xec (0x47771f); names unknown
	virtual void SetForwardEvent(const CUIEvent &cEvent) { delete m_pForwardEvent; m_pForwardEvent = cEvent.Clone(); };
	virtual void SetBackwardEvent(const CUIEvent &cEvent) { delete m_pBackwardEvent; m_pBackwardEvent = cEvent.Clone(); };
};

#endif //__RVTRACKERNAVROTATE_H__