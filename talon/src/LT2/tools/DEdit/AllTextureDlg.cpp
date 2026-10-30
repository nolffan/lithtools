//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// AllTextureDlg.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "alltexturedlg.h"
#include "projectbar.h"
#include "stringdlg.h"
#include "texture.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAllTextureDlg dialog


CAllTextureDlg::CAllTextureDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAllTextureDlg::IDD, pParent),
	m_cPalette(IDS_TEXTUREPALETTE_FILE_HEADER),
	m_csPaletteFileName(""),
	m_bPaletteChanged(FALSE),
	m_eCurMode(MODE_VIEWALL)
{
	//{{AFX_DATA_INIT(CAllTextureDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CAllTextureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAllTextureDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAllTextureDlg, CDialog)
	//{{AFX_MSG_MAP(CAllTextureDlg)
	ON_WM_SIZE()
	ON_COMMAND(ID_POPUP_TEXTUREPALETTE_LOAD, OnPopupTexturePaletteLoad)
	ON_COMMAND(ID_POPUP_TEXTUREPALETTE_SAVE, OnPopupTexturePaletteSave)
	ON_COMMAND(ID_POPUP_TEXTUREPALETTE_VIEWALL, OnPopupTexturePaletteViewAll)
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_POPUP_TEXTUREPALETTE_NEW, OnPopupTexturePaletteNew)
	ON_COMMAND(ID_POPUP_TEXTUREPALETTE_DELETE, OnPopupTexturePaletteDelete)
	ON_COMMAND(ID_POPUP_TEXTUREPALETTE_IMPORT, OnPopupTexturePaletteImport)
	ON_COMMAND(ID_POPUP_TEXTUREPALETTE_SORT, OnPopupTexturePaletteSort)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAllTextureDlg message handlers

BOOL CAllTextureDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect			rcTextureList;

	GetDlgItem(IDC_TEXTURELB)->GetWindowRect( &rcTextureList );
	ScreenToClient( &rcTextureList );

	m_TextureList.m_pNotifier = this;

	m_TextureList.Create(WS_VISIBLE | WS_CHILD | LBS_MULTICOLUMN | LBS_HASSTRINGS | LBS_OWNERDRAWFIXED | WS_HSCROLL |
					   LBS_NOINTEGRALHEIGHT | WS_BORDER | LBS_DISABLENOSCROLL | LBS_NOTIFY,
					   rcTextureList, this, 3456);

	m_TextureList.m_bDrawNumbers = FALSE;

	m_TextureList.SetContextMenu(CG_IDR_POPUP_TEXTUREPALETTE);

	FillTextureList();

	CRect cClientRect;
	GetClientRect(cClientRect);
	// Only initialize the anchors on the first initialize
	if (m_cResizer.GetAnchorList().GetSize() == 0)
	{
		// Set up the resizing anchors
		m_cResizer.AddAnchor(new CResizeAnchorOffset(3456, CResizeAnchorOffset::eAnchorSize));

		// Lock the resizing anchors
		m_cResizer.Lock(this, cClientRect);
	}
	else
		// Reset the sizes if this isn't the first initialization
		m_cResizer.Resize(this, cClientRect);

	// Refresh the caption
	SetPaletteChanged(GetPaletteChanged());

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// Set the palette change flag
void CAllTextureDlg::SetPaletteChanged(BOOL bChanged)
{
	// Save the value
	m_bPaletteChanged = bChanged;

	// Update the window caption
	if (IsWindow(m_hWnd) && (GetViewMode() == MODE_PALETTE))
	{
		CString csNewTitle;
		csNewTitle.Format("Texture Palette \"%s\"%s", m_cPalette.GetName(), bChanged ? " *" : "");
		SetWindowText(csNewTitle);
	}
}

void CAllTextureDlg::AddTextureToPalette(LPCTSTR pTexture)
{
	// Jump out if the dialog's currently in "all textures" mode
	if (GetViewMode() != MODE_PALETTE)
		return;

	// Add the texture to the list
	m_cPalette.AddEntry(pTexture);

	// Refresh the texture list
	FillTextureList();

	// Set the change flag
	SetPaletteChanged(TRUE);
}

