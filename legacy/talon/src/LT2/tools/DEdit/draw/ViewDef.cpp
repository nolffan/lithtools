//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : ViewDef.cpp
//
//	PURPOSE	  : Implements all the CViewDef classes.
//
//	CREATED	  : November 13 1996
//
//
//------------------------------------------------------------------

// Includes....
#include "bdefs.h"
#include "viewdef.h"


//------------------------------------------------------------------------------------------
// CViewDef
//------------------------------------------------------------------------------------------

void CViewDef::InitGrid(	int posX, int posY, int posZ,
							int forwardX, int forwardY, int forwardZ,
							int upX, int upY, int upZ,
							int drawSize
							)
{
	m_Grid.Pos().Init( (CReal)posX, (CReal)posY, (CReal)posZ );
	m_Grid.Forward().Init( (CReal)forwardX, (CReal)forwardY, (CReal)forwardZ );
	m_Grid.Up().Init( (CReal)upX, (CReal)upY, (CReal)upZ );
	m_Grid.MakeRight();
	m_Grid.m_DrawSize = (CReal)drawSize;
}


void CViewDef::InitNav(	CReal posX, CReal posY, CReal posZ,
						int forwardX, int forwardY, int forwardZ,
						int upX, int upY, int upZ,
						CReal lookAtX, CReal lookAtY, CReal lookAtZ )
{
	m_Nav.Init();
	m_Nav.Pos().Init( posX, posY, posZ );
	m_Nav.Forward().Init( (CReal)forwardX, (CReal)forwardY, (CReal)forwardZ );
	m_Nav.Up().Init( (CReal)upX, (CReal)upY, (CReal)upZ );
	m_Nav.MakeRight();
	m_Nav.m_LookAt.Init(lookAtX, lookAtY, lookAtZ);
}
					  
//------------------------------------------------------------------------------------------
// CParallelViewDef
//------------------------------------------------------------------------------------------


// talon: near (0x465679): the same code but for the scheduling of the
// last sum's stores against the copy into the returned ray.
CEditRay CParallelViewDef::MakeRayFromScreenPoint( CPoint point )
{
	CReal pointX = (CReal)point.x;
	CReal pointY = (CReal)point.y;
	CEditRay ray;

	ray.m_Pos = m_Nav.Pos() + 
				(m_Nav.Right() * ((pointX - m_pInfo->m_fHalfWidth) / m_Magnify) * m_pInfo->m_fPixelScaleX) + 
				(m_Nav.Up()    * ((m_pInfo->m_fHalfHeight - pointY) / m_Magnify) * m_pInfo->m_fPixelScaleY);
	ray.m_Dir = m_Nav.Forward();

	return ray;
}


void CParallelViewDef::ProjectPt( CVector &in, CPoint &out )
{
	out.x = (int) (m_pInfo->m_fProjectHalfWidth  + (in.x * m_Magnify));
	out.y = (int) (m_pInfo->m_fProjectHalfHeight - (in.y * m_Magnify));
}


void CParallelViewDef::ProjectPt( CVector &in, CVector &out )
{
	out.x = m_pInfo->m_fProjectHalfWidth  + (in.x * m_Magnify);
	out.y = m_pInfo->m_fProjectHalfHeight - (in.y * m_Magnify);
	out.z = in.z;
}


void CParallelViewDef::SetupFrustumPlanes( CPlane *pPlanes )
{
	pPlanes[0].m_Normal.Init( 1.0f, 0.0f, 0.0f );		// Left.
	pPlanes[0].m_Dist = -m_pInfo->m_fHalfWidth / m_Magnify;

	pPlanes[1].m_Normal.Init( 0.0f, -1, 0.0f );		// Top.
	pPlanes[1].m_Dist = -m_pInfo->m_fHalfHeight / m_Magnify;

	pPlanes[2].m_Normal.Init( -1, 0.0f, 0.0f );		// Right.
	pPlanes[2].m_Dist = -m_pInfo->m_fHalfWidth / m_Magnify;

	pPlanes[3].m_Normal.Init( 0.0f, 1.0f, 0.0f );		// Bottom.
	pPlanes[3].m_Dist = -m_pInfo->m_fHalfHeight / m_Magnify;

	pPlanes[4].m_Normal.Init( 0.0f, 0.0f, 1.0f );		// Near.
	pPlanes[4].m_Dist = m_NearZ;
	
	pPlanes[5].m_Normal.Init( 0.0f, 0.0f, -1 );		// Far.
	pPlanes[5].m_Dist = -m_FarZ;
}



//------------------------------------------------------------------------------------------
// CPerspectiveViewDef
//------------------------------------------------------------------------------------------

// talon: 2001's perspective is a fixed 90 degrees: the projection scale is
// the half size (0x4658d4..0x465b37).
CEditRay CPerspectiveViewDef::MakeRayFromScreenPoint( CPoint point )
{
	CReal		aspect;
	CReal		pointX = (CReal)point.x;
	CReal		pointY = (CReal)point.y;
	CEditRay	ray;

	aspect = m_pInfo->m_fWidth / m_pInfo->m_fHeight;
	ray.m_Pos = m_Nav.Pos();
	ray.m_Dir = m_Nav.Forward() * m_pInfo->m_fHalfWidth + m_Nav.Right() * (pointX - m_pInfo->m_fHalfWidth) - m_Nav.Up() * aspect * (pointY - m_pInfo->m_fHalfHeight);

	return ray;
}


void CPerspectiveViewDef::ProjectPt( CVector &in, CPoint &out )
{
	CReal oneOverZ = 1.0f / in.z;

	out.x = (int)(m_pInfo->m_fProjectHalfWidth  + ((m_pInfo->m_fProjectHalfWidth * in.x) * oneOverZ));
	out.y = (int)(m_pInfo->m_fProjectHalfHeight - ((in.y * m_pInfo->m_fProjectHalfHeight) * oneOverZ));
}


void CPerspectiveViewDef::ProjectPt( CVector &in, CVector &out )
{
	CReal oneOverZ = 1.0f / in.z;

	out.x = m_pInfo->m_fProjectHalfWidth  + ((m_pInfo->m_fProjectHalfWidth * in.x) * oneOverZ);
	out.y = m_pInfo->m_fProjectHalfHeight - ((in.y * m_pInfo->m_fProjectHalfHeight) * oneOverZ);
	out.z = in.z;
}


void CPerspectiveViewDef::SetupFrustumPlanes( CPlane *pPlanes )
{
	CReal			cosAngle = (CReal)cos(MATH_PI / 4.0f);
	CReal			sinAngle = (CReal)sin(MATH_PI / 4.0f);

	// Setup the clipping planes.
	pPlanes[0].m_Normal.Init( cosAngle, 0.0f, sinAngle );		// Left.
	pPlanes[0].m_Dist = 0.0f;

	pPlanes[1].m_Normal.Init( 0.0f, -cosAngle, sinAngle );		// Top.
	pPlanes[1].m_Dist = 0.0f;

	pPlanes[2].m_Normal.Init( -cosAngle, 0.0f, sinAngle );		// Right.
	pPlanes[2].m_Dist = 0.0f;

	pPlanes[3].m_Normal.Init( 0.0f, cosAngle, sinAngle );		// Bottom.
	pPlanes[3].m_Dist = 0.0f;

	pPlanes[4].m_Normal.Init( 0.0f, 0.0f, 1.0f );				// Near.
	pPlanes[4].m_Dist = m_NearZ;
	
	pPlanes[5].m_Normal.Init( 0.0f, 0.0f, -1 );					// Far.
	pPlanes[5].m_Dist = -m_FarZ;
}
