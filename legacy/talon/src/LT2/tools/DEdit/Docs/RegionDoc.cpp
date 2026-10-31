// RegionDoc.cpp : implementation file
//

#include "bdefs.h"
#include "dedit.h"
#include "regiondoc.h"
#include "edithelpers.h"
#include "propertiesdlg.h"
#include "resource.h"
#include "projectbar.h"
#include "regionview.h"
#include "mainfrm.h"
#include "propertyhelpers.h"
#include "edit_actions.h"
#include "deditinternal.h"
#include "ExportObjFile.h"
#include "LoadLTADialog.h"
#include "LightMapDefs.h"
#include "ltamgr.h"
#include "CObjInterface.h"
#include "geomroutines.h"
#include "edithelpers.h"
#include "optionsrun.h"
#include "draw_d3d.h"
#include "colorselectdlg.h"
#include "preworld.h"
#include "prelightmap.h"
#include "d3d_texturemgr.h"

// talon: the views' update hint for a lightmap change (0x45103a pushes
// 103); RegionView.h has no name for it.
#ifndef REGIONVIEWUPDATE_LIGHTMAPS
#define REGIONVIEWUPDATE_LIGHTMAPS	103
#endif

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRegionDoc

// talon: the colour dialog's callback (0x44f005): the colour goes to the
// selected lightmap texels (the name is ours).
static void RegionDocColorChangeCB(COLORREF newColor, void *pUser)
{
	((CRegionDoc*)pUser)->SetTexelColor(newColor);
}

IMPLEMENT_DYNCREATE(CRegionDoc, CDocument)

// 0x44f04e: 2001's has a fixed undo depth (40, 0x44f18f) and registers for
// the colour dialog's changes.
CRegionDoc::CRegionDoc() 
{
	GetColorSelectDlg()->AddCallback(RegionDocColorChangeCB, this);

	m_LightPolyWorld = (uint32)-1;
	m_LightPolyName = (uint32)-1;

	m_TaggedPolies.SetCacheSize(4096);

	m_RegionStringHolder.SetAllocSize( 1000 );

	m_bCanSynchronize		= FALSE;
	m_Region.m_pStringHolder = &m_RegionStringHolder;

	m_UndoMgr.Init(this, 40, 40);

	m_TaggedPolies.SetSize(1);

	m_TaggedVerts.Init(
		64,
		&m_TaggedVertsHelper);

	m_pLightWorld = NULL;
}


// 0x44f237
CRegionDoc::~CRegionDoc()
{
	GetColorSelectDlg()->RemoveCallback(RegionDocColorChangeCB, this);

	DeleteFile(m_BackupName);

	GetProjectBar()->RemoveRegionDoc(this);
	GetPropertiesDlg()->Term();

	TermLightAnims();
}



// ---------------------------------------------------------- //
// Initializes the properties dialog based on the current selections.
// ---------------------------------------------------------- //

void CRegionDoc::SetupPropertiesDlg( bool bShow )
{
	char		typeName[256];
	CEditRegion	*pRegion = GetRegion();
	CPropList	*pList;

	
	if(pRegion->m_Selections == 0)
	{
		GetPropertiesDlg()->Term();
		return;
	}
	else if(pRegion->m_Selections == 1)
	{
		strcpy(typeName, pRegion->m_Selections[0]->GetClassName());
	}
	else
	{
		strcpy(typeName, "N/A");
	}

	pList = CreateMainPropertyList(this);
	GetPropertiesDlg()->Init(pList, this, typeName);
}			



// 0x44f3ff
void CRegionDoc::NotifyPropertiesChange( BOOL bUpdateView )
{
	// Setup undos.
	SetupUndoForSelections();
	
	ReadPropertiesIntoSelections(this);
	if( bUpdateView )
		RedrawAllViews( );
}


// 0x44f429
void CRegionDoc::NotifyPropertyChange( CBaseProp *pProp, BOOL bUpdateView )
{
	// Setup undos.
	SetupUndoForSelections();
	
	ReadPropertyIntoSelections( this, pProp );
	if( bUpdateView )
		RedrawAllViews( );
}


// 0x44f458
BOOL CRegionDoc::SaveModified()
{
	int			status;
	CString		prompt;

	if( IsModified() )
	{	
		AfxFormatString1( prompt, AFX_IDP_ASK_TO_SAVE, (LPCTSTR)m_WorldName );
		status = AfxMessageBox( prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE );

		if( status == IDYES )
		{
			if( !SaveRegion(TRUE))
				return FALSE;
		}
		else if( status == IDCANCEL )
		{
			return FALSE;
		}
	}

	SetModifiedFlag( FALSE );
	return TRUE;
}


// 0x44f4dc: the world goes to the backup file first; a real save then
// copies it over the world file, once the world file is known to be
// writable (the name is ours: Build 69's SaveRegion is commented out).
// NEAR (277 of 277 bytes): the shipped build numbers the two error
// strings' unwind states the other way round (the backup's 1, the world
// file's 2); putting the backup's test first in the source moves the
// blocks, not the numbers.
BOOL CRegionDoc::SaveRegion(BOOL bFullSave)
{
	CMoFileIO	file;

	BeginWaitCursor();

	if( file.Open(m_BackupName, "wb") )
	{
		m_Region.SaveFile(file, TRUE);
		file.Close();

		if( bFullSave )
		{
			if( file.Open(m_FileName, "wb") )
			{
				file.Close();
				CopyFile(m_BackupName, m_FileName, FALSE);
			}
			else
			{
				CString str;
				str.FormatMessage(IDS_ERROR_SAVING_FILE, (LPCTSTR)m_FileName);
				AfxMessageBox( str );
				return FALSE;
			}
		}

		EndWaitCursor();
		return TRUE;
	}
	else
	{
		CString str;
		str.FormatMessage(IDS_ERROR_SAVING_FILE, (LPCTSTR)m_BackupName);
		AfxMessageBox( str );
		return FALSE;
	}
}