void CAllTextureDlg::FillTextureList()
{
	DFileIdent	*pIdent;

	m_TextureList.ResetContent();

	// Check for "view all textures" mode
	if (GetViewMode() == MODE_VIEWALL)
	{
		// Fill the texture list from the texture dialog
		CTextureDlg* pParent = GetTextureDlg();
		CString searchSpec, fullName;
		CFileIterator iterator;

		if((!pParent) || (!pParent->GetSelectedDirectory()) || (!IsWindow(pParent->m_hWnd)))
			return;

		// Add each .spr file to the list (with its FileIdent as the data).
		searchSpec = dfm_BuildName(pParent->GetSelectedDirectory()->m_Filename, "*.dtx");
		while(iterator.Next(searchSpec, TRUE))
		{
			fullName = dfm_BuildName(pParent->GetSelectedDirectory()->m_Filename, iterator.GetFilename());
			dfm_GetFileIdentifier(GetFileMgr(), fullName, &pIdent);
			if(pIdent)
			{
				// Find a location for the entry
				for (int iIndex = 0; iIndex < m_TextureList.GetCount(); iIndex++)
				{
					CString csHold;
					m_TextureList.GetText(iIndex, csHold);
					if (csHold.CompareNoCase(iterator.GetFilename()) > 0)
						break;
				}
				m_TextureList.InsertString(iIndex, iterator.GetFilename() );
				m_TextureList.SetItemDataPtr(iIndex, pIdent);
			}
		}

		SetWindowText(pParent->GetSelectedDirectory()->m_Filename);
	}
	else
	{
		// Fill the texture list from the palette list
		for (DWORD uLoop = 0; uLoop < m_cPalette.GetSize(); uLoop++)
		{
			dfm_GetFileIdentifier(GetFileMgr(), m_cPalette[uLoop], &pIdent);
			if (pIdent)
			{
				int index = m_TextureList.AddString(m_cPalette[uLoop]);
				m_TextureList.SetItemDataPtr(index, pIdent);
			}
		}
	}
}

void CAllTextureDlg::DeleteSelectedTexture()
{
	// Jump out if a palette isn't loaded
	if (GetViewMode() != MODE_PALETTE)
		return;

	int iCurSel = m_TextureList.GetCurSel();
	
	// Make sure a texture is selected
	if (iCurSel < 0) 
		return;

	// Remove it from the palette list
	m_cPalette.Remove(iCurSel);

	// Remove it from the viewing list
	m_TextureList.DeleteString(iCurSel);

	// Set the change flag
	SetPaletteChanged(TRUE);
}

int CAllTextureDlg::ConfirmSave(char *pMessage)
{
	if (!GetPaletteChanged())
		return IDOK;

	CString csMessage;
	if (!pMessage)
		csMessage.Format("The palette \"%s\" has not been saved!  Would you like to save it now?", m_cPalette.GetName());
	else
		csMessage = pMessage;

	int iResult = ::MessageBox(NULL, csMessage, "Confirmation",  MB_ICONINFORMATION | MB_YESNOCANCEL);
	
	switch (iResult)
	{
		case IDYES :
			OnPopupTexturePaletteSave();
			break;
	}
	return iResult;
}


void CAllTextureDlg::OnCancel()
{
	DestroyWindow( );

	return;
}

void CAllTextureDlg::NotifyDblClk( CFrameList *pList, int curSel )
{
	// Try to find it in the current directory
	DFileIdent *pListItem = (DFileIdent *)pList->GetItemData(curSel);
	CString csTemp(pListItem->m_Filename);
	CListCtrl *pDlgList = &(GetTextureDlg()->m_TextureList);

	int iSelItem = -1;
	for (int iLoop = 0; (iLoop < pDlgList->GetItemCount()) && (iSelItem < 0); iLoop++)
	{
		DFileIdent *pSearchItem = (DFileIdent *)pDlgList->GetItemData(iLoop);
		if (!csTemp.CompareNoCase(pSearchItem->m_Filename))
			iSelItem = iLoop;
	}

	// Just change the selection if it's already in the right directory
	if (iSelItem >= 0)
		GetTextureDlg()->ChangeSelection(iSelItem);
	else
	{
		GetTextureDlg()->FindTexture(pListItem);
		// Re-update the selection pointer because of the directory change
		m_TextureList.SetCurSel(curSel);
	}

	return;
}


