// EditSoftRender.cpp - talon: stand-in for the 2001 software renderer.
//
// The image has a 16-bit (565) software renderer that Build 69 doesn't:
// 0x429b55..0x42b077, between EditRegion's code and ExportObjFile's, so its
// file sorts between theirs (the name is ours). A DrawBase-derived class of
// 0x3c0 bytes (constructor 0x42a9d0, initialised by 0x42ac19) drawing into a
// DIB section shown with StretchDIBits. dm_CreateNormalDrawMgr (0x42b02b)
// makes one for a view whose renderer isn't Direct3D (CViewRender::
// RestartRender), and when Direct3D can't be started.
//
// Not written yet: until it is, the views that would use it draw nothing.

#include "bdefs.h"
#include "drawmgr.h"

// talon: stand-in (0x42b02b): 2001's is `new` of the software renderer
// (0x3c0 bytes) and its Init(pView).
DrawMgr* dm_CreateNormalDrawMgr(CRegionView *pView)
{
	return new DrawMgr;
}
