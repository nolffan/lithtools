//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : EditObjects.h
//
//	PURPOSE	  : Defines all the edit object stuff.
//
//	CREATED	  : November 28 1996
//
//
//------------------------------------------------------------------

#ifndef __EDITOBJECTS_H__
#define __EDITOBJECTS_H__

// Includes....
#include "editvert.h"
#include "proplist.h"
#include "worldnode.h"
#include "classbind.h"

#ifdef DIRECTEDITOR_BUILD
#	include "modelhandle.h"
#	include "iltpreinterface.h"
#endif



class CBaseEditObj;
class CEditRegion;
class TemplateClass;
class CLTANode;
class CLTANodeBuilder;
class CLTAFile;
class CModelMgr;

#define MAX_OBJECTNAME_LEN		30
extern char *g_NameName, *g_PosName, *g_AnglesName;
extern CVector g_DummyPos;
extern LTVector g_DummyRotation;
extern CStringProp g_DummyString;

class CBaseEditObj;
typedef CMoArray<CBaseEditObj*> BaseEditObjArray;



void GetClassDefProps(ClassDef *pClass, CMoArray<PropDef*> &propList);

// If you set pNameOverride, it won't search through all the current objects
// to find a unique name.
BOOL SetupWorldNodeFromClass(
	ClassDef		*pClass, 
	TemplateClass	*pTemplate, 
	CWorldNode		*pNode, 
	CEditRegion		*pRegion,
	char			*pNameOverride=NULL);

CBaseProp*		CreatePropFromCode( int code, char *pName );

void	SetupNewProp(CBaseProp *pProp, PropDef *pVar);
BOOL	SetupWorldNode(const char *pClassName, CWorldNode *pNode, CEditRegion *pRegion);

BOOL	LoadObjectList(CAbstractIO &file, CMoArray<CBaseEditObj*> &theList);
BOOL	LoadNodeProperties(CAbstractIO &file, CWorldNode *pNode, CEditProjectMgr* pProject = NULL);

void	SaveObjectList(CAbstractIO &file, CMoArray<CBaseEditObj*> &theList);
uint32	GetObjectListSize(CMoArray<CBaseEditObj*> &theList);
void	SaveNodeProperties(CAbstractIO &file, CWorldNode *pNode);
uint32	GetNodePropertiesSize(CWorldNode* pNode);

// talon: the 2001 CopyObjectList (0x40f2e0; Processing.cpp's name for it,
// called at 0x440baa): it leaves out the hidden objects with bIgnoreHidden.
void	CopyRegionObjects( CMoArray<CBaseEditObj*> &in, CMoArray<CBaseEditObj*> &out, BOOL bIgnoreHidden );

#ifdef DIRECTEDITOR_BUILD
// talon: Build 69's, for DEdit (its LTA world, and its copy of the object
// list), until they are brought back to 2001.
class CPropListContainer;
BOOL	LoadNodePropertiesLTA(CLTANode* pParseNode, CWorldNode *pNode, CPropListContainer& container, CEditProjectMgr* pProject = NULL );
void	SaveObjectListLTA(CLTANodeBuilder &lb, CMoArray<CBaseEditObj*> &theList);
void	SaveNodePropertiesLTA(CLTAFile* pFile, CWorldNode *pNode, uint32 level);
void	CopyObjectList( CMoArray<CBaseEditObj*> &in, CMoArray<CBaseEditObj*> &out );
#endif
// Duplicates the node heirarchy between two lists of objects.  
// Note that the objects must be in exactly the same order in the list.
// Returns false on failure
bool	DuplicateObjectHeirarchy( CMoArray<CBaseEditObj*> &in, CMoArray<CBaseEditObj*> &out );


// The base class for all edit objects.
// talon: the 2001 class, 0x88 bytes (new at 0x40ef93, 0x40f36d): a
// CWorldNode (0x60 bytes) and its CEditVert at 0x60 (DoCopy, 0x40f540); no
// m_pRegion. Its vtable (0x47486c) has CWorldNode's five slots: the scalar
// deleting destructor (0x40f4d0), Term (CWorldNode's, 0x44e750),
// AllocateSameKind (0x40f440), DoCopy (0x40f540) and GetUpperLeftCornerPos
// (0x40f4a0).
class CBaseEditObj : public CWorldNode 
{

// Member Operations...

public:

							CBaseEditObj();
							CBaseEditObj( const CBaseEditObj& src );
	virtual					~CBaseEditObj();
	
	virtual CWorldNode*		AllocateSameKind()	{return new CBaseEditObj;}
	virtual void			DoCopy(CWorldNode *pOther);
					
	// Overloaded operators...
	CBaseEditObj&			operator=( const CBaseEditObj& src );

	void					SetPos(const LTVector &v);		// Sets the position
	void					SetOr(const LTVector &v);		// Sets the rotation

	CVector					GetUpperLeftCornerPos()		{ return GetPos(); }

// Member data...

public:

	// Misc. data for the editor stuff to use.
	CEditVert		m_Vert;

#ifdef DIRECTEDITOR_BUILD

	// talon: DEdit's (2001): 0xb8 bytes (`push $0xb8` before each
	// CBaseEditObj(), 0x427925), the dims after the vertex: the search flag
	// at 0x8c (the constructor sets it to 1), the dims at 0x90 and their
	// flags at 0xa4 (0x424080). No ILTPreInterface: the vtable (0x4d77e4)
	// is CWorldNode's five slots, as the processor's.

	// Initializes/Terminates the dims
	void					InitDims();
	void					TermDims()								{ m_dimsArray.SetSize(0); }

	// Access to the dims
	int						GetNumDims()							{ return m_dimsArray.GetSize(); }
	CVector					*GetDim(int nIndex)						{ return &m_dimsArray[nIndex]; }		
	DWORD					GetDimsFlags(int nIndex)				{ return m_dimsFlagArray[nIndex]; }

	BOOL					ShouldSearchForDims()					{ return m_bSearchForDims; }	
	void					SetSearchForDims(BOOL bSearch)			{ m_bSearchForDims=bSearch; }

	// talon: stand-ins: Build 69's class icon, model and visible radius,
	// for the rest of DEdit until it is brought back to 2001, which has
	// none of them: no icon, no radius (objects are culled as points).
	static struct DFileIdent_t*	m_pClassImageFile;
	void					UpdateClassName()						{}
	void					UpdateObjectClassImage()				{}
	float					GetVisibleRadius() const				{ return 0.0f; }
	void					SetVisibleRadius(float fRad)			{}

protected:

	BOOL					m_bSearchForDims;		// Indicates if the dims should be searched for or not
	CMoArray<CVector>		m_dimsArray;			// Array of the dims
	CMoArray<DWORD>			m_dimsFlagArray;		// Array of dims flags	

#endif // DIRECTEDITOR_BUILD
};



#endif  // __EDITOBJECTS_H__







