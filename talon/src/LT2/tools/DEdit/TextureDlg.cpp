// TextureDlg.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "texturedlg.h"
#include "editprojectmgr.h"
#include "edithelpers.h"
#include "mainfrm.h"
#include "projectbar.h"
#include "resnewdir.h"
#include "texture.h"
#include "textureprop.h"
#include "spriteeditdlg.h"
#include "sysstreamsim.h"
#include "stringdlg.h"
#include "alphafromcolordlg.h"
#include "dtxmgr.h"
#include "s3tc_compress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern FormatMgr g_FormatMgr;



/////////////////////////////////////////////////////////////////////////////
// CTextureDlg dialog

BOOL GetTextureDimensions(DFileIdent *pIdent, DWORD *pWidth, DWORD *pHeight)
{
	CMoFileIO file;
	DtxHeader hdr;
	
	*pWidth = *pHeight = 0;

	if(!pIdent)
		return FALSE;

	if(!dfm_OpenFileRelative(GetFileMgr(), pIdent->m_Filename, file))
		return FALSE;

	LithTry
	{				
		file.Read(&hdr, sizeof(hdr));
		if(hdr.m_Version == CURRENT_DTX_VERSION)
		{
			*pWidth = hdr.m_BaseWidth >> hdr.GetUIMipmapOffset();
			*pHeight = hdr.m_BaseHeight >> hdr.GetUIMipmapOffset();
		}
	}
	LithCatch(CLithIOException &exc)
	{
		exc = exc;
		*pWidth = *pHeight = 0;
		return FALSE;
	}

	return TRUE;
}



// --------------------------------------------------------------------------------- //
// The TextureUtils class makes it easier to modify textures.
// Note: This should get its own file but doesn't have one yet due to logistical puff.
// --------------------------------------------------------------------------------- //
class TextureUtils
{
public:

					TextureUtils();

	// Create backups and setup m_PValueMipmaps.
	BOOL			FromTextureData(TextureData *pData);

	// Moves the data from m_PValueMipmaps into the TextureData.
	// If bRestoreColorData is TRUE, it calls RestoreColorData.
	BOOL			ToTextureData(TextureData *pData, BOOL bRestoreColorData);

	// This restores the color data into the texture (ie: if you only wanted to modify the
	// alpha data, you would replace it, recompress, and put the color back the way it was..)
	// If you didn't restore the color, it would have gotten re-compressed and lost quality.
	// Note: this only restores color if it's a compressed format.
	BOOL			RestoreColorData(TextureData *pData);

public:

	// What format m_MipmapBackups are in.
	BPPIdent		m_BPPIdent;

	// How many mipmaps we have setup.
	DWORD			m_nMipmaps;

	// The mipmaps in PValue format.
	CMoDWordArray	m_PValueMipmaps[MAX_DTX_MIPMAPS];
	
	// Backups of the mipmap data.
	CMoByteArray	m_MipmapBackups[MAX_DTX_MIPMAPS];
};
	

TextureUtils::TextureUtils()
{
	m_BPPIdent = BPP_32;
	m_nMipmaps = 0;
}


BOOL TextureUtils::FromTextureData(TextureData *pData)
{
	DWORD i, y, size;
	TextureMipData *pMip;
	FormatMgr formatMgr;
	FMConvertRequest cRequest;
	DRESULT dResult;
	BYTE *pInLine, *pOutLine;


	if(pData->m_Header.m_nMipmaps > MAX_DTX_MIPMAPS)
		return FALSE;

	m_BPPIdent = pData->m_Header.GetBPPIdent();
	m_nMipmaps = pData->m_Header.m_nMipmaps;

	for(i=0; i < pData->m_Header.m_nMipmaps; i++)
	{
		pMip = &pData->m_Mips[i];

		// Setup the PValue version of the mipmap.
		if(!m_PValueMipmaps[i].SetSize(pMip->m_Width * pMip->m_Height))
			return FALSE;

		dtx_SetupDTXFormat(pData, cRequest.m_pSrcFormat);
		cRequest.m_pSrc = pMip->m_Data;
		cRequest.m_SrcPitch = pMip->m_Pitch;
		cRequest.m_pDestFormat->InitPValueFormat();
		cRequest.m_pDest = (BYTE*)m_PValueMipmaps[i].GetArray();
		cRequest.m_DestPitch = pMip->m_Width * sizeof(DWORD);
		cRequest.m_Width = pMip->m_Width;
		cRequest.m_Height = pMip->m_Height;
		cRequest.m_Flags = 0;
		dResult = formatMgr.ConvertPixels(&cRequest);
		if(dResult != LT_OK)
			return FALSE;

		// Backup the mipmaps.
		size = CalcImageSize(m_BPPIdent, pMip->m_Width, pMip->m_Height);
		if(!m_MipmapBackups[i].SetSize(size))
			return FALSE;
		
		if(m_BPPIdent == BPP_32)
		{
			pInLine = pMip->m_Data;
			pOutLine = (BYTE*)m_MipmapBackups[i].GetArray();
			for(y=0; y < pMip->m_Height; y++)
			{
				memcpy(pOutLine, pInLine, pMip->m_Width * sizeof(DWORD));
				pInLine += pMip->m_Pitch;
				pOutLine += pMip->m_Width * sizeof(DWORD);
			}
		}
		else
		{
			memcpy(m_MipmapBackups[i].GetArray(), pMip->m_Data, size);
		}
	}

	return TRUE;
}


BOOL TextureUtils::ToTextureData(TextureData *pData, BOOL bRestoreColorData)
{
	DWORD i, y, size;
	TextureMipData *pMip;
	BYTE *pInLine, *pOutLine;
	S3TC_Compressor cS3TC;
	DRESULT dResult;


	if(m_BPPIdent != pData->m_Header.GetBPPIdent() || 
		m_nMipmaps != pData->m_Header.m_nMipmaps)
	{
		return FALSE;
	}

	// Either just copy the data in or compress it.
	if(m_BPPIdent == BPP_32)
	{
		for(i=0; i < m_nMipmaps; i++)
		{
			pMip = &pData->m_Mips[i];

			pInLine = (BYTE*)m_PValueMipmaps[i].GetArray();
			pOutLine = pMip->m_Data;
			for(y=0; y < pMip->m_Height; y++)
			{
				memcpy(pOutLine, pInLine, pMip->m_Width * sizeof(DWORD));
				pInLine += pMip->m_Width * sizeof(DWORD);
				pOutLine += pMip->m_Pitch;
			}
		}
	}
	else
	{
		for(i=0; i < m_nMipmaps; i++)
		{
			pMip = &pData->m_Mips[i];

			cS3TC.m_Format = m_BPPIdent;
			cS3TC.m_pData = m_PValueMipmaps[i].GetArray();
			cS3TC.m_Width = pMip->m_Width;
			cS3TC.m_Height = pMip->m_Height;
			cS3TC.m_Pitch = pMip->m_Width * sizeof(DWORD);
			cS3TC.m_DataFormat.InitPValueFormat();

			dResult = cS3TC.CompressUsingLibrary();
			if(dResult != LT_OK)
				return FALSE;

			size = CalcImageSize(m_BPPIdent, pMip->m_Width, pMip->m_Height);
			if(size != cS3TC.m_OutDataSize)
				return FALSE;

			memcpy(pMip->m_Data, cS3TC.m_pOutData, size);
		}
	}
		
	return bRestoreColorData ? RestoreColorData(pData) : TRUE;
}