void CAllTextureDlg::NotifyReCreate(CFrameList *pList)
{
	m_TextureList.DestroyWindow();
	OnInitDialog();
}

void CAllTextureDlg::NotifyDirChange()
{
	// Don't do anything if it's not in view all mode
	if (GetViewMode() != MODE_VIEWALL)
		return;

	// Don't do anything if the window isn't valid
	if(!IsWindow(m_hWnd))
		return;

	// Re-load the texture list
	FillTextureList();
}

void CAllTextureDlg::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	
	// Don't resize if the window doesn't really exist..
	if (!IsWindow(m_hWnd))
		return;

	// Resize the controls
	CRect cNewSize(0,0, cx, cy);
	m_cResizer.Resize(this, cNewSize);
}

void CAllTextureDlg::OnPopupTexturePaletteLoad() 
{
	// Make sure a changed palette doesn't accidentally get lost
	if (ConfirmSave() == IDCANCEL)
		return;

	// Display the load dialog
	CHelperFileDlg	dlg( TRUE, "*.pal", m_csPaletteFileName, OFN_FILEMUSTEXIST, "Palette Files (*.pal)|*.pal||", this );
	dlg.m_ofn.lpstrInitialDir = GetProject()->m_BaseProjectDir;
	if( dlg.DoModal() != IDOK )
		return;

	// Load the texture palette
	if (!m_cPalette.Load(dlg.GetPathName()))
		return;

	// Switch to palette mode
	SetViewMode(MODE_PALETTE);

	// Display the loaded palette
	FillTextureList();

	// Turn off the change flag
	SetPaletteChanged(FALSE);
}

void CAllTextureDlg::OnPopupTexturePaletteSave() 
{
	// Display the save dialog
	CString csFileName(m_csPaletteFileName);
	if (!m_csPaletteFileName.GetLength())
	{
		csFileName = m_cPalette.GetName();
		csFileName += ".pal";
	}
	CHelperFileDlg	dlg(FALSE, "*.pal", csFileName, OFN_OVERWRITEPROMPT, "Palette Files (*.pal)|*.pal||", this );
	dlg.m_ofn.lpstrInitialDir = GetProject()->m_BaseProjectDir;
	if( dlg.DoModal() != IDOK )
		return;

	// Change the file name if it was empty before
	if (!m_csPaletteFileName.GetLength())
		m_csPaletteFileName = dlg.GetPathName();

	// Save the texture palette
	if (!m_cPalette.Save(m_csPaletteFileName))
		return;

	// Turn off the change flag
	SetPaletteChanged(FALSE);
}

void CAllTextureDlg::OnPopupTexturePaletteViewAll() 
{
	// Make sure a changed palette doesn't accidentally get lost
	if (ConfirmSave() == IDCANCEL)
		return;

	// Clear the palette list
	m_cPalette.Term();
	m_cPalette.SetName("");
	m_csPaletteFileName = "";

	// Go to "view all" mode
	SetViewMode(MODE_VIEWALL);

	// Load the texture list from the project bar
	FillTextureList();

	// Turn off the change flag
	SetPaletteChanged(FALSE);
}

void CAllTextureDlg::OnPopupTexturePaletteNew() 
{
	// Make sure a changed palette doesn't accidentally get lost
	if (ConfirmSave() == IDCANCEL)
		return;

	// Get a palette name
	CStringDlg		dlg;

	dlg.m_bAllowFile = TRUE;
	dlg.m_MaxStringLen = 70;
	dlg.m_bBeeping = TRUE;

	if( dlg.DoModal(IDS_TEXTUREPALETTE_NEW_TITLE, IDS_TEXTUREPALETTE_NEW_CAPTION) != IDOK )
		return;

	// Set up for an empty palette
	CString csTemp(dlg.m_EnteredText);
	if (!csTemp.GetLength())
		m_cPalette.SetName("Unnamed");
	else
		m_cPalette.SetName(csTemp);
	m_csPaletteFileName = "";
	m_cPalette.Term();

	// Go to palette mode
	SetViewMode(MODE_PALETTE);

	// Clear the texture list
	FillTextureList();

	// Set the change flag
	SetPaletteChanged(TRUE);
}

