//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
#ifndef __DEDIT_H__
#define __DEDIT_H__


// DEdit.h : main header file for the DEDIT application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
#include "deditoptions.h"
#include "oldtypes.h"

class CMainFrame;
class CDEditOptions;


/////////////////////////////////////////////////////////////////////////////
// CDEditApp:
// See DEdit.cpp for the implementation of this class
//
// talon: 2001's layout (the constructor 0x41653e, InitInstance 0x416687):
// m_WorkingDir 0xc0, m_ExeDirectory 0x1c4, m_bFullUpdate 0x2c8,
// m_TextureDisplaySize 0x2cc, m_pWorldTemplate 0x2d0, m_genRegMgr 0x2d4,
// m_DEditOptions 0x2dc, m_pClassHelpButeMgr 0x328. No hot keys, model
// manager or class help aggregate.
class CButeMgr;
class CDEditApp : public CWinApp
{
	
// Member functions...

public:

	CDEditApp();
	~CDEditApp();
	
	virtual BOOL			PreTranslateMessage(MSG* pMsg);
	void					LoadRegistryStuff();
	void					SaveRegistryStuff();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDEditApp)
public:
		virtual BOOL InitInstance();
		virtual BOOL OnIdle(LONG lCount);
		virtual BOOL SaveAllModified();
//}}AFX_VIRTUAL


// Member data...
	
public:

	char		m_WorkingDir[MAX_PATH];		// The working directory
	char		m_ExeDirectory[MAX_PATH];	// The directory which the DEdit.exe file was run resides	

	BOOL		m_bFullUpdate;
	int			m_TextureDisplaySize;

	CMultiDocTemplate		*m_pWorldTemplate;

	CDEditOptions			&GetOptions()			{ return m_DEditOptions; }

	CButeMgr				*GetClassHelpButeMgr()	{ return m_pClassHelpButeMgr; }


// Implementation

	//{{AFX_MSG(CDEditApp)
		afx_msg void OnAppAbout();
		afx_msg void OnQuickHelp();
		afx_msg void OnKeyMap();
		afx_msg void OnFileNewProject();
		afx_msg void OnFileSave();
		afx_msg void OnUpdateFileSave(CCmdUI *pCmdUI);
		afx_msg void OnFileCloseProject();
		afx_msg void OnUpdateFileCloseProject(CCmdUI *pCmdUI);
		afx_msg void OnFileOpen();
		afx_msg void OnFileNewWorld();
		afx_msg void OnUpdateFileNewWorld(CCmdUI *pCmdUI);
		afx_msg void OnUpdateFileCloseWorld(CCmdUI *pCmdUI);
		afx_msg void OnFileCloseWorld();
		afx_msg void OnFileSaveWorldAs();
		afx_msg void OnUpdateFileSaveWorldAs(CCmdUI *pCmdUI);
		afx_msg BOOL OnOpenRecentFile(UINT nID);
		afx_msg void OnDEHomePage();
		afx_msg void OnOnlineDoc();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:

	CGenRegMgr			m_genRegMgr;
	CDEditOptions		m_DEditOptions;

	CButeMgr			*m_pClassHelpButeMgr;
};

#include "edithelpers.h"

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif

/////////////////////////////////////////////////////////////////////////////


#endif  // __DEDIT_H__


