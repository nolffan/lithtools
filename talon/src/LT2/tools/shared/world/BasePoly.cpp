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


CBasePoly::CBasePoly( CEditBrush *pBrush, uint32 i0, uint32 i1, uint32 i2, bool bUpdatePlane )
{
	m_pBrush = pBrush;
	m_Indices.SetSize( 3 );
	m_Indices[0] = i0;
	m_Indices[1] = i1;
	m_Indices[2] = i2;

	if( bUpdatePlane )
		UpdatePlane();
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

bool CBasePoly::IsValid()
{
	for( uint32 i=0; i < NumVerts(); i++ )
		if( m_Indices[i] >= m_pBrush->m_Points )
			return false;

	return true;
}


void CBasePoly::DecrementPoints( uint32 index )
{
	for( uint32 i=0; i < m_Indices; i++ )
		if( m_Indices[i] >= index )
			--m_Indices[i];
}


bool CBasePoly::PointInPlane( const LTVector &point )
{
	CReal dist = Normal().Dot( point ) - Dist();
	return (dist > -POINT_SIDE_EPSILON) && (dist < POINT_SIDE_EPSILON);
}



bool CBasePoly::PointInPoly( const LTVector &point )
{
	CReal		area;

	for(uint32 i=0; i < NumVerts(); i++ )
	{
		area = g_TriArea2( Normal(), Pt(i), NextPt(i), point );
		if( area > 0 )
			return false;
	}

	return true;
}


void CBasePoly::RemoveCollinearVertices()
{
	int32			i;
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


bool CBasePoly::IsConvex()
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
				return false;
			else if( sign > 0.0f && dot < 0.0f )
				return false;
		}
	}

	return true;
}


void CBasePoly::GetCenterPoint( CVector &centerPt )
{
	centerPt.Init( 0.0f, 0.0f, 0.0f );
	for(uint32 i=0; i < NumVerts(); i++ )
		centerPt += Pt(i);

	centerPt /= (CReal)NumVerts();
}


void CBasePoly::Flip()
{
	for(uint32 i=0; i < m_Indices/2; i++ )
	{
		uint32 temp = m_Indices[i];
		m_Indices[i] = m_Indices[m_Indices-i-1];
		m_Indices[m_Indices-i-1] = temp;
	}

	Normal() = -Normal();
	Dist() = -Dist();
}


void CBasePoly::UpdatePlane()
{
	g_GenerateNormal( this );
}


bool CBasePoly::GetTriangles( CMoArray<CBasePoly*> &polies )
{
	uint32			i, i1, i2;
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
	
	return true;
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

bool CBasePoly::Intersect( const LTVector &a, const LTVector &b, const LTVector &c, const LTVector &d )
{
	CPlane		plane;
	bool		bIntersect;

	
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
		return false;	
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CBasePoly::Diagonalie
//		PURPOSE:		Returns True if line ab is a proper (internal OR external)
//						diagonal of poly, or |a-b|=2 and a and b are collinear
//						with the middle vertex; only checks nonblocked condition.
// ----------------------------------------------------------------------- //

bool CBasePoly::Diagonalie( uint32 a, uint32 b )
{
	uint32		k, k1;

	for( k=0; k < m_Indices; k++ )
	{
		k1 = m_Indices.NextI( k );

		// Skip edges incident to a or b.
		if( !( (k==a) || (k1==a) || (k==b) || (k1==b) ) )
		{
			if( Intersect(Pt(a), Pt(b), Pt(k), Pt(k1)) )
				return false;
		}
	}

	return true;
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CBasePoly::LeftOn
//		PURPOSE:		Tells if C is left of or on line AB.
// ----------------------------------------------------------------------- //

bool CBasePoly::LeftOn( uint32 a, uint32 b, uint32 c )
{
	CReal area = g_TriArea2( Normal(), Pt(a), Pt(b), Pt(c) );
	return (area <= 0.0f);
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CBasePoly::Left
//		PURPOSE:		Tells if C is left of line AB.
// ----------------------------------------------------------------------- //

bool CBasePoly::Left( uint32 a, uint32 b, uint32 c )
{
	CReal area = g_TriArea2( Normal(), Pt(a), Pt(b), Pt(c) );
	return (area < 0.0f);
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CBasePoly::InsideDiagonal
//		PURPOSE:		Tells if the diagonal (a,b) lies inside the polygon
//                      in the vicinity of a.
// ----------------------------------------------------------------------- //

bool CBasePoly::InsideDiagonal( uint32 a, uint32 b )
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

bool CBasePoly::ValidTriangle( uint32 a, uint32 b, uint32 c )
{
	if( InsideDiagonal(a, c) )
	{
		return Diagonalie( a, c );
	}
	else
		return false;
}




