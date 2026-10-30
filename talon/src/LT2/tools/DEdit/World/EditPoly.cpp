//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : EditPoly.cpp
//
//	PURPOSE	  : Defines the CEditPoly class.
//
//	CREATED	  : October 5 1996
//
//
//------------------------------------------------------------------

// talon: the 2001 one, 0x40f620..0x410110, written from the image: the
// texture planes, CEditPoly (one texture, its triangles, the .ed
// load/save; its vtable 0x474880). Build 69's other functions are not in
// the image.

// Includes....
#include "bdefs.h"
#include "processing.h"
#include "editobjects.h"
#include "editplane.h"
#include "stringholder.h"
#include "editpoly.h"


// The main texture alignment planes (0x484d70, 0x40f620 -> 0x40f630) and
// right vectors (0x484db8, 0x40f6f0 -> 0x40f700): Build 69's.
CVector g_TexturePlanes[] =
{
	CVector(0.0f, 1.0f, 0.0f),		// Bottom
	CVector(0.0f, -1.0f, 0.0f),		// Top

	CVector(1.0f, 0.0f, 0.0f),		// East-facing wall
	CVector(-1.0f, 0.0f, 0.0f),		// West-facing wall
	CVector(0.0f, 0.0f, 1.0f),		// North-facing wall
	CVector(0.0f, 0.0f, -1.0f)		// South-facing wall
};

CVector g_RightVectors[] =
{
	CVector(1.0f, 0.0f, 0.0f),
	CVector(-1.0f, 0.0f, 0.0f),

	CVector(0.0f, 0.0f, 1.0f),	
	CVector(0.0f, 0.0f, -1.0f),
	CVector(-1.0f, 0.0f, 0.0f),
	CVector(1.0f, 0.0f, 0.0f),	
};


inline void CEditPoly::CommonConstructor( )
{
	m_pUser1				= NULL;
	m_Unknown_78			= 0;
	m_pBrush				= NULL;
	m_Unknown_30			= 0;
	m_Unknown_2c			= 0;

	m_pTextureName			= "Default";
	m_SurfaceFlags			= 0;

	P.Init( 1.0f, 0.0f, 0.0f );
	Q.Init( 0.0f, 0.0f, 1.0f );
	O.Init( 0.0f, 0.0f, 0.0f );

	m_Unknown_70			= 1;
	m_Ambient[2]			= 0;
	m_Ambient[1]			= 0;
	m_Ambient[0]			= 0;
}


// CEditPoly::CEditPoly (0x40f7c0).
CEditPoly::CEditPoly()
{
	CommonConstructor( );
}


// CEditPoly::CEditPoly( CEditBrush *pBrush ) (0x40f8f0).
CEditPoly::CEditPoly( CEditBrush *pBrush )
{
	CommonConstructor( );
	m_pBrush = pBrush;
}


// CEditPoly::~CEditPoly (0x40f9e0).
CEditPoly::~CEditPoly()
{
	Term();
}


// CEditPoly::Term (0x40faa0).
void CEditPoly::Term()
{
	DeleteAndClearArray(m_Triangles);
	m_Indices.SetSize(0);
	m_Unknown_30 = 0;
	m_Unknown_2c = 0;
}


// CEditPoly::LoadEditPolyTBW (0x40fb30, Build 69's name; the .ed's poly, called
// at 0x40dbb6). The third argument is never read. A texture name over 100
// characters is dropped (0x40fc08).
BOOL CEditPoly::LoadEditPolyTBW( CAbstractIO &file, CStringHolder *pStringHolder, uint32 unused )
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

	file >> m_SurfaceFlags;
	file >> m_Ambient[0];
	file >> m_Ambient[1];
	file >> m_Ambient[2];

	return TRUE;
}


// CEditPoly::SaveEditPolyTBW (0x40fc70, Build 69's name).
void CEditPoly::SaveEditPolyTBW( CAbstractIO &file )
{
	SaveBasePolyTBW(file);
	SaveTextureInfoTBW(file);

	file << m_SurfaceFlags;
	file << m_Ambient[0];
	file << m_Ambient[1];
	file << m_Ambient[2];
}


// CEditPoly::SaveTextureInfoTBW (0x40fcf0; Build 69's CTexturedPlane name, also called at
// 0x40dcfd).
void CEditPoly::SaveTextureInfoTBW( CAbstractIO &file )
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
void CEditPoly::CopyAttributes( CBasePoly *pBase, CStringHolder *pStringHolder )
{
	CEditPoly	*pOther = (CEditPoly*)pBase;

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

	m_Ambient[0] = pOther->m_Ambient[0];
	m_Ambient[1] = pOther->m_Ambient[1];
	m_Ambient[2] = pOther->m_Ambient[2];
	m_Unknown_70 = pOther->m_Unknown_70;
	m_SurfaceFlags = pOther->m_SurfaceFlags;
}


// CEditPoly::UpdateTriangles (0x40fec0, the name ours; called at
// 0x411225): triangulates a copy of the poly into m_Triangles.
void CEditPoly::UpdateTriangles()
{
	CBasePoly	*pPoly;

	DeleteAndClearArray(m_Triangles);

	pPoly = new CBasePoly(m_pBrush, m_Indices);
	pPoly->m_Plane = m_Plane;
	pPoly->GetTriangles(m_Triangles);

	delete pPoly;
}


// 0x410000: as Build 69 has it (CopyAttributes is the 2001 CBasePoly's only
// virtual, slot 0).
BOOL CEditPoly::CopyEditPoly( CEditPoly *pPoly, BOOL bCopyIndices, CStringHolder *pStringHolder )
{
	if( bCopyIndices )
		m_Indices.CopyArray( pPoly->m_Indices );

	CopyAttributes( pPoly, pStringHolder );
	m_pBrush = pPoly->m_pBrush;

	return TRUE;
}
