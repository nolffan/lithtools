//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//

// talon: the 2001 texture manager (0x415075..0x415db6), written from the
// image in its order.  Besides the editor's textures it makes the processed
// world's lightmap textures (d3d_SetupLMTexture) and counts the time spent
// in the frame statistics (CCycleCounter).  Build 69's detail textures,
// d3d_AddBinding and d3d_CreateTextureSurface are not in it.

#include "bdefs.h"
#include "draw_d3d.h"
#include "d3d_texturemgr.h"
#include "texture.h"
#include "cyclemgr.h"
#include "pixelformat.h"
#include "preworld.h"
#include "prepoly.h"
#include "prelightmap.h"


#define NUM_VALID_TEXTURE_SIZES (sizeof(g_TextureSizes) / sizeof(g_TextureSizes[0]))


// ------------------------------------------------------------------------ //
// Structures.
// ------------------------------------------------------------------------ //

typedef struct Direct3dTexture_t
{

						Direct3dTexture_t()
						{
							m_Link.Init();
							m_pSurface = NULL;
							m_pTexture = NULL;
							m_pSystemTexture = NULL;
							m_pDrawMgr = NULL;
							m_uMul = m_vMul = 1.0f;
						}

	DLink				m_Link;

	LPDIRECTDRAWSURFACE4	m_pSurface;
	IDirect3DTexture2		*m_pTexture;

	CTexture			*m_pSystemTexture;
	D3DRender			*m_pDrawMgr;
	float				m_uMul, m_vMul;
} Direct3dTexture;



// ------------------------------------------------------------------------ //
// Globals.
// ------------------------------------------------------------------------ //

DWORD g_TextureSizes[] = {4, 8, 16, 32, 64, 128, 256, 512};

extern float g_TextureMipScale;
extern float g_uMul, g_vMul;

// talon: this module's own format manager, 0x508f70: its initializer is the
// module's first code (0x415050, 0x415055) and only d3d_PutLightmapIntoSurface
// uses it (Texture.cpp has its own, 0x63c658), so it is static.  The name is
// ours.
static FormatMgr g_FormatMgr;


// ------------------------------------------------------------------------ //
// Internals.
// ------------------------------------------------------------------------ //

static HRESULT WINAPI d3d_TextureFormatCallback(LPDDPIXELFORMAT pPF, LPVOID pUser);
static WORD d3d_GetBitCount(DWORD mask);
static int d3d_GetFormatCode(TextureFormat *pFormat);
static void d3d_GetMaskShifts(DWORD mask, DWORD *pRightShift, DWORD *pLeftShift);
static TextureFormat* d3d_SelectIdealTextureFormat(D3DRender *pRender);
static void d3d_PrintTextureFormats(D3DRender *pRender);
static void d3d_PrintTextureFormat(char *pPrefix, TextureFormat *pFormat);
static char* d3d_GetPFFlagName(DWORD flags);
static void d3d_SetupTextureSlot(D3DRender *pRender, Direct3dTexture *pTexture);
static BOOL d3d_PutLightmapIntoSurface(D3DRender *pRender, CPreLightMap *pLightMap, LPDIRECTDRAWSURFACE4 pSurface);
static PFormat d3d_GetPValueFormat();
static void d3d_DDPFToPFormat(DDPIXELFORMAT *pDDPF, PFormat *pFormat);
static Direct3dTexture* d3d_SetupNewTexture(D3DRender *pRender, CTexture *pInTexture);
static BOOL d3d_FreeSomeSpace(D3DRender *pRender);
static BOOL d3d_PutTextureIntoSurface(D3DRender *pRender,
	CTexture *pSystemTexture, LPDIRECTDRAWSURFACE4 pSurface, IDirect3DTexture2 *pTexture);
static BOOL CheckTextureSize(D3DRender *pRender, DWORD width, DWORD height);
static void d3d_UnloadTexture(Direct3dTexture *pTexture, int iBindingNumber);