BOOL TextureUtils::RestoreColorData(TextureData *pData)
{
	DWORD iMipmap, nBlocks, iBlock, blockSize, colorBlockSize, alphaBlockSize;
	TextureMipData *pMip;
	BYTE *pSrcPos, *pDestPos;

	// No need....
	if(!IsBPPCompressed(m_BPPIdent))
		return TRUE;
	
	if(m_BPPIdent != pData->m_Header.GetBPPIdent() ||
		m_nMipmaps != pData->m_Header.m_nMipmaps)
	{
		return FALSE;
	}

	for(iMipmap=0; iMipmap < m_nMipmaps; iMipmap++)
	{
		pMip = &pData->m_Mips[iMipmap];

		nBlocks = (pMip->m_Width/4) * (pMip->m_Height/4);
		blockSize = GetBlockSize(m_BPPIdent);
		alphaBlockSize = GetAlphaBlockSize(m_BPPIdent);
		colorBlockSize = GetColorBlockSize(m_BPPIdent);

		pSrcPos = m_MipmapBackups[iMipmap].GetArray();
		pDestPos = pMip->m_Data;

		// Alpha block comes first.. skip past it.
		pSrcPos += alphaBlockSize;
		pDestPos += alphaBlockSize;

		for(iBlock=0; iBlock < nBlocks; iBlock++)
		{
			memcpy(pDestPos, pSrcPos, colorBlockSize);
			pSrcPos += blockSize;
			pDestPos += blockSize;
		}
	}

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CTextureDlg dialog


CTextureDlg::CTextureDlg()
{
	//{{AFX_DATA_INIT(CTextureDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_pIconList				= NULL;
	m_pIconList2			= NULL;
	m_hFont					= NULL;
	m_hCurrentItem			= NULL;
	m_hCurrentTextureDir	= 0;
	m_nCurrentTextureIndex	= -1;
	InitBaseRezDlg("dtx", &m_TextureTree, &m_TextureList, RESTYPE_TEXTURE);
}

CTextureDlg::~CTextureDlg()
{
	if (m_pIconList)
	{
		delete m_pIconList;
	}
	if (m_pIconList2)
	{
		delete m_pIconList2;
	}
}

void CTextureDlg::DoDataExchange(CDataExchange* pDX)
{
	CMRCSizeDialogBar::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTextureDlg)
	DDX_Control(pDX, IDC_BASEREZ_TREE, m_TextureTree);
	DDX_Control(pDX, IDC_BASEREZ_LIST, m_TextureList);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTextureDlg, CMRCSizeDialogBar)
	//{{AFX_MSG_MAP(CTextureDlg)
	ON_WM_CONTEXTMENU()
	ON_WM_SIZE()
	ON_NOTIFY( NM_CLICK, IDC_BASEREZ_LIST, OnSelTexture )
	ON_NOTIFY( NM_DBLCLK, IDC_BASEREZ_LIST, OnOpenTexture )
	ON_NOTIFY(LVN_KEYDOWN, IDC_BASEREZ_LIST, OnKeySelTexture)
	ON_NOTIFY(TVN_SELCHANGED, IDC_BASEREZ_TREE, OnSelchangedDirectory)
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_NOTIFY(TVN_ITEMEXPANDING, IDC_BASEREZ_TREE, OnItemexpandingBaserezTree)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextureDlg message handlers

BOOL CTextureDlg::OnInitDialogBar() 
{
	CMRCSizeDialogBar::OnInitDialogBar();
	
	// TODO: Add extra initialization here
	char szSysPath[MAX_PATH];
	char szIconDLL[MAX_PATH];

	GetSystemDirectory(szSysPath,MAX_PATH);
	sprintf(szIconDLL,"%s\\shell32.dll",szSysPath);

	if (m_pIconList)
	{
		delete m_pIconList;
	}
	if (m_pIconList2)
	{
		delete m_pIconList2;
	}
	m_pIconList=new CImageList;
	m_pIconList2=new CImageList;

	//tree control image list
	m_pIconList->Create(16,16,ILC_COLOR16,10,5);
	m_pIconList->SetBkColor(RGB(255,255,255));
	m_pIconList->Add(ExtractIcon(AfxGetInstanceHandle(),szIconDLL,3));	//closed folder
	m_pIconList->Add(ExtractIcon(AfxGetInstanceHandle(),szIconDLL,4));	//open folder
	m_TextureTree.SetImageList(m_pIconList,TVSIL_NORMAL);

	//list control image list
	m_pIconList2->Create(16,16,ILC_COLOR4,10,5);
	m_pIconList2->SetBkColor(RGB(255,255,255));
	m_pIconList2->Add(AfxGetApp()->LoadIcon(IDI_TEXTURE_TAB_ICON));
	m_TextureList.SetImageList(m_pIconList2,LVSIL_SMALL);
	
	CRect rect;
	m_TextureList.GetClientRect( &rect );

	m_TextureList.InsertColumn(0,"Name",LVCFMT_LEFT,(rect.Width()-35)/2,-1);
	m_TextureList.InsertColumn(1,"Size",LVCFMT_RIGHT,(rect.Width()-35)/2,-1);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTextureDlg::OnContextMenu(CWnd* pWnd, CPoint point) 
{
	// TODO: Add your message handler code here
	CMenu menu;
	
	if(!GetProjectBar()->IsProjectOpen())
		return;

	if(pWnd->m_hWnd == m_TextureTree.m_hWnd)
		VERIFY(menu.LoadMenu(CG_IDR_POPUP_WORLDTREE));
	else if(pWnd->m_hWnd == m_TextureList.m_hWnd)
		VERIFY(menu.LoadMenu(CG_IDR_POPUP_TEXTURELIST));
	else return;

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	CWnd* pWndPopupOwner = GetProjectBar();

	pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);	
}

void CTextureDlg::OnSize(UINT nType, int cx, int cy) 
{
	CMRCSizeDialogBar::OnSize(nType, cx, cy);
	
	// Reposition the controls
	RepositionControls();
}

/************************************************************************/
// This is called to reposition the controls
void CTextureDlg::RepositionControls()
{
	if( ::IsWindow(m_hWnd))
	{
		CRect rect;
		GetClientRect( &rect );

		// Move the tree
		CWnd *pWnd = GetDlgItem(IDC_BASEREZ_TREE);
		if (pWnd)
		{
			pWnd->MoveWindow( 0, 0, rect.Width(), rect.Height()/3 );
		}

		// Move the list
		pWnd = GetDlgItem(IDC_BASEREZ_LIST);
		if (pWnd)
		{
			pWnd->MoveWindow( 0, rect.Height()/3, rect.Width(), rect.Height()/3 );
		}
	}		
}

void CTextureDlg::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	RenderLargeImage();
}

void CTextureDlg::OnSelchangedDirectory(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	DDirIdent *pIdent;

	HTREEITEM hItem=pNMTreeView->itemNew.hItem;

	if(hItem)
	{
		pIdent = (DDirIdent*)m_TextureTree.GetItemData(hItem);
		if(pIdent)
		{
			m_csCurrentDir = pIdent->m_Filename;
			m_hCurrentItem = hItem;

			PopulateList();

			// Make sure only the current directory has the selected list item set
			if (m_hCurrentItem == m_hCurrentTextureDir)
				m_TextureList.SetItemState(m_nCurrentTextureIndex, LVIS_SELECTED, LVIS_SELECTED);
		}

		m_AllTextureDlg.NotifyDirChange();
	}

	*pResult = 0;
}

void CTextureDlg::OnSelTexture( NMHDR * pNMHDR, LRESULT * pResult )
{
	POINT point;
	UINT nFlags;

	GetCursorPos(&point);
	m_TextureList.ScreenToClient(&point);

	int nItem=m_TextureList.HitTest(point,&nFlags);

	if(nItem >= 0)
	{
		ChangeSelection(nItem, FALSE);
		RenderLargeImage();
	}

	*pResult = 0;
}

void CTextureDlg::OnOpenTexture( NMHDR * pNMHDR, LRESULT * pResult )
{
	POINT point;
	UINT nFlags;
	DFileIdent *pIdent;

	GetCursorPos(&point);
	m_TextureList.ScreenToClient(&point);

	int nItem=m_TextureList.HitTest(point,&nFlags);

	if(nItem >= 0)
	{
		pIdent = (DFileIdent*)m_TextureList.GetItemData(nItem);
		GetProjectBar()->SetCurTextureSel(pIdent);
		RenderLargeImage();

		if( IsWindow( GetProjectBar( )->m_SpriteEditDlg.m_hWnd ))
		{
			GetProjectBar( )->m_SpriteEditDlg.AddTexture( pIdent );
		}
		else
		{
			DoTextureProperties();
		}
	}

	*pResult = 0;
}

