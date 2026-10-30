#if !defined(AFX_MAPTEXTURECOORDSDLG_H__E21766E1_D02B_11D0_99E3_0060970987C3__INCLUDED_)
#define AFX_MAPTEXTURECOORDSDLG_H__E21766E1_D02B_11D0_99E3_0060970987C3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// MapTextureCoordsDlg.h : header file
//

class CRegionView;

#include "editpoly.h"

/////////////////////////////////////////////////////////////////////////////
// CMapTextureCoordsDlg dialog

// talon: the 2001 dialog (0x431464..0x431bd2): applied with Apply or OK,
// relative to the polies' current mapping; no undo of its own, no auto
// apply, no saved originals.
class CMapTextureCoordsDlg : public CDialog
{
// Construction
public:
	CMapTextureCoordsDlg(CRegionView *pView);	// 0x431464

// Dialog Data
	//{{AFX_DATA(CMapTextureCoordsDlg)
	enum { IDD = IDD_MAPTEXTURECOORDS };
	float	m_UOffset;			// 0x5c
	float	m_VOffset;			// 0x60
	float	m_UScale;			// 0x64
	float	m_VScale;			// 0x68
	float	m_Rotation;			// 0x6c
	BOOL	m_bStick;			// 0x70: "Stick", copied to each poly's 0x70
	BOOL	m_bXZOnly;			// 0x74: "XZ Only"
	//}}AFX_DATA

	CRegionView *m_pView;		// 0x78
	int		m_TextureWidth, m_TextureHeight;	// 0x7c, 0x80
	float	m_RefRotation;		// 0x84: the rotation already applied
	float	m_UOffsetDelta, m_VOffsetDelta;		// 0x88, 0x8c: the offsets' changes not yet applied

	void	DoFlip(DWORD iAxis);
	void	ApplyChanges();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapTextureCoordsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMapTextureCoordsDlg)
	afx_msg void OnApply();
	afx_msg void OnFlipX();
	afx_msg void OnFlipY();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPTEXTURECOORDSDLG_H__E21766E1_D02B_11D0_99E3_0060970987C3__INCLUDED_)