// 0x44f5f1
BOOL CRegionDoc::SaveOBJ(BOOL bFullSave)
{
	SaveRegion(bFullSave);
	CObjInterface objInterface;
	// fill in objInterface with data from the saved ed file

	CEditRegion		region;
	CMoFileIO		file;

	CExportObjFile  exportObjFileDlg;
	exportObjFileDlg.DoModal();

	if( exportObjFileDlg.m_isOK )
	{
		BeginWaitCursor( ); // Put the wait cursor on the screen

		if(file.Open(m_FileName, "rb"))
		{
			if( REGIONLOAD_OK == region.LoadFile(file, NULL, NULL) )
			{
				CWorldNode *pRoot = region.GetRootNode();
				CMoArray<CWorldNode*> nodeArray;

				if( pRoot )
				{
					region.FlattenTreeToArray(pRoot, nodeArray);
					int brushCount = 0;
					int i = 0;

					for( i = 0; i < nodeArray.GetSize(); i++ )
					{
						if( Node_Brush == nodeArray[i]->GetType() )
						{
							if( (exportObjFileDlg.m_exportHiddenBrushes) || 
								!(NODEFLAG_HIDDEN &  nodeArray[i]->GetFlags()) )
							{
								CEditBrush*	pBrush = nodeArray[i]->AsBrush();
								int j = 0;
								int k = 0;
								for( j = 0; j < pBrush->m_Points.GetSize(); j++ )
								{
									if(exportObjFileDlg.m_exportFor3DSMax)
									{
										LTVector v(pBrush->m_Points[j][0],pBrush->m_Points[j][2],pBrush->m_Points[j][1]);
										objInterface.m_vertexList.Append(v);
									}
									else
									{
										LTVector v(pBrush->m_Points[j][0],pBrush->m_Points[j][2],pBrush->m_Points[j][1]);
										objInterface.m_vertexList.Append(v);
									}
								}
							}
						}
				
					}

					int vertFound = 1;
					for( i = 0, vertFound = 1; i < nodeArray.GetSize(); i++ )
					{
						if( Node_Brush == nodeArray[i]->GetType() )
						{
							if( (exportObjFileDlg.m_exportHiddenBrushes) || 
								!(NODEFLAG_HIDDEN &  nodeArray[i]->GetFlags()) )
							{
								CEditBrush*			pBrush = nodeArray[i]->AsBrush();
							
								int j = 0;
								int k = 0;

								for( j = 0; j < pBrush->m_Polies.GetSize(); j++ )
								{
									CMoArray< int > iv;

									for( k = 0; k < pBrush->m_Polies[j]->m_Indices.GetSize(); k++ )
									{
										iv.Append(vertFound+(pBrush->m_Polies[j]->m_Indices[k]));
									}

									objInterface.m_faceSetList.Append(iv);
								}
								for( j = 0; j < pBrush->m_Points.GetSize(); j++ )
								{
									vertFound++;
								}
							}
						}
					}
				}
			}
		}


		char	pathName[256], fileName[256];
		char	worldName[256], extension[256], backupName[256];

		CHelpers::ExtractPathAndFileName( m_FileName, pathName, fileName );
		CHelpers::ExtractFileNameAndExtension( fileName, worldName, extension );

		CString objFilename;

		if( stricmp(extension, "obj" ) == 0 )
		{
			objFilename = m_FileName;
		}
		else
		{
			objFilename = m_FileName + ".obj";
		}

		FILE* objFile = fopen(LPCTSTR(objFilename), "wb");
		if(objFile)
		{
			objInterface.ExportObj(objFile, exportObjFileDlg.m_reverseNormals);
			fclose(objFile);
		}

		if( stricmp(extension, "obj" ) == 0 )
		{
			m_FileName = CString(worldName) + ".ed";
		}
		else
		{
			m_FileName;
		}
		SaveRegion(bFullSave);

		EndWaitCursor( ); // End the wait cursor
	}
	return FALSE;
}


// 0x44facf: 2001's runs "Processor" on the world's file name.
void CRegionDoc::StartPreProcessor()
{
	CMoFileIO	file;
	char		fileName[256], extension[256];
	char		szWorkingDirectory[MAX_PATH];
	char		szCommand[500];

	OnSaveDocument(NULL);
	CHelpers::ExtractFileNameAndExtension( GetPathName(), fileName, extension );

	try
	{
		SaveRegion(TRUE);

		// Save and then set the working directory
		GetCurrentDirectory(sizeof(szWorkingDirectory), szWorkingDirectory);
		SetCurrentDirectory(GetApp()->m_ExeDirectory);

		sprintf(szCommand, "\"%s\\Processor\" \"%s\"", GetApp()->m_ExeDirectory, fileName);

		if( WinExec(szCommand, SW_SHOW) <= 31 )
			AppMessageBox( IDS_ERRORRUNNINGPREPROCESSOR, MB_OK );  

		// Reset the current working directory
		SetCurrentDirectory(szWorkingDirectory);
	}
	catch( CMemoryException e )
	{
		AppMessageBox( IDS_OUTOFMEMORY_SAVING, MB_OK );
	}
}


