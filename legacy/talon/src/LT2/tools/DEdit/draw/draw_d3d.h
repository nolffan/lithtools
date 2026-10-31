//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//

// Defines structures used by DEdit's d3d renderer.
#ifndef __DRAW_D3D_H__
#define __DRAW_D3D_H__

	#include "drawmgr.h"
	#include "regiondoc.h"
	#include "regionview.h"
	#include "d3d.h"
	#include "edithelpers.h"
	#include "drawbase.h"

	class CPreMainWorld;
	class CPreSurface;
	class CPrePoly;


	typedef struct TextureFormat_t
	{
		DDPIXELFORMAT	m_PF;
		
		// The texture type.
		// Higher numbers are better.  -1 means we don't support this format.
		int			m_TextureCode;
		
		// How many bits for r, g, and b (if not palettized).
		DWORD		m_RBits, m_GBits, m_BBits, m_ABits;

		// Shifting information for converting colors.
		// Note: there is no need for an alpha shift, because you can just
		//       OR by the (1-bit) alpha mask to turn alpha on in a pixel.
		DWORD		m_RRightShift, m_RLeftShift;
		DWORD		m_GRightShift, m_GLeftShift;
		DWORD		m_BRightShift, m_BLeftShift;

		DLink		m_Link;
	} TextureFormat;

	
	// talon: the renderer's frame statistics, 0x3c bytes at 0x94, cleared
	// each frame (Draw, 0x41a97a).  Cycle counts of CCycleCounter
	// (cyclemgr.h); the names are ours, by where each is added to.
	struct D3DRenderStats
	{
		uint32		m_Unknown_94;
		uint32		m_TextureSetupCycles;	// 0x98 (d3d_SetupPolyTexture, 0x415884)
		uint32		m_TextureConvertCycles;	// 0x9c (d3d_PutTextureIntoSurface)
		uint32		m_Unknown_a0[3];
		uint32		m_DrawCycles;			// 0xac (Draw, 0x41a9f7)
		uint32		m_BlitCycles;			// 0xb0 (Draw, 0x41aaa8)
		uint32		m_Unknown_b4[7];
	};

	
	// talon: 2001's D3DRender, 0x234 bytes (dm_CreateDirect3dDrawMgr's new,
	// 0x41ab12), laid out from its constructor (0x418d0e), destructor
	// (0x418e55), Init (0x418eac) and vtable (0x4d74d0).
	class D3DRender : public DrawBase
	{
	public:

					D3DRender(CRegionView *pView);
					~D3DRender();

		BOOL		Init(int deviceNum, int renderMode, CRect *pRect);
		void		Term();
	
	// Overrides.
	public:

		void		Draw();
		void		Resize(int width, int height);

		void		DrawWorld(CPreMainWorld *pWorld, BOOL bBlend);

		void		DrawFlatPoly2(TLVertex *pPoints, DWORD nPoints, CVector *pNormal, COLORREF color);
		void		DrawTexturedPoly(CEditPoly *pPoly);

		void		DrawLine(TLVertex *pVerts);
		void		DrawVert(CVector &pos, D3DCOLOR borderColor, D3DCOLOR fillColor, int nSize);
		
		BOOL		ZEnable(BOOL bEnable);


	public:

		void		InitVars();
		void		DrawTagRectPoly();

		// talon: binds a processed world to this renderer (0x419f0e), and
		// unbinds every world bound to it (0x419f46).  The names are ours.
		BOOL		BindWorld(CPreMainWorld *pWorld);
		void		UnbindWorlds();


	public:

		// Used to determine the Z format we'll be using.
		BOOL					m_bGotZFormat;			// 0x30
		DDPIXELFORMAT			m_ZFormat;				// 0x34

		BOOL					m_bPerspectiveProjection;	// 0x54
		CDibMgr					*m_pDibMgr;				// 0x58
		DLink					m_Link;					// 0x5c
		
		LPDIRECTDRAW4			m_pDirectDraw;			// 0x68
		LPDIRECT3D3				m_pDirect3d;
		LPDIRECTDRAWSURFACE4	m_pSurface;
		LPDIRECTDRAWSURFACE4	m_pZSurface;
		LPDIRECTDRAWSURFACE4	m_pPrimarySurface;
		LPDIRECT3DDEVICE3		m_pDevice;				// 0x7c
		LPDIRECT3DMATERIAL3		m_pMaterial;
		LPDIRECT3DVIEWPORT3		m_pViewport;
		LPDIRECTDRAWCLIPPER		m_pClipper;				// 0x88

		DWORD					m_SurfaceWidth, m_SurfaceHeight;	// 0x8c

		D3DRenderStats			m_Stats;				// 0x94

		// Device caps.
		D3DDEVICEDESC			m_Caps;					// 0xd0

		// talon: 0x1cc..0x1f8, not referenced by the renderer's code.
		uint32					m_Unknown_1cc[11];

		// All the texture formats supported by this card.
		DLink					m_TextureFormats;		// 0x1f8
		TextureFormat			*m_pTextureFormat;		// 0x204: the texture format we chose to use.

		DLink					m_Textures;				// 0x208
		int						m_BindingNumber;		// 0x214: each drawmgr gets a unique number..

		int						m_DeviceNum, m_RenderMode;	// 0x218

		// talon: 0x220, the processed worlds bound to this renderer
		// (BindWorld, 0x419f0e).  The name is ours.
		CMoArray<CPreMainWorld*>	m_Worlds;

	protected:

		// talon: a processed-world poly with its lightmap (0x419c68) and
		// its lightmap samples as points (0x419f68); DrawWorld calls them.
		// The names are ours.
		void		DrawPrePoly(CPrePoly *pPoly, float lmGridSize);
		void		DrawLightmapPoints(CPrePoly *pPoly, float lmGridSize);
	};


#endif  // __DRAW_D3D_H__



