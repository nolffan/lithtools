//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : BasePoly.cpp
//
//	PURPOSE	  : Defines the CBasePoly class.
//
//	CREATED	  : October 15 1996
//
//
//------------------------------------------------------------------

// Includes....
#include "bdefs.h"
#include "basepoly.h"
#include "geomroutines.h"
#include "editpoly.h"
#include "editregion.h"
#include "editbrush.h"
// talon: what Build 69's adds, for its functions below
#include "ltamgr.h"
#include "ltasaveutils.h"



#define BASEPOLY_EPSILON	0.001f


CBasePoly::CBasePoly()
{
	m_pBrush = NULL;
}


CBasePoly::CBasePoly( CEditBrush *pBrush )
{
	m_pBrush = pBrush;
}


CBasePoly::CBasePoly( CEditBrush *pBrush, CMoWordArray &indices )
{
	m_pBrush = pBrush;
	m_Indices.CopyArray( indices );
}


CBasePoly::CBasePoly( CEditBrush *pBrush, WORD i0, WORD i1, WORD i2, BOOL bUpdatePlane )
{
	m_pBrush = pBrush;
	m_Indices.SetSize( 3 );
	m_Indices[0] = i0;
	m_Indices[1] = i1;
	m_Indices[2] = i2;

	if( bUpdatePlane )
		UpdatePlane();
}


// talon: DEdit's LTA loader (0x40f830), first after the constructors in
// 2001; DEdit 2001 has no LTA writer here (none of Build 69's
// SaveBasePolyLTA strings is in the image).
BOOL CBasePoly::LoadBasePolyLTA( CLTANode* pNode )
{
	// CLTANode* pIndicesNode = pNode->getElem(0); // shallow_find_list(pNode, "indices");
	CLTANode* pIndex = pNode->GetElement(1); //PairCdrNode(pIndicesNode);
	uint32 listSize = pIndex->GetNumElements();

	assert(listSize > 0);
	m_Indices.SetSize( listSize - 1 );
	
	for( uint32 i = 1; i < listSize; i++ )
	{
		Index(i-1) = GetUint32(pIndex->GetElement(i));
		if( Index(i-1) >= m_pBrush->m_Points )
		{
			Index(i-1) = 0;
			return FALSE;
		}
	}	
	

	CLTANode* pNormalNode = pNode->GetElement(2); //shallow_find_list(pNode, "normal");
	if( pNormalNode )
	{
		Normal().x = GetFloat(pNormalNode->GetElement(1));
		Normal().y = GetFloat(pNormalNode->GetElement(2));
		Normal().z = GetFloat(pNormalNode->GetElement(3));
	}
	
	CLTANode* pDistNode = pNode->GetElement(3); //shallow_find_list(pNode, "dist");
	if( pDistNode )
	{
		Dist() = GetFloat(PairCdrNode(pDistNode));
	}
	
	return TRUE;
}



// talon: the polygon in a binary .ed file (Build 69 has these as the .tbw
// ones): its index count, its indices as WORDs, its plane. 0x4012c0 reads
// each index straight into the array and rejects one past the brush's
// points; 0x401380 writes them back.
BOOL CBasePoly::LoadBasePolyTBW( CAbstractIO& InFile )
{
	uint32 nNumVerts;
	InFile >> nNumVerts;

	m_Indices.SetSize( nNumVerts );

	for(uint32 nCurrVert = 0; nCurrVert < nNumVerts; nCurrVert++)
	{
		InFile >> m_Indices[nCurrVert];

		if( m_Indices[nCurrVert] >= m_pBrush->m_Points.GetSize() )
		{
			m_Indices[nCurrVert] = 0;
			return FALSE;
		}
	}

	InFile >> Normal().x;
	InFile >> Normal().y;
	InFile >> Normal().z;

	InFile >> Dist();

	return TRUE;
}

void CBasePoly::SaveBasePolyTBW( CAbstractIO& OutFile )
{
	uint32 i;

	OutFile << m_Indices.GetSize();
	for( i=0; i < m_Indices.GetSize(); i++ )
		OutFile << m_Indices[i];

	OutFile << m_Plane.m_Normal.x << m_Plane.m_Normal.y << m_Plane.m_Normal.z;
	OutFile << m_Plane.m_Dist;
}

/*
( basepoly  
	( indices ( 6 0 3 7 ) )
	( normal ( 1.000000 0.000000 0.000000 ) )
	( dist 292.000000 )
)
*/

BOOL CBasePoly::IsValid()
{
	for( uint32 i=0; i < NumVerts(); i++ )
		if( m_Indices[i] >= m_pBrush->m_Points )
			return FALSE;

	return TRUE;
}


