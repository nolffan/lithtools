//////////////////////////////////////////////////////////////////////
// RVTrackerNodeMove.h - Header for the node movement tracker 

#ifndef __RVTRACKERNODEMOVE_H__
#define __RVTRACKERNODEMOVE_H__

#include "rvtracker.h"

class CRVTrackerNodeMove : public CRegionViewTracker
{
public:
	enum EctorFlags { 
		FLAG_NONE = 0,
		FLAG_SNAP = 1, 
		FLAG_HANDLE = 2,
		FLAG_PERP = 4,
		FLAG_CLONE = 8,
	};

	CRVTrackerNodeMove(LPCTSTR pName = "", CRegionView *pView = NULL, int iFlags = FLAG_NONE);
	virtual ~CRVTrackerNodeMove();

	BOOL m_bSnap, m_bHandle, m_bPerp;

	// talon: 2001's keys are events (the constructor 0x477c48), where Build 69
	// has child trackers from the hotkeys
	BOOL					m_bLockAxis;
	CUIEvent*				m_pLockAxisEvent;

	// (Note : This only has an effect at OnStart)
	BOOL					m_bClone;
	CUIEvent*				m_pCloneEvent;

	CVector m_vStartVec, m_vTotalMoveOffset, m_vMoveSnapAxis;

	virtual BOOL OnStart();
	virtual BOOL OnUpdate(const CUIEvent &cEvent);
	virtual BOOL OnEnd();

	virtual void WatchEvent(const CUIEvent &cEvent);

	// talon: vtable 0x4dfa60 slots 0xe8 (0x47771f), 0xec (0x477e44); names unknown
	virtual void SetLockAxisEvent(const CUIEvent &cEvent) { delete m_pLockAxisEvent; m_pLockAxisEvent = cEvent.Clone(); };
	virtual void SetCloneEvent(const CUIEvent &cEvent) { delete m_pCloneEvent; m_pCloneEvent = cEvent.Clone(); };
};

#endif //__RVTRACKERNODEMOVE_H__