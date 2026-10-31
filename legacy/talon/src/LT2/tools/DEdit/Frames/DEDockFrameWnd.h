#ifndef USE_DOCKFRAMEWND
#define USE_DOCKFRAMEWND

#undef DWORD
#include "afxpriv.h"
#define DWORD unsigned long	// talon: the LT2 SDK's uint32 (Build 69's is unsigned int)
#include "mrcpriv.h"

/////////////////////////////////////////////////////////////////////////////
// CSizeDockFrame window
class CDEDockFrameWnd : public CSizableMiniDockFrameWnd
{
private:
    DECLARE_DYNCREATE(CDEDockFrameWnd)

	// talon: 2001's is empty (CreateObject, 0x41868a)
	CDEDockFrameWnd::CDEDockFrameWnd()
	{
	}

    //{{AFX_VIRTUAL(CSizableMiniDockFrameWnd)
    protected:    
    //}}AFX_VIRTUAL

    
	//{{AFX_MSG(CSizableMiniDockFrameWnd)    	
    afx_msg void OnClose();		
	//}}AFX_MSG

    DECLARE_MESSAGE_MAP()
};

#endif