void CBasePoly::DecrementPoints( uint32 index )
{
	for( WORD i=0; i < NumVerts(); i++ )
		if( m_Indices[i] >= (WORD)index )
			--m_Indices[i];
}


BOOL CBasePoly::PointInPlane( const LTVector &point )
{
	CReal dist = Normal().Dot( point ) - Dist();
	return (dist > -POINT_SIDE_EPSILON) && (dist < POINT_SIDE_EPSILON);
}



BOOL CBasePoly::PointInPoly( const LTVector &point )
{
	CReal		area;

	for(WORD i=0; i < NumVerts(); i++ )
	{
		area = g_TriArea2( Normal(), Pt(i), NextPt(i), point );
		if( area > 0 )
			return FALSE;
	}

	return TRUE;
}


void CBasePoly::RemoveCollinearVertices()
{
	int16			i;
	CVector			v1, v2;

	for( i=0; i < (int32)m_Indices; i++ )
	{
		v1 = Pt(i) - PrevPt(i);
		v2 = NextPt(i) - Pt(i);
	
		v1.Norm();
		v2.Norm();
		
		if( v1.Equals(v2, 0.00001f) )
		{
			m_Indices.Remove(i);
			i = -1;
		}
	}
}


BOOL CBasePoly::IsConvex()
{
	CVector			v1, v2, v3;
	CVector			vec1, vec2;
	uint32			i;
	CDReal			dot, sign=0.0f;

	
	
	for( i=0; i < NumVerts(); i++ )
	{
		v1 = Pt(i);
		v2 = NextPt(i);
		v3 = NextPt(m_Indices.NextI(i) );
	
		vec1 = v2 - v1;
		vec2 = v3 - v2;

		vec1.Norm();
		vec2.Norm();

		dot = Normal().Dot( vec1.Cross(vec2) );
		if( (sign == 0.0f) && (dot != 0.0f) )
		{
			sign = dot;
		}
		else
		{
			if( sign < 0.0f && dot > 0.0f )
				return FALSE;
			else if( sign > 0.0f && dot < 0.0f )
				return FALSE;
		}
	}

	return TRUE;
}


void CBasePoly::GetCenterPoint( CVector &centerPt )
{
	centerPt.Init( 0.0f, 0.0f, 0.0f );
	for(WORD i=0; i < NumVerts(); i++ )
		centerPt += Pt(i);

	centerPt /= (CReal)NumVerts();
}


void CBasePoly::UpdatePlane()
{
	g_GenerateNormal( this );
}


// talon: 2001's (0x40fcda): Build 69's CEditPoly::IntersectRay up to the
// intersection point, then PointInPoly on it (CEditPoly::IntersectRay,
// 0x4253e3, runs it on each of its triangles).
BOOL CBasePoly::IntersectRay( CEditRay &ray, CReal &t, BOOL bBackface )
{
	CReal dot1, dot2, div;
	LTVector testPt;

	// Trivial reject.
	if( (Normal().Dot(ray.m_Pos) - Dist()) >= 0.0f )
	{
		// It's pointing away from (or is perpendicular to) the polygon.
		if( Normal().Dot(ray.m_Dir) >= -0.001f )
			return FALSE;
	}
	else
	{
		// Backfacing.  (talon: 2001 rejects it when bBackface is set,
		// 0x40fdd9; Build 69 when it isn't.)
		if( bBackface )
		{
			return FALSE;
		}
		else
		{
			// It's pointing away from (or is perpendicular to) the polygon.
			if( Normal().Dot(ray.m_Dir) <= 0.001f )
				return FALSE;
		}
	}

	dot1 = Normal().Dot( ray.m_Pos ) - Dist();
	dot2 = Normal().Dot( ray.m_Pos + ray.m_Dir ) - Dist();

	div = dot2 - dot1;
	if(fabs(div) < 0.00001f)
		t = 0.0f;
	else
		t = -dot1 / div;
	
	testPt = ray.m_Pos + (ray.m_Dir * t);
	
	// See if the intersection point lies inside the poly.
	return PointInPoly( testPt );
}