CRegionView* CRegionDoc::CreateNewRegionView()
{
	CDocTemplate	*pDocTemp = GetApp()->m_pWorldTemplate;
	CFrameWnd		*pFrameWnd;
	
	pFrameWnd = pDocTemp->CreateNewFrame( this, NULL );
	pDocTemp->InitialUpdateFrame( pFrameWnd, this, TRUE );
	return (CRegionView*)pFrameWnd->GetActiveView();
}


// 0x44fc1f: an empty file is a new world (the name is ours).
BOOL CRegionDoc::LoadRegion(CAbstractIO &file)
{
	if( file.GetLen() )
	{
		file.SeekTo(0);
		if( m_Region.LoadFile(file, NULL, GetProject()) != REGIONLOAD_OK )
			return FALSE;

		m_Region.UpdateTextureIDs( GetProject() );
		m_Region.UpdatePolyTriangles();
		SynchronizePropertyLists(&m_Region);
		NotifySelectionChange();
	}

	return TRUE;
}


void CRegionDoc::Modify(BOOL bAddUndo, CPreAction *pAction, BOOL bDeleteAction)
{
	SetModifiedFlag( TRUE );
	SetTitle(m_WorldName + " *");

	if(bAddUndo && pAction)
		m_UndoMgr.PreAction(pAction);

	if(pAction && bDeleteAction)
		delete pAction;
}


void CRegionDoc::Modify(PreActionList *pActionList, BOOL bDeleteList)
{
	if(!pActionList || pActionList->GetSize() == 0)
		return;

	SetModifiedFlag( TRUE );
	SetTitle(m_WorldName + " *");

	m_UndoMgr.PreActions(pActionList, bDeleteList);
}


void CRegionDoc::SetupUndoForSelections(BOOL bOnlyOfType, int type)
{
	PreActionList actionList;
	DWORD i;

	for(i=0; i < m_Region.m_Selections; i++)
	{
		if(bOnlyOfType && m_Region.m_Selections[i]->GetType() == type)
		{
			actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, m_Region.m_Selections[i]));
		}
		else
		{
			actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, m_Region.m_Selections[i]));
		}
	}

	Modify(&actionList, TRUE);
}


void CRegionDoc::RedrawAllViews( DWORD extraHint )
{
	if(extraHint)
	{
		UpdateAllViews( NULL, extraHint );
	}
	else
	{
		UpdateAllViews( NULL, REGIONVIEWUPDATE_REDRAW );
	}
}


// 0x44fe1f: 2001's box is the brushes' points and the objects' positions,
// and the status bar shows it with its size.
void CRegionDoc::UpdateSelectionBox( LPARAM extraHint )
{
	DWORD			i, k;
	CWorldNode		*pNode;
	CEditBrush		*pBrush;

	m_SelectionMin.Init( (CReal)MAX_CREAL, (CReal)MAX_CREAL, (CReal)MAX_CREAL );
	m_SelectionMax.Init( (CReal)-MAX_CREAL, (CReal)-MAX_CREAL, (CReal)-MAX_CREAL );

	for( i=0; i < m_Region.m_Selections; i++ )
	{
		pNode = m_Region.m_Selections[i];
		if(pNode->GetType() == Node_Brush)
		{
			pBrush = pNode->AsBrush();

			for( k=0; k < pBrush->m_Points; k++ )
			{
				CEditVert &vPt = pBrush->m_Points[k];

				if( vPt.x < m_SelectionMin.x )	m_SelectionMin.x = vPt.x;
				if( vPt.y < m_SelectionMin.y )	m_SelectionMin.y = vPt.y;
				if( vPt.z < m_SelectionMin.z )	m_SelectionMin.z = vPt.z;

				if( vPt.x > m_SelectionMax.x )	m_SelectionMax.x = vPt.x;
				if( vPt.y > m_SelectionMax.y )	m_SelectionMax.y = vPt.y;
				if( vPt.z > m_SelectionMax.z )	m_SelectionMax.z = vPt.z;
			}
		}
		else if(pNode->GetType() == Node_Object)
		{
			CVector &vPt = pNode->GetPos();

			if( vPt.x < m_SelectionMin.x )	m_SelectionMin.x = vPt.x;
			if( vPt.y < m_SelectionMin.y )	m_SelectionMin.y = vPt.y;
			if( vPt.z < m_SelectionMin.z )	m_SelectionMin.z = vPt.z;

			if( vPt.x > m_SelectionMax.x )	m_SelectionMax.x = vPt.x;
			if( vPt.y > m_SelectionMax.y )	m_SelectionMax.y = vPt.y;
			if( vPt.z > m_SelectionMax.z )	m_SelectionMax.z = vPt.z;
		}
	}

	CString str;
	if( m_Region.m_Selections != 0 )
	{
		str.Format( "(%1.0f, %1.0f, %1.0f)->(%1.0f, %1.0f, %1.0f) = (%1.0f, %1.0f, %1.0f)",
			m_SelectionMin.x, m_SelectionMin.y, m_SelectionMin.z,
			m_SelectionMax.x, m_SelectionMax.y, m_SelectionMax.z,
			m_SelectionMax.x - m_SelectionMin.x,
			m_SelectionMax.y - m_SelectionMin.y,
			m_SelectionMax.z - m_SelectionMin.z );
	}

	CMainFrame *pFrame = (CMainFrame*)AfxGetMainWnd();
	pFrame->UpdateStatusText( ID_INDICATOR_BOUNDING_BOX, str );

	RedrawAllViews( extraHint );
}


