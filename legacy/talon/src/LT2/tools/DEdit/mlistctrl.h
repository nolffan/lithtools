// mlistctrl.h - a list control with drag and drop, mtreectl's list
// counterpart.  talon: 2001's, written from the image (0x431bf2..0x431f18:
// its message handlers and message map only; nothing in the image makes
// one, so its constructor and vtable are dead-stripped).  The file and
// class names are ours.

#ifndef INC_MYLISTCTRL_H
#define INC_MYLISTCTRL_H

class CMyListCtrl : public CListCtrl
{
public:
	BOOL		m_bDragging;		// 0x3c
	int			m_iItemDrag;		// 0x40
	int			m_iItemDrop;		// 0x44
	CPoint		m_ptHotSpot;		// 0x48
	CPoint		m_ptOrigin;			// 0x50
	CSize		m_sizeDelta;		// 0x58
	CImageList	*m_pImageList;		// 0x60

protected:
	void		OnButtonUp(CPoint point);	// 0x431d96

	//{{AFX_MSG(CMyListCtrl)
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif
