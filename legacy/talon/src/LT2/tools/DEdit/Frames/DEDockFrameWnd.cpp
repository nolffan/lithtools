#include "bdefs.h"
#include "resource.h"

//#include "texturedlg.h"
//#include "edithelpers.h"
//#include "projectbar.h"
#include "dedockframewnd.h"
#include "mrcext.h"	// talon: for CMRCSizeControlBar; mainfrm.h not needed


IMPLEMENT_DYNCREATE(CDEDockFrameWnd, CSizableMiniDockFrameWnd)


BEGIN_MESSAGE_MAP(CDEDockFrameWnd, CSizableMiniDockFrameWnd)
	//{{AFX_MSG_MAP(CDEDockFrameWnd)    
    ON_WM_CLOSE()	
	//}}AFX_MSG_MAP
    // Global help commands
END_MESSAGE_MAP()


/************************************************************************/
void CDEDockFrameWnd::OnClose()
{
	// Get the control bar for this window
	CMRCSizeControlBar* pBar = ((CSizeDockBar *)(&m_wndDockBar))->GetFirstControlBar();

	// Call through the hide functionality
	pBar->OnButtonHide();
}