// 0x41505f: before d3d_UnloadTexture in the image.
void d3d_UnbindFromTexture(void *pBinding)
{
	Direct3dTexture *pTexture;

	pTexture = (Direct3dTexture*)pBinding;
	d3d_UnloadTexture(pTexture, pTexture->m_pDrawMgr->m_BindingNumber);
}


// 0x415075
static void d3d_UnloadTexture(Direct3dTexture *pTexture, int iBindingNumber)
{
	dl_Remove(&pTexture->m_Link);

	pTexture->m_pTexture->Release();
	pTexture->m_pSurface->Release();

	if(pTexture->m_pSystemTexture)
	{
		pTexture->m_pSystemTexture->m_Bindings[iBindingNumber] = NULL;
	}

	delete pTexture;
}


// talon: 0x4150be.  Frees a processed poly's lightmap texture (the
// CPrePoly's 0x20, a Direct3dTexture; it has no system texture, so the
// binding number doesn't matter).  The name is ours.
void d3d_UnbindPrePoly(CPrePoly *pPoly)
{
	if(pPoly->m_Unknown_20)
	{
		d3d_UnloadTexture((Direct3dTexture*)pPoly->m_Unknown_20, 0);
		pPoly->m_Unknown_20 = 0;
	}
}


// talon: 0x4150da.  Takes a processed world off the renderer it is bound to
// (CPreMainWorld's 0xf0, D3DRender::BindWorld) and frees its lightmap
// textures.  Called by D3DRender::UnbindWorlds and RegionDoc (0x450dd3).
// The name is ours.
void d3d_UnbindWorld(CPreMainWorld *pWorld)
{
	D3DRender *pRender;
	CPreWorld *pModel;
	uint32 i;
	GPOS pos;

	pRender = (D3DRender*)pWorld->m_Unknown_f0;
	if(!pRender)
		return;

	i = pRender->m_Worlds.FindElement(pWorld);
	if(i >= pRender->m_Worlds.GetSize())
		return;

	pRender->m_Worlds.Remove(i);
	pWorld->m_Unknown_f0 = 0;

	for(i=0; i < pWorld->m_WorldModels; i++)
	{
		pModel = pWorld->m_WorldModels[i];

		for(pos=pModel->m_Polies; pos; )
		{
			d3d_UnbindPrePoly(pModel->m_Polies.GetNext(pos));
		}
	}
}


// 0x415181
BOOL d3d_InitTextureStuff(D3DRender *pRender)
{
	// Select a texture format.
	pRender->m_pDevice->EnumTextureFormats(d3d_TextureFormatCallback, pRender);

	pRender->m_pTextureFormat = d3d_SelectIdealTextureFormat(pRender);

	// Print out the texture formats.
	d3d_PrintTextureFormats(pRender);

	if(!pRender->m_pTextureFormat)
	{
		AddDebugMessage("Couldn't find a supported texture format!");
		return FALSE;
	}

	return TRUE;
}


// 0x4151c5
static HRESULT WINAPI d3d_TextureFormatCallback(LPDDPIXELFORMAT pPF, LPVOID pUser)
{
	TextureFormat *pFormat;
	D3DRender *pRender = (D3DRender*)pUser;

	// Copy the desc over.
	pFormat = (TextureFormat*)dalloc(sizeof(TextureFormat));
	memcpy(&pFormat->m_PF, pPF, sizeof(pFormat->m_PF));

	pFormat->m_RBits = d3d_GetBitCount(pPF->dwRBitMask);
	pFormat->m_GBits = d3d_GetBitCount(pPF->dwGBitMask);
	pFormat->m_BBits = d3d_GetBitCount(pPF->dwBBitMask);
	pFormat->m_ABits = d3d_GetBitCount(pPF->dwRGBAlphaBitMask);

	d3d_GetMaskShifts(pPF->dwRBitMask, &pFormat->m_RRightShift, &pFormat->m_RLeftShift);
	d3d_GetMaskShifts(pPF->dwGBitMask, &pFormat->m_GRightShift, &pFormat->m_GLeftShift);
	d3d_GetMaskShifts(pPF->dwBBitMask, &pFormat->m_BRightShift, &pFormat->m_BLeftShift);

	pFormat->m_TextureCode = d3d_GetFormatCode(pFormat);

	pFormat->m_Link.m_pData = pFormat;
	dl_Insert(&pRender->m_TextureFormats, &pFormat->m_Link);

	return D3DENUMRET_OK;
}


