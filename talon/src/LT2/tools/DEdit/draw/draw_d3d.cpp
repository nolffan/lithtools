//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//

#include "bdefs.h"
#include "dedit.h"
#include "draw_d3d.h"
#include "d3d_texturemgr.h"
#include "projectbar.h"
#include "cyclemgr.h"
#include "geomroutines.h"
#include "preworld.h"
#include "prepoly.h"
#include "presurface.h"
#include "prelightmap.h"


// ------------------------------------------------------------------------ //
// Structures.
// ------------------------------------------------------------------------ //

//#define TLVERTEX_FORMAT (D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_DIFFUSE | D3DFVF_SPECULAR)
#define DRAWPRIM_FLAGS	(D3DDP_DONOTCLIP|D3DDP_DONOTLIGHT|D3DDP_DONOTUPDATEEXTENTS)

#define RMFLAG_VIDEOMEMORY	1	// Use video memory surfaces.
#define RMFLAG_DONTUSE		2	// Don't try to use this mode.

typedef struct RenderModeInfo_t
{
	CLSID	*m_clsid;
	char	*m_ModeName;
	DWORD	m_Flags;
} RenderModeInfo;


// Vertex borders aren't drawn when vertex is smaller than this number.
#define VERT_SIZE_BORDER_CUTOFF 2.0f

#define VIEW pRender->m_pView
#define LIGHTMAP_BIAS_AMT	-1.0f


// ------------------------------------------------------------------------ //
// Globals.
// ------------------------------------------------------------------------ //

static DLink g_Direct3dDrawMgrs(LTLink_Init);


// Set by the texture manager.
extern SDWORD g_bBilinear;


float g_TextureMipScale;
float g_uMul, g_vMul;

float g_fDetailTextureScale;
float g_fDetailTextureAngleCos;
float g_fDetailTextureAngleSin;

static float g_xMax, g_yMax;

static int g_WantedDevice, g_DeviceCount;
static GUID *g_pGuid;
static BOOL g_bFoundDevice;
static DWORD g_DriverCaps;

int g_nNames, g_nMaxNames;
char **g_pNames;


// Render mode infos, in order of preference.
static RenderModeInfo g_RenderModeInfos[] =
{
	(CLSID*)&IID_IDirect3DHALDevice, "Hardware accelerated", RMFLAG_VIDEOMEMORY,
	(CLSID*)&IID_IDirect3DMMXDevice, "Software MMX", 0,
	(CLSID*)&IID_IDirect3DRampDevice, "Software ramp emulation", RMFLAG_DONTUSE,
	(CLSID*)&IID_IDirect3DRGBDevice, "Software RGB emulation", 0,
};

#define NUM_RENDERMODEINFOS (sizeof(g_RenderModeInfos) / sizeof(g_RenderModeInfos[0]))



// ------------------------------------------------------------------------ //
// Enumeration functions..
// ------------------------------------------------------------------------ //

// Used to enumerate the z-buffer formats.  Stores the best format in g_ZFormat.
HRESULT WINAPI D3DRender_EnumZFormats(LPDDPIXELFORMAT pFormat, LPVOID pContext)
{
	D3DRender *pRender;

	pRender = (D3DRender*)pContext;
	if(!pRender)
		return D3DENUMRET_OK;

	if(!pRender->m_bGotZFormat)
	{
		pRender->m_bGotZFormat = TRUE;
		memcpy(&pRender->m_ZFormat, pFormat, sizeof(pRender->m_ZFormat));
	}

	return D3DENUMRET_OK;
}			   

static BOOL PASCAL __stdcall d3d_EnumDDrawObjects(GUID* pGuid, char* sDriverDescription, char* sDriverName, void* pContext)
{
	LPDIRECTDRAW lpDD;
	HRESULT ddrval;
	DDCAPS  driverCaps;
	DDCAPS  helCaps;


	// Create a DirectDraw driver with the supplied guid
	ddrval = DirectDrawCreate(pGuid, &lpDD, NULL);
	if (ddrval != DD_OK)
		return DDENUMRET_OK;

	// Zero caps structure
	memset(&driverCaps, 0, sizeof(DDCAPS));
	driverCaps.dwSize = sizeof(DDCAPS);

	memset(&helCaps, 0, sizeof(DDCAPS));
	helCaps.dwSize = sizeof(DDCAPS);
	
	// Retrieve caps
	ddrval = lpDD->GetCaps(&driverCaps, &helCaps);
	if(ddrval != DD_OK)
	{
		lpDD->Release();
		return DDENUMRET_OK;
	}

	// Test for hardware 3D if need be. 
	if(g_DeviceCount == g_WantedDevice)
	{
		g_DriverCaps = driverCaps.dwCaps;
		if(pGuid)
		{
			g_pGuid = (GUID*)malloc(sizeof(GUID));
			*g_pGuid = *pGuid;
		}
		else
		{
			g_pGuid = NULL;
		}

		g_bFoundDevice = TRUE;
	}
	else
	{
		++g_DeviceCount;
	}

	// talon: 2001 (0x419551).
	if(g_bFoundDevice)
		AddDebugMessage("Chose d3d driver %s, description %s", sDriverName, sDriverDescription);

	// This is not a 3D driver
	lpDD->Release();
	return g_bFoundDevice ? DDENUMRET_CANCEL : DDENUMRET_OK;
}



