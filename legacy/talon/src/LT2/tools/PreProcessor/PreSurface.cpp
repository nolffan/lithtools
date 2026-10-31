// talon: 0x004306e0..0x00430a40, between ProcessorApp and PreWorld: the
// 2001 surface code (CPreSurface's methods, BuildSurfacePolyLists,
// SetNodeIndices). DEdit links the same module between PrePoly and
// PreWorld (0x444af3..0x444c13), which puts 2001's file name there:
// PreSurface.cpp.

#include "bdefs.h"
#include <mmsystem.h>
#include "regmgr.h"
#include "processing.h"
#include "ProcessorOptions_decl.h"
#include "ProcessorApp_decl.h"
#include "presurface.h"
#include "preplane.h"
#include "prepoly.h"
#include "preworld.h"
#include "node.h"
#include "lightmap_planes.h"
#include "bspgen.h"


// ------------------------------------------------------------------------ //
// The surface code (PreSurface.h).
// ------------------------------------------------------------------------ //

// 0x4306e0
CPreSurface::CPreSurface()
{
	m_Unknown_80		= 0;
	m_TextureFlags		= 0;
	m_pTextureName		= "textures\\default.dtx";
	m_pPlane			= NULL;
	m_Unknown_a0		= 0;
	m_Unknown_a4		= 0;
	m_Ambient[2]		= 0;
	m_Ambient[1]		= 0;
	m_Ambient[0]		= 0;
	m_bDetail			= FALSE;
	m_bUsed				= FALSE;
}


// 0x430740: whether the poly may be split.
BOOL CPreSurface::ShouldSubdivide() const
{
	if(!(m_Flags & SURF_SKY) && !(m_Flags & SURF_HULLMAKER))
		return !(m_Flags & SURF_NOSUBDIV);

	return FALSE;
}


// 0x430760
BOOL CPreSurface::ShouldLightmap() const
{
	return (m_Flags & SURF_LIGHTMAP) && !(m_Flags & (SURF_SKY|SURF_INVISIBLE));
}


// 0x430780
BOOL CPreSurface::IsDetail() const
{
	return m_bDetail > 0;
}


// 0x430790
void CPreSurface::SetupLMVectors()
{
	GetLMPlaneVectors(FindLMPlane(m_pPlane->m_Normal), m_pPlane->m_Normal, P, Q);

	// Setup the inverses..
	InverseP = P.Inverse();
	InverseQ = Q.Inverse();
}


// 0x430950 (BspGen.h)
PReal GetPlaneDist(const CPrePlane *pPlane)
{
	return pPlane->m_Dist;
}


// 0x430960
void BuildSurfacePolyLists(CGLinkedList<CPreSurface*> &surfaces, CGLinkedList<CPrePoly*> &polies)
{
	GPOS pos;
	CPrePoly *pPoly;

	TermSurfacePolyLists(surfaces);

	for(pos=polies; pos; )
	{
		pPoly = polies.GetNext(pos);
		pPoly->GetSurface()->m_PolyList.AddTail(pPoly, &pPoly->m_PolyListNode);
	}
}


// 0x4309e0
void TermSurfacePolyLists(CGLinkedList<CPreSurface*> &surfaces)
{
	GPOS pos;

	for(pos=surfaces; pos; )
		surfaces.GetNext(pos)->m_PolyList.RemoveAll();
}


// 0x430a10
void SetNodeIndices(CNodeList &theList)
{
	uint32 curIndex;
	CNode *pNode;
	GPOS pos;

	curIndex = 0;
	for(pos=theList; pos; )
	{
		pNode = theList.GetNext(pos);
		pNode->m_Index = curIndex;
		curIndex++;
	}
}