void CRegionDoc::NotifySelectionChange()
{
	SetupPropertiesDlg(FALSE);
	UpdateSelectionBox();
}

BEGIN_MESSAGE_MAP(CRegionDoc, CDocument)
	//{{AFX_MSG_MAP(CRegionDoc)
		ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
		ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
		ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
		ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
		ON_UPDATE_COMMAND_UI(ID_EDIT_STAMP, OnUpdateEditStamp)
		ON_COMMAND(ID_EDIT_STAMP, OnEditStamp)
	ON_COMMAND(ID_EDIT_PASTE_ALTERNATE, OnEditPasteAlternate)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE_ALTERNATE, OnUpdateEditPasteAlternate)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRegionDoc diagnostics

#ifdef _DEBUG
void CRegionDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CRegionDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CRegionDoc serialization

void CRegionDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CRegionDoc commands

BOOL CRegionDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;
	
	return TRUE;
}


// 0x450064
BOOL CRegionDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	if( !SaveRegion(TRUE))
		return FALSE;

	SetModifiedFlag( FALSE );
	SetTitle( m_WorldName );
	
	return TRUE;
}


// 0x45008c: the backup is a temporary file (GetTempFileName).
BOOL CRegionDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	CMoFileIO	file;
	CString		str;
	char	pathName[_MAX_PATH], fileName[_MAX_PATH];
	char	worldName[_MAX_PATH], extension[_MAX_PATH], backupName[_MAX_PATH];

	CHelpers::ExtractPathAndFileName( lpszPathName, pathName, fileName );
	CHelpers::ExtractFileNameAndExtension( fileName, worldName, extension );

	m_WorldName = worldName;
	m_FileName = lpszPathName;

	GetTempPath(sizeof(pathName), pathName);
	fileName[3] = '\0';
	GetTempFileName(pathName, fileName, 0, backupName);
	m_BackupName = backupName;

	if( file.Open(lpszPathName, "rb") )
	{
		if( LoadRegion(file) )
		{
			file.Close();
			SetModifiedFlag( TRUE );
			return TRUE;
		}

		str.FormatMessage(IDS_CORRUPTED_FILE, (LPCTSTR)worldName);
	}
	else
	{
		str.FormatMessage(IDS_ERROR_OPENING_WORLD, (LPCTSTR)worldName);
	}

	// NEAR (338 of 338 bytes): the shipped build clears the return register
	// before the AppMessageBox call, ours after it; a result local and an
	// AppMessageBox in each arm (382 bytes) don't reach it.
	AppMessageBox( str, MB_OK );
	return FALSE;
}



void CRegionDoc::OnCloseDocument() 
{
	// Make sure the properties dialog isn't sitting on an invalid object.
	for( DWORD i=0; i < m_Region.m_Objects; i++ )
		GetPropertiesDlg()->TermIf( &m_Region.m_Objects[i]->m_PropList );

	if( this == GetNodeView( )->GetDoc( ))
		GetNodeView( )->Term( );

	CDocument::OnCloseDocument();
}



void CRegionDoc::OnUpdateEditCopy( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( m_Region.m_Selections.GetSize( ) != 0 );
}


void CRegionDoc::OnEditCopy()
{
	(( CMainFrame * )AfxGetMainWnd( ))->GetClipboard( )->Copy( GetRegion() );
}


void CRegionDoc::OnUpdateEditPaste( CCmdUI *pCmdUI )
{
	pCmdUI->Enable((( CMainFrame * )AfxGetMainWnd( ))->GetClipboard( )->IsFull( ));
}


void CRegionDoc::OnEditPaste()
{
	PreActionList actionList;

	(( CMainFrame * )AfxGetMainWnd( ))->GetClipboard( )->Paste( GetRegion(), actionList, TRUE );

	// Setup the undo for the new nodes it added.
	Modify(&actionList, TRUE);
	NotifySelectionChange();
}



void CRegionDoc::OnUpdateEditStamp( CCmdUI *pCmdUI )
{
	pCmdUI->Enable( m_Region.m_Selections.GetSize( ) != 0 );
}


void CRegionDoc::OnEditStamp()
{
	PreActionList actionList;

	(( CMainFrame * )AfxGetMainWnd( ))->GetClipboard( )->Copy( GetRegion() );
	(( CMainFrame * )AfxGetMainWnd( ))->GetClipboard( )->Paste(GetRegion(), actionList, TRUE);
	Modify(&actionList, TRUE);

	NotifySelectionChange();
}




// --------------------------------------------------------------- //
// Goes thru all the nodes and synchronizes their property lists
// with their classes.  Returns FALSE if the user says not to 
// synchronize the lists.
// 0x45035a: 2001's records an undo for each node it changes, leaves
// Zones alone, and copies a static list's first string in.
// --------------------------------------------------------------- //

