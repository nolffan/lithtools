//////////////////////////////////////////////////////////////////////
// UITracker.h - Definition of the base tracker class

#ifndef __UITRACKER_H__
#define __UITRACKER_H__

#include "uievent.h"

// Base abstract tracker class
class CUITracker
{
protected:
	BOOL m_bActive;
	CString m_csName;

public:
	CUITracker(LPCTSTR pName = "") : m_bActive(FALSE), m_csName(pName) {};
	virtual ~CUITracker() {};

	virtual const CString &GetName() const { return m_csName; };
	virtual void SetName(const CString &csNewName) { m_csName = csNewName; };

	virtual BOOL GetActive() const { return m_bActive; };
	virtual void SetActive(BOOL bActive) { m_bActive = bActive; };

	// Process the current event
	//		Returns: TRUE if the tracker processes the event & is therefore active
	virtual BOOL ProcessEvent(const CUIEvent &cEvent) { return GetActive(); };
	// Watch an event happen (for notification purposes)
	virtual void WatchEvent(const CUIEvent &cEvent) {};
	// Cancel tracking
	virtual void Cancel() { SetActive(FALSE); };

	// talon: the 2001 tracker has no WasMasked or FlushTracker (Build 69's
	// hotkey configuration): its vtable (0x4df1a0) ends at Cancel, slot 0x1c.
};

typedef CMoArray<CUITracker *> CUITrackerList;

#endif //__UITRACKER_H__