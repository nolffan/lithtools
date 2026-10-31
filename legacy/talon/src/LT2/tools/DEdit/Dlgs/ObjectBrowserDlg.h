#if !defined(AFX_OBJECTBROWSERDLG_H__C92FEA73_01AA_11D3_BE1D_0060971BDC6D__INCLUDED_)
#define AFX_OBJECTBROWSERDLG_H__C92FEA73_01AA_11D3_BE1D_0060971BDC6D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ObjectBrowserDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CObjectBrowserDlg dialog

class CObjectBrowserDlg : public CDialog
{
// Construction
public:
	CObjectBrowserDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CObjectBrowserDlg)
	enum { IDD = IDD_OBJECT_BROWSER };
	CTreeCtrl	m_treeObjects;
	CListBox	m_listObjects;
	BOOL	m_bGroupByType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CObjectBrowserDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CObjectBrowserDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckGroupByType();
	afx_msg void OnSelchangeListObjects();
	afx_msg void OnSelchangedTreeObjects(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchangingTreeObjects(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnButtonSelect();
	afx_msg void OnButtonLocate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	// Sets the selected object
	void			SetSelectedObject(CBaseEditObj *pObject);
	void			SetSelectedObject(CString sObjectName);

	// Sets whether or not the specified control should be visible or not
	void			SetControlVisible(int nID, BOOL bVisible=TRUE);

	// Returns the selected object
	CBaseEditObj	*GetSelectedObject()	{ return m_pSelectedObject; }

protected:
	// Fills the tree control with the objects grouped by their class
	void		BuildObjectTree(CTreeCtrl *pTree);

	// Updates the control states (visible and enabled) based on the
	// options that are selected.
	void		UpdateControlStates();

	// Selects a specific object in the tree.  Returns TRUE if the object
	// has been selected.  The tree is searched recursively.
	BOOL		RecurseSelectObject(CTreeCtrl *pTree, HTREEITEM hParentItem, CBaseEditObj *pObject);

protected:
	BOOL			m_bIsDialogValid;		// This is TRUE after OnInitDialog and before DestroyWindow
	CBaseEditObj	*m_pSelectedObject;		// Contains the currently selected object

	// This is the array of controls that should be made invisible upon
	// the dialog opening.
	CDWordArray		m_controlInvisibleArray;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OBJECTBROWSERDLG_H__C92FEA73_01AA_11D3_BE1D_0060971BDC6D__INCLUDED_)
