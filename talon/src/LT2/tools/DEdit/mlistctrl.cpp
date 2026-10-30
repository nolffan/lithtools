// mlistctrl.cpp - a list control with drag and drop.  talon: 2001's,
// written from the image (0x431bf2..0x431f18; its message map is .rdata
// 0x4d8cc0).  The names are ours.

#include "bdefs.h"
#include "mlistctrl.h"

BEGIN_MESSAGE_MAP(CMyListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CMyListCtrl)
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBeginDrag)
	ON_NOTIFY_REFLECT(LVN_BEGINRDRAG, OnBeginDrag)
	ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndLabelEdit)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// 0x431bf2
void CMyListCtrl::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW *pNMListView = (NM_LISTVIEW*)pNMHDR;
	CPoint ptItem, ptAction, ptImage;

	m_bDragging = TRUE;
	m_iItemDrag = pNMListView->iItem;
	ptAction = pNMListView->ptAction;
	GetItemPosition(m_iItemDrag, &ptItem);
	GetOrigin(&m_ptOrigin);

	m_pImageList = CreateDragImage(m_iItemDrag, &ptImage);
	m_sizeDelta = ptAction - ptImage;
	m_ptHotSpot = ptAction - ptItem + m_ptOrigin;

	CImageList::DragShowNolock(TRUE);
	m_pImageList->SetDragCursorImage(0, m_ptHotSpot);
	m_pImageList->BeginDrag(0, CPoint(0, 0));
	ptAction -= m_sizeDelta;
	CImageList::DragEnter(this, ptAction);
	CImageList::DragMove(ptAction);
	SetCapture();
}

// 0x431cdf
void CMyListCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	long lStyle = GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK;

	if(m_bDragging)
	{
		CImageList::DragMove(point - m_sizeDelta);

		int iItem = HitTest(point, NULL);
		if(iItem != -1)
		{
			m_iItemDrop = iItem;
			CImageList::DragLeave(this);

			if(lStyle == LVS_REPORT || lStyle == LVS_LIST)
			{
				LV_ITEM lvitem;
				lvitem.iItem = iItem;
				lvitem.iSubItem = 0;
				lvitem.mask = LVIF_STATE;
				lvitem.stateMask = LVIS_DROPHILITED;
				SetItem(&lvitem);
			}

			CImageList::DragEnter(this, point - m_sizeDelta);
		}
	}

	CListCtrl::OnMouseMove(nFlags, point);
}

// 0x431d96
void CMyListCtrl::OnButtonUp(CPoint point)
{
	if(m_bDragging)
	{
		CString cstr;
		long lStyle = GetWindowLong(m_hWnd, GWL_STYLE) & LVS_TYPEMASK;

		m_bDragging = FALSE;
		CImageList::DragLeave(this);
		CImageList::EndDrag();
		delete m_pImageList;
		m_pImageList = NULL;

		if(lStyle == LVS_REPORT && m_iItemDrop != m_iItemDrag)
		{
			cstr = GetItemText(m_iItemDrag, 0);
			SetItemText(m_iItemDrop, 1, cstr);
		}

		if(lStyle == LVS_LIST && m_iItemDrop != m_iItemDrag)
		{
			cstr = GetItemText(m_iItemDrop, 0);
			cstr += "**";
			SetItemText(m_iItemDrop, 0, cstr);
		}

		if(lStyle == LVS_ICON || lStyle == LVS_SMALLICON)
		{
			point -= m_ptHotSpot;
			point += m_ptOrigin;
			SetItemPosition(m_iItemDrag, point);
		}

		ReleaseCapture();
	}
}

// 0x431ee3 (the two are one function in the image)
void CMyListCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	OnButtonUp(point);
	CListCtrl::OnLButtonUp(nFlags, point);
}

void CMyListCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	OnButtonUp(point);
	CListCtrl::OnRButtonUp(nFlags, point);
}

// 0x431efe
void CMyListCtrl::OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult)
{
	LV_DISPINFO *pDispInfo = (LV_DISPINFO*)pNMHDR;

	if(pDispInfo->item.pszText)
		SetItemText(pDispInfo->item.iItem, pDispInfo->item.iSubItem, pDispInfo->item.pszText);
}
