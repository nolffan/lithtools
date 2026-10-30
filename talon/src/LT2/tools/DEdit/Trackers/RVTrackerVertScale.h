//////////////////////////////////////////////////////////////////////
// RVTrackerVertScale.h - Header for the vertex scaling tracker 

#ifndef __RVTRACKERVERTSCALE_H__
#define __RVTRACKERVERTSCALE_H__

#include "rvtracker.h"

class CRVTrackerVertScale : public CRegionViewTracker
{
public:
	typedef enum {esm_Poly, esm_SelVert, esm_ImmVert} ESelectMode;

	CRVTrackerVertScale(LPCTSTR pName = "", CRegionView *pView = NULL, ESelectMode eSelMode = esm_Poly);
	virtual ~CRVTrackerVertScale();

	ESelectMode m_eSelectMode;

	CVertRefArray m_cMovingVerts;

	CVector m_vScaleRefPt, m_vPerpNormal;


	// Track whether or not to use perpendicular mode: 2001's key is an event
	// (the constructor 0x47bf0d), where Build 69 has a child tracker
	BOOL m_bPerp;
	CUIEvent *m_pPerpEvent;

	virtual BOOL OnStart();
	virtual BOOL OnUpdate(const CUIEvent &cEvent);
	virtual BOOL OnEnd();

	virtual void WatchEvent(const CUIEvent &cEvent);

	// talon: vtable 0x4e0090 slot 0xe8 (0x47c066); name unknown
	virtual void SetPerpEvent(const CUIEvent &cEvent) { delete m_pPerpEvent; m_pPerpEvent = cEvent.Clone(); };
};

#endif //__RVTRACKERVERTSCALE_H__