//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
#ifndef __REGIONDOC_H__
#define __REGIONDOC_H__


// RegionDoc.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRegionDoc document


#include "editpoly.h"
#include "editregion.h"
#include "editbrush.h"
#include "refs.h"
#include "editprojectmgr.h"
#include "propertiesdlg.h"
#include "editclipboard.h"
#include "undo_mgr.h"

class CEditRegion;

typedef CLinkedList<CVertRef> VertRefLinkedList;
typedef GenHash<CVertRef, VertRefLinkedList> VertRefHash;


// Used for the CVertRef hashed table.
class VertRefHashHelper : public GenHashHelper<CVertRef>
{
public:
	
	// talon: GenHashHelper's DWORD is unsigned long; oldtypes.h has made
	// DWORD uint32 by here
	virtual unsigned long	GetHashCode(const CVertRef &theObject)
	{
		return (DWORD)theObject.m_iVert + ((DWORD)theObject.m_pBrush >> 2);
	}

	virtual BOOL	Compare(const CVertRef &object1, const CVertRef &object2)
	{
		return object1.m_pBrush == object2.m_pBrush &&
			object1.m_iVert == object2.m_iVert;
	}
};		


struct BatchTextureScaleInfo;
class CPreMainWorld;

// talon: a selected lightmap texel (8 bytes, 0x450f7a; the names are ours).
class CTexelRef
{
public:
	// talon: FindElement's compare (0x413e00): m_X, then m_Y.
	BOOL		operator==(const CTexelRef &other) const	{ return m_X == other.m_X && m_Y == other.m_Y; }

	uint32		m_X, m_Y;
};

class CRegionDoc : public CDocument, public CNotifier
{
	friend class CProjectBar;

protected:
	CRegionDoc();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CRegionDoc)




// Attributes
public:

	void				NotifyPropertyChange( CBaseProp *pProp, BOOL bUpdateView );
	void				NotifyPropertiesChange( BOOL bUpdateView );


	void				InitUndoBuffers();
	void				TermUndoBuffers();

	//called to have all the properties in objects checked in the proper order for reporting
	//errors back to the debug window
	void				UpdateAllObjectProperties();

	// talon: 2001's saves (0x44f4dc, the name ours; 0x44f5f1).
	BOOL				SaveRegion(BOOL bFullSave);
	BOOL				SaveOBJ(BOOL bFullSave);


	void				StartPreProcessor();
	BOOL				SaveModifiedLTA() { return SaveModified(); }

	CRegionView*		CreateNewRegionView();
 
	// talon: 0x44fc1f, the name ours.
	BOOL				LoadRegion(CAbstractIO &file);

	CEditRegion*		GetRegion()		{ return &m_Region; }

	// Sets the modified flag and (possibly) adds an undo to the buffer.
	// talon: 2001's takes the undo flag first (0x44fc87; its callers push
	// all three, e.g. 0x43ca74 1, action, 1 and 0x417f0a 0, 0, 0); the
	// flags are BOOLs (cmpl at 0x44fcc9, 0x44fce6).  The defaults are ours.
	void				Modify(BOOL bAddUndo=FALSE, CPreAction *pAction=NULL, BOOL bDeleteAction=FALSE);
	
	// Modify with an action list.  If you pass in NULL or a list with 0 actions, it
	// won't modify the document.
	void				Modify(PreActionList *pActionList, BOOL bDeleteList=FALSE);

	// Automatically sets up an (ACTION_MODIFYNODE) undo list for all the current selections.
	void				SetupUndoForSelections(BOOL bOnlyOfType=FALSE, int type=0);

	//invalidates all views of this document causing them to be redrawn
	void				RedrawAllViews( DWORD extraHint=0 );

	//invalidates all perspective views of this document causing them to
	//be redrawn. This should be done for texture operations, and other
	//things which will not be reflected in wireframe modes
	void				RedrawPerspectiveViews(DWORD extraHint=0 );

	void				UpdateSelectionBox( LPARAM extraHint=0 );
	void				NotifySelectionChange();
	void				SetupPropertiesDlg(bool bShow);

	void				SynchronizePropertyLists( CEditRegion *pRegion );
	BOOL				CheckSynchronize();
	
	// Adds an object to the document
	CBaseEditObj		*AddObject(const char *pClassName, CVector vPos);

	// Binds an object
	void				BindNode(CWorldNode *pChild, CWorldNode *pParent, PreActionList *pActionList = NULL);
	void				BindNodeToSelected(CWorldNode *pNode, PreActionList *pActionList = NULL);

	// Selects a node
	void				SelectNode(CWorldNode *pNode, BOOL bClearSelections=TRUE);
	
	// Moves the selected nodes to the indicated parent node.
	// This sets up the undo information as well.
	void				MoveSelectedNodes(CWorldNode *pParentNode);

	// Converts an object from one class to another.  A pointer to the object is then returned.
	CBaseEditObj		*ConvertObjectClass(CBaseEditObj *pOldObject, const char *lpszClassName);

	// Clone the selected nodes (Note : This uses the clipboard to make the clone)
	void				Clone();
 
	// Select the brushes which are using a given texture
	void				SelectBrushesByTexture(const char *pFilename);

	// Tag the indicated polygon.  Note that this handles filtering out polys that
	// are already tagged.
	void				TagPoly(const CPolyRef &cPoly);

	// talon: 2001's lightmap editing (0x450b6a..0x451049; the names are
	// ours): the world's .dat file, its base light animation's lightmaps,
	// and the selected texels.
	CString				GetLightAnimFileName();
	void				LoadLightAnims();
	void				TermLightAnims();
	void				SaveLightAnims();
	void				BrightenTexels(BOOL bBrighter);
	void				SetTexelColor(COLORREF newColor);