static BOOL RecurseAndSynchronize(CRegionDoc *pDoc, CEditRegion *pRegion, CWorldNode *pNode, PreActionList *pActionList)
{
	DWORD			j, k, nProps, propIndex;
	CPropList		*pList;
	ClassDef		*pClass;
	CBaseProp		*pProp;
	PropDef			*pVar;
	char			szDebug[64];

	CWorldNode *pChild;
	GPOS pos;
	CMoArray<PropDef*> *pPropList;


	for(pos=pNode->m_Children; pos; )
	{
		pChild = pNode->m_Children.GetNext(pos);

		if(!RecurseAndSynchronize(pDoc, pRegion, pChild, pActionList))
			return FALSE;
	}


	pList = &pNode->m_PropList;
	pClass = GetProject()->FindClassDef(pNode->GetClassName());
	
	if(pClass && stricmp(pClass->m_ClassName, "Zone") && (pPropList = GetProject()->GetClassDefProps(pClass)))	// If the class exists, sync stuff up.
	{
		// Remove variables that don't exist anymore.
		nProps = pList->m_Props;
		for( j=0; j < nProps; j++ )
		{
			pProp = pList->m_Props[j];
			
			if(!GetProject()->FindPropInList(*pPropList, pProp->m_Name, NULL))
			{
				sprintf(szDebug,"Unknown prop name: %s",pProp->m_Name);
				AddDebugMessage(szDebug);

				// Make sure they want to synchronize things.
				if( !pDoc->m_bCanSynchronize && !pDoc->CheckSynchronize() )
					return FALSE;

				AddToActionListIfNew(pActionList, new CPreAction(ACTION_MODIFYNODE, pNode), TRUE);
				pDoc->SetModifiedFlag();
				delete pList->m_Props[j];
				pList->m_Props.Remove(j);
				--j;
				nProps--;
			}
		}

		// Add variables that are new.
		for(k=0; k < pPropList->GetSize(); k++)
		{
			pVar = pPropList->Get(k);

			pProp = pList->GetProp(pVar->m_PropName, FALSE, &propIndex);
			if(pProp)
			{
				if(pProp->m_Type != pVar->m_PropType)
				{
					sprintf(szDebug,"Unknown prop type: %s",pProp->m_Name);
					AddDebugMessage(szDebug);

					// Make sure they want to synchronize things.
					if( !pDoc->m_bCanSynchronize && !pDoc->CheckSynchronize() )
						return FALSE;
				
					delete pProp;
					AddToActionListIfNew(pActionList, new CPreAction(ACTION_MODIFYNODE, pNode), TRUE);

					pProp = CreatePropFromCode(pVar->m_PropType, pVar->m_PropName);
					if(pProp)
					{
						SetupNewProp(pProp, pVar);
						pDoc->SetModifiedFlag();
						pList->m_Props[propIndex] = pProp;
					}
					else
						pList->m_Props.Remove(propIndex);
				}
				// {BP 1/19/98}
				// Make sure the propflags jive...
				if(pProp->m_PropFlags != pVar->m_PropFlags)
				{
					sprintf(szDebug,"Unknown prop flags: %s",pProp->m_Name);
					AddDebugMessage(szDebug);

					// Make sure they want to synchronize things.
					if( !pDoc->m_bCanSynchronize && !pDoc->CheckSynchronize() )
						return FALSE;
				
					AddToActionListIfNew(pActionList, new CPreAction(ACTION_MODIFYNODE, pNode), TRUE);
					pProp->m_PropFlags = pVar->m_PropFlags;
				}

				// If it's a static list, make sure the value matches something in the list
				DEditInternal* pDEditInternal = pVar->m_pDEditInternal;
				if(pDEditInternal && (pVar->m_PropFlags & PF_STATICLIST))
				{
					const char* szString = ((CStringProp*)pProp)->m_String;
					bool bMatch = false;
					for ( int iString = 0 ; iString < pDEditInternal->m_cStrings ; iString++ )
					{
						if ( !strcmp(szString, pDEditInternal->m_aszStrings[iString]) )
						{
							bMatch = TRUE;
							break;
						}
					}
					if ( !bMatch )
					{
						sprintf(szDebug,"Unmatchable string for static list: %s",pProp->m_Name);
						AddDebugMessage(szDebug);

						// Make sure they want to synchronize things.
						if( !pDoc->m_bCanSynchronize && !pDoc->CheckSynchronize() )
							return FALSE;

						AddToActionListIfNew(pActionList, new CPreAction(ACTION_MODIFYNODE, pNode), TRUE);
						strcpy(((CStringProp*)pProp)->m_String, pDEditInternal->m_aszStrings[0]);
					}
				}
			}
			else
			{
				// Make sure they want to synchronize things.
				if( !pDoc->m_bCanSynchronize && !pDoc->CheckSynchronize() )
					return FALSE;

				AddToActionListIfNew(pActionList, new CPreAction(ACTION_MODIFYNODE, pNode), TRUE);
				pProp = CreatePropFromCode(pVar->m_PropType, pVar->m_PropName);
				if(pProp)
				{
					SetupNewProp(pProp, pVar);
					pDoc->SetModifiedFlag();
					pList->m_Props.Append(pProp);
				}
			}
		}
	}

	return TRUE;
}


// 0x450765: the changes go in as one undo.
void CRegionDoc::SynchronizePropertyLists( CEditRegion *pRegion )
{
	PreActionList actionList;
	RecurseAndSynchronize(this, pRegion, pRegion->GetRootNode(), &actionList);
	Modify(&actionList, TRUE);
	
	m_bCanSynchronize = FALSE;
}