// ------------------------------------------------------------------------ //
// Initialization.
// ------------------------------------------------------------------------ //

D3DRender::D3DRender(CRegionView *pView) : DrawBase(pView)
{
	m_bPerspectiveProjection = FALSE;
	m_pDibMgr = NULL;
	dl_TieOff(&m_Link);
	m_pDirectDraw = NULL;
	m_pDirect3d = NULL;
	m_pSurface = NULL;
	m_pZSurface = NULL;
	m_pPrimarySurface = NULL;
	m_pDevice = NULL;
	m_pMaterial = NULL;
	m_pViewport = NULL;
	m_pClipper = NULL;
	m_SurfaceWidth = m_SurfaceHeight = 0;
	dl_TieOff(&m_TextureFormats);
	m_pTextureFormat = NULL;
	dl_TieOff(&m_Textures);
	m_BindingNumber = 0;
}


D3DRender::~D3DRender()
{
	Term();
}


BOOL D3DRender::Init(int deviceNum, int renderMode, CRect *pRect)
{
	HRESULT hResult;
	DDSURFACEDESC2 ddsd;
	DWORD memoryTypeFlag;
	D3DVIEWPORT2 viewportData;
	D3DMATERIALHANDLE handle;
	int uniqueID;
	BOOL bUnique;
	DLink *pCurMgr;
	D3DDEVICEDESC helCaps;
	LPDIRECTDRAW pDDrawBase;
	CLSID id;


	id = *g_RenderModeInfos[renderMode].m_clsid;


	// Don't try to use a mode that isn't enabled.
	if(g_RenderModeInfos[renderMode].m_Flags & RMFLAG_DONTUSE)
		return FALSE;


	g_WantedDevice = deviceNum;
	g_DeviceCount = 0;
	g_pGuid = NULL;
	g_bFoundDevice = FALSE;

	// Enumerate the devices.
	DirectDrawEnumerate(d3d_EnumDDrawObjects, NULL);

	if(!g_bFoundDevice)
	{
		AddDebugMessage("Couldn't find device %d", deviceNum);
		return FALSE;
	}

	// Create the directdraw and direct3d objects.
	hResult = DirectDrawCreate(g_pGuid, &pDDrawBase, NULL);

	if(g_pGuid)
		free(g_pGuid);

	if(hResult != DD_OK)
	{
		AddDebugMessage("DirectDrawCreate failed.");
		Term();
		return FALSE;
	}

	// Get the real DDraw we need.
	m_pDirectDraw = NULL;
	hResult = pDDrawBase->QueryInterface(IID_IDirectDraw4, (void**)&m_pDirectDraw);
	if(hResult != DD_OK)
	{
		AddDebugMessage("QueryInterface(IID_IDirectDraw4) failed.");
		pDDrawBase->Release();
		Term();
		return FALSE;
	}	


	hResult = m_pDirectDraw->SetCooperativeLevel(m_pView->GetSafeHwnd(), DDSCL_NORMAL);
	if(hResult != DD_OK)
	{
		AddDebugMessage("SetCooperativeLevel failed.");
		Term();
		return FALSE;
	}

	hResult = m_pDirectDraw->QueryInterface(IID_IDirect3D3, (void**)&m_pDirect3d);
	if(hResult != DD_OK)
	{
		AddDebugMessage("QueryInterface(IID_IDirect3D) failed.");
		Term();
		return FALSE;
	}


	// First, create primary surface
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize				= sizeof(ddsd);
	ddsd.dwFlags			= DDSD_CAPS;
	ddsd.ddsCaps.dwCaps		= DDSCAPS_PRIMARYSURFACE;

	hResult = m_pDirectDraw->CreateSurface(&ddsd, &m_pPrimarySurface, NULL);
	if(hResult != DD_OK)
	{
		AddDebugMessage("Unable to create a primary surface.");
		Term();
		return FALSE;
	}

	
	if(g_RenderModeInfos[renderMode].m_Flags & RMFLAG_VIDEOMEMORY)
		memoryTypeFlag = DDSCAPS_VIDEOMEMORY;
	else
		memoryTypeFlag = DDSCAPS_SYSTEMMEMORY;

	// Create the rendering surface.
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
	ddsd.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | memoryTypeFlag;
	ddsd.dwBackBufferCount = 0;
	ddsd.dwWidth = pRect->Width();
	ddsd.dwHeight = pRect->Height();
	
	hResult = m_pDirectDraw->CreateSurface(&ddsd, &m_pSurface, NULL);
	if(hResult != DD_OK)
	{
		AddDebugMessage("Failed to make rendering surface.");
		Term();
		return FALSE;
	}

	// Create the clipper
	hResult = m_pDirectDraw->CreateClipper(0, &m_pClipper, NULL);
	if(hResult != DD_OK)
	{
		AddDebugMessage("Failed to make clipper.");
		Term();
		return FALSE;
	}

	m_pClipper->SetHWnd(0, m_pView->GetSafeHwnd());

	// Set the clipper for the rendering surface
	m_pPrimarySurface->SetClipper(m_pClipper);	

	// Then, create ZBuffer surface
	m_bGotZFormat = FALSE;
	hResult = m_pDirect3d->EnumZBufferFormats(id, D3DRender_EnumZFormats, this);
	if(!m_bGotZFormat)
	{
		AddDebugMessage("Unable to find a Z format in EnumZBufferFormats.");
		Term();
		return FALSE;
	}

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
	ddsd.dwWidth = pRect->Width();
	ddsd.dwHeight = pRect->Height();
	ddsd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | memoryTypeFlag;
	memcpy(&ddsd.ddpfPixelFormat, &m_ZFormat, sizeof(ddsd.ddpfPixelFormat));

	hResult = m_pDirectDraw->CreateSurface(&ddsd, &m_pZSurface, NULL);
	if(hResult != DD_OK)
	{
		AddDebugMessage("Failed to make z-buffer.");
		Term();
		return FALSE;
	}

	m_pSurface->AddAttachedSurface(m_pZSurface);

	hResult = m_pDirect3d->CreateDevice(id, m_pSurface, &m_pDevice, NULL);
	if(hResult == D3D_OK)
	{
		// talon: 2001's success message (Init, 0x418eac).
		AddDebugMessage("Created '%s' device", g_RenderModeInfos[renderMode].m_ModeName);
	}
	else
	{
		AddDebugMessage("Unable to create '%s' device", g_RenderModeInfos[renderMode].m_ModeName);
		Term();
		return FALSE;
	}

	memset(&m_Caps, 0, sizeof(m_Caps));
	m_Caps.dwSize = sizeof(m_Caps);
	memset(&helCaps, 0, sizeof(helCaps));
	helCaps.dwSize = sizeof(helCaps);
	m_pDevice->GetCaps(&m_Caps, &helCaps);


	// Create a viewport.
	hResult = m_pDirect3d->CreateViewport(&m_pViewport, NULL);
	if(hResult != D3D_OK)
	{
		AddDebugMessage("Unable to create Direct3D viewport.");
		Term();
		return FALSE;
	}

	hResult = m_pDevice->AddViewport(m_pViewport);
	if(hResult != D3D_OK)
	{
		AddDebugMessage("IDirect3DDevice2::AddViewport failed.");
		Term();
		return FALSE;
	}

	memset(&viewportData, 0, sizeof(D3DVIEWPORT2));
	viewportData.dwSize = sizeof(D3DVIEWPORT2);
	viewportData.dwX = 0;
	viewportData.dwY = 0;
	viewportData.dwWidth = pRect->Width();
	viewportData.dwHeight = pRect->Height();
	viewportData.dvClipX = D3DVALUE(-1.0f);
	viewportData.dvClipWidth = D3DVALUE(2.0f);
	viewportData.dvClipHeight = D3DVALUE((float)pRect->Height() * 2.0f / (float)pRect->Width());
	viewportData.dvClipY = D3DVALUE(viewportData.dvClipHeight / 2.0f);
	viewportData.dvMinZ = D3DVALUE(0.0f);
	viewportData.dvMaxZ	= D3DVALUE(1.0f);

	m_pViewport->SetViewport2(&viewportData);

	hResult = m_pDevice->SetCurrentViewport(m_pViewport);
	if(hResult != D3D_OK)
	{
		AddDebugMessage("IDirect3DDevice2::SetCurrentViewport failed.");
		Term();
		return FALSE;
	}

	// Create a material for the background clears.
	hResult = m_pDirect3d->CreateMaterial(&m_pMaterial, NULL);
	if(hResult != D3D_OK)
	{
		AddDebugMessage("IDirect3D2::CreateMaterial failed.");
		Term();
		return FALSE;
	}

	hResult = m_pMaterial->GetHandle(m_pDevice, &handle);
	if(hResult != D3D_OK)
	{
		AddDebugMessage("IDirect3DMaterial::GetHandle failed.");
		Term();
		return FALSE;
	}

	m_pViewport->SetBackground(handle);


	// Store more variables.
	m_SurfaceWidth = pRect->Width();
	m_SurfaceHeight = pRect->Height();

	
	// Set render states.
	m_pDevice->SetRenderState(D3DRENDERSTATE_SPECULARENABLE, FALSE);
	m_pDevice->SetRenderState(D3DRENDERSTATE_SUBPIXEL, TRUE);
	//m_pDevice->SetRenderState(D3DRENDERSTATE_EDGEANTIALIAS, TRUE);
	m_pDevice->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);	// talon: 2001 (0x419396)
	m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
	m_pDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
	m_pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREPERSPECTIVE, TRUE);
	
	m_pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSU, D3DTADDRESS_WRAP);
	m_pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREADDRESSV, D3DTADDRESS_WRAP);

	m_pDevice->SetRenderState(D3DRENDERSTATE_WRAPU, FALSE);
	m_pDevice->SetRenderState(D3DRENDERSTATE_WRAPV, FALSE);

	m_DeviceNum = deviceNum;
	m_RenderMode = renderMode;

	// Come up with a unique ID.
	for(uniqueID=0; uniqueID < 32000; uniqueID++)
	{
		bUnique = TRUE;
		
		pCurMgr = g_Direct3dDrawMgrs.m_pNext;
		while(pCurMgr != &g_Direct3dDrawMgrs)
		{
			if(((D3DRender*)pCurMgr->m_pData)->m_BindingNumber == uniqueID)
			{
				bUnique = FALSE;
				break;
			}

			pCurMgr = pCurMgr->m_pNext;
		}

		if(bUnique)
			break;
	}

	m_BindingNumber = uniqueID;

	// Add it to the list of draw managers.
	m_Link.m_pData = this;
	dl_Insert(&g_Direct3dDrawMgrs, &m_Link);

	m_pDibMgr = GetProject()->GetDibMgr();

	
	// Initialize the texture manager.
	if(d3d_InitTextureStuff(this))
	{
		m_pDevice->SetRenderState(D3DRENDERSTATE_ZFUNC, D3DCMP_LESSEQUAL);

		AddDebugMessage("Successfully initialized Direct3D renderer!");
		return TRUE;
	}

	Term();
	return FALSE;
}


