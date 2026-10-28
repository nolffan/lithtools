// talon: 0x0040eb10..0x0040fff9, two of the 2001 shared world modules,
// written from the image (the file name is ours and provisional):
//
//   0x40eb10..0x40f600    EditObjects: the dummy globals, CreatePropFromCode,
//                         the binary property and object list load/save,
//                         CopyRegionObjects and CBaseEditObj (its vtable
//                         0x47486c);
//   0x40f620..0x40fff9    EditPoly: the texture planes, CEditPoly (one
//                         texture, its triangles, the .ed load/save; its
//                         vtable 0x474880).
//
// The .data order agrees: g_NameName/g_PosName/g_AnglesName (0x47e488..),
// "Rotation", "Pos", "Name", "DUMMY", the EH type descriptors of
// CMemoryException*, CLithException and CLithMemException (LithCatch in
// LoadNodeProperties), then EditPoly's "Default" (0x47e518).
//
// The shared EditObjects.cpp/EditPoly.cpp/BasePoly.h are Build 69's; what
// the 2001 classes are is written here on stand-ins (CBaseEditObj2001,
// CBasePoly2001, CEditPoly2001) and functions with a 2001 suffix, so
// nothing collides at link time. To be folded into the shared files.

#include "bdefs.h"
#include "processing.h"
#include "editobjects.h"
#include "editplane.h"
#include "stringholder.h"


// ======================================================================== //
// EditObjects (2001).
// ======================================================================== //

// The basic properties (0x47e488, 0x47e48c, 0x47e490). In 2001 there is no
// g_DummyRotation: the only dynamic initializers are g_DummyPos's
// (0x40eb10 -> 0x40eb20, 0x4848f0) and g_DummyString's (0x40eb40 ->
// 0x40eb50, 0x484900). CStringProp has no destructor: no atexit.
char		*g_NameName2001="Name", *g_PosName2001="Pos", *g_AnglesName2001="Rotation";
CVector		g_DummyPos2001(0.0f, 0.0f, 0.0f);
CStringProp	g_DummyString2001("DUMMY");


// CreatePropFromCode (0x40eb60): Build 69's. The new's EH states follow the
// source order (real 0, vector 1, colour 2, string 3, rotation 4, bool 5).
CBaseProp* CreatePropFromCode2001( int code, char *pName )
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
BOOL LoadNodeProperties2001(CAbstractIO &file, CWorldNode *pNode, CEditProjectMgr* pProject)
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

			if( (pProp = CreatePropFromCode2001(propCode, propName)) )
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


// The 2001 CBaseEditObj (0x88 bytes: new at 0x40ef93, 0x40f36d): a
// CWorldNode (0x60 bytes) and its CEditVert at 0x60 (DoCopy, 0x40f540);
// no m_pRegion. Its vtable (0x47486c) has five slots, CWorldNode's
// (0x475d00) layout: the scalar deleting destructor (0x40f4d0), Term
// (CWorldNode's, 0x44e750), AllocateSameKind (0x40f440), DoCopy (0x40f540)
// and GetUpperLeftCornerPos (0x40f4a0; CWorldNode's is 0x44e870).
class CBaseEditObj2001 : public CWorldNode
{
public:
							CBaseEditObj2001();
	virtual					~CBaseEditObj2001();

	virtual CWorldNode*		AllocateSameKind()	{return new CBaseEditObj2001;}
	virtual void			DoCopy(CWorldNode *pOther);

	CVector					GetUpperLeftCornerPos()		{ return GetPos(); }

	CEditVert				m_Vert;
};