// 0x415279
static WORD d3d_GetBitCount(DWORD mask)
{
	WORD	i, bits=0;

	for(i=0; i < 32; i++)
	{
		if(mask & 1)
			++bits;

		mask >>= 1;
	}

	return bits;
}


// 0x41528e
static int d3d_GetFormatCode(TextureFormat *pFormat)
{
	// We only support 16 bit textures currently..
	if(pFormat->m_PF.dwRGBBitCount != 16)
		return -1;

	if(pFormat->m_PF.dwFlags & DDPF_ALPHAPIXELS)
	{
		// We like alpha pixels a little better than normal so add 3 to its value.
		return pFormat->m_RBits + pFormat->m_GBits + pFormat->m_BBits + 3;
	}
	else if(pFormat->m_PF.dwFlags & DDPF_RGB)
	{
		// The more color bits, the better!
		return pFormat->m_RBits + pFormat->m_GBits + pFormat->m_BBits;
	}

	// Unsupported format.
	return -1;
}


// --------------------------------------------------------------------------------- //
// Gets the right and left shifts that you'll need to take and 8-bit color and turn
// it into a WORD, given its mask (the bits it occupies) in a WORD.
// --------------------------------------------------------------------------------- //

// 0x4152bf
static void d3d_GetMaskShifts(DWORD mask, DWORD *pRightShift, DWORD *pLeftShift)
{
	DWORD	bitCount = d3d_GetBitCount(mask);
	DWORD	i, shift, testMask;

	*pRightShift = 8 - bitCount;

	// Starting with the MSB of a WORD, find out where mask starts in the WORD.
	testMask = 0x8000;
	for(i=0; i < 16; i++)
	{
		if(testMask & mask)
			break;

		testMask >>= 1;
	}

	shift = 16 - i;
	*pLeftShift = shift - bitCount;
}


// 0x4152fb
static TextureFormat* d3d_SelectIdealTextureFormat(D3DRender *pRender)
{
	DLink *pCur;
	TextureFormat *pFormat, *pBest=NULL;
	int iBest = -1;

	for(pCur=pRender->m_TextureFormats.m_pNext; pCur != &pRender->m_TextureFormats; pCur=pCur->m_pNext)
	{
		pFormat = (TextureFormat*)pCur->m_pData;

		if(pFormat->m_TextureCode > iBest)
		{
			iBest = pFormat->m_TextureCode;
			pBest = pFormat;
		}
	}

	return pBest;
}


// talon: 0x415330, 0x415376 and 0x4153b1: the texture formats printed to
// the debug window ("CURRENT - " in front of the chosen one).  The names
// are ours.
static void d3d_PrintTextureFormats(D3DRender *pRender)
{
	DLink *pCur;

	for(pCur=pRender->m_TextureFormats.m_pNext; pCur != &pRender->m_TextureFormats; pCur=pCur->m_pNext)
	{
		d3d_PrintTextureFormat("", (TextureFormat*)pCur->m_pData);
	}

	if(pRender->m_pTextureFormat)
	{
		d3d_PrintTextureFormat("CURRENT - ", pRender->m_pTextureFormat);
	}
}