void D3DRender::Term()
{
	// talon: 2001 first unbinds its processed worlds (0x419582).
	UnbindWorlds();

	d3d_TermTextureStuff(this);
	
	if(m_pZSurface)
	{
		m_pZSurface->Release();
		m_pZSurface = NULL;
	}

	if(m_pSurface)
	{
		m_pSurface->Release();
		m_pSurface = NULL;
	}

	if(m_pClipper)
	{
		m_pClipper->Release();
		m_pClipper = NULL;
	}

	if(m_pPrimarySurface)
	{
		m_pPrimarySurface->Release();
		m_pPrimarySurface = NULL;
	}

	if(m_pMaterial)
	{
		m_pMaterial->Release();
		m_pMaterial = NULL;
	}

	if(m_pViewport)
	{
		m_pViewport->Release();
		m_pViewport = NULL;
	}

	if(m_pDevice)
	{
		m_pDevice->Release();
		m_pDevice = NULL;
	}
	
	if(m_pDirect3d)
	{
		m_pDirect3d->Release();
		m_pDirect3d = NULL;
	}

	if(m_pDirectDraw)
	{
		m_pDirectDraw->Release();
		m_pDirectDraw = NULL;
	}

	if(m_Link.m_pData)
	{
		dl_Remove(&m_Link);
		m_Link.m_pData = NULL;
	}
}