void CTextureDlg::OnKeySelTexture( NMHDR * pnkd, LRESULT * pResult )
{
	int nIndex = m_TextureList.GetNextItem(-1,LVNI_SELECTED);
	int nOldIndex = nIndex;
	*pResult = 0;
	
	LV_KEYDOWN *pPnkd = reinterpret_cast<LV_KEYDOWN FAR*>(pnkd);

	switch(pPnkd->wVKey)
	{
		// Up arrow
		case VK_UP:	
			nIndex--; break;
		// Down Arrow
		case VK_DOWN: 
			nIndex++; break;
		// Page up
		case VK_PRIOR: 
			nIndex -= m_TextureList.GetCountPerPage(); break;
		// Page down
		case VK_NEXT: 
			nIndex += m_TextureList.GetCountPerPage(); break;
		// Anything else
		default:	return;
	}

	// Keep the index inside the bounds of the list
	if (nIndex < 0)
		nIndex = 0;
	else if (nIndex >= m_TextureList.GetItemCount())
		nIndex = m_TextureList.GetItemCount() - 1;

	// Re-display the texture if it's changed
	if (nOldIndex != nIndex)
		ChangeSelection(nIndex);

	return;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::ChangeSelection
//
//	PURPOSE:
//
// ----------------------------------------------------------------------- //

void CTextureDlg::ChangeSelection(int nIndex, BOOL bClearSelect)
{
	CMainFrame* pFrame = (CMainFrame*)AfxGetMainWnd();
	pFrame->UpdateStatusText(ID_INDICATOR_TEXTURE,((DFileIdent*)m_TextureList.GetItemData(nIndex))->m_Filename);

	//make sure that no other textures are selected
	if (bClearSelect)
	{
		for (int iLoop = 0; iLoop < m_TextureList.GetItemCount(); iLoop++)
			m_TextureList.SetItemState(iLoop, 0, LVIS_SELECTED);
	}

	//set up information about the texture
	m_hCurrentTextureDir = m_hCurrentItem;
	m_nCurrentTextureIndex = nIndex;
	m_TextureList.SetItemState(nIndex, LVIS_SELECTED, LVIS_SELECTED);
	m_TextureList.SetSelectionMark(nIndex);
	m_TextureList.EnsureVisible(nIndex, FALSE);

	GetProjectBar()->SetCurTextureSel((DFileIdent*)m_TextureList.GetItemData(nIndex));
	RenderLargeImage();	
}

// 2001's (0x409427): the bottom third of the dialog cleared, the texture
// centred in it with a white pen selected, and its name under it.
// NEAR (661 of 696 bytes): the shipped build keeps 0 in ebx and expands every
// CGdiObject destructor, with one failure block of its own (0x409503) that
// does not merge with the final ReleaseDC; ours spends the inline budget on
// CRgn's constructor, keeps CPen's vtable in ebx and sends the failures to
// the final ReleaseDC. Tried: Build 69's two separate failure returns (732
// bytes: the two blocks don't merge either), a CRect copy for the name's
// rectangle (worse).
void CTextureDlg::RenderLargeImage()
{
	CPen Pen;

	//get a DC
	CDC *pDC = GetDC();
	if(!pDC)
		return;

	CRect DrawRect;
	GetClientRect( &DrawRect );
	//bring the height down so we are dealing with just the bottom 1/3 of the dlg
	DrawRect.top = DrawRect.bottom / 3 * 2;

	//make a region to clear that covers the rect
	CRgn Rgn;
	Rgn.CreateRectRgn(DrawRect.left,DrawRect.top,DrawRect.right,DrawRect.bottom);

	//clear the region
	CBrush Brush(RGB(0,0,0));
	pDC->FillRgn(&Rgn,&Brush);

	DFileIdent	*pRez = NULL;

	if(GetProjectBar()->IsProjectOpen())
		pRez = GetProjectBar()->m_pCurTextureSel;

	CTexture *pTexture;
	if(pRez == NULL || (pTexture = dib_GetDibTexture(pRez)) == NULL)
	{
		ReleaseDC(pDC);
		return;
	}

	CDib *pDib = pTexture->m_pDib;
	CRect texRect(0,0,pDib->GetWidth(),pDib->GetHeight());
	texRect.right >>= pTexture->m_UIMipmapOffset;
	texRect.bottom >>= pTexture->m_UIMipmapOffset;

	//adjust the origin of the texture
	int nTmp = (DrawRect.top + (DrawRect.bottom/6)) - (texRect.bottom/2);
	if(nTmp > DrawRect.top)
		DrawRect.top = nTmp;

	DrawRect.left = (DrawRect.right/2) - (texRect.right/2);
	
	nTmp = DrawRect.top + texRect.bottom;
	if(nTmp < DrawRect.bottom)
		DrawRect.bottom = nTmp;

	DrawRect.right = DrawRect.left + texRect.right;

	// Blit the texture in.
	Pen.CreatePen(PS_SOLID, 0, RGB(255,255,255));
	CPen *pOldPen = pDC->SelectObject(&Pen);

	SetStretchBltMode( pDC->m_hDC, COLORONCOLOR );
	pDib->Blt( pDC->m_hDC, DrawRect.left, DrawRect.top, DrawRect.Width()+1, DrawRect.Height()+1 );

	pDC->SelectObject(pOldPen);
	Pen.DeleteObject();

	// Draw the texture name.
	if( m_hFont )
	{
		CRect textRect(DrawRect.left, DrawRect.bottom, DrawRect.right, DrawRect.bottom + 20);
		char pszToPrint[MAX_PATH];
		CHelpers::ExtractNames( pRez->m_Filename, NULL, NULL, pszToPrint, NULL );

		::SetBkMode( pDC->m_hDC, TRANSPARENT );
		::SetTextColor( pDC->m_hDC, RGB(0,0,0) );

		HFONT hOldFont = (HFONT)::SelectObject( pDC->m_hDC, m_hFont );
		DrawText( pDC->m_hDC, pszToPrint, strlen(pszToPrint), &textRect, DT_SINGLELINE | DT_VCENTER );
		::SelectObject( pDC->m_hDC, hOldFont );
	}

	ReleaseDC(pDC);
}

void CTextureDlg::SetListItemText(int nItem, CString &relativeFilename)
{
	DFileIdent *pIdent;
	char szTemp[MAX_PATH];
	DWORD width, height;

	// insert the texture dimensions
	pIdent = (DFileIdent*)m_TextureList.GetItemData(nItem);
	if(pIdent)
	{
		GetTextureDimensions(pIdent, &width, &height);

		sprintf(szTemp, "%d x %d", width, height);
		m_TextureList.SetItemText(nItem, 1, szTemp);
	}
}



// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::DoImportOperation
//
//	PURPOSE:
//
// ----------------------------------------------------------------------- //

BOOL CTextureDlg::DoImportOperation(DWORD flags)
{
	CString			importExt, fileMask, newExtension, sFileName;
	CString			pathName, targetFilename, tempStr, relativeTargetFilename;

	POSITION		pos;
	char			fileTitle[256];
	char			fileName[256], fileExt[256];
	
	int				status, item;
	CMoFileIO		file;
	CStringDlg stringDlg;
	DWORD textureFlags;

	
	// If no directory is selected, then we can't import anything.
	if(!IsDirectorySelected())
		return FALSE;


	if(flags & PCX_TO_TEXTURE || flags & TGA_TO_TEXTURE)
	{
		if(flags & PCX_TO_TEXTURE)
		{
			importExt.LoadString( IDS_PCX_EXTENSION );
			fileMask.LoadString( IDS_PCX_FILEMASK );
		}
		else
		{
			importExt.LoadString( IDS_TGA_EXTENSION );
			fileMask.LoadString( IDS_TGA_FILEMASK );
		}

		textureFlags = 0;
	}
	else
	{
		importExt.LoadString( IDS_TEXTURE_EXTENSION );
		fileMask.LoadString( IDS_TEXTURE_FILEMASK );
	}
	
	newExtension.LoadString( IDS_TEXTURE_EXTENSION );
	sFileName = GetProject()->m_BaseProjectDir + "\\Textures\\*" + importExt;

	CHelperFileDlg	dlg( TRUE, importExt, (LPCTSTR)sFileName, OFN_ALLOWMULTISELECT, fileMask, this );
	if( dlg.DoModal() == IDOK )
	{
		BeginWaitCursor( );

		// Note: pPos is NOT used as the filename here!!!
		for( pos=dlg.GetStartPosition(); pos != NULL; )
		{		
			pathName = dlg.GetNextPathName( pos );
			
			GetFileTitle( (LPCTSTR)pathName, fileTitle, 256 );
			CHelpers::ExtractFileNameAndExtension( fileTitle, fileName, fileExt );
		
			// Does the file already exist?
			relativeTargetFilename = dfm_BuildName(m_csCurrentDir, CString(fileName) + newExtension);
			targetFilename = dfm_GetFullFilename(GetFileMgr(), relativeTargetFilename);
			
			if( file.Open(targetFilename, "rb") )
			{
				file.Close();

				tempStr.FormatMessage( IDS_TEXTURE_ALREADY_EXISTS, fileName );
				status = MessageBox( tempStr, AfxGetAppName(), MB_YESNOCANCEL );
			
				if( status == IDYES )
				{
					// Copy the file.
					if( flags & PCX_TO_TEXTURE )
					{
						CopyImageToTexture( ITYPE_PCX, pathName, targetFilename, textureFlags );
					}
					else if(flags & TGA_TO_TEXTURE)
					{
						CopyImageToTexture( ITYPE_TGA, pathName, targetFilename, textureFlags );
					}
					else
					{
						CopyFile( pathName, targetFilename, FALSE );
					}

					// Update the listbox and select the new texture.
					if( GetProjectBar()->m_pCurTextureSel )
						SelectFileInList( GetProjectBar()->m_pCurTextureSel, FALSE );
					item = AddFileToList(relativeTargetFilename);
					GetProjectBar()->SetCurTextureSel((DFileIdent*)m_TextureList.GetItemData(item));
					SelectFileInList( GetProjectBar()->m_pCurTextureSel, TRUE );
				}
				else if( status == IDCANCEL )
				{
					break;
				}
			}
			else
			{
				// Copy the file.
				if( flags & PCX_TO_TEXTURE )
				{
					CopyImageToTexture( ITYPE_PCX, pathName, targetFilename, textureFlags );
				}
				else if(flags & TGA_TO_TEXTURE)
				{
					CopyImageToTexture( ITYPE_TGA, pathName, targetFilename, textureFlags );
				}
				else
				{
					CopyFile( pathName, targetFilename, FALSE );
				}

				// Update the listbox and select the new texture.
				if( GetProjectBar()->m_pCurTextureSel )
					SelectFileInList( GetProjectBar()->m_pCurTextureSel, FALSE );
				item = AddFileToList(relativeTargetFilename);
				GetProjectBar()->SetCurTextureSel((DFileIdent*)m_TextureList.GetItemData(item));
				SelectFileInList( GetProjectBar()->m_pCurTextureSel, TRUE );
			}
		}

		EndWaitCursor( );
		return TRUE;
	}

	return FALSE;
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::DoImportMipPcxOperation
//
//	PURPOSE:
//
// ----------------------------------------------------------------------- //

BOOL CTextureDlg::DoImportMipPcxOperation( )
{
	static const int knMaxMipMaps = 4;

	CString			importExt, fileMask, newExtension, newFileMask, sFileName;
	CString			pathName[knMaxMipMaps], targetFilename, targetName, tempStr, relativeTargetFilename;

	POSITION		pos;
		
	int				status;
	int				nFileCount;

	char			fileTitle[knMaxMipMaps][_MAX_PATH+1];
	char			fileName[knMaxMipMaps][_MAX_FNAME+1];
	char			fileExt[knMaxMipMaps][_MAX_EXT+1];


	importExt.LoadString( IDS_TGA_EXTENSION );
	fileMask.LoadString( IDS_TGA_FILEMASK );
	
	newExtension.LoadString( IDS_TEXTURE_EXTENSION );
	newFileMask.LoadString( IDS_TEXTURE_FILEMASK );

	// Get the mip tga files...
	nFileCount = 0;
	while( nFileCount < knMaxMipMaps )
	{
		tempStr.FormatMessage( IDS_OPENMIPFILE, nFileCount + 1 );
		sFileName = GetProject()->m_BaseProjectDir + "\\Textures\\*" + importExt;
		CHelperFileDlg dlg( TRUE, importExt, (LPCTSTR)sFileName, OFN_ALLOWMULTISELECT, fileMask, this );
		dlg.m_ofn.lpstrTitle = LPCTSTR( tempStr );

		if( dlg.DoModal() == IDOK )
		{
			// Note: pPos is NOT used as the filename here!!!
			pos = dlg.GetStartPosition();
			while( pos != NULL && nFileCount < knMaxMipMaps )
			{
				pathName[nFileCount] = dlg.GetNextPathName( pos );
				GetFileTitle( (LPCTSTR)pathName[nFileCount], fileTitle[nFileCount], _MAX_PATH );
				CHelpers::ExtractFileNameAndExtension( fileTitle[nFileCount], fileName[nFileCount], fileExt[nFileCount] );

				nFileCount++;
			}
		}
		else
		{
			return FALSE;
		}
	}

	// Get the dtx file outputname...
	targetName = CString(fileName[0]) + newExtension;
	targetFilename = dfm_BuildName(m_csCurrentDir, targetName);
	
	CHelperFileDlg dlgSave( FALSE, newExtension, LPCTSTR( targetName ), OFN_OVERWRITEPROMPT, newFileMask, this );
	tempStr.LoadString( IDS_SAVEDTXFILE );
	dlgSave.m_ofn.lpstrTitle = LPCTSTR( tempStr );
	dlgSave.m_ofn.lpstrInitialDir = GetCurDirPath( );
	if( dlgSave.DoModal( ) == IDOK )
	{
		targetFilename = dlgSave.GetPathName( );
	}
	else
	{
		return FALSE;
	}

	BeginWaitCursor( );

	// Copy the file.
	if( !CopyMipTGAToTexture( pathName, targetFilename ))
	{
		EndWaitCursor( );
		return FALSE;
	}

	//AddFileToList(relativeTargetFilename);
	PopulateList(); // {MD 4/14/98} Do this instead because the file may not be in resource dirs.

	EndWaitCursor( );
	return TRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::DoExportAllAsBPP_32POperation
//
//	PURPOSE:
//
// ----------------------------------------------------------------------- //

//! todo
BOOL CTextureDlg::DoExportAllAsBPP_32POperation( )
{
	CMoFileIO	outFile;
	int			palNumber, temp;
	WORD		wWidth, wHeight;
	DStream *pStream;

	CString pcxExt, fileMask, pcxFilename, pcxName, str;
	char dtxName[_MAX_FNAME], dtxTitle[_MAX_FNAME], dtxExt[_MAX_EXT];
	CString dtxFilename;
	DFileIdent *pRez;
	CMoArray<DFileIdent*> files;
	int iItem;
	
	pcxExt = ".dt8";

	//! get all the filenames
	for(int iIndex = 0; iIndex < m_TextureList.GetItemCount(); iIndex++)
	{
		// OutputDebugString("\n"); OutputDebugString(pIdent->m_Filename);
		files.Append((DFileIdent*) m_TextureList.GetItemData(iIndex));
	}

	// create a subdirectory under Textures\..\palettized
	// convert into BPP_32P textures
	for(int i=0; i < files; i++)
	{
		dtxFilename = dfm_GetFullFilename(GetFileMgr(), files[i]->m_Filename);
		if(dtxFilename.GetLength() > 4)
		{
			pcxFilename = dtxFilename.Left(dtxFilename.GetLength()-4) + pcxExt;

			if(!outFile.Open(pcxFilename, "wb"))
			{
				str.FormatMessage( IDS_ERRORSAVEPCX, pcxFilename );
				AppMessageBox( str, MB_OK );
				continue;
			}

			if(!(pStream = streamsim_Open((LPCTSTR)dtxFilename, "rb")))
			{
				outFile.Close();
				str.LoadString( IDS_ERR_OPENFILE );
				AppMessageBox( str, MB_OK );
				continue;
			}

			if(!SaveDtxAs8Bit(pStream, outFile, &pcxFilename))
			{
				pStream->Release();
				EndWaitCursor( );
				str.FormatMessage( IDS_ERRORSAVEDTX, pcxFilename );
				AppMessageBox( str, MB_OK );
				continue;
			}

			pStream->Release();
		}
	}

	return TRUE;
}



// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::DoExport8BitDTXOperation
//
//	PURPOSE:
//
// ----------------------------------------------------------------------- //

BOOL CTextureDlg::DoExport8BitDTXOperation( )
{
	CMoFileIO	outFile;
	int			palNumber, temp;
	WORD		wWidth, wHeight;
	DStream *pStream;

	CString pcxExt, fileMask, pcxFilename, pcxName, str;
	char dtxName[_MAX_FNAME], dtxTitle[_MAX_FNAME], dtxExt[_MAX_EXT];
	CString dtxFilename;
	DFileIdent *pRez;
	CMoArray<DFileIdent*> files;
	int iItem;
	DWORD i;
	
	if(!GetProjectBar()->IsProjectOpen())
		return false;

	iItem = -1;
	while((iItem = m_TextureList.GetNextItem(iItem, LVNI_ALL|LVNI_SELECTED)) != -1)
	{
		files.Append((DFileIdent*)m_TextureList.GetItemData(iItem));
	}

	// pcxExt.LoadString( IDS_TEXTURE_EXTENSION );
	// fileMask.LoadString( IDS_TEXTURE_FILEMASK );

	pcxExt = ".dtx";
	fileMask = "DTX Files (*.dtx)|*.dtx||";

	if(files > 1)
	{
		if(AppMessageBox(IDS_MULTIPLETEXTURESSELECTED, MB_YESNO) == IDNO)
			return FALSE;

		BeginWaitCursor( );
		
		for(i=0; i < files; i++)
		{
			dtxFilename = dfm_GetFullFilename(GetFileMgr(), files[i]->m_Filename);
			if(dtxFilename.GetLength() > 4)
			{
				pcxFilename = dtxFilename.Left(dtxFilename.GetLength()-4) + pcxExt;

				if(!outFile.Open(pcxFilename, "wb"))
				{
					str.FormatMessage( IDS_ERRORSAVEPCX, pcxFilename );
					AppMessageBox( str, MB_OK );
					continue;
				}

				if(!(pStream = streamsim_Open((LPCTSTR)dtxFilename, "rb")))
				{
					outFile.Close();
					str.LoadString( IDS_ERR_OPENFILE );
					AppMessageBox( str, MB_OK );
					continue;
				}

				if(!SaveDtxAs8Bit(pStream, outFile, &pcxFilename))
				{
					pStream->Release();
					EndWaitCursor( );
					str.FormatMessage( IDS_ERRORSAVEDTX, pcxFilename );
					AppMessageBox( str, MB_OK );
					continue;
				}

				pStream->Release();
			}
		}

		EndWaitCursor( );
		return TRUE;
	}
	else
	{
		pRez = GetProjectBar()->m_pCurTextureSel;
		if(!pRez)
			return FALSE;

		dtxFilename = dfm_GetFullFilename(GetFileMgr(), pRez->m_Filename);

		GetFileTitle( (LPCTSTR)dtxFilename, dtxTitle, _MAX_PATH );
		CHelpers::ExtractFileNameAndExtension( dtxTitle, dtxName, dtxExt );

		pcxFilename = dfm_BuildName(m_csCurrentDir, CString(dtxName) + pcxExt);
		pcxName = dtxName + pcxExt;
		CHelperFileDlg dlgSave( FALSE, pcxExt, LPCTSTR( pcxName ), OFN_OVERWRITEPROMPT, fileMask, this );
		dlgSave.m_ofn.lpstrInitialDir = GetCurDirPath( );
		if( dlgSave.DoModal( ) == IDOK )
		{
			pcxFilename = dlgSave.GetPathName( );
		}
		else
		{
			return FALSE;
		}

		if(!outFile.Open(pcxFilename, "wb"))
		{
			str.FormatMessage( IDS_ERRORSAVEDTX, pcxFilename );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}

		if(!(pStream = streamsim_Open((LPCTSTR)dtxFilename, "rb")))
		{
			outFile.Close();
			str.LoadString( IDS_ERR_OPENFILE );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}

		BeginWaitCursor( );

		if(!SaveDtxAs8Bit(pStream, outFile, &pcxFilename ))
		{
			pStream->Release();
			EndWaitCursor( );
			str.FormatMessage( IDS_ERRORSAVEDTX, pcxFilename );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}

		pStream->Release();
		EndWaitCursor( );
		return TRUE;
	}
}



// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::DoExportPcxOperation
//
//	PURPOSE:
//
// ----------------------------------------------------------------------- //

BOOL CTextureDlg::DoExportPcxOperation( )
{
	CMoFileIO	outFile;
	int			palNumber, temp;
	WORD		wWidth, wHeight;
	DStream *pStream;

	CString pcxExt, fileMask, pcxFilename, pcxName, str;
	char dtxName[_MAX_FNAME], dtxTitle[_MAX_FNAME], dtxExt[_MAX_EXT];
	CString dtxFilename;
	DFileIdent *pRez;
	CMoArray<DFileIdent*> files;
	int iItem;
	DWORD i;

	
	if(!GetProjectBar()->IsProjectOpen())
		return FALSE;

	iItem = -1;
	while((iItem = m_TextureList.GetNextItem(iItem, LVNI_ALL|LVNI_SELECTED)) != -1)
	{
		files.Append((DFileIdent*)m_TextureList.GetItemData(iItem));
	}

	pcxExt.LoadString( IDS_TGA_EXTENSION );
	fileMask.LoadString( IDS_TGA_FILEMASK );

	if(files > 1)
	{
		if(AppMessageBox(IDS_MULTIPLETEXTURESSELECTED, MB_YESNO) == IDNO)
			return FALSE;

		BeginWaitCursor( );
		
		for(i=0; i < files; i++)
		{
			dtxFilename = dfm_GetFullFilename(GetFileMgr(), files[i]->m_Filename);
			if(dtxFilename.GetLength() > 4)
			{
				pcxFilename = dtxFilename.Left(dtxFilename.GetLength()-4) + pcxExt;

				if(!outFile.Open(pcxFilename, "wb"))
				{
					str.FormatMessage( IDS_ERRORSAVEPCX, pcxFilename );
					AppMessageBox( str, MB_OK );
					continue;
				}

				if(!(pStream = streamsim_Open((LPCTSTR)dtxFilename, "rb")))
				{
					outFile.Close();
					str.LoadString( IDS_ERR_OPENFILE );
					AppMessageBox( str, MB_OK );
					continue;
				}

				if(!SaveDtxAsTga(pStream, outFile))
				{
					pStream->Release();
					EndWaitCursor( );
					str.FormatMessage( IDS_ERRORSAVEPCX, pcxFilename );
					AppMessageBox( str, MB_OK );
					continue;
				}

				pStream->Release();
			}
		}

		EndWaitCursor( );
		return TRUE;
	}
	else
	{
		pRez = GetProjectBar()->m_pCurTextureSel;
		if(!pRez)
			return FALSE;

		dtxFilename = dfm_GetFullFilename(GetFileMgr(), pRez->m_Filename);

		GetFileTitle( (LPCTSTR)dtxFilename, dtxTitle, _MAX_PATH );
		CHelpers::ExtractFileNameAndExtension( dtxTitle, dtxName, dtxExt );

		pcxFilename = dfm_BuildName(m_csCurrentDir, CString(dtxName) + pcxExt);
		pcxName = dtxName + pcxExt;
		CHelperFileDlg dlgSave( FALSE, pcxExt, LPCTSTR( pcxName ), OFN_OVERWRITEPROMPT, fileMask, this );
		dlgSave.m_ofn.lpstrInitialDir = GetCurDirPath( );
		if( dlgSave.DoModal( ) == IDOK )
		{
			pcxFilename = dlgSave.GetPathName( );
		}
		else
		{
			return FALSE;
		}

		if(!outFile.Open(pcxFilename, "wb"))
		{
			str.FormatMessage( IDS_ERRORSAVEPCX, pcxFilename );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}

		if(!(pStream = streamsim_Open((LPCTSTR)dtxFilename, "rb")))
		{
			outFile.Close();
			str.LoadString( IDS_ERR_OPENFILE );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}

		BeginWaitCursor( );

		if(!SaveDtxAsTga(pStream, outFile))
		{
			pStream->Release();
			EndWaitCursor( );
			str.FormatMessage( IDS_ERRORSAVEPCX, pcxFilename );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}

		pStream->Release();
		EndWaitCursor( );
		return TRUE;
	}
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::DoTextureProperties
//
//	PURPOSE:
//
// ----------------------------------------------------------------------- //

// 2001's (0x40ad24): one function, the selection gathered here, and the
// texture's LIGHTDEFS section edited along with the header.
BOOL CTextureDlg::DoTextureProperties( )
{
	DFileIdent *pRez;
	CString fullName, str;
	DStream *pStream;
	TextureData *pTexture;
	DRESULT dResult;
	DtxSection *pSection;
	DWORD i;
	CMoArray<DFileIdent*> selections;
	CTextureProp dlgProp;


	if(!GetProjectBar()->IsProjectOpen())
		return FALSE;

	// Get the selections into an array.
	int nIndex = -1;
	while((nIndex = m_TextureList.GetNextItem(nIndex, LVNI_SELECTED)) != -1)
	{
		pRez = (DFileIdent*)m_TextureList.GetItemData(nIndex);
		if(pRez)
			selections.Append(pRez);
	}

	if(selections.GetSize() == 0)
		return FALSE;

	// Setup the dialog from the properties in the first texture.
	BeginWaitCursor();

		fullName = dfm_GetFullFilename(GetFileMgr(), selections[0]->m_Filename);
		pStream = streamsim_Open((LPCTSTR)fullName, "rb");
		if(!pStream)
		{
			AppMessageBox( IDS_ERR_OPENFILE, MB_OK );
			return FALSE;
		}

		dResult = dtx_Create(pStream, &pTexture, TRUE);
		pStream->Release();

		if(dResult != DE_OK)
		{
			str.FormatMessage(IDS_ERRORLOADINGDTX, fullName);
			AppMessageBox(str, MB_OK);
			return FALSE;
		}

		pSection = dtx_FindSection(pTexture, "LIGHTDEFS");
		dlgProp.m_LightDefs = pSection ? pSection->m_Data : "";

		if(pTexture->m_Header.m_Extra[1] == 0)
			dlgProp.m_nMipmaps = 4;
		else
			dlgProp.m_nMipmaps = pTexture->m_Header.m_Extra[1];

		if(pTexture->m_Header.m_Extra[2] & 0x80)
			dlgProp.m_AlphaCutoff = pTexture->m_Header.m_Extra[2] & ~0x80;
		else
			dlgProp.m_AlphaCutoff = 8;

		dlgProp.m_AverageAlpha = pTexture->m_Header.m_Extra[3];

		dlgProp.m_bFullBrights = !!(pTexture->m_Header.m_IFlags&DTX_FULLBRITE);
		dlgProp.m_bPrefer4444 = !!(pTexture->m_Header.m_IFlags & DTX_PREFER4444);
		dlgProp.m_bPrefer5551 = !!(pTexture->m_Header.m_IFlags & DTX_PREFER5551);
		dlgProp.m_bPrefer16Bit = !!(pTexture->m_Header.m_IFlags & DTX_PREFER16BIT);
		dlgProp.m_TextureFlags = pTexture->m_Header.m_UserFlags;
		dlgProp.m_TextureGroup = pTexture->m_Header.m_Extra[0];
		dlgProp.m_BPPIdent = pTexture->m_Header.GetBPPIdent();
		dlgProp.m_NonS3TCMipmapOffset = pTexture->m_Header.GetNonS3TCMipmapOffset();
		dlgProp.m_UIMipmapOffset = pTexture->m_Header.GetUIMipmapOffset();
		SAFE_STRCPY(dlgProp.m_CommandString, pTexture->m_Header.m_CommandString);
		dlgProp.m_TexturePriority = pTexture->m_Header.GetTexturePriority();
		dlgProp.m_DetailTextureScale = pTexture->m_Header.GetDetailTextureScale();
		dlgProp.m_DetailTextureAngle = pTexture->m_Header.GetDetailTextureAngle();

		dlgProp.m_pPreviewTexture = pTexture;

	EndWaitCursor( );
	

	DO_DIALOG_AGAIN:;

	if(dlgProp.DoModal() == IDOK)
	{
		for(i=0; i < selections; i++)
		{
			fullName = dfm_GetFullFilename(GetFileMgr(), selections[i]->m_Filename);
			pStream = streamsim_Open((LPCTSTR)fullName, "rb");
			if(!pStream)
			{
				str.FormatMessage(IDS_ERRORLOADINGDTX, fullName);
				AppMessageBox(str, MB_OK);
				continue;
			}

			dResult = dtx_Create(pStream, &pTexture, TRUE);
			pStream->Release();

			if(dResult != DE_OK)
			{
				str.FormatMessage(IDS_ERRORLOADINGDTX, fullName);
				AppMessageBox(str, MB_OK);
				continue;
			}

			// Replace the light definitions.
			if(dlgProp.m_ChangeFlags & TPROP_LIGHTDEFS)
			{
				pSection = dtx_FindSection(pTexture, "LIGHTDEFS");
				if(pSection)
					dtx_RemoveSection(pTexture, pSection);

				const char *pLightDefs = dlgProp.m_LightDefs;
				if(strlen(pLightDefs) > 0)
				{
					pSection = (DtxSection*)dalloc(sizeof(DtxSection) + strlen(pLightDefs));
					strcpy(pSection->m_Header.m_Type, "LIGHTDEFS");
					strcpy(pSection->m_Header.m_Name, "");
					pSection->m_Header.m_DataLen = strlen(pLightDefs) + 1;
					strcpy(pSection->m_Data, pLightDefs);

					pSection->m_pNext = pTexture->m_pSections;
					pTexture->m_pSections = pSection;
				}
			}

			// Replace flags.
			if(dlgProp.m_ChangeFlags & TPROP_FLAGS)
			{
				pTexture->m_Header.m_UserFlags = dlgProp.m_TextureFlags;
			}
			
			if(dlgProp.m_ChangeFlags & TPROP_GROUP)
			{
				pTexture->m_Header.m_Extra[0] = (BYTE)dlgProp.m_TextureGroup;
			}

			if(dlgProp.m_ChangeFlags & TPROP_NUMMIPMAPS)
			{
				pTexture->m_Header.m_Extra[1] = (BYTE)dlgProp.m_nMipmaps;
				pTexture->m_Header.m_Extra[1] = (pTexture->m_Header.m_Extra[1] > 4) ? 4 : pTexture->m_Header.m_Extra[1];
			}

			if(dlgProp.m_ChangeFlags & TPROP_FULLBRITES)
			{
				pTexture->m_Header.m_IFlags &= ~DTX_FULLBRITE;
				if(dlgProp.m_bFullBrights)
					pTexture->m_Header.m_IFlags |= DTX_FULLBRITE;
			}

			if(dlgProp.m_ChangeFlags & TPROP_PREFER4444)
			{
				pTexture->m_Header.m_IFlags &= ~DTX_PREFER4444;
				if(dlgProp.m_bPrefer4444)
					pTexture->m_Header.m_IFlags |= DTX_PREFER4444;
			}

			if(dlgProp.m_ChangeFlags & TPROP_PREFER5551)
			{
				pTexture->m_Header.m_IFlags &= ~DTX_PREFER5551;
				if(dlgProp.m_bPrefer5551)
					pTexture->m_Header.m_IFlags |= DTX_PREFER5551;
			}

			if(dlgProp.m_ChangeFlags & TPROP_PREFER16BIT)
			{
				pTexture->m_Header.m_IFlags &= ~DTX_PREFER16BIT;
				if(dlgProp.m_bPrefer16Bit)
					pTexture->m_Header.m_IFlags |= DTX_PREFER16BIT;
			}

			if(dlgProp.m_ChangeFlags & TPROP_COMMANDSTRING)
			{
				SAFE_STRCPY(pTexture->m_Header.m_CommandString, dlgProp.m_CommandString);
			}

			if(dlgProp.m_ChangeFlags & TPROP_DATAFORMAT)
			{
				if(pTexture->m_Header.GetBPPIdent() != dlgProp.m_BPPIdent)
				{
					if(!ConvertTextureData(pTexture, dlgProp.m_BPPIdent))
					{
						dtx_Destroy(pTexture);
						str.FormatMessage(IDS_ERRORCONVERTINGTEXTURE, fullName);
						AppMessageBox(str, MB_OK);
						if(selections.GetSize() == 1)
							goto DO_DIALOG_AGAIN;
						else
							continue;
					}
				}
			}

			if(dlgProp.m_ChangeFlags & TPROP_NONS3TCMIPMAPOFFSET)
			{
				pTexture->m_Header.SetNonS3TCMipmapOffset(dlgProp.m_NonS3TCMipmapOffset);
			}

			if(dlgProp.m_ChangeFlags & TPROP_UIMIPMAPOFFSET)
			{
				pTexture->m_Header.SetUIMipmapOffset(dlgProp.m_UIMipmapOffset);
			}

			if(dlgProp.m_ChangeFlags & TPROP_TEXTUREPRIORITY)
			{
				pTexture->m_Header.SetTexturePriority(dlgProp.m_TexturePriority);
			}

			if(dlgProp.m_ChangeFlags & TPROP_DETAILTEXTURESCALE)
			{
				pTexture->m_Header.SetDetailTextureScale(dlgProp.m_DetailTextureScale);
			}

			if(dlgProp.m_ChangeFlags & TPROP_DETAILTEXTUREANGLE)
			{
				pTexture->m_Header.SetDetailTextureAngle(dlgProp.m_DetailTextureAngle);
			}

			// Save it out.
			pStream = streamsim_Open((LPCTSTR)fullName, "wb");
			if(!pStream)
			{
				dtx_Destroy(pTexture);
				str.FormatMessage(IDS_ERRORSAVEDTX, fullName);
				AppMessageBox(str, MB_OK);
				if(selections.GetSize() == 1)
					goto DO_DIALOG_AGAIN;
				else
					continue;
			}

			dtx_Save(pTexture, pStream);
			pStream->Release();
			dtx_Destroy(pTexture);
		}
	}

	// Free the textures so changes take effect.
	dib_FreeAllTextures();
		
	return TRUE;
}


BOOL CTextureDlg::DoCreateAlphaMask(CAMType camType)
{
	DFileIdent *pRez;
	CString fullName;
	int tempInt;
	DStream *pStream;
	TextureData *pTexture;
	DRESULT dResult;
	CString importExt, fileMask, str;
	LoadedBitmap pcxTexture;
	DWORD i, curWidth, curHeight, x, y;
	WORD wVal, iColor;
	TextureMipData *pMip;
	RPaletteColor *pColors;
	AlphaFromColorDlg afcDlg;
	float alphaScale, fVal;
	int alphaOffset, sum, theValue;
	DWORD aVal, rVal, gVal, bVal;
	BOOL bLoaded;
	BYTE *pCurLine, *pPos, fillValue;
	TextureUtils tUtils;


	// Load the DTX file.
	pRez = GetProjectBar()->m_pCurTextureSel;
	if(!pRez)
		return FALSE;

	// 2001: the solid alpha is the alpha-from-color dialog preset to an
	// offset of 255 and no scale (0x40b439); no SolidAlphaDlg.
	if(camType == CAM_Solid)
	{
		afcDlg.m_Offset = "255";
		afcDlg.m_Scale = "0";
	}

	if(camType == CAM_Solid || camType == CAM_FromColor)
	{
		if(afcDlg.DoModal() != IDOK)
			return FALSE;
	}

	fullName = dfm_GetFullFilename(GetFileMgr(), pRez->m_Filename);
	pStream = streamsim_Open((LPCTSTR)fullName, "rb");
	if(!pStream)
	{
		AppMessageBox( IDS_ERR_OPENFILE, MB_OK );
		return FALSE;
	}

	dResult = dtx_Create(pStream, &pTexture, TRUE);
	pStream->Release();

	if(dResult != DE_OK)
	{
		str.FormatMessage(IDS_ERRORLOADINGDTX, fullName);
		AppMessageBox(str, MB_OK);
		return FALSE;
	}


	// Convert to RGBA..
	if(!tUtils.FromTextureData(pTexture))
	{
		str.FormatMessage(IDS_ERRORLOADINGDTX, fullName);
		AppMessageBox(str, MB_OK);
		return FALSE;
	}

	alphaScale = (float)atoi(afcDlg.m_Scale) / 100.0f;
	alphaOffset = atoi(afcDlg.m_Offset);


	// Load the PCX file.

	if(camType == CAM_Solid)
	{
		// Setup solid alpha.
		theValue = (int)(255.0f * alphaScale) + alphaOffset;
		theValue = DCLAMP(theValue, 0, 255);
		fillValue = (BYTE)theValue;
		for(i=0; i < pTexture->m_Header.m_nMipmaps; i++)
		{
			pMip = &pTexture->m_Mips[i];
			
			pCurLine = (BYTE*)tUtils.m_PValueMipmaps[i].GetArray();
			for(y=0; y < pMip->m_Height; y++)
			{
				pPos = pCurLine;

				for(x=0; x < pMip->m_Width; x++)
				{
					PValue &pValue = *((PValue*)pPos);
					pValue = PValue_Set(fillValue, PValue_GetR(pValue), PValue_GetG(pValue), PValue_GetB(pValue));

					pPos += sizeof(DWORD);
				}

				pCurLine += pMip->m_Width * sizeof(DWORD);
			}
		}
	}
	else if(camType == CAM_FromColor)
	{
		// Just get alpha from color.
		for(i=0; i < pTexture->m_Header.m_nMipmaps; i++)
		{
			pMip = &pTexture->m_Mips[i];
			
			pCurLine = (BYTE*)tUtils.m_PValueMipmaps[i].GetArray();
			for(y=0; y < pMip->m_Height; y++)
			{
				pPos = pCurLine;

				for(x=0; x < pMip->m_Width; x++)
				{
					PValue &pValue = *((PValue*)pPos);
					
					PValue_Get(pValue, aVal, rVal, gVal, bVal);
					aVal = (rVal + gVal + bVal) / 3;
					pValue = PValue_Set(aVal, rVal, gVal, bVal);

					pPos += sizeof(DWORD);
				}

				pCurLine += pMip->m_Width * sizeof(DWORD);
			}
		}
	}
	else
	{
		importExt.LoadString( IDS_PCX_EXTENSION );
		fileMask.LoadString( IDS_PCX_FILEMASK );
		str = GetProject()->m_BaseProjectDir + "\\Textures\\*" + importExt;
		CFileDialog dlg(TRUE, importExt, (LPCTSTR)str, 0, fileMask, this);

		if(dlg.DoModal() == IDOK)
		{
			bLoaded = FALSE;
			if(pStream = streamsim_Open(dlg.GetPathName(), "rb"))
			{
				if(pcx_Create2(pStream, &pcxTexture))
				{
					bLoaded = TRUE;
				}
				pStream->Release();
			}

			if(!bLoaded)
			{
				dtx_Destroy(pTexture);
				str.FormatMessage(IDS_CANTLOADPCX, dlg.GetPathName());
				AppMessageBox( str, MB_OK );
				return FALSE;
			}

			if(pcxTexture.m_Width != pTexture->m_Header.m_BaseWidth || 
				pcxTexture.m_Height != pTexture->m_Header.m_BaseHeight ||
				pcxTexture.m_Format.m_BPP != BPP_8P)
			{
				dtx_Destroy(pTexture);
				str.FormatMessage(IDS_SIZEDOESNTMATCH);
				AppMessageBox( str, MB_OK );
				return FALSE;
			}
		}
		else
		{
			dtx_Destroy(pTexture);
			return FALSE;
		}


		// This routine wants it in 8-bit form.
		if(camType == CAM_From4BitPCX)
		{
			for(y=0; y < pcxTexture.m_Height; y++)
			{
				for(x=0; x < pcxTexture.m_Width; x++)
				{
					fVal = pcxTexture.m_Data[y*pcxTexture.m_Pitch+x];
					fVal = (fVal * 255.0f) / 15.0f;
					pcxTexture.m_Data[y*pcxTexture.m_Pitch+x] = (BYTE)fVal;
				}
			}
		}


		// Fill in the alpha part of the texture.
		for(i=0; i < pTexture->m_Header.m_nMipmaps; i++)
		{
			pMip = &pTexture->m_Mips[i];
			
			pCurLine = (BYTE*)tUtils.m_PValueMipmaps[i].GetArray();
			for(y=0; y < pMip->m_Height; y++)
			{
				pPos = pCurLine;

				for(x=0; x < pMip->m_Width; x++)
				{
					PValue &pValue = *((PValue*)pPos);
					
					PValue_Get(pValue, aVal, rVal, gVal, bVal);
					pValue = PValue_Set(pcxTexture.Pixel(x, y), rVal, gVal, bVal);

					pPos += sizeof(DWORD);
				}

				pCurLine += pMip->m_Width * sizeof(DWORD);
			}

			// Reduce (for the next mipmap).
			for(y=0; y < pMip->m_Height; y+=2)
			{
				for(x=0; x < pMip->m_Width; x+=2)
				{
					wVal = (WORD)pcxTexture.m_Data[y*pcxTexture.m_Pitch+x] + (WORD)pcxTexture.m_Data[y*pcxTexture.m_Pitch+x+1]
						+ (WORD)pcxTexture.m_Data[(y+1)*pcxTexture.m_Pitch+(x+1)] + (WORD)pcxTexture.m_Data[(y+1)*pcxTexture.m_Pitch+x];

					pcxTexture.m_Data[(y>>1)*pcxTexture.m_Pitch+(x>>1)] = (BYTE)(wVal >> 2);
				}
			}
		}
	}


	// Reconvert into the TextureData.
	if(!tUtils.ToTextureData(pTexture, TRUE))
	{
		str.FormatMessage(IDS_ERRORSAVEDTX, fullName);
		AppMessageBox(str, MB_OK);
		dtx_Destroy(pTexture);	
		return FALSE;		
	}


	// Save the DTX back out.
	pStream = streamsim_Open(fullName, "wb");
	if(!pStream)
	{
		str.FormatMessage(IDS_ERRORSAVEDTX, fullName);
		AppMessageBox( str, MB_OK );
		dtx_Destroy(pTexture);	
		return FALSE;		
   	}
	
	dResult = dtx_Save(pTexture, pStream);
	if(dResult != DE_OK)
	{
		str.FormatMessage(IDS_ERRORSAVEDTX, fullName);
		AppMessageBox( str, MB_OK );
	}

	pStream->Release();
	dtx_Destroy(pTexture);	

	AppMessageBox(IDS_ALPHA_SET_SUCCESSFUL, MB_OK);
	return TRUE;
}


// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::DoViewAllTextures
//
//	PURPOSE:
//
// ----------------------------------------------------------------------- //

BOOL CTextureDlg::DoViewAllTextures()
{
	if( IsWindow( m_AllTextureDlg.m_hWnd ))
		return FALSE;

	m_AllTextureDlg.Create(IDD_ALLTEXTUREDLG, this );
	m_AllTextureDlg.ShowWindow( SW_SHOW );

	return TRUE;
}

BOOL CTextureDlg::DoAddToPalette()
{
	// Make sure the texture dialog is open
	if (!IsWindow(m_AllTextureDlg.m_hWnd))
		return FALSE;

	int iItem;

	iItem = -1;
	while((iItem = m_TextureList.GetNextItem(iItem, LVNI_ALL|LVNI_SELECTED)) != -1)
	{
		DFileIdent *pItemData = (DFileIdent *)m_TextureList.GetItemData(iItem);
		m_AllTextureDlg.AddTextureToPalette(pItemData->m_Filename);
	}

	return TRUE;
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::CopyPcxToTexture
//
//	PURPOSE:
//
// ----------------------------------------------------------------------- //

// 2001's (0x40bd08) returns nothing, and says it can't load the PCX whatever
// the image type (Build 69 says TGA for a TGA).
void CTextureDlg::CopyImageToTexture( 
	ImageType iType, const char *pPcxFilename, const char *pTextureFilename, DWORD textureFlags )
{
	LoadedBitmap bitmap;
	CString str;
	DStream *pStream, *pInFile;


	if( pInFile = streamsim_Open(pPcxFilename, "rb") )
	{
		if( (iType == ITYPE_PCX ? pcx_Create2(pInFile, &bitmap) : tga_Create2(pInFile, &bitmap)) )
		{
			if( dtx_IsTextureSizeValid(bitmap.m_Width) && dtx_IsTextureSizeValid(bitmap.m_Height) )
			{
				pStream = streamsim_Open(pTextureFilename, "wb");
				if(pStream)
				{
					SavePcxAsTexture(&bitmap, pStream, textureFlags);
					pStream->Release();
				}
			}
			else
			{
				str.FormatMessage( IDS_INVALIDTEXTURESIZE, pPcxFilename, bitmap.m_Width, bitmap.m_Height );
				AppMessageBox( str, MB_OK );
			}
		}
		else
		{
			str.FormatMessage( IDS_CANTLOADPCX, pPcxFilename );
			AppMessageBox( str, MB_OK );
		}
	
		pInFile->Release();
	}
}


// ----------------------------------------------------------------------- //
//	ROUTINE:	CTextureDlg::CopyMipTGAToTexture
//	PURPOSE:	Builds a DTX from the 4 input TGA files.
// ----------------------------------------------------------------------- //
// 2001's: always four mipmaps (0x40be38: the loop to 4, dtx_Alloc's 4).
BOOL CTextureDlg::CopyMipTGAToTexture( const CString *pFileNames, const char *outFilename )
{
	CMoFileIO outFile;
	DStream *pInFile;
	LoadedBitmap theTexture;
	CString str;
	int	i, prevWidth, prevHeight, x, y;
	TextureData *pDtx;
	TextureMipData *pMip;
	DRESULT dResult;
	BYTE *pInLine, *pOutLine;
	DStream *pStream;
	FMConvertRequest cRequest;
	BOOL bRet;

	
	// Figure out how large the texture will be.
	if(pInFile = streamsim_Open((LPCTSTR)pFileNames[0], "rb"))
	{
		bRet = tga_Create2(pInFile, &theTexture);
		pInFile->Release();
		
		if(!bRet)
		{
			str.FormatMessage( IDS_CANTLOADTGA, pFileNames[0] );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}
	}
	else
	{
		str.FormatMessage( IDS_CANTLOADTGA, pFileNames[0] );
		AppMessageBox( str, MB_OK );
		return FALSE;
	}

	// Create the DTX.
	pDtx = dtx_Alloc(BPP_32, theTexture.m_Width, theTexture.m_Height, 4, NULL, NULL);
	if(!pDtx)
		return FALSE;

	// Convert the mipmaps.
	for(i=0; i < 4; i++)
	{
		pMip = &pDtx->m_Mips[i];
		
		pInFile = streamsim_Open(LPCTSTR( pFileNames[i] ), "rb");
		if(!pInFile)
		{
			dtx_Destroy(pDtx);	
			str.FormatMessage( IDS_CANTLOADTGA, pFileNames[i] );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}

		// Read in image data.
		if(!tga_Create2(pInFile, &theTexture))
		{
			pInFile->Release();
			dtx_Destroy(pDtx);	
			str.FormatMessage( IDS_CANTLOADTGA, pFileNames[i] );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}
		pInFile->Release();

		
		// Make sure sizes are valid.
		if(theTexture.m_Width != pMip->m_Width || theTexture.m_Height != pMip->m_Height)
		{
			dtx_Destroy(pDtx);	
			str.FormatMessage( IDS_INVALIDMIPTGA, i + 1, i );
			AppMessageBox( str, MB_OK );
			return FALSE;
		}

		// Convert the data.
		cRequest.m_pSrc = theTexture.m_Data.GetArray();
		cRequest.m_pSrcPalette = theTexture.m_Palette;
		cRequest.m_SrcPitch = theTexture.m_Pitch;
		*cRequest.m_pSrcFormat = theTexture.m_Format;
		cRequest.m_pDest = pMip->m_Data;
		cRequest.m_DestPitch = pMip->m_Pitch;
		cRequest.m_pDestFormat->InitPValueFormat();
		cRequest.m_Width = pMip->m_Width;
		cRequest.m_Height = pMip->m_Height;
		cRequest.m_Flags = 0;
		
		dResult = g_FormatMgr.ConvertPixels(&cRequest);
		if(dResult != LT_OK)
		{
			dtx_Destroy(pDtx);	
			str.FormatMessage( IDS_INVALIDMIPTGA, i + 1, i );
			AppMessageBox( str, MB_OK );
			return FALSE;			
		}
	}

	// Save the data!
	pStream = streamsim_Open(outFilename, "wb");
	if(!pStream)
	{
		dtx_Destroy(pDtx);	
		str.FormatMessage( IDS_ERRORSAVEDTX, outFilename );
		AppMessageBox( str, MB_OK );
		return FALSE;
	}

	dResult = dtx_Save(pDtx, pStream);
	pStream->Release();

	dtx_Destroy(pDtx);	
	return dResult == DE_OK;
}


void CTextureDlg::OnDestroy() 
{
	CMRCSizeDialogBar::OnDestroy();
	
	// TODO: Add your message handler code here
	m_AllTextureDlg.DestroyWindow();	
}

// ----------------------------------------------------------------------- //
//
//	ROUTINE:	CTextureDlg::FindTexture
//
//	PURPOSE:	Finds a texture in the tree & selects it
//
// ----------------------------------------------------------------------- //

BOOL CTextureDlg::FindTexture(DFileIdent *pTexture, BOOL bRecurse, HTREEITEM hBaseTree)
{
	if (!pTexture || !pTexture->m_Filename)
		return FALSE;

	// Handle the entry point
	if (!hBaseTree)
	{
		// Try to find it the fast way...
		if (FastFindTexture(pTexture))
			return TRUE;
		if (!bRecurse)
			return FALSE;
		// Otherwise, use the root item and try to find it by recursing through the directories
		hBaseTree = m_TextureTree.GetRootItem();
	}

	// Check the list at this level
	DDirIdent *pIdent = (DDirIdent*)m_TextureTree.GetItemData(hBaseTree);
	if(pIdent)
	{
		m_csCurrentDir = pIdent->m_Filename;
		m_hCurrentItem = hBaseTree;

		// Note : I hate visibly populating the list like this, but it's the only clean way to get
		//		  the list of files at this level that I'm aware of
		PopulateList();
		
		int iListIndex = FindFileInList(pTexture);
		if (iListIndex >= 0)
		{
			// If we found it, select this item and update the All Textures dialog
			m_TextureTree.Select(hBaseTree, TVGN_CARET);
			ChangeSelection(iListIndex);
			m_AllTextureDlg.NotifyDirChange();
			// And return that we found it
			return TRUE;
		}
	}

	// Check the first child
	HTREEITEM hChild = m_TextureTree.GetChildItem(hBaseTree);
	if (hChild)
	{
		if (FindTexture(pTexture, bRecurse, hChild))
			return TRUE;
	}

	// Check the first sibling
	hBaseTree = m_TextureTree.GetNextSiblingItem(hBaseTree);
	if (hBaseTree)
	{
		if (FindTexture(pTexture, bRecurse, hBaseTree))
			return TRUE;
	}

	// It wasn't in this branch, so return false
	return FALSE;
}

BOOL CTextureDlg::FastFindTexture(DFileIdent *pTexture)
{
	if (!pTexture || !pTexture->m_Filename)
		return FALSE;

	// Make a copy of the name
	CString csName(pTexture->m_Filename);
	// Find the last slash
	int iFinger = csName.ReverseFind('\\');
	if (iFinger < 0)
		return FALSE;

	// Cut off the string at the last slash
	csName = csName.Left(iFinger);

	// Structure for searching the tree
	char cBuffer[256];
	TVITEM sTreeItem;
	memset(&sTreeItem, 0, sizeof(sTreeItem));
	sTreeItem.pszText = cBuffer;
	sTreeItem.cchTextMax = 256;
	sTreeItem.mask = TVIF_TEXT | TVIF_HANDLE;

	// Find the path in the tree
	HTREEITEM hTreeFinger = m_TextureTree.GetRootItem();
	while ((hTreeFinger) && (!csName.IsEmpty()))
	{
		CString csNodeName;

		iFinger = csName.Find('\\');
		// Strip off leading slashes..
		if (!iFinger)
		{
			csName.Delete(0);
			continue;
		}
		// Handle a mid-level node
		else if (iFinger > 0)
		{
			csNodeName = csName.Left(iFinger);
			csName.Delete(0, iFinger + 1);
		}
		// handle the final node
		else
		{
			csNodeName = csName;
			csName.Empty();
		}
		// Find the node at this level of the tree
		while (hTreeFinger) 
		{
			sTreeItem.hItem = hTreeFinger;
			m_TextureTree.GetItem(&sTreeItem);
			// If we found a tree item with the right name, go back through the main loop
			if (!stricmp(cBuffer, (LPCTSTR)csNodeName))
				break;
			// Move to the next sibling
			hTreeFinger = m_TextureTree.GetNextSiblingItem(hTreeFinger);
		}
		// Go down a level if we found a match for this name
		if ((hTreeFinger) && (!csName.IsEmpty()))
			hTreeFinger = m_TextureTree.GetChildItem(hTreeFinger);
	}

	// If we found the folder, try to find it in that directory..
	if (hTreeFinger)
		return FindTexture(pTexture, FALSE, hTreeFinger);

	// Otherwise, it wasn't found...
	return FALSE;
}


static void ExpandTreeChildren(CTreeCtrl &cTree, HTREEITEM hItem, UINT action);

void CTextureDlg::OnItemexpandingBaserezTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	if (pNMTreeView->action == TVE_COLLAPSE)
		ExpandTreeChildren(m_TextureTree, pNMTreeView->itemNew.hItem, TVE_COLLAPSE);
	
	*pResult = 0;
}

// Function to distribute an expansion action to a tree item's children because MFC is too lame
//		to provide this functionality on its own.
static void ExpandTreeChildren(CTreeCtrl &cTree, HTREEITEM hItem, UINT action)
{
	HTREEITEM hFinger = cTree.GetChildItem(hItem);
	while (hFinger)
	{
		// If this node has children, recurse into them..
		if (cTree.GetChildItem(hFinger))
			ExpandTreeChildren(cTree, hFinger, action);
		// Move to the next sibling
		hFinger = cTree.GetNextSiblingItem(hFinger);
	}
	// Visit this node
	if (cTree.GetChildItem(hItem))
		cTree.Expand(hItem, action);
}
