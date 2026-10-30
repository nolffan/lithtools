//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//

// This module defines the DrawBase class.  This derives from DrawMgr and defines
// most of the rendering functionality.  D3DRender and CEditRender derive from this.

#ifndef __DRAWBASE_H__
#define __DRAWBASE_H__



	#include "editgrid.h"
	#include "editpoly.h"
	#include "drawmgr.h"
	#include "viewdef.h"

	class CRegionView;
	class CMeshShapeList;
	class CBaseEditObj;
	class CPreMainWorld;
	class CDecalInfo;


	#define ORIGIN_LINE_SIZE 10.0f
	#define D3DRGB_255(r,g,b) (0xFF000000 | ((long)r << 16) | ((long)g << 8) | (long)b)
	#define D3DRGB_GETR(color)	(((color) >> 16) & 0xFF)
	#define D3DRGB_GETG(color)	(((color) >> 8) & 0xFF)
	#define D3DRGB_GETB(color)	(((color) >> 0) & 0xFF)



	// talon: the 2001 renderer's projection constants (Draw, 0x41ad81 and
	// 0x41ad8c): z' = (g_A * z + g_B) / z, g_B = -g_A.
	extern float g_A, g_B;


	// talon: 2001's DrawBase, 0x30 bytes: its constructor (0x41ad0a), its
	// destructor (0x418db0, emitted by draw_d3d.obj) and its vtable
	// (0x4d7510, twelve slots after DrawMgr's four at 0x4d7500).  The 2001
	// editor draws the region's brush list, not Build 69's node tree, and
	// reads the display options through GetApp() each time (no cached
	// pointer, no render flags).
	class DrawBase : public DrawMgr
	{
	// The main interface used by the views.
	public:
		
						DrawBase(CRegionView *pView);
		virtual			~DrawBase() {}
					
		// This is overridden by the subclasses and they call it when they're ready to have 
		// drawing commands issued.
		virtual void	Draw();
		
		virtual void	Resize(int x, int y) {}
		virtual void	Term() {}

		// talon: slot 4 (0x10): two arguments, empty here (0x418e17);
		// D3DRender's (0x41a0d9) draws a processed world's surfaces.  The
		// name is ours.
		virtual void	DrawWorld(CPreMainWorld *pWorld, BOOL bBlend) {}

	// The software/d3d abstraction functions.
	public:

		virtual void	DrawFlatPoly2(TLVertex *pPoints, DWORD nPoints, CVector *pNormal, COLORREF color) {}
		virtual void	DrawTexturedPoly(CEditPoly *pPoly) {}

		// talon: slot 7 (0x1c): one argument, empty here and in D3DRender
		// (0x401006).  What it was is unknown.
		virtual void	Unknown_1c(void *p) {}

		// Draw the line (it already has been transformed and clipped).
		virtual void	DrawLine(TLVertex *pVerts) {}

		// Draws a projected vertex.  Z value should be intact.
		virtual void	DrawVert(CVector &pos, DWORD borderColor, DWORD fillColor, int nSize)=0;

		// talon: slot 10 (0x28): one argument, empty here and in D3DRender
		// (0x401006).  What it was is unknown.
		virtual void	Unknown_28(void *p) {}

		// Enable/disable z-buffer (reads AND writes).
		// Returns the previous z-enable state.
		virtual BOOL	ZEnable(BOOL bEnable) {return FALSE;}

	// talon: the functions of DrawBase.cpp, 0x41ad54..0x41f004.
	public:

		// Clips/projects/draws the line (0x41aea9).
		void			Draw3dLine(TLVertex *pVerts);
		void			Draw3dLine2(TLVertex &vert1, TLVertex &vert2);

		// talon: 0x41af6e, the view transform applied to both vertices in
		// place, then Draw3dLine.  The name is ours.
		void			TransformAndDraw3dLine(TLVertex *pVerts);

		// Transforms and calls Draw3dLine (0x41b00b).
		void			TransformAndDrawLine(const CVector &Pt1, const CVector &Pt2, DWORD lineColor);

		// talon: 0x41b051, the same with its points by value.  The name is ours.
		void			TransformAndDrawLine2(CVector Pt1, CVector Pt2, DWORD lineColor);

		// Transforms and draws the vertex (0x41b067).
		void			TransformAndDrawVert(CVector &pos, DWORD borderColor, DWORD fillColor, int nSize);

		// Draw the grid (0x41b0be).
		BOOL			DrawEditGrid(CEditGrid *pGrid);

		// Draw the poly edges (0x41b5bb).
		void			DrawPolyLines(CEditPoly *pPoly, int xOffset, int yOffset, DWORD lineColor);

		// talon: 0x41b751, DrawPolyLines for a processed-world poly (its
		// vertices transformed with Apply4x4); called by D3DRender::DrawWorld
		// (0x41a22f).  The name is ours; the poly's type is part 6's 2001
		// CPrePoly (stand-in: P1PrePolyView, DrawBase_p1.cpp).
		void			DrawPrePolyLines(CPrePoly *pPoly, int xOffset, int yOffset, DWORD lineColor);

		// Clips the poly to the plane (0x41b8a5, the template's body: 2001
		// has only the TLVertex one).
		BOOL			ClipPolyToPlane(LTPlane *pPlane, TLVertex *pPoints, WORD &pointStart, WORD &pointEnd);

		// Sets up the point to point map and returns a cookie that you can 
		// fill each entry with when you use it (0x41bb8d).
		uint16			InitPointToPointMap(uint16 nPoints);

		// Fills in 8 points given box corners (0x41bbe2).
		static void		GetBoxPoints(const LTVector &min, const LTVector &max, LTVector *pPoints);

		// Translates to TLVertices and calls DrawFlatPoly2 (0x41bd09).
		void			DrawFlatPoly(CBasePoly *pPoly, COLORREF color);

		// talon: 0x41bd77, the region's brushes, flat, textured or as lines,
		// and the processed world.  The name is ours.
		void			DrawBrushes();

		// talon: 0x41bfc7, the brushes' vertices.  The name is ours.
		void			DrawBrushVerts();

		// Calls DrawVert for each vertex (0x41c0aa).
		void			DrawPolyVerts(CEditPoly *pPoly, DWORD lineColor, DWORD fillColor);

		// Draws the 'drawing' poly (the one they're laying out with spacebar) (0x41c118).
		void			DrawDrawingPoly();

		void			InternalDrawBox(CVector *pPoints, BOOL bFilled, COLORREF color);	// 0x41c1ec

		// talon: P2's functions that Draw (0x41ad54) calls, declared here
		// (P2 defines them): the names of 0x41d9a8 and 0x41ea57 are ours.
		void			DrawObjects();			// 0x41d9a8
		void			DrawPaths();			// 0x41d9f3
		void			DrawSelectedBrushBox();	// 0x41e40d
		void			DrawSelections();		// 0x41e108
		void			DrawMarker();			// 0x41ecab
		void			DrawOrigin();			// 0x41eba7
		void			DrawHandles();			// 0x41e561
		void			DrawUnknown_b4();		// 0x41ea57


		void			DrawRotatedBox(CVector *pPos, CVector *pDims, CVector &right, CVector &up, CVector &forward, BOOL bFilled, COLORREF color);	// 0x41c5bb
		void			DrawBox(CVector *pPos, CVector *pDims, BOOL bFilled, COLORREF color);		// 0x41cc29
		void			DrawCircle(CVector *pCenter, CReal fRadius, UINT nLineSegs, CVector *pForward, CVector *pUp, DWORD color);	// 0x41cd93

		// 0x41cef7: 2001's takes the object's euler angles, not a matrix.
		void			DrawFieldOfView(DVector *pPos, LTVector vAngles,
										float fov, float length, DWORD color);

		void			DrawArrowLine(DVector *pSrc, DVector *pDest, DWORD color);				// 0x41d116
		void			DrawLineToObjects(CBaseEditObj *pSrcObject, char *pDestName, DWORD color);	// 0x41d3d8

		// 0x41d454 (to 0x41d967): one object, its props' radius, FOV and
		// links, its dims and its orientation axes.
		void			DrawObject(CBaseEditObj *pObj);

		// (DrawObjects 0x41d9a8, DrawPaths 0x41d9f3, DrawSelections 0x41e108,
		// DrawSelectedBrushBox 0x41e40d, DrawHandles 0x41e561, DrawUnknown_b4
		// 0x41ea57, DrawOrigin 0x41eba7 and DrawMarker 0x41ecab are part 2's
		// but declared by DrawBase_decl_p1.h, as Draw calls them.)

		// 0x41eda4: Build 69's InitFrame, which in 2001 also transforms and
		// projects every brush's points and every object's position.
		BOOL			InitFrame();


		// talon: 0x419c08 is a MEMBER (every caller sets ecx to the
		// renderer: 0x41e155, 0x41e2bc, 0x41ea84) that doesn't use this.
		// DrawBase.h's free GetDisplayColorD3D should become this; declared
		// here until the fold so that part 2's calls are thiscalls.
		DWORD			GetDisplayColorD3D(uint32 nColor)
		{
			COptionsDisplay *pDisp = GetApp()->GetOptions().GetDisplayOptions();

			COLORREF g = GetApp()->GetOptions().GetDisplayOptions()->GetColor(nColor);
			COLORREF r = GetApp()->GetOptions().GetDisplayOptions()->GetColor(nColor);

			return RGB_MAKE(GetRValue(r), GetGValue(g), GetBValue(pDisp->GetColor(nColor)));
		}

	// Vars.
	public:

		// The current view we're rendering (0x04).
		CRegionView		*m_pView;

		CViewDef		*m_pViewDef;		// 0x08

		// talon: 0x0c, the view's navigator position, copied each frame
		// (Draw, 0x41adb2).  The name is ours.
		CVector			m_ViewPos;

		// talon: 0x18, the point-to-point map: a cookie per point pair
		// (InitPointToPointMap, 0x41bb8d), so each wireframe edge is drawn
		// once per brush.
		CMoArray<uint16>	m_PointToPoint;
		uint16			m_nPointsInMap;		// 0x2c
		uint16			m_nCookie;			// 0x2e
	};


	// 2001: no parallel case; the vertex z is always set up for perspective.
	template <class T>
	inline void SetupVertZ(T &vert, float z)
	{
		vert.rhw = 1.0f / z;
		vert.m_Vec.z = (g_A * z + g_B) * vert.rhw;
	}

	template <class T>
	inline void SetupVert(T *pVert, float x, float y, float z, DWORD theColor)
	{
		pVert->m_Vec.x = x;
		pVert->m_Vec.y = y;
		pVert->color = theColor;
		SetupVertZ(*pVert, z);
	}


#endif