// ------------------------------------------------------------------------ //
// Overrides.
// ------------------------------------------------------------------------ //

// 0x419650.  2001's takes the points and normal in world space and
// transforms them itself.
void D3DRender::DrawFlatPoly2(TLVertex *pPoints, DWORD nPoints, CVector *pNormal, COLORREF color)
{
	CVector pt, normal;
	TLVertex points[256];
	TLVertex *pCur, *pOutputVerts;	
	DWORD i;
	WORD pointStart, pointEnd, nOutputVerts;
	HRESULT hResult;
	float dot;

	m_pDevice->SetTexture(0, 0);

	// Backface test in camera space.
	m_pView->m_Rotation.Apply3x3(*pNormal, normal);
	m_pView->m_Transform.Apply(pPoints[0].m_Vec, pt);
	dot = pt.Dot(normal);
	if(dot >= 0.0f)
		return;

	for(i=0; i < nPoints; i++)
	{
		m_pView->m_Transform.Apply(pPoints[i].m_Vec, points[i].m_Vec);
		points[i].color = color;
	}

	// Clip.
	pointEnd = (WORD)nPoints;
	pointStart = 0;
	for(i=0; i < m_pView->m_nClipPlanesToUse; i++)
	{
		if(!ClipPolyToPlane(&m_pView->m_ClipPlanes[i], points, pointStart, pointEnd))
			return;
	}

	pOutputVerts = &points[pointStart];
	nOutputVerts = pointEnd - pointStart;

	// Project.
	pCur = pOutputVerts;
	for(i=0; i < nOutputVerts; i++)
	{
		m_pView->m_pViewDef->ProjectPt(pCur->m_Vec, pCur->m_Vec);
		SetupVertZ(*pCur, pCur->m_Vec.z);
		++pCur;
	}

	hResult = m_pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, TLVERTEX_FORMAT, pOutputVerts, nOutputVerts, DRAWPRIM_FLAGS);
}


// talon: stand-in: 0x446328, a CPreMainWorld member (part 6's range) that
// finds the region document's selected lightmap poly from its world model
// and name (CRegionDoc's 0x320 and 0x324, pushed by value).  Declared here
// until DEdit's CPreMainWorld has it; the name is ours.
class CPreMainWorld2001
{
public:
	CPrePoly*	FindPoly(uint32 iWorld, uint32 nName);
};