static void d3d_PrintTextureFormat(char *pPrefix, TextureFormat *pFormat)
{
	AddDebugMessage("%s%s - %d bits (%d %d %d %d) (%x %x %x %x)",
		pPrefix, d3d_GetPFFlagName(pFormat->m_PF.dwFlags), pFormat->m_PF.dwRGBBitCount,
		pFormat->m_RBits, pFormat->m_GBits, pFormat->m_BBits, pFormat->m_ABits,
		pFormat->m_PF.dwRBitMask, pFormat->m_PF.dwGBitMask, pFormat->m_PF.dwBBitMask,
		pFormat->m_PF.dwRGBAlphaBitMask);
}


static char* d3d_GetPFFlagName(DWORD flags)
{
	if(flags & DDPF_PALETTEINDEXED1)
		return "DDPF_PALETTEINDEXED1";
	else if(flags & DDPF_PALETTEINDEXED2)
		return "DDPF_PALETTEINDEXED2";
	else if(flags & DDPF_PALETTEINDEXED4)
		return "DDPF_PALETTEINDEXED4";
	else if(flags & DDPF_PALETTEINDEXED8)
		return "DDPF_PALETTEINDEXED8";
	else if(flags & DDPF_PALETTEINDEXEDTO8)
		return "DDPF_PALETTEINDEXEDTO8";
	else if(flags & DDPF_ALPHAPIXELS)
		return "DDPF_ALPHAPIXELS";
	else if(flags & DDPF_RGB)
		return "DDPF_RGB";
	else
		return "UNKNOWN";
}


// 0x415402
void d3d_TermTextureStuff(D3DRender *pRender)
{
	DLink *pCur, *pNext;

	// Unload all the textures.
	pCur = pRender->m_Textures.m_pNext;
	while(pCur != &pRender->m_Textures)
	{
		pNext = pCur->m_pNext;
		d3d_UnloadTexture((Direct3dTexture*)pCur->m_pData, pRender->m_BindingNumber);
		pCur = pNext;
	}

	// Unload all the texture formats.
	pCur = pRender->m_TextureFormats.m_pNext;
	while(pCur != &pRender->m_TextureFormats)
	{
		pNext = pCur->m_pNext;
		dfree(pCur->m_pData);
		pCur = pNext;
	}

	dl_TieOff(&pRender->m_Textures);
	dl_TieOff(&pRender->m_TextureFormats);
}


// talon: 0x415462.  The format for lightmaps: an 8888 one if the card has
// it, else 555.  The name is ours.
static TextureFormat* d3d_SelectLightmapFormat(D3DRender *pRender)
{
	DLink *pCur;
	TextureFormat *pFormat;

	for(pCur=pRender->m_TextureFormats.m_pNext; pCur != &pRender->m_TextureFormats; pCur=pCur->m_pNext)
	{
		pFormat = (TextureFormat*)pCur->m_pData;
		if(pFormat->m_RBits == 8 && pFormat->m_GBits == 8 && pFormat->m_BBits == 8 && pFormat->m_ABits == 8)
			return pFormat;
	}

	for(pCur=pRender->m_TextureFormats.m_pNext; pCur != &pRender->m_TextureFormats; pCur=pCur->m_pNext)
	{
		pFormat = (TextureFormat*)pCur->m_pData;
		if(pFormat->m_RBits == 5 && pFormat->m_GBits == 5 && pFormat->m_BBits == 5)
			return pFormat;
	}

	return NULL;
}


// talon: inline in 2001: d3d_SetupLMTexture expands it (0x415501) and
// there is no copy of it out of line.
//this function will, given the desired size for a texture, find the appropriate
//size that should be used
static inline void d3d_FindTextureSize(uint16 nDesiredX, uint16 nDesiredY, uint32& nOutX, uint32& nOutY)
{
	nOutX = nOutY = 0;
	for(uint32 nCurrSize = 0; nCurrSize < NUM_VALID_TEXTURE_SIZES; nCurrSize++)
	{
		if((nOutX == 0) && (nDesiredX <= g_TextureSizes[nCurrSize]))
			nOutX = g_TextureSizes[nCurrSize];

		if((nOutY == 0) && (nDesiredY <= g_TextureSizes[nCurrSize]))
			nOutY = g_TextureSizes[nCurrSize];
	}
}