// LoadObjectList (0x40ef40): Build 69's.
BOOL LoadObjectList2001( CAbstractIO &file, CMoArray<CBaseEditObj2001*> &theList)
{
	uint32			i, nObjects;
	CBaseEditObj2001	*pObj;


	// Load in all the objects.
	file >> nObjects;
	theList.SetCacheSize( 30 );

	for( i=0; i < nObjects; i++ )
	{
		pObj = new CBaseEditObj2001;

		if(!LoadNodeProperties2001(file, pObj, NULL))
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
void SaveNodeProperties2001(CAbstractIO &file, CWorldNode *pObj)
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
void SaveObjectList2001(CAbstractIO &file, CMoArray<CBaseEditObj2001*> &theList)
{
	uint32			i;
	CBaseEditObj2001	*pObj;

	file << (uint32)theList.GetSize();
	for( i=0; i < theList; i++ )
	{
		pObj = theList[i];
		SaveNodeProperties2001(file, pObj->AsNode());
	}
}


// CopyRegionObjects (0x40f2e0; Processing.cpp's name for it, called at
// 0x440baa): the 2001 CopyObjectList, which leaves out the hidden objects
// (NODEFLAG_HIDDEN, 0x40f328) with bIgnoreHidden, and copies only the
// properties and the class name (no m_Vert, no DoCopy). Unlike Build 69's
// it does not test what new returned.
void CopyRegionObjects(CMoArray<CBaseEditObj2001*> &in, CMoArray<CBaseEditObj2001*> &out, BOOL bIgnoreHidden)
{
	uint32			i, nObjects, iOut;

	nObjects = 0;
	DeleteAndClearArray( out );

	for( i=0; i < in.GetSize(); i++ )
	{
		if(!bIgnoreHidden || !(in[i]->m_Flags & NODEFLAG_HIDDEN))
			nObjects++;
	}

	out.SetSize( nObjects );

	iOut = 0;
	for( i=0; i < in.GetSize(); i++ )
	{
		if(bIgnoreHidden && (in[i]->m_Flags & NODEFLAG_HIDDEN))
			continue;

		out[iOut] = new CBaseEditObj2001;
		out[iOut]->m_PropList.CopyValues(&in[i]->m_PropList);
		strcpy(out[iOut]->m_TypeName, in[i]->m_TypeName);
		iOut++;
	}
}


// CBaseEditObj::CBaseEditObj (0x40f420).
CBaseEditObj2001::CBaseEditObj2001()
{
	m_Type				= Node_Object;
}


// CBaseEditObj::~CBaseEditObj (0x40f4f0; its scalar deleting destructor
// 0x40f4d0).
CBaseEditObj2001::~CBaseEditObj2001()
{
	Term();
}


// CBaseEditObj::DoCopy (0x40f540). The vertex goes through a copy of the
// whole CEditVert (0x40f558) and only its position is assigned
// (CEditVert's operator=(const LTVector&)).
void CBaseEditObj2001::DoCopy(CWorldNode *pOther)
{
	CEditVert vert = ((CBaseEditObj2001*)pOther)->m_Vert;
	m_Vert.operator=((const LTVector&)vert);

	CWorldNode::DoCopy(pOther);
}



// ======================================================================== //
// EditPoly (2001).
// ======================================================================== //

// The 2001 CBasePoly (BasePoly.cpp, 0x401000..): BasePoly.h's layout
// (vtable 0, m_Plane 4, m_Indices 0x14, m_pBrush 0x28; 0x2c bytes, new at
// 0x40ff4d) but one virtual only: its vtable (0x474470) is CopyAttributes
// (0x44d400), and CEditPoly's (0x474880) has CopyAttributes alone
// (0x40fe10). No destructor of its own: the compiler's (0x40f8a0) destroys
// m_Indices and stores no vtable, and it is not virtual (UpdateTriangles
// and ~CEditPoly inline it, CEditBrush's DeleteAndClearArray calls
// ~CEditPoly directly, 0x40d97b). The constructors and methods are
// BasePoly.cpp's (matched there), only declared here.
class CBasePoly2001
{
	public:

							CBasePoly2001();
							CBasePoly2001( CEditBrush *pBrush, CMoWordArray &indices );

		virtual void		CopyAttributes( CBasePoly2001 *pOther, CStringHolder *pStringHolder=NULL )	{}

		BOOL				LoadBasePolyTBW( CAbstractIO& InFile );
		void				SaveBasePolyTBW( CAbstractIO& OutFile );

		BOOL				GetTriangles( CMoArray<CBasePoly2001*> &polies );

	public:

		CEditPlane			m_Plane;
		CMoWordArray		m_Indices;
		CEditBrush			*m_pBrush;
};


// The main texture alignment planes (0x484d70, 0x40f620 -> 0x40f630) and
// right vectors (0x484db8, 0x40f6f0 -> 0x40f700): Build 69's.
CVector g_TexturePlanes2001[] =
{
	CVector(0.0f, 1.0f, 0.0f),		// Bottom
	CVector(0.0f, -1.0f, 0.0f),		// Top

	CVector(1.0f, 0.0f, 0.0f),		// East-facing wall
	CVector(-1.0f, 0.0f, 0.0f),		// West-facing wall
	CVector(0.0f, 0.0f, 1.0f),		// North-facing wall
	CVector(0.0f, 0.0f, -1.0f)		// South-facing wall
};

CVector g_RightVectors2001[] =
{
	CVector(1.0f, 0.0f, 0.0f),
	CVector(-1.0f, 0.0f, 0.0f),

	CVector(0.0f, 0.0f, 1.0f),	
	CVector(0.0f, 0.0f, -1.0f),
	CVector(-1.0f, 0.0f, 0.0f),
	CVector(1.0f, 0.0f, 0.0f),	
};


// The 2001 CEditPoly (0x88 bytes: new at 0x40db72, 0x40dda2): one
// texture, its fields in the poly (Build 69's CTexturedPlane: the
// constructor's "Default", P (1,0,0), Q (0,0,1), O (0,0,0), 0x40f858..),
// and its triangles (0x5c, a CMoArray<CBasePoly*>, vtable 0x474884, made by
// UpdateTriangles). The .ed order (0x40fb30, 0x40fc70, 0x40fcf0): the base
// poly, O P Q, 0x70, the texture name, 0x84, the three bytes 0x58..0x5a.
// m_pUser1 is at 0x7c (EditPolyToPrePoly, 0x402bd8). 0x84 is OR'd into the
// surface flags (RaySelecter.cpp, 0x445663). 0x80 is never initialized.
class CEditPoly2001 : public CBasePoly2001
{
	public:

							CEditPoly2001();
							CEditPoly2001( CEditBrush *pBrush );
							~CEditPoly2001();

		void				Term();

	private:

		void				CommonConstructor( );

	public:

		BOOL				LoadEditPolyTBW( CAbstractIO &file, CStringHolder *pStringHolder, uint32 unused );
		void				SaveEditPolyTBW( CAbstractIO &file );
		void				SaveTextureInfoTBW( CAbstractIO &file );

		virtual void		CopyAttributes( CBasePoly2001 *pOther, CStringHolder *pStringHolder=NULL );

		// Remakes m_Triangles from the poly (the name ours).
		void				UpdateTriangles();

	public:

		uint32				m_Unknown_2c;
		uint32				m_Unknown_30;
		LTVector			O, P, Q;
		uint8				m_Unknown_58;
		uint8				m_Unknown_59;
		uint8				m_Unknown_5a;
		CMoArray<CBasePoly2001*>	m_Triangles;
		uint32				m_Unknown_70;
		char				*m_pTextureName;
		uint32				m_Unknown_78;
		void				*m_pUser1;
		uint32				m_Unknown_80;
		uint32				m_Unknown_84;
};


inline void CEditPoly2001::CommonConstructor( )
{
	m_pUser1				= NULL;
	m_Unknown_78			= 0;
	m_pBrush				= NULL;
	m_Unknown_30			= 0;
	m_Unknown_2c			= 0;

	m_pTextureName			= "Default";
	m_Unknown_84			= 0;

	P.Init( 1.0f, 0.0f, 0.0f );
	Q.Init( 0.0f, 0.0f, 1.0f );
	O.Init( 0.0f, 0.0f, 0.0f );

	m_Unknown_70			= 1;
	m_Unknown_5a			= 0;
	m_Unknown_59			= 0;
	m_Unknown_58			= 0;
}


// CEditPoly::CEditPoly (0x40f7c0).
CEditPoly2001::CEditPoly2001()
{
	CommonConstructor( );
}


// CEditPoly::CEditPoly( CEditBrush *pBrush ) (0x40f8f0).
CEditPoly2001::CEditPoly2001( CEditBrush *pBrush )
{
	CommonConstructor( );
	m_pBrush = pBrush;
}


// CEditPoly::~CEditPoly (0x40f9e0).
CEditPoly2001::~CEditPoly2001()
{
	Term();
}


// CEditPoly::Term (0x40faa0).
void CEditPoly2001::Term()
{
	DeleteAndClearArray(m_Triangles);
	m_Indices.SetSize(0);
	m_Unknown_30 = 0;
	m_Unknown_2c = 0;
}


// CEditPoly::LoadEditPolyTBW (0x40fb30, Build 69's name; the .ed's poly, called
// at 0x40dbb6). The third argument is never read. A texture name over 100
// characters is dropped (0x40fc08).
BOOL CEditPoly2001::LoadEditPolyTBW( CAbstractIO &file, CStringHolder *pStringHolder, uint32 unused )
{
	char texName[256];

	if( !LoadBasePolyTBW(file) )
		return FALSE;

	file >> O.x;
	file >> O.y;
	file >> O.z;

	file >> P.x;
	file >> P.y;
	file >> P.z;

	file >> Q.x;
	file >> Q.y;
	file >> Q.z;

	file >> m_Unknown_70;

	if( !file.ReadString(texName, 256) )
		return FALSE;

	if( strlen(texName) > 100 )
		texName[0] = 0;

	m_pTextureName = pStringHolder->AddString(texName);

	file >> m_Unknown_84;
	file >> m_Unknown_58;
	file >> m_Unknown_59;
	file >> m_Unknown_5a;

	return TRUE;
}


// CEditPoly::SaveEditPolyTBW (0x40fc70, Build 69's name).
void CEditPoly2001::SaveEditPolyTBW( CAbstractIO &file )
{
	SaveBasePolyTBW(file);
	SaveTextureInfoTBW(file);

	file << m_Unknown_84;
	file << m_Unknown_58;
	file << m_Unknown_59;
	file << m_Unknown_5a;
}


// CEditPoly::SaveTextureInfoTBW (0x40fcf0; Build 69's CTexturedPlane name, also called at
// 0x40dcfd).
void CEditPoly2001::SaveTextureInfoTBW( CAbstractIO &file )
{
	file << O.x << O.y << O.z;
	file << P.x << P.y << P.z;
	file << Q.x << Q.y << Q.z;

	file << m_Unknown_70;

	if( strlen(m_pTextureName) < 256 )
		file.WriteString( m_pTextureName );
	else
		file.WriteString( "" );
}


// CEditPoly::CopyAttributes (0x40fe10).
void CEditPoly2001::CopyAttributes( CBasePoly2001 *pBase, CStringHolder *pStringHolder )
{
	CEditPoly2001	*pOther = (CEditPoly2001*)pBase;

	m_Plane = pOther->m_Plane;

	O = pOther->O;
	P = pOther->P;
	Q = pOther->Q;

	if (pStringHolder)
	{
		m_pTextureName=pStringHolder->AddString(pOther->m_pTextureName);
	}
	else
	{
		m_pTextureName=pOther->m_pTextureName;
	}	

	m_Unknown_58 = pOther->m_Unknown_58;
	m_Unknown_59 = pOther->m_Unknown_59;
	m_Unknown_5a = pOther->m_Unknown_5a;
	m_Unknown_70 = pOther->m_Unknown_70;
	m_Unknown_84 = pOther->m_Unknown_84;
}


// CEditPoly::UpdateTriangles (0x40fec0, the name ours; called at
// 0x411225): triangulates a copy of the poly into m_Triangles.
void CEditPoly2001::UpdateTriangles()
{
	CBasePoly2001	*pPoly;

	DeleteAndClearArray(m_Triangles);

	pPoly = new CBasePoly2001(m_pBrush, m_Indices);
	pPoly->m_Plane = m_Plane;
	pPoly->GetTriangles(m_Triangles);

	delete pPoly;
}
