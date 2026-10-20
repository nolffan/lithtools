#include "bdefs.h"
#include "preworld.h"
#include "pregeometry.h"
#include "preleaf.h"
#include "prelightmap.h"
#include "lightmap_compress.h"
#include "node.h"
#include "editobjects.h"
#include "brushtoworld.h"
#include "splitpoly.h"
#include "editbrush.h"
#include "editpoly.h"
#include "findworldmodel.h"
#include "proputils.h"
#include "processing.h"

// talon: 0x42b8e0..0x42c9c0 in the image. The real name is lost; this one sorts
// between the physics module and pregeometry, as the image needs.

// A big square on pPlane around pOrigin, cut down by the other planes of
// the brush to its face; NULL if nothing is left (0x42b8e0, the name is
// ours).
static TempPoly* CreatePolyFromPlane(CPrePlane *pPlane, PVector *pOrigin, PReal size,
	CPreSurface *pSurface, CPrePlane *pPlanes, BOOL *pValid, uint32 nPlanes)
{
	PVector right, up, forward, vNormal;
	TempPoly *pPoly, *sides[2];
	SplitStruct split;
	PolySide side;
	uint32 i;

	BuildFrameOfReference(&pPlane->m_Normal, NULL, &right, &up, &forward);

	pPoly = CreatePoly(TempPoly, 4, TRUE);
	pPoly->m_pSurface = pSurface;

	right *= size;
	up *= size;

	pPoly->Pt(0) = *pOrigin + (-right - up);
	pPoly->Pt(1) = *pOrigin + (right - up);
	pPoly->Pt(2) = *pOrigin + (up + right);
	pPoly->Pt(3) = *pOrigin + (-right + up);

	// Face the plane's way.
	vNormal = (pPoly->Pt(2) - pPoly->Pt(0)).Cross(pPoly->Pt(1) - pPoly->Pt(0));
	if(vNormal.Dot(pPlane->m_Normal) < 0.0f)
	{
		PVector temp;

		temp = pPoly->Pt(0);
		pPoly->Pt(0) = pPoly->Pt(3);
		pPoly->Pt(3) = temp;

		temp = pPoly->Pt(1);
		pPoly->Pt(1) = pPoly->Pt(2);
		pPoly->Pt(2) = temp;
	}

	for(i=0; i < nPlanes; i++)
	{
		if(pPlane == &pPlanes[i] || !pValid[i])
			continue;

		side = GetPolySide(&pPlanes[i], pPoly, &split);
		if(side == FrontSide)
		{
			DeletePoly(pPoly);
			return NULL;
		}
		else if(side == Intersect)
		{
			SplitPoly(&pPlanes[i], pPoly, sides, &split);
			DeletePoly(sides[FrontSide]);
			DeletePoly(pPoly);
			pPoly = sides[BackSide];
		}
	}

	return pPoly;
}


// A normal component, snapped to -1, 0 or 1 when it's that close (0x42bde0).
static PReal SnapNormalComponent(PReal val)
{
	if(val < -0.9999f)
		return -1.0f;

	if(val > 0.9999f)
		return 1.0f;

	if(fabs(val) < 0.00001)
		return 0.0f;

	return val;
}


// The poly's plane through its first three points snapped to integers;
// FALSE if they don't make one (0x42be30).
static BOOL CalcSnappedPlane(CEditPoly *pPoly, PVector *pPts, CPrePlane *pPlane)
{
	uint32 i, j;
	PReal mag;

	for(i=0; i < 3; i++)
	{
		for(j=0; j < 3; j++)
		{
			pPts[i][j] = (PReal)floor(pPoly->Pt(i)[j] + 0.5f);
		}
	}

	pPlane->m_Normal = (pPts[2] - pPts[0]).Cross(pPts[1] - pPts[0]);
	pPlane->m_Normal.Norm();

	mag = pPlane->m_Normal.Mag();
	if(mag >= 0.5f && mag <= 1.5f)
	{
		pPlane->m_Normal.x = SnapNormalComponent(pPlane->m_Normal.x);
		pPlane->m_Normal.y = SnapNormalComponent(pPlane->m_Normal.y);
		pPlane->m_Normal.z = SnapNormalComponent(pPlane->m_Normal.z);
		pPlane->m_Dist = pPlane->m_Normal.Dot(pPts[0]);
		return TRUE;
	}

	return FALSE;
}