// talon: 0x41987f.  2001's draws the poly's one texture itself: texture
// coordinates from the poly's O, P and Q (0x34, 0x40, 0x4c), the shading
// from the normal when the display options say so, and a selected brush
// darkened and fogged in the selection colour.
void D3DRender::DrawTexturedPoly(CEditPoly *pPoly)
{
	TLVertex verts[256];
	COptionsDisplay *pDisp;
	CVector pos;
	float pDotO, qDotO, u, v;
	float fXDot, fZDot;
	uint32 i;
	WORD pointStart, pointEnd, nVerts;
	uint8 r, g, b;
	TLVertex *pOutputVerts, *pCur;

	if(!d3d_SetupPolyTexture(this, pPoly))
	{
		DrawFlatPoly(pPoly, (COLORREF)pPoly * 0x9aeb0);
		return;
	}

	pDotO = pPoly->O.Dot(pPoly->P);
	qDotO = pPoly->O.Dot(pPoly->Q);

	pDisp = GetApp()->GetOptions().GetDisplayOptions();
	if(!pDisp)
		return;

	for(i=0; i < pPoly->NumVerts(); i++)
	{
		CEditVert &vert = pPoly->Pt(i);

		verts[i].m_Vec = vert.m_Transformed;
		v = (pPoly->Q.Dot(vert) - qDotO) * g_vMul;
		u = (pPoly->P.Dot(vert) - pDotO) * g_uMul;
		verts[i].tu = u;
		verts[i].tv = v;
	}

	// Clip.
	pointEnd = pPoly->NumVerts();
	pointStart = 0;
	for(i=0; i < m_pView->m_nClipPlanesToUse; i++)
	{
		if(!ClipPolyToPlane(&m_pView->m_ClipPlanes[i], verts, pointStart, pointEnd))
			return;
	}

	r = 0xFF;
	g = 0xFF;
	b = 0xFF;
	if(pDisp->IsShadePolygons())
	{
		fXDot = (float)sqrt(CVector(1.0f, 0.0f, 0.0f).Dot(pPoly->Normal()));
		fZDot = (float)sqrt(CVector(0.0f, 0.0f, 1.0f).Dot(pPoly->Normal()));
		r = (uint8)(fZDot * 96.0f);
		r = (uint8)(fXDot * -32.0f) - r + 128;
		g = r;
		b = r;
	}

	if(pPoly->m_pBrush->m_Flags & 1)
	{
		g = (uint8)LTMIN((uint32)(g * 3) >> 2, (uint32)0xFF);
		r = (uint8)LTMIN((uint32)(r * 3) >> 2, (uint32)0xFF);
		b = (uint8)LTMIN((uint32)(b * 3) >> 2, (uint32)0xFF);

		m_pDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, TRUE);
		m_pDevice->SetRenderState(D3DRENDERSTATE_FOGCOLOR, GetDisplayColorD3D(4));
	}
	else
	{
		m_pDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
	}

	// Project.
	pOutputVerts = &verts[pointStart];
	nVerts = pointEnd - pointStart;
	pCur = pOutputVerts;
	for(i=0; i < nVerts; i++)
	{
		m_pView->m_pViewDef->ProjectPt(pCur->m_Vec, pCur->m_Vec);
		SetupVertZ(*pCur, pCur->m_Vec.z);
		pCur->color = RGBA_MAKE(r, g, b, 0xFF);
		pCur->specular.color = 0xC0000000;
		++pCur;
	}

	m_pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, TLVERTEX_FORMAT, pOutputVerts, nVerts, DRAWPRIM_FLAGS);
	m_pDevice->SetRenderState(D3DRENDERSTATE_FOGENABLE, FALSE);
}


// talon: 0x419c68.  A processed-world poly with its lightmap: lightmap
// coordinates from the poly's origin and the surface's P and Q over the
// world's lightmap grid size; the points are pulled 1 unit toward the eye.
// The name is ours.
void D3DRender::DrawPrePoly(CPrePoly *pPoly, float lmGridSize)
{
	TLVertex verts[256];
	uint32 i;
	WORD pointStart, pointEnd, nVerts;
	TLVertex *pOutputVerts, *pCur;
	float u, v;

	for(i=0; i < pPoly->NumVerts(); i++)
	{
		m_pView->m_Transform.Apply4x4(pPoly->Vert(i).m_Vec, verts[i].m_Vec);

		u = pPoly->m_pSurface->P.Dot(pPoly->Vert(i).m_Vec - pPoly->m_PolyO) / lmGridSize + 0.5f;
		v = pPoly->m_pSurface->Q.Dot(pPoly->Vert(i).m_Vec - pPoly->m_PolyO) / lmGridSize + 0.5f;
		verts[i].tu = u;
		verts[i].tv = v;
	}

	// Clip.
	pointEnd = pPoly->NumVerts();
	pointStart = 0;
	for(i=0; i < m_pView->m_nClipPlanesToUse; i++)
	{
		if(!ClipPolyToPlane(&m_pView->m_ClipPlanes[i], verts, pointStart, pointEnd))
			return;
	}

	pOutputVerts = &verts[pointStart];
	nVerts = pointEnd - pointStart;

	if(!d3d_SetupLMTexture(this, pPoly))
		return;

	// Project.
	pCur = pOutputVerts;
	for(i=0; i < nVerts; i++)
	{
		pCur->tu *= g_uMul;
		pCur->tv *= g_vMul;
		m_pView->m_pViewDef->ProjectPt(pCur->m_Vec, pCur->m_Vec);
		pCur->color = 0xFFFFFFFF;
		pCur->m_Vec.z -= 1.0f;
		SetupVertZ(*pCur, pCur->m_Vec.z);
		++pCur;
	}

	m_pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, TLVERTEX_FORMAT, pOutputVerts, nVerts, DRAWPRIM_FLAGS);
}


// talon: 0x419f0e.  Binds a processed world to this renderer: TRUE if it
// is already ours, FALSE if another renderer has it.  The name is ours.
BOOL D3DRender::BindWorld(CPreMainWorld *pWorld)
{
	if(pWorld->m_Unknown_f0)
		return pWorld->m_Unknown_f0 == (uint32)this;

	pWorld->m_Unknown_f0 = (uint32)this;
	m_Worlds.Append(pWorld);
	return TRUE;
}


// talon: 0x419f46.  The name is ours.
void D3DRender::UnbindWorlds()
{
	while(m_Worlds > 0)
	{
		d3d_UnbindWorld(m_Worlds[0]);
	}
}


