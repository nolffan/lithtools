//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// ResNewDir.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "resnewdir.h"
#include "resourcemgr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CResNewDir dialog


CResNewDir::CResNewDir(CWnd* pParent /*=NULL*/)
	: CDialog(CResNewDir::IDD, pParent)
{
	//{{AFX_DATA_INIT(CResNewDir)
	m_szNewDir = _T("");
	//}}AFX_DATA_INIT

	m_cDirType = RESTYPE_WORLD;
}


void CResNewDir::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CResNewDir)
	DDX_Text(pDX, IDC_ENTEREDTEXT, m_szNewDir);
	DDV_MaxChars(pDX, m_szNewDir, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CResNewDir, CDialog)
	//{{AFX_MSG_MAP(CResNewDir)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResNewDir message handlers


/////////////////////////////////////////////////////////////////////////////
// talon: a second dialog of 2001's ResNewDir.cpp, of which only the
// message map (0x4dd108, WM_DESTROY) and its OnDestroy (0x45f0f9) are in
// the image: nothing constructs it, so the link dropped its constructor
// and vtable.  A tree control at 0x5c and the chosen item's data at 0x98
// are all it shows; the names are ours, and the dialog id is not known.

class CResDirTreeDlg : public CDialog
{
public:
	CTreeCtrl	m_Tree;
	DWORD		m_dwSelData;

protected:
	afx_msg void OnDestroy();
	DECLARE_MESSAGE_MAP()
};


BEGIN_MESSAGE_MAP(CResDirTreeDlg, CDialog)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


// 0x45f0f9: keeps the selected item's data and frees the tree's image list.
void CResDirTreeDlg::OnDestroy()
{
	m_dwSelData = m_Tree.GetItemData(m_Tree.GetSelectedItem());
	delete m_Tree.GetImageList(TVSIL_NORMAL);

	CDialog::OnDestroy();
}