public:	

	// Used while attempting to synchronize property lists.
	// talon: 2001's is a BOOL at 0x54, right after the bases, and m_WorldName
	// follows at 0x58 (0x4507d3 movl $1,0x54; 0x44fc9d lea 0x58): Build 69's
	// saved splitter centre (two floats before it) is not there.
	BOOL				m_bCanSynchronize;

	// World name (like 'World1')
	CString				m_WorldName;
	
	// The full filename for the world (like 'c:\proj\blah\World1.lta').
	CString				m_FileName;
	CString				m_BackupName;

	// Current selection boundaries.
	CVector				m_SelectionMin, m_SelectionMax;


	// All the selection stuff.
	CVertRef					m_ImmediateVertexTag;
	CEdgeRef					m_ImmediateEdgeTag;
	CBrushRef					m_ImmediateBrushTag;
	
	CEditBrush					m_DrawingBrush;
	
	VertRefHash					m_TaggedVerts;
	VertRefHashHelper			m_TaggedVertsHelper;

	CBrushRefArray				m_TaggedBrushes;
	
	// Note: there is ALWAYS one entry in here.  It is the immediate poly.
	CPolyRefArray				m_TaggedPolies;

	// talon: 2001's (0x184): the world's .dat file, loaded by
	// LoadLightAnims; the name is ours.
	CPreMainWorld				*m_pLightWorld;


public:

	CEditRegion					m_Region;
	CStringHolder				m_RegionStringHolder;


	// Backups for the undo buffer.
	CUndoMgr					m_UndoMgr;

	// talon: 2001's lightmap texel selection (0x31c..0x33c; the names are
	// ours).  What the string is for is not known yet: only the
	// constructor and destructor touch it here.
	CString						m_Unknown_31c;
	uint32						m_LightPolyWorld;	// the poly's world model and
	uint32						m_LightPolyName;	// m_nName, -1 for none (0x44f158)
	CMoArray<CTexelRef>			m_SelTexels;
			
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRegionDoc)
	public:
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void OnCloseDocument();
	protected:
	virtual BOOL OnNewDocument();
	virtual BOOL SaveModified();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CRegionDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CRegionDoc)
	afx_msg void	OnUpdateEditCopy( CCmdUI *pCmdUI );
	afx_msg void	OnEditCopy();
	afx_msg void	OnUpdateEditCut( CCmdUI *pCmdUI );
	afx_msg void	OnEditCut();
	afx_msg void	OnUpdateEditPaste( CCmdUI *pCmdUI );
	afx_msg void	OnEditPaste();
	afx_msg void	OnUpdateEditStamp( CCmdUI *pCmdUI );
	afx_msg void	OnEditStamp();
afx_msg void OnEditPasteAlternate();
afx_msg void OnUpdateEditPasteAlternate(CCmdUI* pCmdUI);
//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


#endif  // __REGIONDOC_H__


