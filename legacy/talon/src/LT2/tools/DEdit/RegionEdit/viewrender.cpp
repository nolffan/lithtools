#include "bdefs.h"
#include "viewrender.h"
#include "regiondoc.h"
#include "regionview.h"
#include "optionsdisplay.h"

// talon: 2001's (0x46d22a..0x46db7a), from the image.

// talon: the plain (non-D3D) draw manager's creator, 0x42b02b: a 2001
// module of part 4's range (after EditRegion) that the project hasn't got.
extern DrawMgr* dm_CreateNormalDrawMgr(CRegionView *pView);

// talon: stand-in: Build 69's lighting state (see viewrender.h).
LTVector	CViewRender::m_vWorldSpaceLight;
LTVector	CViewRender::m_vCameraSpaceLight;
CReal		CViewRender::m_fAmbientScale;
CReal		CViewRender::m_fAmbientLight;


CViewRender::CViewRender()
{
	InitViewDefs();

	m_ViewDefInfo.m_fPixelScaleY = 1.0f;
	m_nViewMode = VM_TOP;
	m_DeviceNum = 0;
	m_ViewDefInfo.m_fPixelScaleX = 1.0f;
	m_RenderMode = 0;
	m_DrawType = 0;
	m_pDrawMgr = NULL;
}


CViewRender::~CViewRender()
{
	if(m_pDrawMgr)
	{
		delete m_pDrawMgr;
		m_pDrawMgr = NULL;
	}
}


void CViewRender::SetViewRenderRegionView(CRegionView *pView)
{
	m_pView = pView;
}


void CViewRender::DrawRect(CRect *pRect)
{
	InitFrame();
	
	if(m_pDrawMgr)
	{
		m_pDrawMgr->Draw();
	}
}


void CViewRender::RestartRender(BOOL bForce, int drawType, int deviceNum, int renderMode, BOOL bRedraw)
{
	if(drawType == -1) drawType = m_DrawType;
	if(deviceNum == -1) deviceNum = m_DeviceNum;
	if(renderMode == -1) renderMode = m_RenderMode;
	
	// Try to avoid it..
	if(!bForce)
	{
		if((drawType == m_DrawType) && (deviceNum == m_DeviceNum) && (renderMode == m_RenderMode))
			return;
	}
	
	if(m_pDrawMgr)
	{
		delete m_pDrawMgr;
		m_pDrawMgr = NULL;
	}

	m_DrawType = drawType;
	m_DeviceNum = deviceNum;
	m_RenderMode = renderMode;

	if(drawType == 0)
	{
		m_pDrawMgr = dm_CreateNormalDrawMgr(m_pView);
	}
	else
	{
		m_pDrawMgr = dm_CreateDirect3dDrawMgr(m_pView, &m_DeviceNum, &m_RenderMode);
		if(!m_pDrawMgr)
			m_pDrawMgr = dm_CreateNormalDrawMgr(m_pView);
	}

	if(bRedraw)
	{
		DrawRect();
	}
}


void CViewRender::SetupInitialDrawMgr()
{
	// The display options class
	COptionsDisplay *pDisplayOptions=GetApp()->GetOptions().GetDisplayOptions();
	
	// Determine if the D3D renderer should be used by default
	if(pDisplayOptions->IsUseD3DAsDefault())
	{
		RestartRender(TRUE, 1, pDisplayOptions->GetDefaultD3DDevice(), pDisplayOptions->GetDefaultD3DMode(), FALSE);
	}
	else
	{
		RestartRender(TRUE, -1, -1, -1, FALSE);
	}
}