// talon: 0x4154bd.  Sets up a processed poly's lightmap as the current
// texture, making it the first time (CPrePoly's 0x20 keeps it, 0x24 is the
// lightmap).  Called by D3DRender::DrawPrePoly (0x419de4).  The name is
// ours.
BOOL d3d_SetupLMTexture(D3DRender *pRender, CPrePoly *pPoly)
{
	uint32 width, height;
	CPreLightMap *pLightMap;
	Direct3dTexture *pRet;
	TextureFormat *pFormat;
	LPDIRECTDRAWSURFACE4 pSurface;
	IDirect3DTexture2 *pTexture;

	pLightMap = (CPreLightMap*)pPoly->m_Unknown_24;
	if(!pLightMap)
		return FALSE;

	if(pPoly->m_Unknown_20)
	{
		d3d_SetupTextureSlot(pRender, (Direct3dTexture*)pPoly->m_Unknown_20);
		return TRUE;
	}

	if(!pPoly->m_LMWidth || !pPoly->m_LMHeight)
		return FALSE;

	d3d_FindTextureSize(pPoly->m_LMWidth, pPoly->m_LMHeight, width, height);
	if(!width || !height)
		return FALSE;

	pFormat = d3d_SelectLightmapFormat(pRender);
	if(!pFormat)
		return FALSE;

	pSurface = d3d_CreateTextureSurface2(pRender, width, height, DDSCAPS_VIDEOMEMORY, pFormat);
	if(!pSurface)
		return FALSE;

	if(!d3d_PutLightmapIntoSurface(pRender, pLightMap, pSurface))
	{
		pSurface->Release();
		return FALSE;
	}

	pSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&pTexture);
	if(!pTexture)
	{
		pSurface->Release();
		return FALSE;
	}

	pRet = new Direct3dTexture;
	pRet->m_Link.m_pData = pRet;
	pRet->m_pTexture = pTexture;
	pRet->m_pSurface = pSurface;
	pRet->m_pDrawMgr = pRender;
	pRet->m_uMul = 1.0f / (float)width;
	pRet->m_vMul = 1.0f / (float)height;

	dl_Insert(pRender->m_Textures.m_pPrev, &pRet->m_Link);

	pPoly->m_Unknown_20 = (uint32)pRet;
	d3d_SetupTextureSlot(pRender, pRet);
	return TRUE;
}


// Sets D3D up to use this texture slot (0x415639).
static void d3d_SetupTextureSlot(D3DRender *pRender, Direct3dTexture *pTexture)
{
	HRESULT hResult;

	// Setup coordinate multipliers.
	g_uMul = pTexture->m_uMul * g_TextureMipScale;
	g_vMul = pTexture->m_vMul * g_TextureMipScale;

	// Move it to the end of the LRU.
	dl_Remove(&pTexture->m_Link);
	dl_Insert(pRender->m_Textures.m_pPrev, &pTexture->m_Link);

	hResult = pRender->m_pDevice->SetTexture(0, pTexture->m_pTexture);
}


// 0x41569a
LPDIRECTDRAWSURFACE4 d3d_CreateTextureSurface2(
	D3DRender *pRender,
	DWORD width,
	DWORD height,
	DWORD extraCaps,
	TextureFormat *pFormat)
{
	DDSURFACEDESC2 desc;
	LPDIRECTDRAWSURFACE4 pSurface;
	HRESULT hResult;

	memset(&desc, 0, sizeof(desc));
	desc.dwSize = sizeof(desc);
	desc.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
	desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
	desc.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
	desc.dwWidth = width;
	desc.dwHeight = height;
	memcpy(&desc.ddpfPixelFormat, &pFormat->m_PF, sizeof(desc.ddpfPixelFormat));

	hResult = pRender->m_pDirectDraw->CreateSurface(&desc, &pSurface, NULL);
	if(hResult == DD_OK)
		return pSurface;
	else
		return NULL;
}


