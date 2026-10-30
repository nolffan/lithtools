//////////////////////////////////////////////////////////////////////
// RVTrackerNodeRotate.h - Header for the node rotation tracker 

#ifndef __RVTRACKERNODEROTATE_H__
#define __RVTRACKERNODEROTATE_H__

#include "rvtracker.h"

class CRVTrackerNodeRotate : public CRegionViewTracker
{
protected:
	// talon: 2001's step key is an event (the constructor 0x4784d0), where
	// Build 69 has a child tracker from the hotkeys
	CUIEvent *m_pStepEvent;
	BOOL m_bStep;

	int m_iTotalRotate, m_iTotalMove;

public:
	CRVTrackerNodeRotate(LPCTSTR pName = "", CRegionView *pView = NULL);
	virtual ~CRVTrackerNodeRotate();

	CVector	m_vTrackRotationVector, m_vTrackRotationCenter;

	virtual void WatchEvent(const CUIEvent &cEvent);
	virtual BOOL OnStart();
	virtual BOOL OnUpdate(const CUIEvent &cEvent);
	virtual BOOL OnEnd();

	// talon: vtable 0x4dfb50 slots 0xe8 (0x478542), 0xec (0x478569); names unknown
	virtual void SetStepEvent(const CUIEvent &cEvent) { delete m_pStepEvent; m_pStepEvent = cEvent.Clone(); };
	virtual const CUIEvent *GetStepEvent() const { return m_pStepEvent; };
};

#endif //__RVTRACKERNODEROTATE_H__