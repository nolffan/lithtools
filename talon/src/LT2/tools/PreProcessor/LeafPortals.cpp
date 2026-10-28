#include "bdefs.h"
#include "preleaf.h"
#include "splitpoly.h"

// talon: written from the image (0x413a10..0x4144f0); Build 69 has no
// portals. The file's name is ours: it sorts between gettextureinfo and
// the leak file module, as the image's order needs.


CPrePortal* ClonePortal(CPrePortal *pPortal)
{
	CPrePortal *pRet;

	pRet = CreatePoly(CPrePortal, pPortal->NumVerts(), TRUE);
	memcpy(pRet->VertList(), pPortal->VertList(), sizeof(CPolyVert)*pPortal->NumVerts());
	pRet->m_Plane = pPortal->m_Plane;
	pRet->m_Index = pPortal->m_Index;
	return pRet;
}


void CopyPortal(CPrePortal *pSrc, CPrePortal *pDest)
{
	memcpy(pDest->VertList(), pSrc->VertList(), sizeof(CPolyVert)*pSrc->NumVerts());
	pDest->CopySplitAttributes(pSrc);
	pDest->m_BasePoly.SetNumVerts((uint16)pSrc->NumVerts());
}


BOOL ClipPortalToPlane(CPrePlane *pPlane, PortalBuffer *pBuf)
{
	SplitStruct	split;
	PolySide	side;

	side = GetPolySide(pPlane, pBuf->Cur(), &split);
	if(side == BackSide)
		return FALSE;

	// Lying on the plane is as good as gone.
	if(split.m_nIntersect != pBuf->Cur()->NumVerts())
	{
		if(side == FrontSide)
			return TRUE;

		SplitPolySide(FrontSide, pPlane, pBuf->Cur(), pBuf->Other(), &split);
		pBuf->m_iCur = !pBuf->m_iCur;
		return TRUE;
	}

	return FALSE;
}


// -------------------------------------------------------------------------------- //
// 0x00413c90 (929 bytes; the rows 0x413fcc and its tail are inside it: 0x413fcc
// cuts the instruction at 0x413fca in half). MakeBoxPortals,
// which returns TRUE (0x414027).
// -------------------------------------------------------------------------------- //

// The shipped code calls CPreBasePoly::SetVert (0x414040) for the two loops
// and the first ten side vertices and expands the last six (0x413eae..).
// That budget comes out only when the plane call below goes to a function
// template (a plain function left the seventh expanded too, 968 bytes).
template<class P> void PortalGenerateNormal(P *pPoly);

BOOL MakeBoxPortals(CMoArray<CPrePortal*> &portals, PVector &vMin, PVector &vMax)
{
	PVector	pts[8];
	uint32	i;

	pts[0].Init(vMin.x, vMin.y, vMin.z);
	pts[1].Init(vMax.x, vMin.y, vMin.z);
	pts[2].Init(vMax.x, vMax.y, vMin.z);
	pts[3].Init(vMin.x, vMax.y, vMin.z);
	pts[4].Init(vMin.x, vMin.y, vMax.z);
	pts[5].Init(vMax.x, vMin.y, vMax.z);
	pts[6].Init(vMax.x, vMax.y, vMax.z);
	pts[7].Init(vMin.x, vMax.y, vMax.z);

	portals.SetSize(6);
	for(i=0; i < 6; i++)
		portals[i] = CreatePoly(CPrePortal, 4, TRUE);

	// Bottom and top.
	for(i=0; i < 4; i++)
		portals[0]->SetVert(i, pts[i]);

	for(i=0; i < 4; i++)
		portals[1]->SetVert(i, pts[7-i]);

	// The sides.
	portals[2]->SetVert(0, pts[1]);
	portals[2]->SetVert(1, pts[5]);
	portals[2]->SetVert(2, pts[6]);
	portals[2]->SetVert(3, pts[2]);

	portals[3]->SetVert(0, pts[2]);
	portals[3]->SetVert(1, pts[6]);
	portals[3]->SetVert(2, pts[7]);
	portals[3]->SetVert(3, pts[3]);

	portals[4]->SetVert(0, pts[3]);
	portals[4]->SetVert(1, pts[7]);
	portals[4]->SetVert(2, pts[4]);
	portals[4]->SetVert(3, pts[0]);

	portals[5]->SetVert(0, pts[0]);
	portals[5]->SetVert(1, pts[4]);
	portals[5]->SetVert(2, pts[5]);
	portals[5]->SetVert(3, pts[1]);

	for(i=0; i < 6; i++)
		PortalGenerateNormal(portals[i]);

	return TRUE;
}


void CPrePortal::Flip()
{
	uint32	i;
	PVector	temp;

	for(i=0; i < NumVerts()/2; i++)
	{
		temp = Pt(i);
		Pt(i) = Pt(NumVerts()-i-1);
		Pt(NumVerts()-i-1) = temp;
	}

	m_Plane.m_Normal = -m_Plane.m_Normal;
	m_Plane.m_Dist = -m_Plane.m_Dist;
}


// The plane of the portal's vertices, Newell's method (0x4143d0). The
// shipped code is Build 69's g_GenerateNormal (geomroutines.h) for
// CPrePortal, whose Normal() then returned a reference: the reference is
// what lets the loop address the current vertex first. The name is ours.
template<class P>
void PortalGenerateNormal(P *pPoly)
{
	uint32		prev, cur;

	pPoly->m_Plane.m_Normal.Init();
	prev = pPoly->NumVerts()-1;
	for( cur=0; cur < pPoly->NumVerts(); cur++ )
	{
		PVector &vN = pPoly->m_Plane.m_Normal;

		vN.x +=	(pPoly->Pt(prev).y - pPoly->Pt(cur).y) * 
				(pPoly->Pt(prev).z + pPoly->Pt(cur).z);
		
		vN.y +=	(pPoly->Pt(prev).z - pPoly->Pt(cur).z) * 
				(pPoly->Pt(prev).x + pPoly->Pt(cur).x);
		
		vN.z +=	(pPoly->Pt(prev).x - pPoly->Pt(cur).x) * 
				(pPoly->Pt(prev).y + pPoly->Pt(cur).y);

		prev = cur;
	}

	pPoly->m_Plane.m_Normal.Norm();
	pPoly->m_Plane.m_Dist = pPoly->m_Plane.m_Normal.Dot( pPoly->Pt(0) );
}