void CViewRender::SetupNewSize(int width, int height)
{
	m_ViewDefInfo.m_Width		= width;
	m_ViewDefInfo.m_Height		= height;
	m_ViewDefInfo.m_fWidth		= (CReal)width;
	m_ViewDefInfo.m_fHeight		= (CReal)height;
	m_ViewDefInfo.m_fHalfWidth	= m_ViewDefInfo.m_fWidth * 0.5f;
	m_ViewDefInfo.m_fHalfHeight	= m_ViewDefInfo.m_fHeight * 0.5f;
	m_ViewDefInfo.m_fInvHalfWidth	= 1.0f / m_ViewDefInfo.m_fHalfWidth;
	m_ViewDefInfo.m_fInvHalfHeight	= 1.0f / m_ViewDefInfo.m_fHalfHeight;

	m_ViewDefInfo.m_fProjectWidth		= (CReal)(width / m_pView->m_nPixelScale);
	m_ViewDefInfo.m_fProjectHeight		= (CReal)(height / m_pView->m_nPixelScale);
	m_ViewDefInfo.m_fProjectHalfWidth	= m_ViewDefInfo.m_fProjectWidth * 0.5f;
	m_ViewDefInfo.m_fProjectHalfHeight	= m_ViewDefInfo.m_fProjectHeight * 0.5f;
	m_ViewDefInfo.m_fInvProjectHalfWidth	= 1.0f / m_ViewDefInfo.m_fProjectHalfWidth;
	m_ViewDefInfo.m_fInvProjectHalfHeight	= 1.0f / m_ViewDefInfo.m_fProjectHalfHeight;

	if(m_pDrawMgr)
	{
		m_pDrawMgr->Resize(width, height);
	}
}


void CViewRender::InitFrame()
{
	// Setup the transformation.	
	Nav().PutInMatrix( m_Transform, FALSE );
	m_Transform.m[0][3]  = -m_Transform.m[0][3];
	m_Transform.m[1][3]  = -m_Transform.m[1][3];
	m_Transform.m[2][3]  = -m_Transform.m[2][3];

	m_Rotation = m_Transform;
	m_Rotation.m[0][3] = m_Rotation.m[1][3] = m_Rotation.m[2][3] = 0.0f;

	// Setup frustum planes.
	m_pViewDef->SetupFrustumPlanes( m_ClipPlanes );
}


void CViewRender::InitViewDefs()
{
	int				i, defaultDrawSize;
	CReal			defDist;
	CViewDef		*pDef;
	

	defaultDrawSize = 24 * 256;
	defDist = (CReal)defaultDrawSize;
	
	m_ViewDefs[6] = &m_PerspectiveView;
	for( i=0; i < 6; i++ )
		m_ViewDefs[i] = &m_ParallelViews[i];

	for( i=0; i < NUM_VIEWDEFS; i++ )
	{
		m_ViewDefs[i]->m_Magnify	= 1.0f;
		m_ViewDefs[i]->m_pInfo		= ViewDefInfo();
		m_ViewDefs[i]->m_NearZ		= 1.0f;
		m_ViewDefs[i]->m_FarZ		= 10000.0f;
	}

	m_PerspectiveView.m_FarZ = 2000.0f;

	m_pViewDef = &m_PerspectiveView;

	// Init the perspective view.
	pDef = &m_PerspectiveView;
	pDef->InitGrid( 0, 0, 0,    0, 1, 0,    0, 0, 1,    defaultDrawSize );
	pDef->InitNav( 0.0f, 200.1f, -200.1f,   0, 1, 0,    1, 0, 0,    0.0f, 0.0f, 0.0f );
	pDef->m_Nav.UpdateViewerDistance();
	pDef->m_Nav.UpdateLooking();

	// Top.
	m_ViewDefs[VM_TOP]->InitGrid( 0, 0, 0,   0, 1, 0,    0, 0, 1,    defaultDrawSize );
	m_ViewDefs[VM_TOP]->InitNav( 0.0f, defDist, 0.0f,   0, -1, 0,   0, 0, 1,    0.0f, 0.0f, 0.0f );

	// Bottom.
	m_ViewDefs[VM_BOTTOM]->InitGrid( 0, 0, 0,   0, 1, 0,    0, 0, 1,    defaultDrawSize );
	m_ViewDefs[VM_BOTTOM]->InitNav( 0.0f, -defDist, 0.0f,   0, 1, 0,   0, 0, 1,    0.0f, 0.0f, 0.0f );

	// Left.
	m_ViewDefs[VM_LEFT]->InitGrid( 0, 0, 0,   1, 0, 0,    0, 1, 0,    defaultDrawSize );
	m_ViewDefs[VM_LEFT]->InitNav( -defDist, 0.0f, 0.0f,   1, 0, 0,   0, 1, 0,    0.0f, 0.0f, 0.0f );

	// Right.
	m_ViewDefs[VM_RIGHT]->InitGrid( 0, 0, 0,   1, 0, 0,    0, 1, 0,    defaultDrawSize );
	m_ViewDefs[VM_RIGHT]->InitNav( defDist, 0.0f, 0.0f,   -1, 0, 0,   0, 1, 0,    0.0f, 0.0f, 0.0f );

	// Front.
	m_ViewDefs[VM_FRONT]->InitGrid( 0, 0, 0,   0, 0, 1,    0, 1, 0,    defaultDrawSize );
	m_ViewDefs[VM_FRONT]->InitNav( 0.0f, 0.0f, -defDist,   0, 0, 1,   0, 1, 0,    0.0f, 0.0f, 0.0f );

	// Back.
	m_ViewDefs[VM_BACK]->InitGrid( 0, 0, 0,   0, 0, 1,    0, 1, 0,    defaultDrawSize );
	m_ViewDefs[VM_BACK]->InitNav( 0.0f, 0.0f, defDist,   0, 0, -1,   0, 1, 0,    0.0f, 0.0f, 0.0f );
}


