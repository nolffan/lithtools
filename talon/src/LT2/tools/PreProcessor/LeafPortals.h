// ------------------------------------------------------------------ //
//	The portal functions (LeafPortals.cpp, 0x413a10..0x4144f0). The
//	file's name is ours, like LeafPortals.cpp's. They were PreLeaf.h's
//	until the declaration count said otherwise: PreWorld.cpp and
//	Processing.cpp include PreLeaf.h and use none of them, and what
//	PreLeaf.h declares moves code in both (tools/guides/09, "The
//	declaration count is a symbol count, mod 64").
// ------------------------------------------------------------------ //

#ifndef __LEAFPORTALS_H__
#define __LEAFPORTALS_H__

	#include "preleaf.h"

	// A copy of pPortal: its vertices, plane and index (0x413a10).
	CPrePortal* ClonePortal(CPrePortal *pPortal);

	// pSrc's vertices and split attributes into pDest (0x413a70, the name
	// is ours).
	void CopyPortal(CPrePortal *pSrc, CPrePortal *pDest);

	// Two portals to clip back and forth between (the PVS's); m_iCur is
	// the one holding the current shape. The names are ours.
	struct PortalBuffer
	{
		CPrePortal*		Cur()		{return m_pPortals[m_iCur];}
		CPrePortal*		Other()		{return m_pPortals[!m_iCur];}

		int				m_iCur;
		CPrePortal		*m_pPortals[2];
	};

	// Clips the current portal to pPlane's front side; FALSE when nothing
	// is left (0x413ad0, the name is ours).
	BOOL ClipPortalToPlane(CPrePlane *pPlane, PortalBuffer *pBuf);

	// The six portals of a box (0x413c90).
	BOOL MakeBoxPortals(CMoArray<CPrePortal*> &portals, PVector &vMin, PVector &vMax);

#endif