void CAllTextureDlg::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	switch (nChar)
	{
		case VK_DELETE :
			DeleteSelectedTexture();
			break;
	}

	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}


BOOL CAllTextureDlg::DestroyWindow() 
{
	if (ConfirmSave() == IDCANCEL)
		return FALSE;

	// Make sure the change dialog never comes up again
	SetPaletteChanged(FALSE);

	if (!IsWindow(m_hWnd))
		return TRUE;

	return CDialog::DestroyWindow();
}

void CAllTextureDlg::OnPopupTexturePaletteDelete() 
{
	DeleteSelectedTexture();
}

void CAllTextureDlg::OnPopupTexturePaletteImport() 
{
	// Jump out if the dialog's not in palette mode
	if (GetViewMode() != MODE_PALETTE)
		return;

	// Get the active document
	CRegionDoc *pDoc = GetActiveRegionDoc();
	if (!pDoc)
		return;

	// Show the hourglass cursor
	BeginWaitCursor();

	// Go through the brush list
	CLinkedList<CEditBrush*> *pBrushList = &(pDoc->m_Region.m_Brushes);
	LPOS lpFinger;
	BOOL bChanged = FALSE;
	for (lpFinger = pBrushList->GetHeadPosition(); lpFinger; )
	{
		CEditBrush *pBrush = pBrushList->GetNext(lpFinger);

		// Go through the polygon list
		for (DWORD uLoop = 0; uLoop < pBrush->m_Polies.GetSize(); uLoop++)
		{
			CString csTextureName(pBrush->m_Polies[uLoop]->m_pTextureName);
			// Skip over untextured polygons
			if (!csTextureName.GetLength())
				continue;
			// Skip over sprites
			CString csTemp;
			csTemp = csTextureName.Right(3);
			if (!csTemp.CompareNoCase("SPR"))
				continue;
			// Add the texture
			if (m_cPalette.AddEntry(csTextureName))
			{
				DFileIdent *pIdent;

				dfm_GetFileIdentifier(GetFileMgr(), csTextureName, &pIdent);
				if (pIdent)
				{
					int index = m_TextureList.AddString(csTextureName);
					m_TextureList.SetItemDataPtr(index, pIdent);
					m_TextureList.UpdateWindow();
					bChanged = TRUE;
				}
			}
		}
	}

	// Update the change flag
	if (bChanged)
		SetPaletteChanged(TRUE);

	// Bring back the old cursor
	EndWaitCursor();
}

void CAllTextureDlg::OnPopupTexturePaletteSort() 
{
	// Jump out if the dialog's not in palette mode
	if (GetViewMode() != MODE_PALETTE)
		return;

	// Jump out if nothing's in the palette to sort
	if (m_cPalette.GetSize() <= 1)
		return;

	BeginWaitCursor();

	// Use a simple bubble sort on the palette..  (Not optimal, but whatever, it's a short list..)
	for (DWORD uTop = 0; uTop < (m_cPalette.GetSize() - 1); uTop++)
	{
		CString csLowest = m_cPalette[uTop];
		for (DWORD uSearch = uTop + 1; uSearch < m_cPalette.GetSize(); uSearch++)
		{
			if (csLowest.CompareNoCase(m_cPalette[uSearch]) > 0)
			{
				// Swap the entries
				CString csTemp = m_cPalette[uSearch];
				m_cPalette[uSearch] = csLowest;
				csLowest = csTemp;
			}
		}
		// Store the lowest entry in the array
		m_cPalette[uTop] = csLowest;
	}

	// Refill the palette list
	FillTextureList();

	// Set the change flag
	SetPaletteChanged(TRUE);

	EndWaitCursor();
}
