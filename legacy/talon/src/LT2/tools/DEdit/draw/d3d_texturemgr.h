//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//

// Defines the texture management routines for the d3d renderer.

#ifndef __D3D_TEXTUREMGR_H__
#define __D3D_TEXTUREMGR_H__

	//forward declarations
	class  D3DRender;
	class  CEditPoly;
	class  CPrePoly;

	class CPrePoly;
	class CPreMainWorld;

	//---functions for hanlding texture usage in D3D---//

	BOOL d3d_InitTextureStuff(D3DRender *pRender);
	void d3d_TermTextureStuff(D3DRender *pRender);
	
	// talon: 2001's binds the poly's one texture (0x415872).
	BOOL d3d_SetupPolyTexture(D3DRender *pRender, CEditPoly *pPoly);

	// talon: a processed world's lightmap textures (0x4150be, 0x4150da,
	// 0x4154bd).  The names are ours.
	void d3d_UnbindPrePoly(CPrePoly *pPoly);
	void d3d_UnbindWorld(CPreMainWorld *pWorld);
	BOOL d3d_SetupLMTexture(D3DRender *pRender, CPrePoly *pPoly);

	//creates texture surfaces
	LPDIRECTDRAWSURFACE4 d3d_CreateTextureSurface2(D3DRender *pRender, DWORD width, DWORD height, DWORD extraCaps, TextureFormat *pFormat);


#endif  // __D3D_TEXTUREMGR_H__


