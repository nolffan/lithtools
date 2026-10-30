//////////////////////////////////////////////////////////////////////
// RVTrackerNavMove.h - Header for the navigation movement tracker 

#ifndef __RVTRACKERNAVMOVE_H__
#define __RVTRACKERNAVMOVE_H__

#include "rvtracker.h"

class CRVTrackerNavMove : public CRegionViewTracker
{
public:
	CRVTrackerNavMove(LPCTSTR pName = "", CRegionView *pView = NULL);
	virtual ~CRVTrackerNavMove();

	// talon: a member the 2001 class has before its own (0xb8), never
	// initialised; see the .cpp
	int m_nUnknownB8;

	// Track whether or not the mouse buttons are down: 2001's are events (the
	// constructor), where Build 69 has child trackers from the hotkeys
	BOOL m_bPerp;
	CUIEvent *m_pPerpEvent;

	BOOL m_bFast;
	CUIEvent *m_pFastEvent;

	// Only update on idle processing
	BOOL m_bIdleOnly;

	virtual BOOL OnStart();
	virtual BOOL OnUpdate(const CUIEvent &cEvent);
	virtual BOOL OnEnd();

	virtual void WatchEvent(const CUIEvent &cEvent);

	// talon: vtable slots 0xe8 (0x477248), 0xec (0x47771f); names unknown
	virtual void SetPerpEvent(const CUIEvent &cEvent) { delete m_pPerpEvent; m_pPerpEvent = cEvent.Clone(); };
	virtual void SetFastEvent(const CUIEvent &cEvent) { delete m_pFastEvent; m_pFastEvent = cEvent.Clone(); };
};

#endif //__RVTRACKERNAVMOVE_H__