TempBrush* MakeTempBrush(CEditBrush *pBrush)
{
	CGLinkedList<TempPoly*> polies;
	TempBrush *pRet;
	TempPoly *pPoly;
	CEditPoly *pEditPoly;
	CPrePlane *pPlanes;
	PVector *pPts;
	CEditPoly **pPlanePolies;
	BOOL *pValid;
	uint32 i, j, nPlanes, size;
	PReal polySize, dist, maxDist;
	BOOL bOutOfBounds, bBadPlane;
	GPOS pos;


	if(GetBoolProp(&pBrush->m_PropList, "NoSnap"))
	{
		// As it is: a TempPoly for each poly.
		nPlanes = pBrush->m_Polies.GetSize();
		size = sizeof(TempBrush) + sizeof(TempNode)*(nPlanes-1);
		pRet = (TempBrush*)malloc(size);
		memset(pRet, 0, size);
		pRet->m_nNodes = (uint16)nPlanes;

		for(i=0; i < pBrush->m_Polies; i++)
		{
			pEditPoly = pBrush->m_Polies[i];

			pPoly = CreatePoly(TempPoly, pEditPoly->NumVerts(), FALSE);
			pPoly->m_pSurface = (CPreSurface*)pEditPoly->m_pUser1;
			pPoly->m_Unknown_10 = 0;
			pPoly->m_Unknown_0c = 1;

			for(j=0; j < pEditPoly->NumVerts(); j++)
			{
				CEditVert &pt = pEditPoly->Pt(j);
				pPoly->AddVert(PVector(pt.x, pt.y, pt.z));
			}

			pRet->m_Polies.AddTail(pPoly);
		}

		return pRet;
	}

	pBrush->UpdatePlanes();

	// Big enough to cross the brush.
	polySize = (pBrush->m_BoundingBox.m_Max - pBrush->m_BoundingBox.m_Min).Mag();
	polySize += polySize;

	pPlanes = new CPrePlane[pBrush->m_Polies.GetSize()];
	pPts = new PVector[pBrush->m_Polies.GetSize()*3];
	pPlanePolies = new CEditPoly*[pBrush->m_Polies.GetSize()];
	pValid = new BOOL[pBrush->m_Polies.GetSize()];

	nPlanes = 0;
	maxDist = 0.0f;
	bOutOfBounds = FALSE;
	bBadPlane = FALSE;
	for(i=0; i < pBrush->m_Polies; i++)
	{
		pEditPoly = pBrush->m_Polies[i];

		for(j=0; j < pEditPoly->NumVerts(); j++)
		{
			if(pEditPoly->Pt(j).x > g_pGlobs->m_MaxBounds || pEditPoly->Pt(j).x < -g_pGlobs->m_MaxBounds ||
				pEditPoly->Pt(j).y > g_pGlobs->m_MaxBounds || pEditPoly->Pt(j).y < -g_pGlobs->m_MaxBounds ||
				pEditPoly->Pt(j).z > g_pGlobs->m_MaxBounds || pEditPoly->Pt(j).z < -g_pGlobs->m_MaxBounds)
			{
				bOutOfBounds = TRUE;
			}

			// How far from the first poly's plane (a flat brush has no thickness).
			dist = (PReal)fabs(VEC_DOT(pBrush->m_Polies[0]->Normal(), pEditPoly->Pt(j)) - pBrush->m_Polies[0]->Dist());
			if(dist > maxDist)
				maxDist = dist;
		}

		if(CalcSnappedPlane(pEditPoly, &pPts[nPlanes*3], &pPlanes[nPlanes]))
		{
			((CPreSurface*)pEditPoly->m_pUser1)->m_pPlane->m_Normal = pPlanes[nPlanes].m_Normal;
			((CPreSurface*)pEditPoly->m_pUser1)->m_pPlane->m_Dist = pPlanes[nPlanes].m_Dist;
			pValid[nPlanes] = TRUE;
			pPlanePolies[nPlanes] = pEditPoly;
			nPlanes++;
		}
		else
		{
			bBadPlane = TRUE;
		}
	}

	if(!(GetBrushType(pBrush) & SURF_HULLMAKER))
	{
		if(maxDist < 0.5f)
		{
			DrawStatusText("** Found a ZERO THICKNESS BRUSH (%d)", g_ProblemBrushes.GetSize());
			if(g_ProblemBrushes.FindElement(pBrush) == BAD_INDEX)
				g_ProblemBrushes.Append(pBrush);
		}
		else if(bBadPlane)
		{
			DrawStatusText("** Unable to generate a plane (%d)", g_ProblemBrushes.GetSize());
			if(g_ProblemBrushes.FindElement(pBrush) == BAD_INDEX)
				g_ProblemBrushes.Append(pBrush);
		}
	}

	if(bOutOfBounds)
	{
		DrawStatusText("** Found an out-of-bounds brush");
		if(g_OutOfBoundsBrushes.FindElement(pBrush) == BAD_INDEX)
			g_OutOfBoundsBrushes.Append(pBrush);
	}

	for(i=0; i < nPlanes; i++)
	{
		pPoly = CreatePolyFromPlane(&pPlanes[i], &pPts[i*3], polySize,
			(CPreSurface*)pPlanePolies[i]->m_pUser1, pPlanes, pValid, nPlanes);

		if(pPoly)
		{
			pPoly->m_pSurface = (CPreSurface*)pPlanePolies[i]->m_pUser1;
			pPoly->m_Unknown_10 = 0;
			pPoly->m_Unknown_0c = 1;
			polies.AddTail(pPoly);
		}
		else
		{
			pValid[i] = FALSE;
			CreatePolyFromPlane(&pPlanes[i], &pPts[i*3], polySize,
				(CPreSurface*)pPlanePolies[i]->m_pUser1, pPlanes, pValid, nPlanes);

			if(g_ProblemBrushes.FindElement(pBrush) == BAD_INDEX)
				g_ProblemBrushes.Append(pBrush);
		}
	}

	delete [] pPlanes;
	delete [] pPts;
	delete [] pPlanePolies;
	delete [] pValid;

	size = sizeof(TempBrush) + sizeof(TempNode)*(polies.GetSize()-1);
	pRet = (TempBrush*)malloc(size);
	memset(pRet, 0, size);
	pRet->m_nNodes = (uint16)polies.GetSize();

	for(pos=polies; pos; )
	{
		pRet->m_Polies.AddTail(polies.GetNext(pos));
	}

	return pRet;
}
