// ------------------------------------------------------------------------
// LVEditList.h
//
// talon: reconstructed from ModelEdit.exe (Talon, 2001-12-19); see
// LVEditList.cpp. The file and class names are ours.
// ------------------------------------------------------------------------

#ifndef __LVEDITLIST_H__
#define __LVEDITLIST_H__

class CLVEditList : public CListCtrl
{
public:
	CLVEditList();

protected:
	//{{AFX_MSG(CLVEditList)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif
