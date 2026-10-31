//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : EditObjects.cpp
//
//	PURPOSE	  : 
//
//	CREATED	  : November 28 1996
//
//
//------------------------------------------------------------------

// talon: the 2001 one, 0x40eae0..0x40f620, written from the image: the
// dummy globals, CreatePropFromCode, the binary property and object list
// load/save, CopyRegionObjects and CBaseEditObj (its vtable 0x47486c).
// Build 69's other functions (the class definitions, the LTA files) are not
// in the image. The .data order agrees: g_NameName/g_PosName/g_AnglesName
// (0x47e488..), "Rotation", "Pos", "Name", "DUMMY", the EH type descriptors
// of CMemoryException*, CLithException and CLithMemException (LithCatch in
// LoadNodeProperties), then EditPoly's "Default" (0x47e518).

// Includes....
#include "bdefs.h"
#include "processing.h"
#include "editobjects.h"
#include "editplane.h"
#include "stringholder.h"


// The basic properties (0x47e488, 0x47e48c, 0x47e490), and the dummies, whose
// dynamic initializers open the module: g_DummyPos (0x40eae0 -> 0x40eaf0,
// 0x484a38: CWorldNode::GetPos returns it, 0x44e8fa), g_DummyRotation
// (0x40eb10 -> 0x40eb20, 0x4848f0, which nothing in Processor reads) and
// g_DummyString (0x40eb40 -> 0x40eb50, 0x484900). CStringProp has no
// destructor: no atexit.
char		*g_NameName="Name", *g_PosName="Pos", *g_AnglesName="Rotation";
CVector		g_DummyPos(0.0f, 0.0f, 0.0f);
LTVector	g_DummyRotation(0.0f, 0.0f, 0.0f);
CStringProp	g_DummyString("DUMMY");


// CreatePropFromCode (0x40eb60): Build 69's. The new's EH states follow the
// source order (real 0, vector 1, colour 2, string 3, rotation 4, bool 5).
CBaseProp* CreatePropFromCode( int code, char *pName )
{
	CBaseProp		*pRet = NULL;

	if(code == PT_REAL || code == PT_FLAGS || code == PT_LONGINT)
		pRet = new CRealProp(pName);
	else if(code == PT_VECTOR)
		pRet = new CVectorProp(pName);
	else if(code == PT_COLOR)
		pRet = new CColorProp(pName);
	else if(code == PT_STRING)
		pRet = new CStringProp(pName);
	else if(code == PT_ROTATION)
		pRet = new CRotationProp(pName);
	else if(code == PT_BOOL)
		pRet = new CBoolProp(pName);
	else
		ASSERT(FALSE);

	if(pRet)
		pRet->m_Type = code;

	return pRet;
}


// LoadNodeProperties (0x40ecd0): Build 69's, but the class name goes
// straight into the node (strncpy, 30 characters, 0x40ed3f: the 2001
// SetClassName, if there is one, is inline and calls no UpdateClassName).
// LithTry/LithCatch with STDLITH_MFC: the catch of the CMemoryException*
// (0x40eefe) and of the CLithException (0x40ef24) are this function's
// funclets, not functions.
BOOL LoadNodeProperties(CAbstractIO &file, CWorldNode *pNode, CEditProjectMgr* pProject)
{
	uint32			k, nProperties, objDataStart;
	char			typeName[256], propName[256];
	CBaseProp		*pProp;
	BYTE			propCode;
	WORD			propLen, objDataLen;
	uint32 propFlags;


	LithTry
	{
		file >> objDataLen;
		objDataStart = file.GetCurPos();

		if( !file.ReadString(typeName, 256) )
			return FALSE;

		strncpy(pNode->m_TypeName, typeName, TYPENAME_LEN);

		file >> nProperties;
		pNode->m_PropList.m_Props.SetCacheSize(nProperties);

		for( k=0; k < nProperties; k++ )
		{
			if( !file.ReadString(propName,256) )
				return FALSE;

			file >> propCode;
			file >> propFlags;
			file >> propLen;

			if( (pProp = CreatePropFromCode(propCode, propName)) )
			{
				pProp->m_PropFlags = propFlags;
				pProp->LoadData( file );

				pNode->m_PropList.m_Props.Append( pProp );
			}
			else
			{
				file.SeekTo( file.GetCurPos() + propLen );
			}
		}
	}
	LithCatch(CLithException &exception)
	{
		exception=exception;
		return FALSE;
	}

	return TRUE;
}


// LoadObjectList (0x40ef40): Build 69's.
BOOL LoadObjectList( CAbstractIO &file, CMoArray<CBaseEditObj*> &theList)
{
	uint32			i, nObjects;
	CBaseEditObj	*pObj;


	// Load in all the objects.
	file >> nObjects;
	theList.SetCacheSize( 30 );

	for( i=0; i < nObjects; i++ )
	{
		pObj = new CBaseEditObj;

		if(!LoadNodeProperties(file, pObj, NULL))
		{
			delete pObj;
			return FALSE;
		}

		theList.Append(pObj);
	}

	return TRUE;
}


// SaveNodeProperties (0x40f100): Build 69's. propLen is not reset for each
// property: the placeholder written before a property's data is the last
// property's length (0x40f1f0), overwritten once the data is out.
void SaveNodeProperties(CAbstractIO &file, CWorldNode *pObj)
{
	uint32			k, curPos1, curPos2;
	uint32			curPos, objDataStart;
	WORD			objDataLen=0, propLen=0;
	CBaseProp		*pProp;


	file << objDataLen;
	objDataStart = file.GetCurPos();

	// Write the object data.
	if( strlen( pObj->GetClassName() ) < 256 )
		file.WriteString( (char*)pObj->GetClassName() );
	else
		file.WriteString( "" );

	file << pObj->m_PropList.m_Props.GetSize();
	for( k=0; k	< pObj->m_PropList.m_Props; k++ )
	{
		pProp = pObj->m_PropList.m_Props[k];

		if( strlen( pProp->m_Name ) < 256 )
			file.WriteString( pProp->m_Name );
		else
			file.WriteString( "" );

		file << (BYTE)pProp->m_Type;
		file << (uint32)pProp->m_PropFlags;

		curPos1 = file.GetCurPos();
		file << propLen;

		pProp->SaveData( file );

		// Go back and write the length of the property's data.
		curPos2 = file.GetCurPos();
		file.SeekTo( curPos1 );
		propLen = (WORD)(curPos2 - curPos1) - sizeof(WORD);
		file << propLen;
		file.SeekTo( curPos2 );
	}


	// Write how big the data was.
	curPos = file.GetCurPos();
	objDataLen = (WORD)(curPos - objDataStart);
	file.SeekTo( objDataStart - sizeof(objDataLen) );
	file << objDataLen;
	file.SeekTo( curPos );
}


// SaveObjectList (0x40f290): Build 69's.
void SaveObjectList(CAbstractIO &file, CMoArray<CBaseEditObj*> &theList)
{
	uint32			i;
	CBaseEditObj	*pObj;

	file << (uint32)theList.GetSize();
	for( i=0; i < theList; i++ )
	{
		pObj = theList[i];
		SaveNodeProperties(file, pObj->AsNode());
	}
}


// CopyRegionObjects (0x40f2e0; Processing.cpp's name for it, called at
// 0x440baa): the 2001 CopyObjectList, which leaves out the hidden objects
// (NODEFLAG_HIDDEN, 0x40f328) with bIgnoreHidden, and copies only the
// properties and the class name (no m_Vert, no DoCopy). Unlike Build 69's
// it does not test what new returned.
// talon: NOLF's (0x417ce0, called at 0x450140): two arguments, every
// object copied (no bIgnoreHidden).
void CopyRegionObjects(CMoArray<CBaseEditObj*> &in, CMoArray<CBaseEditObj*> &out)
{
	uint32			i;

	DeleteAndClearArray( out );
	out.SetSize( in.GetSize() );

	for( i=0; i < in.GetSize(); i++ )
	{
		out[i] = new CBaseEditObj;
		out[i]->m_PropList.CopyValues(&in[i]->m_PropList);
		strcpy(out[i]->m_TypeName, in[i]->m_TypeName);
	}
}


// CBaseEditObj::CBaseEditObj (0x40f420).
CBaseEditObj::CBaseEditObj()
{
	m_Type				= Node_Object;
}


// CBaseEditObj::~CBaseEditObj (0x40f4f0; its scalar deleting destructor
// 0x40f4d0).
CBaseEditObj::~CBaseEditObj()
{
	Term();
}


// CBaseEditObj::DoCopy (0x40f540). The vertex goes through a copy of the
// whole CEditVert (0x40f558) and only its position is assigned
// (CEditVert's operator=(const LTVector&)).
void CBaseEditObj::DoCopy(CWorldNode *pOther)
{
	CEditVert vert = ((CBaseEditObj*)pOther)->m_Vert;
	m_Vert.operator=((const LTVector&)vert);

	CWorldNode::DoCopy(pOther);
}


// talon: 0x4180c0, the processor's: `return TRUE` (node_ops'
// SetupBrushProperties calls it, 0x431c20). Build 69's sets the node up from
// its class; the 2001 processor has no class information to do it with.
BOOL SetupWorldNode(const char *pClassName, CWorldNode *pNode, CEditRegion *pRegion)
{
	return TRUE;
}
