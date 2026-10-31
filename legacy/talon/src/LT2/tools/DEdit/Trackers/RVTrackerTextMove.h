//////////////////////////////////////////////////////////////////////
// RVTrackerTextMove.h - Header for the texture movement tracker 

#ifndef __RVTRACKERTEXTMOVE_H__
#define __RVTRACKERTEXTMOVE_H__

#include "rvtracker.h"

// talon: the 2001 tracker (vtable 0x4dfd38, 0x110 bytes) moves the texture
// of one polygon, the immediate one (OnStart 0x47962c), where Build 69 moves
// every selected polygon's; its keys are events (the constructor 0x47938d)
class CRVTrackerTextureMove : public CRegionViewTracker
{
protected:

	// The polygon and its original texture space
	CEditPoly *m_pPoly;
	CVector m_vOriginalO, m_vOriginalP, m_vOriginalQ;

	CUIEvent *m_pRotateEvent, *m_pScaleEvent, *m_pStepEvent;
	BOOL m_bRotate, m_bScale, m_bStep;

	int32 m_iTotalRotate;
	int32 m_iTotalScaleX, m_iTotalScaleY;
	int32 m_iTotalOfsX, m_iTotalOfsY;

	BOOL m_bSaveUndo;

public:
	CRVTrackerTextureMove(LPCTSTR pName = "", CRegionView *pView = NULL);
	virtual ~CRVTrackerTextureMove();

	// Overridden functions
	virtual void WatchEvent(const CUIEvent &cEvent);

	virtual BOOL OnStart();
	virtual BOOL OnUpdate(const CUIEvent &cEvent);
	virtual BOOL OnEnd();

	// talon: vtable slots 0xe8..0xfc (0x479475..0x4794f8); names unknown
	virtual void SetRotateEvent(const CUIEvent &cEvent) { delete m_pRotateEvent; m_pRotateEvent = cEvent.Clone(); };
	virtual const CUIEvent *GetRotateEvent() const { return m_pRotateEvent; };
	virtual void SetScaleEvent(const CUIEvent &cEvent) { delete m_pScaleEvent; m_pScaleEvent = cEvent.Clone(); };
	virtual const CUIEvent *GetScaleEvent() const { return m_pScaleEvent; };
	virtual void SetStepEvent(const CUIEvent &cEvent) { delete m_pStepEvent; m_pStepEvent = cEvent.Clone(); };
	virtual const CUIEvent *GetStepEvent() const { return m_pStepEvent; };
};

#endif //__RVTRACKERTEXTMOVE_H__