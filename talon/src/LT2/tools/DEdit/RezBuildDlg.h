//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// RezBuildDlg.h : the "Building Rez File" dialog.
//
// talon: 2001's, written from the image (0x45f4a3..0x45f68b); nothing of
// it survives in Build 69.  The class and file names are ours (the module
// sits between ResourceMgr and RotateSelection).  MainFrame's
// OnCreateRezFile (0x42fbe3) makes one, sets the two names and runs it;
// the dialog builds the file from a timer once it has painted, through
// the rez builder at 0x4312f2.
//

#ifndef __REZBUILDDLG_H__
#define __REZBUILDDLG_H__


// talon: what the rez builder (0x4312f2) is handed (the names are ours).
typedef void (*RezBuildFileCB)(const char *pFilename);
typedef void (*RezBuildMessageCB)(const char *pMsg);

struct RezBuildInfo
{
	const char			*m_pDir;		// the directory to pack
	const char			*m_pRezFile;	// the file to make
	RezBuildFileCB		m_FileCB;		// each file as it goes in
	RezBuildMessageCB	m_MessageCB;	// messages for the log
};

// talon: 0x4312f2 (in MainFrame's range, part 5's); 1 if the rez file
// can't be opened (OnTimer then reports IDS_CANTOPENOUTPUTREZFILE).
int BuildRezFile(RezBuildInfo *pInfo);


class CRezBuildDlg : public CDialog
{
public:
	CRezBuildDlg(CWnd* pParent = NULL);

	// talon: dialog 108 of the image's resources, which resource.h names
	// CG_ID_VIEW_MYDIALOGBAR.
	enum { IDD = 108 };

	UINT		m_nTimer;			// 0x5c
	CString		m_sRezFile;			// 0x60
	CString		m_sProjectDir;		// 0x64
	BOOL		m_bDone;			// 0x68
	BOOL		m_bPainted;			// 0x6c

protected:
	virtual BOOL OnInitDialog();

	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
};


#endif  // __REZBUILDDLG_H__