// talon: 0x419f68.  A poly's lightmap samples as vertices, the ones in the
// region document's list (0x328, 0x413e00) larger and bordered in red.
// The name is ours.
void D3DRender::DrawLightmapPoints(CPrePoly *pPoly, float lmGridSize)
{
	uint32 data[1024];
	uint32 x, y;
	uint32 color;
	CVector rowPos, pos;
	CTexelRef texel;

	if(!pPoly->m_Unknown_24)
		return;

	if(!((CPreLightMap*)pPoly->m_Unknown_24)->Decompress(data))
		return;

	rowPos = pPoly->m_PolyO;
	for(y=0; y < pPoly->m_LMHeight; y++)
	{
		pos = rowPos;
		for(x=0; x < pPoly->m_LMWidth; x++)
		{
			color = data[y * pPoly->m_LMWidth + x];
			color = 0xFF000000 | (color & 0xFFFF0000) | (((color & 0xFF0000) | (((color >> 16) & 0xFF) << 8)) >> 8);

			texel.m_X = x;
			texel.m_Y = y;
			if(m_pView->GetRegionDoc()->m_SelTexels.FindElement(texel) == BAD_INDEX)
				TransformAndDrawVert(pos, 0xFFFFFFFF, color, 4);
			else
				TransformAndDrawVert(pos, 0xFFFF0000, color, 6);

			pos += pPoly->m_pSurface->InverseP * lmGridSize;
		}

		rowPos += pPoly->m_pSurface->InverseQ * lmGridSize;
	}
}


// talon: 0x41a0d9, slot 4.  Draws a processed world's polies with their
// lightmaps (blended over the textures when bBlend), then the selected
// poly's lightmap points.
void D3DRender::DrawWorld(CPreMainWorld *pWorld, BOOL bBlend)
{
	DWORD oldBlend, oldSrc, oldDest, oldZEnable;
	CPreWorld *pModel;
	CPrePoly *pPoly;
	uint32 i;
	GPOS pos;

	if(!BindWorld(pWorld))
		return;

	if(bBlend)
	{
		m_pDevice->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, &oldBlend);
		m_pDevice->GetRenderState(D3DRENDERSTATE_SRCBLEND, &oldSrc);
		m_pDevice->GetRenderState(D3DRENDERSTATE_SRCBLEND, &oldDest);
		m_pDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
		m_pDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_DESTCOLOR);
		m_pDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_SRCCOLOR);
	}

	for(i=0; i < pWorld->m_WorldModels; i++)
	{
		pModel = pWorld->m_WorldModels[i];
		for(pos=pModel->m_Polies; pos; )
		{
			pPoly = pModel->m_Polies.GetNext(pos);
			DrawPrePoly(pPoly, pWorld->m_LMGridSize);
		}
	}

	if(bBlend)
	{
		m_pDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, oldBlend);
		m_pDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, oldSrc);
		m_pDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, oldDest);
	}

	m_pDevice->GetRenderState(D3DRENDERSTATE_ZENABLE, &oldZEnable);
	m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);

	pPoly = ((CPreMainWorld2001*)pWorld)->FindPoly(m_pView->GetRegionDoc()->m_LightPolyWorld,
		m_pView->GetRegionDoc()->m_LightPolyName);
	if(pPoly)
	{
		DrawPrePolyLines(pPoly, 1, 1, GetDisplayColorD3D(4));
		DrawLightmapPoints(pPoly, pWorld->m_LMGridSize);
		m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, oldZEnable);
	}
}


// 0x41a25b
void D3DRender::DrawLine(TLVertex *pVerts)
{
	COptionsDisplay *pDisp = GetApp()->GetOptions().GetDisplayOptions();

	if(!pDisp->IsZBufferLines())
		m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);

	m_pDevice->SetTexture(0, 0);
	m_pDevice->DrawPrimitive(D3DPT_LINELIST, TLVERTEX_FORMAT, pVerts, 2, DRAWPRIM_FLAGS);

	if(!pDisp->IsZBufferLines())
		m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_TRUE);
}


