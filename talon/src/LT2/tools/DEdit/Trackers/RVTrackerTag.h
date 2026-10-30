//////////////////////////////////////////////////////////////////////
// RVTrackerTag.h - Header for the tagging tracker 

#ifndef __RVTRACKERTAG_H__
#define __RVTRACKERTAG_H__

#include "rvtracker.h"

class CRVTrackerTag : public CRegionViewTracker
{
protected:
	enum ESelectMode {
		SELECT_OFF,
		SELECT_ON,
		SELECT_INVERT
	};

	// Supporting function for selecting a node
	void SelectNode(CEditRegion *pRegion, CWorldNode *pNode, HTREEITEM hItem, ESelectMode eMode);

	// Find the polygon that's closest to the ray
	CPolyRef FindBestPoly(CEditRay ray);

public:
	// talon: 2001 has a fourth mode flag, for LIGHT_EDITMODE (OnStart 0x478b89,
	// the constructor 0x4789d8); its name is not in the image
	enum EctorFlags { 
		FLAG_NONE = 0,
		FLAG_OBJECT = 1, 
		FLAG_BRUSH = 2,
		FLAG_GEOMETRY = 4,
		FLAG_LIGHT = 8,
		FLAG_ALL = 31
	};

	CRVTrackerTag(LPCTSTR pName = "", CRegionView *pView = NULL, int iFlags = FLAG_NONE);
	virtual ~CRVTrackerTag();

	BOOL m_bObject, m_bBrush, m_bGeometry, m_bLight;

	// Track whether or not the "add" key is down
	BOOL m_bAdd;
	CUIEvent *m_pAddEvent;

	// Track whether or not the "invert" key is down
	BOOL m_bInvert;
	CUIEvent *m_pInvertEvent;

	virtual BOOL OnStart();
	virtual BOOL OnUpdate(const CUIEvent &cEvent);
	virtual BOOL OnEnd();
	virtual void OnCancel();

	virtual void WatchEvent(const CUIEvent &cEvent);

	// talon: the add and invert keys (vtable 0x4dfc40 slots 0xe8, 0xec: 0x478aeb,
	// 0x478b12); Build 69 has child trackers from the hotkeys. Names unknown.
	virtual void SetAddEvent(const CUIEvent &cEvent) { delete m_pAddEvent; m_pAddEvent = cEvent.Clone(); };
	virtual void SetInvertEvent(const CUIEvent &cEvent) { delete m_pInvertEvent; m_pInvertEvent = cEvent.Clone(); };
};

#endif //__RVTRACKERTAG_H__