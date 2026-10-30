//////////////////////////////////////////////////////////////////////
// RVTrackerFocus.h - Header for the focus tracker 

#ifndef __RVTRACKERFOCUS_H__
#define __RVTRACKERFOCUS_H__

#include "rvtracker.h"

class CRVTrackerFocus : public CRegionViewTracker
{
private:
	BOOL m_bWasActive;

	// The event which locks the focus (0x476be6)
	CUIEvent *m_pLockEvent;
	
	static BOOL m_bLocked;

	BYTE m_aKeyStates[256];

	// Synchronize the trackers with the current key state
	void ResyncKeyList();

public:
	// Refresh the current key list from windows
	void RefreshKeyList();

	CRVTrackerFocus(LPCTSTR pName = "", CRegionView *pView = NULL);
	virtual ~CRVTrackerFocus();

	virtual BOOL OnStart();
	virtual void OnCancel();

	virtual void WatchEvent(const CUIEvent &cEvent);

	// talon: 2001's lock event (vtable 0x4df5a8 slots 0xe8, 0xec), where
	// Build 69 has a lock tracker set up from the hotkeys
	virtual void SetLockEvent(const CUIEvent &cEvent) { delete m_pLockEvent; m_pLockEvent = cEvent.Clone(); };
	virtual const CUIEvent *GetLockEvent() const { return m_pLockEvent; };

	virtual void SetLocked(BOOL bLocked) { m_bLocked = bLocked; };
	virtual BOOL GetLocked() const { return m_bLocked; };
};

#endif //__RVTRACKERFOCUS_H__