void D3DRender::DrawVert(CVector &pos, DWORD borderColor, DWORD fillColor, int nSize)
{
	COptionsDisplay *pDisp = GetApp()->GetOptions().GetDisplayOptions();
	D3DTLVERTEX verts[5], *pVert;
	HRESULT hResult;
	
	// talon: retail bug: z and oneOverZ are only set in a perspective view
	// (0x41a2f6..0x41a328); a parallel view's vertices get whatever was on
	// the stack for rhw and z.
	float z;
	float oneOverZ;
	float vertSize;
	BOOL bDrawOutline;

	m_pDevice->SetTexture(0, 0);

	if(m_bPerspectiveProjection)
	{
		z = pos.z;
		if(z > m_pViewDef->m_FarZ || z < m_pViewDef->m_NearZ)
			return;
		
		oneOverZ = 1.0f / pos.z;
		vertSize = 1.0f - (z / m_pViewDef->m_FarZ);
		vertSize *= nSize;

		bDrawOutline = vertSize > VERT_SIZE_BORDER_CUTOFF;
	}
	else
	{
		vertSize = (float)nSize;
		bDrawOutline = TRUE;
	}

	pVert = &verts[0];
	pVert->sx = pos.x - vertSize;
	pVert->sy = pos.y - vertSize;
	pVert->rhw = oneOverZ;
	float fSZ = (g_A * z + g_B) * oneOverZ;
	pVert->sz = fSZ;
	pVert->color = fillColor;
	if(pVert->sx < 0.0f || pVert->sy > g_xMax || pVert->sy < 0.0f || pVert->sy > g_yMax)
		return;

	++pVert;
	pVert->sx = pos.x + vertSize;
	pVert->sy = pos.y - vertSize;
	pVert->rhw = oneOverZ;
	pVert->sz = fSZ;
	pVert->color = fillColor;
	if(pVert->sx < 0.0f || pVert->sy > g_xMax || pVert->sy < 0.0f || pVert->sy > g_yMax)
		return;

	++pVert;
	pVert->sx = pos.x + vertSize;
	pVert->sy = pos.y + vertSize;
	pVert->rhw = oneOverZ;
	pVert->sz = fSZ;
	pVert->color = fillColor;
	if(pVert->sx < 0.0f || pVert->sy > g_xMax || pVert->sy < 0.0f || pVert->sy > g_yMax)
		return;

	++pVert;
	pVert->sx = pos.x - vertSize;
	pVert->sy = pos.y + vertSize;
	pVert->rhw = oneOverZ;
	pVert->sz = fSZ;
	pVert->color = fillColor;
	if(pVert->sx < 0.0f || pVert->sy > g_xMax || pVert->sy < 0.0f || pVert->sy > g_yMax)
		return;

	hResult = m_pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, TLVERTEX_FORMAT, 
		&verts, 4, DRAWPRIM_FLAGS);

	if(bDrawOutline)
	{
		verts[0].color = verts[1].color = verts[2].color = verts[3].color = borderColor;
		verts[4] = verts[0];		

		if (!pDisp->IsZBufferLines())
		{
			// Disable the Z buffer
			m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
		}

		hResult = m_pDevice->DrawPrimitive(D3DPT_LINESTRIP, TLVERTEX_FORMAT, 
			&verts, 5, DRAWPRIM_FLAGS);

		if (!pDisp->IsZBufferLines())
		{
			// Enable the Z buffer
			m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, TRUE);
		}
	}
}


BOOL D3DRender::ZEnable(BOOL bEnable)
{
	DWORD prev;

	m_pDevice->GetRenderState(D3DRENDERSTATE_ZENABLE, (unsigned long *)&prev);

	m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, bEnable);
	m_pDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, bEnable);

	return (BOOL)prev;
}


// 0x41a636.  The mipmap offset halves the texture coordinate scale.
void D3DRender::InitVars()
{
	DWORD i;

	g_xMax = (float)m_SurfaceWidth - 1.0f;
	g_yMax = (float)m_SurfaceHeight - 1.0f;

	m_bPerspectiveProjection = m_pViewDef->ViewType() == PERSPECTIVE_VIEWTYPE;

	i = GetApp()->GetOptions().GetDisplayOptions()->GetMipMapOffset();
	g_TextureMipScale = 1.0f;
	for(; i > 0; i--)
	{
		g_TextureMipScale *= 0.5f;
	}

	m_pDevice->SetTexture(0, 0);
	m_pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMIN, g_bBilinear ? D3DFILTER_LINEAR : D3DFILTER_NEAREST);
	m_pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREMAG, g_bBilinear ? D3DFILTER_LINEAR : D3DFILTER_NEAREST);
}


void D3DRender::DrawTagRectPoly()
{
	CRect tagRect, clientRect;
	TLVertex verts[4];
	HRESULT hResult;
	DWORD oldAlpha, oldSrc, oldDest, oldZEnable, oldZWriteEnable;

	
	m_pDevice->SetTexture(0, 0);

	tagRect = m_pView->m_TagDrawRect;
	tagRect.NormalizeRect();
	
	m_pView->GetClientRect( &clientRect );
	tagRect.left = MAX( tagRect.left, 0 );
	tagRect.top = MAX( tagRect.top, 0 );
	tagRect.right = MIN( tagRect.right, clientRect.right );
	tagRect.bottom = MIN( tagRect.bottom, clientRect.bottom );

	// talon: 2001 has no empty-rect test (0x41a707).

	verts[0].m_Vec.Init((float)tagRect.left, (float)tagRect.top, 0.0f);
	verts[1].m_Vec.Init((float)tagRect.right, (float)tagRect.top, 0.0f);
	verts[2].m_Vec.Init((float)tagRect.right, (float)tagRect.bottom, 0.0f);
	verts[3].m_Vec.Init((float)tagRect.left, (float)tagRect.bottom, 0.0f);
	verts[0].color = verts[1].color = verts[2].color = verts[3].color = 0xFFFFFFFF;

	// Invert color..
	m_pDevice->GetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, (unsigned long *)&oldAlpha);
	m_pDevice->GetRenderState(D3DRENDERSTATE_SRCBLEND, (unsigned long *)&oldSrc);
	m_pDevice->GetRenderState(D3DRENDERSTATE_DESTBLEND, (unsigned long *)&oldDest);
	m_pDevice->GetRenderState(D3DRENDERSTATE_ZENABLE, (unsigned long *)&oldZEnable);
	m_pDevice->GetRenderState(D3DRENDERSTATE_ZWRITEENABLE, (unsigned long *)&oldZWriteEnable);

	m_pDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
	m_pDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_INVDESTCOLOR);
	m_pDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_ZERO);
	m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, FALSE);
	m_pDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
	
	hResult = m_pDevice->DrawPrimitive(D3DPT_TRIANGLEFAN, 
		TLVERTEX_FORMAT, verts, 4, DRAWPRIM_FLAGS);

	m_pDevice->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, oldAlpha);
	m_pDevice->SetRenderState(D3DRENDERSTATE_SRCBLEND, oldSrc);
	m_pDevice->SetRenderState(D3DRENDERSTATE_DESTBLEND, oldDest);
	m_pDevice->SetRenderState(D3DRENDERSTATE_ZENABLE, oldZEnable);
	m_pDevice->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, oldZWriteEnable);
}