// talon: 0x41570a.  Decompresses a lightmap (0x4442b7) and converts it into
// the surface through the format manager.  The name is ours.
static BOOL d3d_PutLightmapIntoSurface(D3DRender *pRender, CPreLightMap *pLightMap, LPDIRECTDRAWSURFACE4 pSurface)
{
	uint32 data[1024];
	DDSURFACEDESC2 desc;
	FMConvertRequest request;

	if(pLightMap->Decompress(data))
	{
		memset(&desc, 0, sizeof(desc));
		desc.dwSize = sizeof(desc);
		if(pSurface->Lock(NULL, &desc, DDLOCK_WAIT, NULL) == DD_OK)
		{
			*request.m_pSrcFormat = d3d_GetPValueFormat();
			request.m_pSrc = (uint8*)data;
			request.m_SrcPitch = pLightMap->m_Width << 2;
			d3d_DDPFToPFormat(&desc.ddpfPixelFormat, request.m_pDestFormat);
			request.m_pDest = (uint8*)desc.lpSurface;
			request.m_DestPitch = desc.lPitch;
			request.m_Width = pLightMap->m_Width;
			request.m_Height = pLightMap->m_Height;
			request.m_Flags = 0;
			g_FormatMgr.ConvertPixels(&request);

			pSurface->Unlock(NULL);
			return TRUE;
		}
	}

	return FALSE;
}


// talon: 0x415804, the PValue format by value.  The name is ours.
static PFormat d3d_GetPValueFormat()
{
	PFormat ret;

	ret.InitPValueFormat();
	return ret;
}


// talon: 0x41584b.  The name is ours.
static void d3d_DDPFToPFormat(DDPIXELFORMAT *pDDPF, PFormat *pFormat)
{
	BPPIdent bpp;

	if(pDDPF->dwRGBBitCount == 16)
		bpp = BPP_16;
	else
		bpp = BPP_32;

	pFormat->Init(bpp, pDDPF->dwRGBAlphaBitMask,
		pDDPF->dwRBitMask, pDDPF->dwGBitMask, pDDPF->dwBBitMask);
}


// 0x415872.  2001's takes the poly and binds its one texture; the time it
// takes goes to the frame statistics.
BOOL d3d_SetupPolyTexture(D3DRender *pRender, CEditPoly *pPoly)
{
	CCycleCounter counter(&pRender->m_Stats.m_TextureSetupCycles);
	CTexture *pSystemTexture;
	Direct3dTexture *pTexture;
	void **pNewBindings;

	if(!pPoly->m_pTextureFile)
		return FALSE;

	// Get the system memory texture in memory.
	pSystemTexture = dib_GetDibTexture(pPoly->m_pTextureFile);
	if(!pSystemTexture)
	{
		pRender->m_pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
		return FALSE;
	}

	// Get its binding.
	if(pRender->m_BindingNumber < pSystemTexture->m_nBindings)
	{
		pTexture = (Direct3dTexture*)pSystemTexture->m_Bindings[pRender->m_BindingNumber];
		if(pTexture)
		{
			d3d_SetupTextureSlot(pRender, pTexture);
			return TRUE;
		}
	}
	else
	{
		// Add bindings for it..
		pNewBindings = (void**)dalloc(sizeof(void*) * (pRender->m_BindingNumber + 1));
		memset(pNewBindings, 0, sizeof(void*) * (pRender->m_BindingNumber + 1));
		memcpy(pNewBindings, pSystemTexture->m_Bindings, sizeof(void*) * pSystemTexture->m_nBindings);

		if(pSystemTexture->m_Bindings)
		{
			dfree(pSystemTexture->m_Bindings);
		}

		pSystemTexture->m_Bindings = pNewBindings;
		pSystemTexture->m_nBindings = pRender->m_BindingNumber + 1;
	}

	// No binding.. make a Direct3dTexture and get it in texture memory.
	pTexture = d3d_SetupNewTexture(pRender, pSystemTexture);
	if(pTexture)
	{
		d3d_SetupTextureSlot(pRender, pTexture);
		return TRUE;
	}
	else
	{
		pRender->m_pDevice->SetRenderState(D3DRENDERSTATE_TEXTUREHANDLE, 0);
		return FALSE;
	}
}


