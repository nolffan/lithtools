// ------------------------------------------------------------------------
// LVEditList.cpp
//
// A list control that starts editing the selected item's label on F2.
//
// talon: no source of this survives, nor its name or its class's: the
// module at 0x415ee9..0x415fc9, after ltquatbase and before ModelEdit by
// name, with no model-header initializer (it includes only MFC). Written
// from ModelEdit.exe (Talon, 2001-12-19): the constructor 0x415ee9, the
// destructor 0x415f17 (??_G 0x415efb), the message map 0x474f28
// (WM_KEYDOWN) and OnKeyDown 0x415f28; 0x415f91 and 0x415fa5 are
// afxcmn.inl's GetFirstSelectedItemPosition and GetNextSelectedItem.
// CDragList (0x402463) and the main dialog's two lists (0x416399,
// 0x4163a8) are of this class.
// ------------------------------------------------------------------------

#include "stdafx.h"
#include "LVEditList.h"


BEGIN_MESSAGE_MAP(CLVEditList, CListCtrl)
	//{{AFX_MSG_MAP(CLVEditList)
	ON_WM_KEYDOWN()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


// 0x415ee9
CLVEditList::CLVEditList()
{
}


// 0x415f28
void CLVEditList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == VK_F2 )
	{
		if( !(GetStyle() & LVS_EDITLABELS) )
			return;

		if( GetSelectedCount() == 1 )
		{
			POSITION pos = GetFirstSelectedItemPosition();
			if( pos )
				EditLabel( GetNextSelectedItem( pos ) );
		}
	}

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}