BOOL CRegionDoc::CheckSynchronize()
{
	if( AppMessageBox(IDS_OUTOFDATEOBJECTS, MB_YESNO) == IDYES )
	{
		m_bCanSynchronize = TRUE;
	}
	else
	{
		m_bCanSynchronize = FALSE;
	}

	return m_bCanSynchronize;
}


// Adds an object to the document
// NEAR (227 vs 220 bytes, 0x4507e5): 2001's CWorldNode::SetPos takes the
// vector by value (0x450882 copies it to the stack before 0x46f282);
// WorldNode.h's takes a reference.
CBaseEditObj *CRegionDoc::AddObject(const char *pClassName, CVector vPos)
{
	CBaseEditObj *pObj = new CBaseEditObj;
	CEditRegion *pRegion=GetRegion();
	if(SetupWorldNode(pClassName, pObj, pRegion))
	{
		if (!pRegion)
		{
			return NULL;
		}

		// Add the object to the region
		// talon: 2001's CEditRegion::AddObject is inline, m_Objects.Append
		// (0x450838; EditRegion.h has it out of line): written out here.
		pRegion->m_Objects.Append(pObj);
		pRegion->AddNodeToRoot( pObj->AsNode() );

		Modify(TRUE, new CPreAction(ACTION_ADDEDNODE, pObj), TRUE);

		// Set the objects position
		pObj->SetPos(vPos);

		// Add the object to the node view		
		GetNodeView( )->AddNode( pObj->AsNode() );

		// Return the object
		return pObj;
	}
	else
	{
		delete pObj;
		return NULL;
	}
}

// Binds an object
void CRegionDoc::BindNode(CWorldNode *pChild, CWorldNode *pParent, PreActionList *pActionList)
{
	CEditRegion *pRegion = GetRegion();
	
	if (pActionList)
		pActionList->AddTail(new CPreAction(ACTION_MODIFYNODE, pChild));
	else
		GetNodeView( )->GetDoc( )->Modify(TRUE, new CPreAction(ACTION_MODIFYNODE, pChild), TRUE );
	pRegion->AttachNode( pChild, pParent );
}

// Binds an object to the selected nodes
void CRegionDoc::BindNodeToSelected(CWorldNode *pNode, PreActionList *pActionList)
{
	CEditRegion *pRegion = GetRegion();

	unsigned int i;
	for(i = 0; i < pRegion->m_Selections.GetSize(); i++)
	{
		CWorldNode *pChild = pRegion->m_Selections[i];

		// Attach node to new parent if it is not a container
		if (pChild->GetType() != Node_Null)
		{
			BindNode(pChild, pNode, pActionList);
		}
	}
}

// Selects a node and updates the views
void CRegionDoc::SelectNode(CWorldNode *pNode, BOOL bClearSelections)
{
	CEditRegion *pRegion = GetRegion();
	if (!pRegion)
	{
		return;
	}

	if (bClearSelections)
	{
		pRegion->ClearSelections( );
	}

	pRegion->SelectNode(pNode);
	NotifySelectionChange( );
	RedrawAllViews( );
}

/************************************************************************/
// Moves the selected nodes to the indicated parent node.
// This sets up the undo information as well.
// 0x4509b3: 2001's works on the region's selections in place.
void CRegionDoc::MoveSelectedNodes(CWorldNode *pParentNode)
{	
	// Get the region
	CEditRegion *pRegion=GetRegion();
	if (!pRegion)
	{
		return;
	}	

	// Get the selected nodes
	CMoArray<CWorldNode*> &selectedNodes = pRegion->m_Selections;
		
	CWorldNode::FindParentNodes(selectedNodes);

	// Setup the undo information
	PreActionList actionList;	
	DWORD i;
	for(i=0; i < selectedNodes.GetSize(); i++)
	{
		if (selectedNodes[i] != pParentNode)
			actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, selectedNodes[i]));
	}
	Modify(&actionList, TRUE);

	// Attach the topmost nodes to the parent, children will follow...
	for( i = 0; i < selectedNodes.GetSize( ); i++ )
	{
		CWorldNode *pNode = selectedNodes[i];
		if (pNode != pParentNode)
			pRegion->AttachNode( pNode, pParentNode);
	}
}

/************************************************************************/
// Converts an object from one class to another.  A pointer to the object is then returned.
// 0x450a77
CBaseEditObj *CRegionDoc::ConvertObjectClass(CBaseEditObj *pOldObject, const char *lpszClassName)
{
	// Create the new object at the coordinates of the one being converted
	CBaseEditObj *pNewNode=AddObject(lpszClassName, pOldObject->GetPos());

	// Attach the node to the parent
	BindNode(pNewNode, pOldObject->GetParent());

	// Move the child attachments over to the new node
	GPOS gPos;
	for (gPos=pOldObject->m_Children; gPos; )
	{
		BindNode(pOldObject->m_Children.GetNext(gPos), pNewNode);
	}

	// Copy the property values from the old list to the new list
	pNewNode->m_PropList.CopyMatchingValues(&pOldObject->m_PropList);

	// Copy the flags
	pNewNode->SetFlags(pOldObject->GetFlags());

	// Clear the selection flag if it is set
	if (pNewNode->IsFlagSet(NODEFLAG_SELECTED))
	{
		pNewNode->ClearFlag(NODEFLAG_SELECTED);
	}
		
	// Update the label in the node view
	GetNodeView()->UpdateTreeItemLabel(pNewNode);

	return pNewNode;
}