// 0x41a918.  The frame's statistics are cleared, and the drawing and the
// blit timed into them.
void D3DRender::Draw()
{
	HRESULT hResult;
	CRect destRect, srcRect;
	D3DRECT clearRect = {0, 0, m_SurfaceWidth, m_SurfaceHeight};
	CycleMgr drawCounter, blitCounter;


	if(!m_pDevice || !m_pViewport || !m_pPrimarySurface ||
		!m_pSurface || !m_pView)
	{
		return;
	}

	if(!InitFrame())
		return;

	memset(&m_Stats, 0, sizeof(m_Stats));

	StartCycleCount(&drawCounter);
	
	hResult = m_pDevice->BeginScene();
	if(hResult != D3D_OK)
		return;

	// Clear everything.
	DWORD dwColor=GetDisplayColorD3D(COptionsDisplay::kColorBackground);

	hResult = m_pViewport->Clear2(1, &clearRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, dwColor, 1.0, 0);

	// Do the real drawing.
	InitVars();
	DrawBase::Draw();

	DrawTagRectPoly();

	m_pDevice->EndScene();

	m_Stats.m_DrawCycles = EndCycleCount(&drawCounter);
	StartCycleCount(&blitCounter);
	
	// Blit to the primary.
	srcRect.SetRect(0, 0, m_SurfaceWidth, m_SurfaceHeight);
	m_pView->GetRenderRect(&destRect, FALSE);
	// Center the source rectangle around the destination rectangle
	int iWidthDiff = srcRect.Width() - destRect.Width();
	int iHeightDiff = srcRect.Height() - destRect.Height();
	srcRect.DeflateRect(iWidthDiff / 2, iHeightDiff / 2, iWidthDiff - iWidthDiff / 2, iHeightDiff - iHeightDiff / 2);

			
	hResult = m_pPrimarySurface->Blt(&destRect, m_pSurface, &srcRect, DDBLT_WAIT, NULL);
	
	if(hResult != DD_OK)
	{
		hResult = m_pPrimarySurface->BltFast(destRect.left, destRect.top, m_pSurface, &srcRect, DDBLTFAST_NOCOLORKEY);
	}		

	m_Stats.m_BlitCycles = EndCycleCount(&blitCounter);
}


// 0x41aab3.  The surface is the view's size over its pixel scale.
void D3DRender::Resize(int width, int height)
{
	CRect rect(0, 0, width / m_pView->m_nPixelScale, height / m_pView->m_nPixelScale);

	Term();
	Init(m_DeviceNum, m_RenderMode, &rect);
}

	

// ------------------------------------------------------------------------ //
// Interface functions.
// ------------------------------------------------------------------------ //

DrawMgr* dm_CreateDirect3dDrawMgr(CRegionView *pView, int *deviceNum, int *renderMode)
{				
	D3DRender *pRet;
	CRect rect;
	int testDeviceNum, testRenderMode;



	pRet = new D3DRender(pView);

	pView->GetClientRect(&rect);
	rect.right /= pView->m_nPixelScale;
	rect.bottom /= pView->m_nPixelScale;
	
	// In here, it tries device numbers down to zero, and all the render 
	// modes it can for each device.
	testDeviceNum = *deviceNum;
	testRenderMode = *renderMode;
	while(1)
	{
		if(pRet->Init(testDeviceNum, testRenderMode, &rect))
		{
			*deviceNum = testDeviceNum;
			*renderMode = testRenderMode;
			return pRet;
		}

		for(testRenderMode=0; testRenderMode < NUM_RENDERMODEINFOS; testRenderMode++)
		{
			if(pRet->Init(testDeviceNum, testRenderMode, &rect))
			{
				*deviceNum = testDeviceNum;
				*renderMode = testRenderMode;
				return pRet;
			}
		}
		
		if(testDeviceNum == 0)
			break;
			
		--testDeviceNum;	
	}
	
	delete pRet;
	return NULL;
}


static BOOL PASCAL __stdcall d3d_EnumDeviceNames(GUID* pGuid, char* sDriverDescription, char* sDriverName, void* pContext)
{
	if(g_nNames < g_nMaxNames)
	{
		g_pNames[g_nNames] = new char[strlen(sDriverDescription)+1];
		strcpy(g_pNames[g_nNames], sDriverDescription);
		++g_nNames;
	}

	return DDENUMRET_OK;
}


int dm_GetDirectDrawDeviceNames(char **pFillIn, int nMaxNames)
{
	g_nMaxNames = nMaxNames;
	g_nNames = 0;
	g_pNames = pFillIn;
 
	DirectDrawEnumerate(d3d_EnumDeviceNames, NULL);

	return g_nNames;
}






