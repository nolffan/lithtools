//////////////////////////////////////////////////////////////////////
// RVTrackerVertMove.h - Header for the vertex movement tracker 

#ifndef __RVTRACKERVERTMOVE_H__
#define __RVTRACKERVERTMOVE_H__

#include "rvtracker.h"

class CRVTrackerVertMove : public CRegionViewTracker
{
public:
	typedef enum {esm_Poly, esm_SelVert, esm_ImmVert} ESelectMode;

	CRVTrackerVertMove(LPCTSTR pName = "", CRegionView *pView = NULL, ESelectMode eSelMode = esm_SelVert);
	virtual ~CRVTrackerVertMove();

	ESelectMode m_eSelectMode;

	CVertRefArray m_cMovingVerts;

	CVector m_vStartVec;

	// Tracking for the snapping mode: 2001's keys are events (the constructor
	// 0x47b7b3), where Build 69 has child trackers from the hotkeys
	BOOL m_bSnapState, m_bSnap;
	CVector m_vSnapDist;
	CUIEvent *m_pSnapEvent;

	// Track whether or not to use perpendicular mode
	BOOL m_bPerp;
	CUIEvent *m_pPerpEvent;

	virtual BOOL OnStart();
	virtual BOOL OnUpdate(const CUIEvent &cEvent);
	virtual BOOL OnEnd();

	virtual void WatchEvent(const CUIEvent &cEvent);

	// talon: vtable 0x4dff98 slots 0xe8 (0x47b996), 0xec (0x47b9bd); names unknown
	virtual void SetSnapEvent(const CUIEvent &cEvent) { delete m_pSnapEvent; m_pSnapEvent = cEvent.Clone(); };
	virtual void SetPerpEvent(const CUIEvent &cEvent) { delete m_pPerpEvent; m_pPerpEvent = cEvent.Clone(); };

protected:
	// Get a vertex position from the given point
	virtual BOOL GetVertexFromPoint(const CPoint &cPt, CVector &vResult) { return m_pView->GetVertexFromPoint(cPt, vResult); };
};

#endif //__RVTRACKERVERTMOVE_H__