void CRegionDoc::OnEditPasteAlternate() 
{
	PreActionList actionList;

	(( CMainFrame * )AfxGetMainWnd( ))->GetClipboard( )->Paste( GetRegion(), actionList, FALSE);

	// Setup the undo for the new nodes it added.
	Modify(&actionList, TRUE);
	NotifySelectionChange();		
}

void CRegionDoc::OnUpdateEditPasteAlternate(CCmdUI* pCmdUI) 
{
	pCmdUI->Enable((( CMainFrame * )AfxGetMainWnd( ))->GetClipboard( )->IsFull( ));
}


// ---------------------------------------------------------------------- //
// talon: 2001's lightmap editing.  The world's processed file (its .dat)
// is loaded into a CPreMainWorld; the base light animation's first frame
// gives each poly its lightmap, and the texels selected in the views
// (m_SelTexels, on the poly m_LightPolyRef) can be made brighter or
// darker or given the colour dialog's colour, then saved back.  The names
// are ours.
// ---------------------------------------------------------------------- //




// 0x450b6a: the world's file name with its extension (IDS_WORLDEXTENSION)
// replaced by IDS_DAT_EXTENSION.
CString CRegionDoc::GetLightAnimFileName()
{
	CString	worldExt, datExt;
	char	fileName[1024], *pExt;

	strcpy(fileName, m_FileName);

	worldExt.LoadString(IDS_WORLDEXTENSION);
	pExt = strstr(fileName, worldExt);
	if(pExt)
		*pExt = 0;

	datExt.LoadString(IDS_DAT_EXTENSION);
	strcat(fileName, datExt);

	return CString(fileName);
}


// 0x450c20
void CRegionDoc::LoadLightAnims()
{
	CMoFileIO	file;
	CString		fileName, str;
	CPreLightAnim	*pAnim;
	CPreLightFrame	*pFrame;
	CPrePoly		*pPoly;
	uint32		i;

	TermLightAnims();

	fileName = GetLightAnimFileName();
	if(file.Open(fileName, "rb"))
	{
		m_pLightWorld = new CPreMainWorld;
		if(m_pLightWorld->LoadFile(file, TRUE) == 0)
		{
			pAnim = m_pLightWorld->FindLightAnim(BASE_LIGHTANIM_NAME);
			if(pAnim && pAnim->m_Frames.GetSize() > 0)
			{
				pFrame = pAnim->m_Frames[0];
				if(pFrame->m_PolyMaps.GetSize() == pAnim->m_Polies.GetSize())
				{
					for(i=0; i < pAnim->m_Polies.GetSize(); i++)
					{
						pPoly = m_pLightWorld->GetLMPoly(&pAnim->m_Polies[i]);
						if(pPoly)
							pPoly->m_Unknown_24 = pFrame->m_PolyMaps[i];
					}
				}
			}
		}
		else
		{
			str.FormatMessage(IDS_ERROR_LOADING_DAT, (LPCTSTR)fileName);
			AppMessageBox(str, MB_OK);

			delete m_pLightWorld;
			m_pLightWorld = NULL;
		}
	}
	else
	{
		str.FormatMessage(IDS_ERROR_OPENING_DAT, (LPCTSTR)fileName);
		AppMessageBox(str, MB_OK);
	}
}


// 0x450dc4
void CRegionDoc::TermLightAnims()
{
	if(m_pLightWorld)
	{
		d3d_UnbindWorld(m_pLightWorld);
		delete m_pLightWorld;
		m_pLightWorld = NULL;
	}
}


// 0x450dfb (the function table has it inside 0x450dc4)
void CRegionDoc::SaveLightAnims()
{
	CString		fileName, str;
	CMoFileIO	file;

	if(m_pLightWorld)
	{
		fileName = GetLightAnimFileName();
		if(file.Open(fileName, "wb"))
		{
			if(!m_pLightWorld->SaveFile(file))
			{
				str.FormatMessage(IDS_ERROR_SAVING_DAT, (LPCTSTR)fileName);
				AppMessageBox(str, MB_OK);
			}

			file.Close();
		}
		else
		{
			str.FormatMessage(IDS_ERROR_OPENING_DAT, (LPCTSTR)fileName);
			AppMessageBox(str, MB_OK);
		}
	}
}


// 0x450ef8: one step brighter (or darker) on each selected texel.
// NEAR (337 vs 318 bytes): the shipped build keeps 0xFF in ecx and the
// add in ebx, so it spills this, the poly, the width and the x to the
// frame (named width/x locals and an nMax local tried; the rest of the
// shape, the clamps and the recombination, is the same).
void CRegionDoc::BrightenTexels(BOOL bBrighter)
{
	uint32		data[1024];
	CPrePoly	*pPoly;
	CPreLightMap *pMap;
	uint32		i;
	int			nMax = 0xFF;

	if(!m_pLightWorld)
		return;

	pPoly = m_pLightWorld->FindPoly(m_LightPolyWorld, m_LightPolyName);
	if(!pPoly)
		return;

	pMap = pPoly->m_Unknown_24;
	if(!pMap)
		return;

	if(!pMap->Decompress(data))
		return;

	for(i=0; i < m_SelTexels.GetSize(); i++)
	{
		CTexelRef &texel = m_SelTexels[i];

		if(texel.m_X < pPoly->m_LMWidth && texel.m_Y < pPoly->m_LMHeight)
		{
			uint32 *pColor = &data[texel.m_Y * pPoly->m_LMWidth + texel.m_X];
			int b = *pColor & nMax;
			int g = (*pColor >> 8) & nMax;
			int nAdd = bBrighter ? 1 : -1;
			int r = ((*pColor >> 16) & nMax) + nAdd;

			if(r < 0)			r = 0;
			else if(r > nMax)	r = nMax;

			g += nAdd;
			if(g < 0)			g = 0;
			else if(g > nMax)	g = nMax;

			b += nAdd;
			if(b < 0)			b = 0;
			else if(b > nMax)	b = nMax;

			*pColor = (((r << 8) | g) << 8) | b;
		}
	}

	if(pMap->CompressLightMap(pPoly->m_LMWidth, pPoly->m_LMHeight, data))
	{
		d3d_UnbindPrePoly(pPoly);
		RedrawAllViews(REGIONVIEWUPDATE_LIGHTMAPS);
	}
}