// 0x4159d3
static Direct3dTexture* d3d_SetupNewTexture(D3DRender *pRender, CTexture *pInTexture)
{
	Direct3dTexture *pRet;
	CDib *pDib;
	DWORD extraCaps;
	LPDIRECTDRAWSURFACE4 pSurface;
	IDirect3DTexture2 *pTexture;
	BOOL bSpecialMipmap;
	DWORD i, testWidth, testHeight;
	CTexture *pSystemTexture;


	pSystemTexture = pInTexture;
	pDib = pSystemTexture->m_pDib;
	bSpecialMipmap = FALSE;

	if(!CheckTextureSize(pRender, pDib->GetWidth(), pDib->GetHeight()))
	{
		// See if there is an acceptable mipmap.
		testWidth = pSystemTexture->m_Header.m_BaseWidth;
		testHeight = pSystemTexture->m_Header.m_BaseHeight;
		for(i=0; i < pSystemTexture->m_Header.m_nMipmaps; i++)
		{
			if(CheckTextureSize(pRender, testWidth, testHeight))
			{
				bSpecialMipmap = TRUE;

				// Load the mipmap.
				pSystemTexture = dib_LoadMipmap(pInTexture->m_pIdent, i);
				if(!pSystemTexture)
					return NULL;

				pDib = pSystemTexture->m_pDib;

				// Make sure it's really valid.
				if(!CheckTextureSize(pRender, pDib->GetWidth(), pDib->GetHeight()))
					return NULL;

				goto MakeSurface;
			}

			testWidth >>= 1;
			testHeight >>= 1;
		}

		// If we didn't find one, get outta here..
		return NULL;
	}

MakeSurface:
	// Try to create a surface.
	if(pRender->m_Caps.dwDevCaps & D3DDEVCAPS_TEXTURESYSTEMMEMORY)
		extraCaps = DDSCAPS_SYSTEMMEMORY;
	else
		extraCaps = DDSCAPS_VIDEOMEMORY;

	while(1)
	{
		pSurface = d3d_CreateTextureSurface2(pRender, pDib->GetWidth(), pDib->GetHeight(), extraCaps, pRender->m_pTextureFormat);
		if(pSurface)
			break;

		// Try to free some texture memory.
		if(!d3d_FreeSomeSpace(pRender))
			return NULL;
	}

	// Get the IDirect3DTexture2.
	pSurface->QueryInterface(IID_IDirect3DTexture2, (void**)&pTexture);
	if(!pTexture)
	{
		pSurface->Release();
		return NULL;
	}

	// Translate colors and put the texture in video memory.
	if(!d3d_PutTextureIntoSurface(pRender, pSystemTexture, pSurface, pTexture))
	{
		pSurface->Release();
		pTexture->Release();
		return NULL;
	}


	// Create the texture.
	pRet = new Direct3dTexture;
	pRet->m_Link.m_pData = pRet;
	pRet->m_pTexture = pTexture;
	pRet->m_pSurface = pSurface;
	pRet->m_pSystemTexture = pInTexture;
	pRet->m_pDrawMgr = pRender;
	pRet->m_uMul = 1.0f / (float)pSystemTexture->m_Header.m_BaseWidth;
	pRet->m_vMul = 1.0f / (float)pSystemTexture->m_Header.m_BaseHeight;

	pRet->m_uMul = pRet->m_uMul * pInTexture->m_UIMipmapScale;
	pRet->m_vMul = pRet->m_vMul * pInTexture->m_UIMipmapScale;

	pInTexture->m_Bindings[pRender->m_BindingNumber] = pRet;

	dl_Insert(pRender->m_Textures.m_pPrev, &pRet->m_Link);

	if(bSpecialMipmap)
	{
		dib_DestroyDibTexture(pSystemTexture);
	}

	return pRet;
}


