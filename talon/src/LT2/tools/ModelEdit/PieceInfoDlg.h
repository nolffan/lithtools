#if !defined(AFX_PIECEINFODLG_H__INCLUDED_)
#define AFX_PIECEINFODLG_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// PieceInfoDlg.h : header file
//
// 2001's "Piece Info" dialog (dialog 180, 0x424903..0x424cfe in the
// image): a piece's specular power and scale and its texture index.
// No source survives (Build 69 replaced it with CPieceMaterialDlg); the
// class name is ours, the layout is the image's.

#include "model.h"

/////////////////////////////////////////////////////////////////////////////
// CPieceInfoDlg dialog

class CPieceInfoDlg : public CDialog
{
// Construction
public:
	CPieceInfoDlg(ModelPiece *pPiece, CWnd* pParent = NULL);   // 0x424b69

// Dialog Data
	//{{AFX_DATA(CPieceInfoDlg)
	enum { IDD = IDD_PIECEINFO };
	float	m_SpecularPower;	// +0x5c
	float	m_SpecularScale;	// +0x60
	int		m_TextureIndex;		// +0x64
	//}}AFX_DATA

	DWORD		m_ChangedFlags;	// +0x68: which labels have been starred
	ModelPiece	*m_pPiece;		// +0x6c

	// Star a control's label ("* %s") the first time its value changes.
	void		MarkChanged(UINT nLabelID, DWORD flag);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPieceInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPieceInfoDlg)
	afx_msg void OnChangeSpecularPower();
	afx_msg void OnChangeSpecularScale();
	afx_msg void OnChangeTextureIndex();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PIECEINFODLG_H__INCLUDED_)
