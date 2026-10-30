#ifndef __TEXTUREDLG_H__
#define __TEXTUREDLG_H__

#include "resourcemgr.h"
#include "baserezdlg.h"
#include "alltexturedlg.h"


// (Create Alpha Mask type).
typedef enum
{
	CAM_Solid=0,
	CAM_FromColor,
	CAM_From4BitPCX,
	CAM_From8BitPCX
} CAMType;


typedef enum
{
	ITYPE_PCX=0,
	ITYPE_TGA
} ImageType;


/////////////////////////////////////////////////////////////////////////////
// CTextureDlg dialog

// 2001: a CBaseRezDlg of its own (no CBaseImgDlg, no thumbnails): the
// constructor 0x408acc, the layout from its stores (m_TextureTree 0xec,
// m_TextureList 0x128, m_pIconList 0x164, m_pIconList2 0x168, m_hFont 0x16c,
// m_AllTextureDlg 0x170, m_hCurrentTextureDir 0x260,
// m_nCurrentTextureIndex 0x264).
class CTextureDlg : public CBaseRezDlg
{
// Construction
public:
	CTextureDlg();			// standard constructor
	virtual ~CTextureDlg();	// destructor

// Dialog Data
	//{{AFX_DATA(CTextureDlg)
	enum { IDD = IDD_TEXTURE_TABDLG };
	//}}AFX_DATA

	void	SetListItemText(int nItem, CString &relativeFilename);

	CTreeCtrl	m_TextureTree;
	CListCtrl	m_TextureList;

	CImageList	*m_pIconList;
	CImageList	*m_pIconList2;

	HFONT		m_hFont;

	CAllTextureDlg	m_AllTextureDlg;

	HTREEITEM	m_hCurrentTextureDir;
	int			m_nCurrentTextureIndex;
	void		ChangeSelection(int nIndex, BOOL bClearSelect = TRUE);
	BOOL		DoImportOperation(DWORD flags );
	BOOL		DoImportMipPcxOperation( );
	BOOL		DoExportPcxOperation( );
	BOOL		DoExport8BitDTXOperation( );
	BOOL		DoExportAllAsBPP_32POperation( );
	BOOL		DoTextureProperties( );

	BOOL		DoCreateAlphaMask(CAMType type);
	
	BOOL		DoViewAllTextures();
	BOOL		DoAddToPalette();
	void		CopyImageToTexture( ImageType iType,
		const char *pPcxFilename, const char *pTextureFilename, DWORD textureFlags );
	BOOL		CopyMipTGAToTexture( const CString *pTGAFilename, const char *pTextureFilename );

	// Finds a specified texture in the list
	BOOL		FindTexture(DFileIdent *pTexture, BOOL bRecurse = FALSE, HTREEITEM hBaseTree = 0);
	// Tries to find a specified texture based on the filename instead of searching recursively
	BOOL		FastFindTexture(DFileIdent *pTexture);

	// Draws the selected texture in the bottom third of the dialog (0x409427).
	void		RenderLargeImage();


// Overrides
	//{{AFX_VIRTUAL(CTextureDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	BOOL OnInitDialogBar();

	// This is called to reposition the controls
	virtual void	RepositionControls();

	// Generated message map functions
	//{{AFX_MSG(CTextureDlg)	
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelTexture( NMHDR * pNMHDR, LRESULT * pResult );
	afx_msg void OnOpenTexture( NMHDR * pNMHDR, LRESULT * pResult );
	afx_msg void OnKeySelTexture(NMHDR * pnkd, LRESULT * pResult );
	afx_msg void OnSelchangedDirectory(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnItemexpandingBaserezTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}


#endif 