BOOL CBasePoly::GetTriangles( CMoArray<CBasePoly*> &polies )
{
	WORD			i, i1, i2;
	CReal			area;
	
	RestartLoop:;
	if( m_Indices > 3 )
	{	
		for( i=0; i < m_Indices; i++ )
		{
			i1 = m_Indices.NextI( i );
			i2 = m_Indices.NextI( i1 );

			// Get rid of collinear vertices.
			area = g_TriArea2( Normal(), Pt(i), Pt(i1), Pt(i2) );
			if( (area >= -0.1f) && (area <= 0.1f) )
			{
				m_Indices.Remove( i1 );
				goto RestartLoop;
			}

			if( ValidTriangle(i, i1, i2) )
			{
				polies.Append( new CBasePoly(m_pBrush, m_Indices[i], m_Indices[i1], m_Indices[i2], true) );

				m_Indices.Remove( i1 );
				goto RestartLoop;
			}
		}
	}
	
	if( m_Indices == 3 )
	{
		area = g_TriArea2( Normal(), Pt(0), Pt(1), Pt(2) );
		if( area <= -0.1f )
			polies.Append( new CBasePoly(m_pBrush, m_Indices[0], m_Indices[1], m_Indices[2], true) );
	}
	
	return TRUE;
}


/*
	Returns True if ab and cd intersect.
*/

CEditVert& CBasePoly::Pt( uint32 i )
{
	return m_pBrush->m_Points[m_Indices[i]];
}

CEditVert& CBasePoly::NextPt( uint32 i )
{
	return m_pBrush->m_Points[m_Indices.Next(i)];
}

BOOL CBasePoly::Intersect( const LTVector &a, const LTVector &b, const LTVector &c, const LTVector &d )
{
	CPlane		plane;
	BOOL		bIntersect;

	
	plane.m_Normal = Normal().Cross( b - a );
	plane.m_Dist = plane.m_Normal.Dot( a );
	bIntersect = ((plane.m_Normal.Dot(c)-plane.m_Dist)>0.0f) != ((plane.m_Normal.Dot(d)-plane.m_Dist)>0.0f);

	if( bIntersect )
	{
		plane.m_Normal = Normal().Cross( d - c );
		plane.m_Dist = plane.m_Normal.Dot( c );
		bIntersect = ((plane.m_Normal.Dot(a)-plane.m_Dist)>0.0f) != ((plane.m_Normal.Dot(b)-plane.m_Dist)>0.0f);

		return bIntersect;
	}
	else
		return FALSE;	
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CBasePoly::Diagonalie
//		PURPOSE:		Returns True if line ab is a proper (internal OR external)
//						diagonal of poly, or |a-b|=2 and a and b are collinear
//						with the middle vertex; only checks nonblocked condition.
// ----------------------------------------------------------------------- //

// talon: 16-bit in 2001: 0x401db0 compares k and k1 as WORDs and the
// count sign-extended from 16 bits, read again on every pass
BOOL CBasePoly::Diagonalie( WORD a, WORD b )
{
	WORD		k, k1;

	for( k=0; k < (int16)m_Indices.GetSize(); k++ )
	{
		k1 = m_Indices.NextI( k );

		// Skip edges incident to a or b.
		if( !( (k==a) || (k1==a) || (k==b) || (k1==b) ) )
		{
			if( Intersect(Pt(a), Pt(b), Pt(k), Pt(k1)) )
				return FALSE;
		}
	}

	return TRUE;
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CBasePoly::LeftOn
//		PURPOSE:		Tells if C is left of or on line AB.
// ----------------------------------------------------------------------- //

BOOL CBasePoly::LeftOn( WORD a, WORD b, WORD c )
{
	CReal area = g_TriArea2( Normal(), Pt(a), Pt(b), Pt(c) );
	return (area <= 0.0f);
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CBasePoly::Left
//		PURPOSE:		Tells if C is left of line AB.
// ----------------------------------------------------------------------- //

BOOL CBasePoly::Left( WORD a, WORD b, WORD c )
{
	CReal area = g_TriArea2( Normal(), Pt(a), Pt(b), Pt(c) );
	return (area < 0.0f);
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CBasePoly::InsideDiagonal
//		PURPOSE:		Tells if the diagonal (a,b) lies inside the polygon
//                      in the vicinity of a.
// ----------------------------------------------------------------------- //

BOOL CBasePoly::InsideDiagonal( WORD a, WORD b )
{
    uint32	a_plus_1, a_minus_1;

    a_plus_1 = m_Indices.NextI( a );
    a_minus_1 = m_Indices.PrevI( a );

    if( LeftOn(a_minus_1, a, a_plus_1) )
    {
		return Left(a, b, a_minus_1) && Left(b, a, a_plus_1);
    }
    else
    {
        return !( LeftOn(a, b, a_plus_1) && LeftOn(b, a, a_minus_1) );
    }
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CBasePoly::ValidTriangle
//		PURPOSE:		Tells if (a,b,c) is a valid triangle on this poly.
// ----------------------------------------------------------------------- //

BOOL CBasePoly::ValidTriangle( WORD a, WORD b, WORD c )
{
	if( InsideDiagonal(a, c) )
	{
		return Diagonalie( a, c );
	}
	else
		return FALSE;
}
