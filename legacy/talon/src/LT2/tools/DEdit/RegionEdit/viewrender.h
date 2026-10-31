//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//

// This module holds all the rendering-related structures
// for a CRegionView.  (These are all renderer-independent, but
// they define all the viewing parameters).

#ifndef __VIEWRENDER_H__
#define __VIEWRENDER_H__


	#include "drawmgr.h"
	#include "viewdef.h"


	class CRegionView;
	

	// Defines....
	#define VM_TOP				0
	#define VM_BOTTOM			1
	#define VM_LEFT				2
	#define VM_RIGHT			3
	#define VM_FRONT			4
	#define VM_BACK				5
	#define VM_PERSPECTIVE		6

	#define NUM_VIEWDEFS		7


	// talon: 2001's CViewRender (0x514 bytes; CRegionView's second base at
	// +0x40, no vtable): the constructor is 0x46d22a, the functions
	// 0x46d2a7..0x46db4e.  There's a draw manager type besides the D3D
	// device and mode (RestartRender takes five arguments, 0x46d2eb), and no
	// lighting members.
	class CViewRender
	{
		public:

							CViewRender();
							~CViewRender();

			void			SetViewRenderRegionView(CRegionView *pView);
			void			DrawRect(CRect *pRect=NULL);

			// drawType is 0 for the plain draw manager, otherwise D3D's (it
			// falls back to the plain one); -1 keeps the current value.
			void			RestartRender(BOOL bForce, int drawType, int deviceNum, int renderMode, BOOL bRedraw);

			void			SetupInitialDrawMgr();
			void			SetupNewSize(int x, int y);

		private:

			void			InitFrame();


		public:

			// Transformation stuff.
			CMatrix			m_Transform;
			CMatrix			m_Rotation;
			CPlane			m_ClipPlanes[6];
			DWORD			m_nClipPlanesToUse;  // 4 in parallel mode, 6 in perspective.


		public:

			int				m_DrawType;
			DrawMgr			*m_pDrawMgr;
			int				m_DeviceNum;
			int				m_RenderMode;

			CViewDef		*m_pViewDef;
			CRegionView		*m_pView;


		public:

			CViewDefInfo*	ViewDefInfo()			{ return &m_ViewDefInfo; }
			CViewDef*		ViewDef()				{ return m_pViewDef; }
			CNavigator&		Nav()					{ return m_pViewDef->m_Nav; }
			CEditGrid&		EditGrid()				{ return m_pViewDef->m_Grid; }
			CReal&			NearZ()					{ return m_pViewDef->m_NearZ; }
			CReal&			FarZ()					{ return m_pViewDef->m_FarZ; }

			BOOL			IsPerspectiveViewType()	{ return m_pViewDef->ViewType() == PERSPECTIVE_VIEWTYPE; }
			BOOL			IsParallelViewType()	{ return m_pViewDef->ViewType() == PARALLEL_VIEWTYPE; }

			void			InitViewDefs();

			void			SetViewMode( uint32 mode );
			uint32			GetViewMode();


		public:

			BOOL InsideFrustum(const LTVector &vec ) const
			{
				for(uint32 i=0; i < m_nClipPlanesToUse; i++ )
					if( !InsidePlane(m_ClipPlanes[i], vec) )
						return FALSE;

				return TRUE;
			}

			bool SphereInsideFrustum(const LTVector &vCenter, float fRadius) const
			{
				for(uint32 i=0; i < m_nClipPlanesToUse; i++ )
					if( !SphereInsidePlane(m_ClipPlanes[i], vCenter, fRadius) )
						return false;

				return true;
			}

			BOOL				ClipLineToFrustum(TLVertex *pVerts);

			// talon: stand-in: Build 69's clip mask, for its DrawBase.
			BOOL				ClipLineToFrustum(TLVertex *pVerts, uint32 nClipMask)	{ return ClipLineToFrustum(pVerts); }

			BOOL				TransformAndProjectInFrustum( CVector &vec, CPoint &point );

			void				TransformPt( CVector &pt );
			void				TransformPt( CVector &in, CVector &out );

			BOOL				InsidePlane(const LTPlane &plane, const LTVector &vec ) const;
			bool				SphereInsidePlane( const LTPlane& plane, const LTVector &vCenter, float fRadius) const;
			void				IntersectPlane( CPlane &plane, CVector &dest, CVector &start, CVector &end );


		public:

			// talon: stand-in: Build 69's lighting, for its DrawBase; 2001's
			// CViewRender has no room for it (sizeof 0x514), so the state is
			// static.
			void				InitLighting(const LTVector& vWorldSpaceLight, CReal fAmbient);
			CReal				WorldSpaceLightInt(const LTVector& vNormal);
			CReal				CameraSpaceLightInt(const LTVector& vNormal);
			CReal				AdjustDotLight(CReal fDot);

			static LTVector		m_vWorldSpaceLight;
			static LTVector		m_vCameraSpaceLight;
			static CReal		m_fAmbientScale;
			static CReal		m_fAmbientLight;


		public:

			CParallelViewDef	m_ParallelViews[6];
			CPerspectiveViewDef	m_PerspectiveView;

			CViewDefInfo	m_ViewDefInfo;
			CViewDef		*m_ViewDefs[NUM_VIEWDEFS];

		private:

			uint32			m_nViewMode;
	};


	inline CReal CViewRender::WorldSpaceLightInt(const LTVector& vNormal)
	{
		return AdjustDotLight(m_vWorldSpaceLight.Dot(vNormal));
	}

	inline CReal CViewRender::CameraSpaceLightInt(const LTVector& vNormal)
	{
		return AdjustDotLight(m_vCameraSpaceLight.Dot(vNormal));
	}

	//given a number between -1 and 1, it will map it into the appropriate lighting
	//color
	inline CReal CViewRender::AdjustDotLight(CReal fDot)
	{
		return fDot * m_fAmbientScale + m_fAmbientLight;
	}

	inline void CViewRender::TransformPt( CVector &pt )
	{
		m_Transform.Apply( pt );
	}


	inline void CViewRender::TransformPt( CVector &in, CVector &out )
	{
		m_Transform.Apply( in, out );
	}


	inline BOOL CViewRender::InsidePlane(const LTPlane &plane, const LTVector &vec ) const
	{
		return (plane.m_Normal.Dot(vec) - plane.m_Dist) > 0.0f;
	}

	inline bool CViewRender::SphereInsidePlane(const LTPlane& plane, const LTVector& vCenter, float fRadius) const
	{
		return (plane.m_Normal.Dot(vCenter) - plane.m_Dist) > -fRadius;
	}


	inline void CViewRender::IntersectPlane( CPlane &plane, CVector &dest, CVector &start, CVector &end )
	{
		CReal		dot1 = plane.m_Normal.Dot(start) - plane.m_Dist;
		CReal		dot2 = plane.m_Normal.Dot(end) - plane.m_Dist;
		CReal		t = -dot1 / (dot2 - dot1);

		dest = start + ((end - start) * t);
	}


#endif // __VIEWRENDER_H__






