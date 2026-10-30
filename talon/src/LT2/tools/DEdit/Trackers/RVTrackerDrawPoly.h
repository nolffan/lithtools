//////////////////////////////////////////////////////////////////////
// RVTrackerDrawPoly.h - Header for the polygon creation tracker 

#ifndef __RVTRACKERDRAWPOLY_H__
#define __RVTRACKERDRAWPOLY_H__

#include "rvtracker.h"

// talon: the 2001 tracker (vtable 0x4df398, 0xd0 bytes: InitTracker's
// operator new) keeps the keys it reacts to as events (the constructor
// 0x4756cf), where Build 69 has child trackers set up from the hotkeys
class CRVTrackerDrawPoly : public CRegionViewTracker
{
protected:

	CUIEvent	*m_pUndoEvent;
	CUIEvent	*m_pNewVertexEvent;
	CUIEvent	*m_pSplitEvent;
	CUIEvent	*m_pRotateEvent;
	CUIEvent	*m_pNewEdgeEvent;

	// Whether or not to allow cancelling
	BOOL m_bAllowCancel;

	// Replace one of the events (0x4757dc)
	virtual void SetEvent(CUIEvent **ppEvent, const CUIEvent &cEvent) { delete *ppEvent; *ppEvent = cEvent.Clone(); };

	// Action commands in response to events.  Returns TRUE to end the tracker
	virtual BOOL OnUndo();
	virtual BOOL OnNewVertex();
	virtual BOOL OnSplit();
	virtual BOOL OnRotate();
	virtual BOOL OnNewEdge();

public:

	CRVTrackerDrawPoly(LPCTSTR pName = "", CRegionView *pView = NULL);
	virtual ~CRVTrackerDrawPoly();

	// talon: the events' accessors, vtable slots 0x100..0x124 (names unknown)
	virtual const CUIEvent *GetUndoEvent() const { return m_pUndoEvent; };
	virtual void SetUndoEvent(const CUIEvent &cEvent) { SetEvent(&m_pUndoEvent, cEvent); };
	virtual const CUIEvent *GetNewVertexEvent() const { return m_pNewVertexEvent; };
	virtual void SetNewVertexEvent(const CUIEvent &cEvent) { SetEvent(&m_pNewVertexEvent, cEvent); };
	virtual const CUIEvent *GetSplitEvent() const { return m_pSplitEvent; };
	virtual void SetSplitEvent(const CUIEvent &cEvent) { SetEvent(&m_pSplitEvent, cEvent); };
	virtual const CUIEvent *GetRotateEvent() const { return m_pRotateEvent; };
	virtual void SetRotateEvent(const CUIEvent &cEvent) { SetEvent(&m_pRotateEvent, cEvent); };
	virtual const CUIEvent *GetNewEdgeEvent() const { return m_pNewEdgeEvent; };
	virtual void SetNewEdgeEvent(const CUIEvent &cEvent) { SetEvent(&m_pNewEdgeEvent, cEvent); };

	// Cancel override
	virtual void Cancel();

	virtual BOOL OnStart();
	virtual BOOL OnUpdate(const CUIEvent &cEvent);
	virtual BOOL OnEnd();

private:

	// Internal supporting routines for inserting new edges
	BOOL FindSplittingBrushInfo(CEditBrush *pBrush, float &fTestError, CPolyRef &rTestRef, CEditVert &vVert1, CEditVert &vVert2);
	BOOL FindSplittingInfo(CPolyRef &rRef, CEditVert &vVert1, CEditVert &vVert2);

};

#endif //__RVTRACKERDRAWPOLY_H__