// 0x451049: the colour dialog's colour on each selected texel.
void CRegionDoc::SetTexelColor(COLORREF newColor)
{
	uint32		data[1024];
	CPrePoly	*pPoly;
	CPreLightMap *pMap;
	uint32		i, color;

	if(!m_pLightWorld)
		return;

	pPoly = m_pLightWorld->FindPoly(m_LightPolyWorld, m_LightPolyName);
	if(!pPoly)
		return;

	pMap = pPoly->m_Unknown_24;
	if(!pMap)
		return;

	color = (((GetRValue(newColor) << 8) | GetGValue(newColor)) << 8) | GetBValue(newColor);
	if(!pMap->Decompress(data))
		return;

	for(i=0; i < m_SelTexels.GetSize(); i++)
	{
		CTexelRef &texel = m_SelTexels[i];

		if(texel.m_X < pPoly->m_LMWidth && texel.m_Y < pPoly->m_LMHeight)
			data[texel.m_Y * pPoly->m_LMWidth + texel.m_X] = color;
	}

	if(pMap->CompressLightMap(pPoly->m_LMWidth, pPoly->m_LMHeight, data))
	{
		d3d_UnbindPrePoly(pPoly);
		RedrawAllViews(REGIONVIEWUPDATE_LIGHTMAPS);
	}
}



// ---------------------------------------------------------------------- //
// talon: Build 69 only, kept while other files still call them (none of
// them is in the 2001 image).
// ---------------------------------------------------------------------- //

void CRegionDoc::RedrawPerspectiveViews(DWORD extraHint)
{
	POSITION Pos = GetFirstViewPosition();

	//run through all views of this document
	CView* pView;
	while(pView = GetNextView(Pos))
	{
		CRegionView* pRegionView = (CRegionView*)pView;

		//see if this is a perspective view
		if(pRegionView->GetViewMode() == VM_PERSPECTIVE)
		{
			//it is, so trigger the update
			pRegionView->OnUpdate(NULL, (extraHint) ? extraHint : REGIONVIEWUPDATE_REDRAW, NULL);
		}
	}
}


// Clone the selected objects and select the new version
void CRegionDoc::Clone()
{
	// Note : This is lame that it goes through the clipboard, but that was the easy way to implement it..
	OnEditCopy();
	OnEditPasteAlternate();
}

// Select the brushes which are using a given texture
void CRegionDoc::SelectBrushesByTexture(const char *pFilename)
{
	// Get our region
	CEditRegion *pRegion = GetRegion();
	if (!pRegion)
		return;

	// Get the geometry mode state
	POSITION firstPos = GetFirstViewPosition();
	CRegionView *pFirstView = (CRegionView*)GetNextView(firstPos);
	bool bInGeometryMode = (pFirstView && (pFirstView->GetEditMode() == GEOMETRY_EDITMODE));

	// This might take a sec..
	BeginWaitCursor();

	// For each brush..
	for (LPOS iCurBrush = pRegion->m_Brushes.GetHeadPosition(); iCurBrush; )
	{
		CEditBrush *pBrush = pRegion->m_Brushes.GetNext(iCurBrush);

		// For each polygon...
		for (uint32 nPolyLoop = 0; nPolyLoop < pBrush->m_Polies.GetSize(); ++nPolyLoop)
		{
			bool bMatched = false;

			//for each texture
			for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
			{
				// Select this brush if it's got the right texture on it
				if (stricmp(pFilename, pBrush->m_Polies[nPolyLoop]->GetTexture(nCurrTex).m_pTextureName) == 0)
				{
					// Select the brush
					pRegion->SelectNode(pBrush);
					bMatched = true;

					if (bInGeometryMode)
					{
						// Tag the poly if we're in geometry mode
						TagPoly(CPolyRef(pBrush, nPolyLoop));
					}
					
					// Don't bother searching any more texture channels
					break;
				}
			}

			//we don't need to look further if we already matched a poly in this brush and aren't in geo mode
			if(bMatched && !bInGeometryMode)
				break;
		}
	}

	// Redraw stuff..
	NotifySelectionChange( );
	RedrawAllViews( );

	EndWaitCursor();
}

void CRegionDoc::TagPoly(const CPolyRef &cPoly)
{
	for (uint32 nCurPoly = 1; nCurPoly < m_TaggedPolies.GetSize(); ++nCurPoly)
	{
		if (m_TaggedPolies[nCurPoly] == cPoly)
			return;
	}

	m_TaggedPolies.Add(cPoly);
}
