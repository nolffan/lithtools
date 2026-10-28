//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : EditBrush.cpp
//
//	PURPOSE	  : Implements the CEditBrush class.
//
//	CREATED	  : November 12 1996
//
//
//------------------------------------------------------------------

// talon: the 2001 one, 0x40d6f0..0x40eae0 (then the CMoArray<CEditVert>
// it instantiates, its vtable 0x4747dc; among that template code, 0x40e7c0
// is a CMoArray<T*> destructor, vtable 0x47483c, T not known, inside the
// function table's 0x40e790 row), written from the image: Build 69's but
// for the CEditPoly and CEditVert it has, LoadTBW and SaveTBW. Build 69's
// other CEditBrush methods are not in the image.

// Includes....
#include "bdefs.h"
#include "processing.h"
#include "editbrush.h"
#include "editpoly.h"


// 0x40d6f0; Build 69's.
CEditBrush::CEditBrush()
{
	m_Type			= Node_Brush;

	m_Unknown_ac	= 1;		// talon: 0x40d7cf
	m_pRegion		= NULL;

	m_Polies.SetCacheSize( 15 );
	m_Points.SetCacheSize( 15 );

	r = g = b = 255;

}


// 0x40d890; Build 69's.
CEditBrush::~CEditBrush()
{
	Term();
}


// 0x40d960. Build 69's, but the 2001 CEditPoly's destructor is not
// virtual (called directly, 0x40d97b).
void CEditBrush::Term()
{
	DeleteAndClearArray( m_Polies );
	m_Points.Term();

	CWorldNode::Term();
}


// 0x40da00; Build 69's.
void CEditBrush::DoCopy(CWorldNode *pOther)
{
	CopyEditBrush((CEditBrush*)pOther->AsBrush());
	CWorldNode::DoCopy(pOther);
}


// 0x40da20. Build 69's LoadTBW as it was: a third argument handed to each
// poly's loader, WORD counters (the `and $0xffff` at 0x40dae3 and
// 0x40dbcc), each point's coordinates read through m_Points[i] (the array
// reloaded for each, 0x40daba), and no UpdatePlanes or UpdateBoundingInfo
// at the end.
//
// talon: two retail bugs, kept. The WORD counters never reach a count over
// 65535 (the loops do not end). And when a poly fails to load, the
// m_Polies slots after it are left as SetSize allocated them, not
// initialised (BaseNew<CEditPoly*>, 0x40f600), and Term deletes every
// non-NULL slot; the caller (0x410870) has already put the brush in the
// region.
BOOL CEditBrush::LoadTBW( CAbstractIO& InFile, CStringHolder *pStringHolder, uint32 dwUnknown )
{
	//clean up the brush
	Term();

	//load in our color
	InFile >> r >> g >> b;

	//how many points we have
	uint32 nNumPoints;
	InFile >> nNumPoints;

	//resize our point list
	m_Points.SetSize(nNumPoints);

	//read them in
	for( WORD nCurrPoint = 0; nCurrPoint < nNumPoints; nCurrPoint++ )
	{
		//position
		InFile >> m_Points[nCurrPoint].x >> m_Points[nCurrPoint].y >> m_Points[nCurrPoint].z;
	}

	//now read in the polygons
	uint32 nNumPolygons;
	InFile >> nNumPolygons;

	m_Polies.SetSize( nNumPolygons );

	for( WORD nCurrPoly = 0; nCurrPoly < nNumPolygons; nCurrPoly++ )
	{
		m_Polies[nCurrPoly] = new CEditPoly( this );
		if( !m_Polies[nCurrPoly]->LoadEditPolyTBW(InFile, pStringHolder, dwUnknown ) )
			return FALSE;
	}

	return TRUE;
}


// 0x40dc00. Build 69's SaveTBW with a second argument: when it is set,
// each poly is saved as its texture (0x40fcf0) and the CBasePolys of its
// array at 0x5c, the count first.
void CEditBrush::SaveTBW( CAbstractIO& OutFile, BOOL bUnknown )
{
	//write out the brush color
	OutFile << r << g << b;

	//write out the list of points used by this brush
	uint32 nNumPoints = m_Points.GetSize();
	OutFile << nNumPoints;

	for(uint32 nCurrPoint = 0; nCurrPoint < m_Points.GetSize(); nCurrPoint++)
	{
		//position
		OutFile << m_Points[nCurrPoint].x << m_Points[nCurrPoint].y << m_Points[nCurrPoint].z;
	}

	//write out the list of polygons used by this brush
	uint32 nNumPolies = m_Polies.GetSize();
	OutFile << nNumPolies;

	//now write out the polygons
	for(uint32 nCurrPoly = 0; nCurrPoly < m_Polies.GetSize(); nCurrPoly++)
	{
		CEditPoly *pPoly = m_Polies[nCurrPoly];

		if(bUnknown)
		{
			pPoly->SaveTextureInfoTBW(OutFile);

			uint32 nNumBasePolies = pPoly->m_Triangles.GetSize();
			OutFile << nNumBasePolies;

			for(uint32 nCurrBasePoly = 0; nCurrBasePoly < pPoly->m_Triangles.GetSize(); nCurrBasePoly++)
				pPoly->m_Triangles[nCurrBasePoly]->SaveBasePolyTBW(OutFile);
		}
		else
		{
			pPoly->SaveEditPolyTBW(OutFile);
		}
	}
}


// 0x40dd60..0x40df70 (528 bytes; the function table's 0x40ddc2 row is the
// second byte of the call at 0x40ddc1, not a function). Build 69's.
void CEditBrush::CopyEditBrush( CEditBrush *pOther, CStringHolder *pStringHolder )
{
	uint32		i;


	m_Polies.SetSize( pOther->m_Polies.GetSize() );
	for( i=0; i < pOther->m_Polies; i++ )
	{
		m_Polies[i] = new CEditPoly;
		m_Polies[i]->CopyEditPoly( pOther->m_Polies[i], TRUE, pStringHolder );
		m_Polies[i]->m_pBrush = this;
	}

	m_Points.CopyArray( pOther->m_Points );
	m_BoundingBox = pOther->m_BoundingBox;

	r = pOther->r;
	g = pOther->g;
	b = pOther->b;
}


// 0x40df80; Build 69's.
CVector CEditBrush::GetUpperLeftCornerPos()
{
	CVector vUpperLeft;
	vUpperLeft.x=0.0f;
	vUpperLeft.y=0.0f;
	vUpperLeft.z=0.0f;

	if (m_Points <= 0)
	{
		return vUpperLeft;
	}

	// Set the intial upper left point
	vUpperLeft=m_Points[0];

	// Find the smallest bounding edge that would appear in the upper left corner of the DEdit windows
	uint32 i;
	for( i=1; i < m_Points; i++ )
	{
		if (m_Points[i].x < vUpperLeft.x)
		{
			vUpperLeft.x=m_Points[i].x;
		}
		if (m_Points[i].y > vUpperLeft.y)
		{
			vUpperLeft.y=m_Points[i].y;
		}
		if (m_Points[i].z > vUpperLeft.z)
		{
			vUpperLeft.z=m_Points[i].z;
		}
	}

	return vUpperLeft;
}


// 0x40e030; Build 69's.
void CEditBrush::UpdateBSphere()
{
	if(m_Points)
	{
		m_BoundingBox.Init( m_Points[0] );
		for( uint32 i=1; i < m_Points; i++ )
			m_BoundingBox.Extend( m_Points[i] );
	}
}
