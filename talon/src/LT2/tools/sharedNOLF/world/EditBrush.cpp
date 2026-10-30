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
		// talon: this way round (0x415826 loads the point's z first)
		if (vUpperLeft.z < m_Points[i].z)
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


// talon: NOLF's (0x413cd0), Build 69's but for the planes: each poly's is
// updated in the first loop (0x413cfb), there is no UpdatePlanes.
BOOL CEditBrush::IsOpen()
{
	// If it's got less than 4 polies, it's definately open
	if (m_Polies.GetSize() < 4)
		return TRUE;

	// Keep track of the normal directions..
	uint32 nNormalFlags = 0;
	const uint32 NORMAL_FLAG_XPOS = 1 << 0;
	const uint32 NORMAL_FLAG_XNEG = 1 << 1;
	const uint32 NORMAL_FLAG_YPOS = 1 << 2;
	const uint32 NORMAL_FLAG_YNEG = 1 << 3;
	const uint32 NORMAL_FLAG_ZPOS = 1 << 4;
	const uint32 NORMAL_FLAG_ZNEG = 1 << 5;
	const uint32 NORMAL_FLAG_ALL = (1 << 6) - 1;

	uint32 nPolyLoop;

	for (nPolyLoop = 0; nPolyLoop < m_Polies; ++nPolyLoop)
	{
		m_Polies[nPolyLoop]->UpdatePlane();

		CEditPlane *pPlane = &m_Polies[nPolyLoop]->m_Plane;

		// Update the normal directions flags
		if (pPlane->m_Normal.x > 0)
			nNormalFlags |= NORMAL_FLAG_XPOS;
		if (pPlane->m_Normal.x < 0)
			nNormalFlags |= NORMAL_FLAG_XNEG;
		if (pPlane->m_Normal.y > 0)
			nNormalFlags |= NORMAL_FLAG_YPOS;
		if (pPlane->m_Normal.y < 0)
			nNormalFlags |= NORMAL_FLAG_YNEG;
		if (pPlane->m_Normal.z > 0)
			nNormalFlags |= NORMAL_FLAG_ZPOS;
		if (pPlane->m_Normal.z < 0)
			nNormalFlags |= NORMAL_FLAG_ZNEG;
	}

	// If we didn't find all of the cardinal directions in the brush, it's got an open face
	// (This is useful for doing a quick-check and detects a sub-divided dual-sided polygon)
	if (nNormalFlags != NORMAL_FLAG_ALL)
		return TRUE;

	// We need to be sure there's no duplicate vertices for this to work...
	RemoveDuplicatePoints(0.01f);

	// Run through the polygons looking for un-shared edges
	for (nPolyLoop = 0; nPolyLoop < m_Polies.GetSize(); ++nPolyLoop)
	{
		CEditPoly *pPoly = m_Polies[nPolyLoop];
		for (uint32 nEdgeLoop = 0; nEdgeLoop < pPoly->m_Indices.GetSize(); ++nEdgeLoop)
		{
			uint32 nPolyEdge[2];

			nPolyEdge[0] = pPoly->m_Indices[nEdgeLoop];
			nPolyEdge[1] = pPoly->m_Indices.Next(nEdgeLoop);

			bool bFoundEdge = false;

			for (uint32 nOtherPolyLoop = 0; (!bFoundEdge) && (nOtherPolyLoop < m_Polies.GetSize()); ++nOtherPolyLoop)
			{
				if (nOtherPolyLoop == nPolyLoop)
					continue;

				CEditPoly *pOtherPoly = m_Polies[nOtherPolyLoop];

				for (uint32 nOtherEdgeLoop = 0; (!bFoundEdge) && (nOtherEdgeLoop < pOtherPoly->m_Indices.GetSize()); ++nOtherEdgeLoop)
				{
					uint32 nOtherEdge[2];

					nOtherEdge[0] = pOtherPoly->m_Indices[nOtherEdgeLoop];
					nOtherEdge[1] = pOtherPoly->m_Indices.Next(nOtherEdgeLoop);

					// Compare the poly edge and the other edge, in both directions
					bFoundEdge = ((nPolyEdge[0] == nOtherEdge[0]) && (nPolyEdge[1] == nOtherEdge[1])) ||
						((nPolyEdge[0] == nOtherEdge[1]) && (nPolyEdge[1] == nOtherEdge[0]));
				}
			}

			// Found an un-shared edge..  This brush seems open..
			if (!bFoundEdge)
				return TRUE;
		}
	}

	return FALSE;
}


// talon: NOLF's (0x414070): Build 69's, with a WORD index.
void CEditBrush::RemoveVertex( WORD iVert )
{
	for( uint32 i=0; i < m_Polies; i++ )
		m_Polies[i]->DecrementPoints( iVert );

	m_Points.Remove( iVert );
}


// talon: NOLF's (0x4143c0): Build 69's, but the squared threshold is a
// double (0x4143e2) and the distance is from a copy of the current point
// (0x414426).
void CEditBrush::RemoveDuplicatePoints( CReal distThresh )
{
	// Jump out on an empty list
	if (!m_Points.GetSize())
		return;

	// Square the distance threshold to avoid sqrts
	double fThreshSqr = distThresh * distThresh;

	uint32 iCurrPoint;
	for (iCurrPoint = 0; iCurrPoint < m_Points.GetSize() - 1; iCurrPoint++)
	{
		uint32 iOtherPoint;
		for (iOtherPoint = iCurrPoint + 1; iOtherPoint < m_Points.GetSize(); iOtherPoint++)
		{
			CVector vCurr = m_Points[iCurrPoint];
			CVector cDistVect(m_Points[iOtherPoint] - vCurr);
			// Don't join a vertex if it's further away from the main vertex
			if (cDistVect.Dot(cDistVect) > fThreshSqr)
				continue;

			// Scan through the polygons looking for this vertex
			uint32 iPolyLoop;
			for (iPolyLoop = 0; iPolyLoop < m_Polies.GetSize(); iPolyLoop++)
			{
				CEditPoly *pPoly = m_Polies[iPolyLoop];
				uint32 iIndexLoop;
				for (iIndexLoop = 0; iIndexLoop < pPoly->m_Indices.GetSize(); iIndexLoop++)
				{
					// Replace duplicate polies with the correct index
					if (pPoly->m_Indices[iIndexLoop] == iOtherPoint)
						pPoly->m_Indices[iIndexLoop] = (WORD)iCurrPoint;
				}
				pPoly->DecrementPoints((WORD)iOtherPoint);
			}

			// Remove the point from the list
			m_Points.Remove(iOtherPoint);
			iOtherPoint--;
		}
	}
}