void CViewRender::SetViewMode( uint32 nMode )
{
	ASSERT( nMode < NUM_VIEWDEFS );
	
	m_pViewDef = m_ViewDefs[nMode];
	m_nViewMode = nMode;

	//setup the number of clipping planes for each view type; the parallel
	//views always use the plain draw manager
	if(m_pViewDef->ViewType() == PERSPECTIVE_VIEWTYPE)
	{
		m_nClipPlanesToUse = 6;
	}
	else
	{
		RestartRender(FALSE, 0, -1, -1, TRUE);
		m_nClipPlanesToUse = 4;
	}
}


uint32 CViewRender::GetViewMode()
{
	return m_nViewMode;
}


BOOL CViewRender::ClipLineToFrustum(TLVertex *pVerts)
{
	DWORD i;
	BOOL in[2];
	float dot[2];
	TLVertex *pDest;
	float t;

	for( i=0; i < m_nClipPlanesToUse; i++ )
	{
		CPlane &plane = m_ClipPlanes[i];

		dot[0] = DIST_TO_PLANE(pVerts[0].m_Vec, plane);
		dot[1] = DIST_TO_PLANE(pVerts[1].m_Vec, plane);
		in[0] = dot[0] > 0.0f;
		in[1] = dot[1] > 0.0f;

		// Both out?
		if(!in[0] && !in[1])
			return FALSE;

		if( in[0] != in[1] )
		{
			pDest = &pVerts[in[0]];

			t = -dot[0] / (dot[1] - dot[0]);
			VEC_LERP(pDest->m_Vec, pVerts[0].m_Vec, pVerts[1].m_Vec, t);
			TLVertex::ClipExtra(pDest, &pVerts[0], &pVerts[1], t);
		}
	}

	return TRUE;
}


BOOL CViewRender::TransformAndProjectInFrustum( CVector &vec, CPoint &point )
{
	CVector		transformed;

	
	m_Transform.Apply( vec, transformed );
	if( !InsideFrustum(transformed) )
		return FALSE;

	m_pViewDef->ProjectPt( transformed, point );
	return TRUE;
}


// talon: stand-in: Build 69's lighting, for its DrawBase (not in 2001).
void CViewRender::InitLighting(const LTVector& vWorldSpaceLight, CReal fAmbient)
{
	m_vWorldSpaceLight = vWorldSpaceLight;
	m_vWorldSpaceLight.Norm();

	m_vCameraSpaceLight = m_Transform * m_vWorldSpaceLight;
	LTVector vTransOrigin = m_Transform * LTVector(0, 0, 0);
	m_vCameraSpaceLight = m_vCameraSpaceLight - vTransOrigin;

	fAmbient = LTCLAMP(fAmbient, 0.0f, 1.0f);

	m_fAmbientScale = (1.0f - fAmbient) / 2;
	m_fAmbientLight = fAmbient + m_fAmbientScale;

	m_vCameraSpaceLight.Norm();
}