// Frees up a texture.  Returns FALSE if there aren't any textures to free up (0x415bf0).
static BOOL d3d_FreeSomeSpace(D3DRender *pRender)
{
	if(pRender->m_Textures.m_pNext == &pRender->m_Textures)
		return FALSE;

	d3d_UnloadTexture((Direct3dTexture*)pRender->m_Textures.m_pNext->m_pData, pRender->m_BindingNumber);
	return TRUE;
}


// 0x415c1b.  The conversion's time goes to the frame statistics.
static BOOL d3d_PutTextureIntoSurface(D3DRender *pRender,
	CTexture *pSystemTexture, LPDIRECTDRAWSURFACE4 pSurface, IDirect3DTexture2 *pTexture)
{
	WORD *pCurOutPos, *pCurOutLine, *pEndOutLine, *pEndOutPos;
	WORD *pCurInPos, *pCurInLine;
	DDSURFACEDESC2 desc;
	DWORD outPixelPitch;
	WORD rPart, gPart, bPart;
	WORD rRightShift, rLeftShift;
	WORD gRightShift, gLeftShift;
	WORD bRightShift, bLeftShift;
	CDib *pDib;
	HRESULT hResult;


	pDib = pSystemTexture->m_pDib;

	// Convert from 5-5-5 to the destination format.
	desc.dwSize = sizeof(desc);

	hResult = pSurface->Lock(NULL, &desc, DDLOCK_WAIT, NULL);
	if(hResult == DD_OK)
	{
		// Get the shift info on the stack.
		rRightShift = (WORD)pRender->m_pTextureFormat->m_RRightShift;
		rLeftShift = (WORD)pRender->m_pTextureFormat->m_RLeftShift;
		
		gRightShift = (WORD)pRender->m_pTextureFormat->m_GRightShift;
		gLeftShift = (WORD)pRender->m_pTextureFormat->m_GLeftShift;
		
		bRightShift = (WORD)pRender->m_pTextureFormat->m_BRightShift;
		bLeftShift = (WORD)pRender->m_pTextureFormat->m_BLeftShift;

		pCurOutLine = (WORD*)desc.lpSurface;
		outPixelPitch = desc.lPitch >> 1;
		pEndOutLine = pCurOutLine + (pDib->GetHeight() * outPixelPitch);

		{
			CCycleCounter counter(&pRender->m_Stats.m_TextureConvertCycles);

			pCurInLine = pDib->GetBuf16();
			while(pCurOutLine < pEndOutLine)
			{
				pCurOutPos = pCurOutLine;
				pCurInPos = pCurInLine;
				pEndOutPos = pCurOutPos + desc.dwWidth;

				while(pCurOutPos < pEndOutPos)
				{
					rPart = (*pCurInPos >> 7) & 0xF8;
					gPart = (*pCurInPos >> 2) & 0xF8;
					bPart = (*pCurInPos << 3) & 0xF8;
					
					*pCurOutPos = ((rPart >> rRightShift) << rLeftShift) |
						((gPart >> gRightShift) << gLeftShift) |
						((bPart >> bRightShift) << bLeftShift);

					++pCurOutPos;
					++pCurInPos;
				}

				pCurOutLine += outPixelPitch;
				pCurInLine += pDib->GetPitch();
			}
		}
		
		pSurface->Unlock(NULL);
	}
	else
	{	
		return FALSE;
	}

	return TRUE;
}


// 0x415d93
static BOOL CheckTextureSize(D3DRender *pRender, DWORD width, DWORD height)
{
	return width <= pRender->m_Caps.dwMaxTextureWidth &&
		height <= pRender->m_Caps.dwMaxTextureHeight;
}
