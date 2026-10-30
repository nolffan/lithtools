//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : ViewDef.h
//
//	PURPOSE	  : Defines the CViewDef class used to abstract any
//              differences between parallel and perspective views.
//
//	CREATED	  : November 13 1996
//
//
//------------------------------------------------------------------

#ifndef __VIEWDEF_H__
	#define __VIEWDEF_H__


	// Includes....
	#include "navigator.h"
	#include "editray.h"
	#include "editgrid.h"


	#define PERSPECTIVE_VIEWTYPE		0
	#define PARALLEL_VIEWTYPE			1


	// talon: 2001's CViewDefInfo is 0x40 bytes (CViewRender +0x4b4..+0x4f4):
	// CViewRender::SetupNewSize (0x46d3f5) fills it up to +0x34, the pixel
	// scales are 1.0f from the constructor (0x46d22a) and are read through
	// m_pInfo by CParallelViewDef::MakeRayFromScreenPoint (0x465679).
	class CViewDefInfo
	{
		public:

			DWORD	m_Width, m_Height;
			CReal	m_fWidth, m_fHeight;
			CReal	m_fHalfWidth, m_fHalfHeight;
			CReal	m_fInvHalfWidth, m_fInvHalfHeight;

			// The size in pixels of the projection (the window's size over the
			// view's pixel size).
			CReal	m_fProjectWidth, m_fProjectHeight;
			CReal	m_fProjectHalfWidth, m_fProjectHalfHeight;
			CReal	m_fInvProjectHalfWidth, m_fInvProjectHalfHeight;

			CReal	m_fPixelScaleX, m_fPixelScaleY;
	};



	// talon: 2001's view definitions (0x46558c..0x465b76): 0x88 bytes each,
	// no view type member (ViewType() is virtual: 0x46d2a0, 0x46d2a3), a
	// fixed 90 degree perspective (no FOV, aspect or UpdateSize).  The
	// vtables (0x4ddf24, 0x4ddf38) are MakeRayFromScreenPoint, the two
	// ProjectPts (VC6 groups the overloads, the later one first),
	// SetupFrustumPlanes, ViewType.
	class CViewDef
	{
		public:

			void				InitGrid(	int posX, int posY, int posZ,
											int forwardX, int forwardY, int forwardZ,
											int upX, int upY, int upZ,
											int drawSize
										);

			void				InitNav(	CReal posX, CReal posY, CReal posZ,
											int forwardX, int forwardY, int forwardZ,
											int upX, int upY, int upZ,
											CReal lookAtX, CReal lookAtY, CReal lookAtZ );

			virtual CEditRay	MakeRayFromScreenPoint( CPoint point )=0;
			virtual void		ProjectPt( CVector &in, CPoint &out )=0;
			virtual void		ProjectPt( CVector &in, CVector &out )=0;
			virtual void		SetupFrustumPlanes( CPlane *pPlanes )=0;
			virtual uint32		ViewType()=0;

		public:

			CNavigator			m_Nav;
			CEditGrid			m_Grid;
			CReal				m_NearZ, m_FarZ;
			CViewDefInfo		*m_pInfo;
			CReal				m_Magnify;
	};



	class CPerspectiveViewDef : public CViewDef
	{
		public:

			CEditRay	MakeRayFromScreenPoint( CPoint point );
			void		ProjectPt( CVector &in, CPoint &out );
			void		ProjectPt( CVector &in, CVector &out );
			void		SetupFrustumPlanes( CPlane *pPlanes );
			uint32		ViewType()		{ return PERSPECTIVE_VIEWTYPE; }

			// talon: stand-in: Build 69's camera settings, for its callers
			// (MainFrm, CameraFOVDlg): the 2001 perspective is a fixed 90
			// degrees.
			void		SetupCamera(CReal fVertFOV, bool bUseAspect)	{}
			CReal		GetHorzFOV() const			{ return MATH_HALFPI; }
			CReal		GetVertFOV() const			{ return MATH_HALFPI; }
			bool		IsUseAspect() const			{ return false; }
	};



	class CParallelViewDef : public CViewDef
	{
		public:

			CEditRay	MakeRayFromScreenPoint( CPoint point );
			void		ProjectPt( CVector &in, CPoint &out );
			void		ProjectPt( CVector &in, CVector &out );
			void		SetupFrustumPlanes( CPlane *pPlanes );
			uint32		ViewType()		{ return PARALLEL_VIEWTYPE; }
	};


#endif  // __VIEWDEF_H__

