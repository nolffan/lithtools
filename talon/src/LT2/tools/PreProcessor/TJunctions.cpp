// talon: a module of its own in the image, 0x004479c0..0x004498f0, after Processing:
// fixing T-junctions (-FixTJunc: 'Fixing T-Junctions on %d polys...') (the name ours).
// Written from the image; Build 69 has no T-junction fixer in the
// processor (its packer has its own). The names are ours.
//
// Each edge of each poly is tested against the vertices of the polies
// whose extents overlap the poly's: a vertex lying inside the edge is a
// T-junction. A poly that may be split (bSplitPolys and its surface
// allows it) is split in two through the vertex; any other gets the
// vertex in m_Unknown_0c, its vertex list with the extra vertices.

#include "bdefs.h"
#include "preworld.h"
#include "processing.h"
#include "splitpoly.h"
#include "TJunctions_decl.h"


// TRUE if the world's node list, or a sub-BSP's, is past what a WORD
// index holds (0x449340).
static BOOL TJ_IsNodeTreeFull(CPreWorld *pWorld)
{
	uint32 i;

	if(pWorld->m_Nodes.GetSize() > 65000)
		return TRUE;

	for(i=0; i < pWorld->m_PolyArray; i++)
	{
		if(pWorld->m_PolyArray[i]->m_Nodes.GetSize() > 65000)
			return TRUE;
	}

	return FALSE;
}


// Looks for a plane through pPt, perpendicular to pPoly, that cuts pPoly
// through one of its vertices (not iEdge1 or iEdge2, the edge pPt is on)
// and through nothing else of it; the nearest such vertex wins.
// *pbFailed is TRUE if there is none at all (0x4488f0).
// Near: after GetPolySide's loop the shipped code reloads bFound, the
// vertex pointer and i in that order, ours in the reverse (register
// allocation; declaration order, the init order and the shape of the test
// don't move it).
static BOOL TJ_FindSplitPlane(CPrePoly *pPoly, PVector *pPt, BOOL *pbFailed, LTPlane *pPlane, uint32 iEdge1, uint32 iEdge2)
{
	SplitStruct split;
	LTPlane plane;
	PVector vVert, vToVert;
	PReal fDistSqr, fBestDistSqr;
	BOOL bFound;
	uint32 i;

	fBestDistSqr = -1.0f;
	bFound = FALSE;
	*pbFailed = TRUE;
	for(i=0; i < pPoly->NumVerts(); i++)
	{
		if(i == iEdge1 || i == iEdge2)
			continue;

		vVert = pPoly->Pt(i);
		vToVert = vVert - *pPt;
		fDistSqr = vToVert.MagSqr();
		if(fDistSqr < 0.1)
			continue;

		if(fDistSqr > fBestDistSqr && fBestDistSqr > 0.0f)
			continue;

		plane.m_Normal = pPoly->Normal().Cross(vToVert);
		plane.m_Normal.Norm();
		plane.m_Dist = plane.m_Normal.Dot(pPoly->Pt(i));

		if(!bFound)
			*pPlane = plane;

		if(fabs(plane.DistTo(*pPt)) > 0.1)
			continue;

		if(GetPolySide(&plane, pPoly, &split) == Intersect && split.m_nIntersect <= 1)
		{
			bFound = TRUE;
			*pbFailed = FALSE;
			*pPlane = plane;
			fBestDistSqr = fDistSqr;
		}
	}

	return bFound;
}


// Splits *ppPoly on pPlane, through pPt, and puts the two halves in its
// place: in the world's poly list, its nodes and its sub-BSPs. *ppPoly is
// deleted and becomes the first half (0x448d20).
static BOOL TJ_SplitPoly(CPreWorld *pWorld, CPrePoly **ppPoly, LTPlane *pPlane, PVector *pPt)
{
	SplitStruct split;
	CPrePoly *sides[2];
	CPreSubBsp *pBsp;
	BOOL bReplaced;
	uint32 i, j;

	if(GetPolySide(pPlane, *ppPoly, &split) != Intersect)
		return FALSE;

	if(!sub_4475a0(pPt, *ppPoly, sides, &split))
	{
		DeletePoly(sides[0]);
		DeletePoly(sides[1]);

		// Leave the surface untextured so the bad poly shows.
		(*ppPoly)->m_pSurface->m_pTextureName = "";
		return FALSE;
	}

	sides[0]->FindTextureOrigin(NULL, TRUE, pWorld->m_pMainWorld->GetLMGridSize());
	sides[1]->FindTextureOrigin(NULL, TRUE, pWorld->m_pMainWorld->GetLMGridSize());
	((CTJPrePoly*)sides[0])->CalcExtents(0.01f);
	((CTJPrePoly*)sides[1])->CalcExtents(0.01f);

	sub_447940(pWorld, pWorld->m_Nodes, *ppPoly, sides[0], sides[1]);
	for(i=0; i < pWorld->m_PolyArray; i++)
		sub_447940(pWorld, pWorld->m_PolyArray[i]->m_Nodes, *ppPoly, sides[0], sides[1]);

	pWorld->m_Polies.InsertAfter(*ppPoly, sides[0]);
	pWorld->m_Polies.InsertAfter(sides[0], sides[1]);

	for(i=0; i < pWorld->m_PolyArray; i++)
	{
		pBsp = pWorld->m_PolyArray[i];

		bReplaced = FALSE;
		for(j=0; j < pBsp->m_Polies; j++)
		{
			if(pBsp->m_Polies[j] == *ppPoly)
			{
				pBsp->m_Polies[j] = sides[0];
				bReplaced = TRUE;
			}
		}

		if(bReplaced)
			pBsp->m_Polies.Append(sides[1]);
	}

	pWorld->m_Polies.RemoveAt(*ppPoly);
	DeletePoly(*ppPoly);
	*ppPoly = sides[0];
	return TRUE;
}


// TRUE if pt is on the line through linePt along dir (a unit vector); t
// is how far along the line pt's nearest point is.
inline BOOL TJ_IsOnLine(const PVector &pt, const PVector &linePt, const PVector &dir, PReal &t)
{
	t = dir.Dot(pt - linePt);
	return (pt - (linePt + dir * t)).MagSqr() < 0.00001f;
}


// (0x4479c0.) Near (guide 11): the calls agree but one, and the
// differences are register and stack-slot allocation: ours keeps
// touching's size in ebp where the shipped code keeps iVert*40 there, and
// the cleanup loop at the end inlines BaseDelete where the shipped code
// calls it (0x448807; the inline budget: Term, SetSize(0), SetSize2 and a
// variable of its own don't move it). The line test as an inline helper
// is what leaves its vector operators called, as shipped.
void CTJunctionFixer::FixTJunctions(CPreWorld *pWorld, BOOL bSplitPolys)
{
	GPOS pos;
	CPrePoly *pPoly, *pTouch, *pCurPoly;
	PVector *pPt1, *pPt2, *pNextPt;
	PVector vPt, vN1, vN2, vInvalidPt;
	LTPlane splitPlane;
	PReal fEdgeLen, t, fRatio;
	uint32 i, iVert, iTouch, iTouchVert, nLast;
	uint32 nPolysAdded, nVertsAdded, nSkip, nProgress;
	BOOL bSplit, bChanged, bFailed, bInvalidGeometry;
	BOOL bPrevInLine, bCurInLine, bNextInLine, bCurInside, bNextInside;


	DrawStatusTextIfMainWorld("Fixing T-Junctions on %d polys...", pWorld->m_Polies.GetSize());

	nVertsAdded = 0;
	for(pos=pWorld->m_Polies; pos; )
		((CTJPrePoly*)pWorld->m_Polies.GetNext(pos))->CalcExtents(0.01f);

	CMoArray<CPrePoly*> touching;
	touching.SetCacheSize(512);

	nPolysAdded = 0;
	nSkip = 0;
	nProgress = 0;

	if(!g_pGlobs->m_bMakingWorldModel)
		StartProgressStage(0);

	bInvalidGeometry = FALSE;
	for(pos=pWorld->m_Polies; pos; pWorld->m_Polies.GetNext(pos))
	{
		if(bInvalidGeometry)
			break;

		if(!g_pGlobs->m_bMakingWorldModel)
			SetProgressBar((float)++nProgress / pWorld->m_Polies.GetSize());

		if(bSplitPolys && TJ_IsNodeTreeFull(pWorld))
		{
			DrawStatusText("*** WARNING *** WARNING *** WARNING ***");
			DrawStatusText("** Polygon split processing terminated");
			DrawStatusText("** due to full node tree.  Get rid of a");
			DrawStatusText("** large number of T-Junctions and");
			DrawStatusText("** try again.");
			bSplitPolys = FALSE;
		}

		pPoly = pWorld->m_Polies.GetAt(pos);
		if(bSplitPolys && ((CTJPreSurface*)pPoly->GetSurface())->sub_430740())
		{
			bSplit = TRUE;
		}
		else
		{
			bSplit = FALSE;
			pPoly->m_Unknown_0c.SetSize(pPoly->NumVerts());
			for(i=0; i < pPoly->NumVerts(); i++)
				pPoly->m_Unknown_0c[i] = pPoly->Vert(i);
		}

		// The polies it could touch (kept for the halves of a split poly).
		if(nSkip == 0)
		{
			touching.SetSize(0);
			sub_447220(pWorld->m_RootNode, pPoly, touching);
			for(i=0; i < pWorld->m_PolyArray; i++)
				sub_447220(pWorld->m_PolyArray[i]->m_RootNode, pPoly, touching);

			if(touching.GetSize() == 0)
				continue;
		}
		else
		{
			--nSkip;
		}

		for(iVert=0; iVert < (bSplit ? pPoly->NumVerts() : pPoly->m_Unknown_0c.GetSize()); iVert++)
		{
			if(bSplit)
			{
				pPt1 = &pPoly->Pt(iVert);
				pPt2 = &pPoly->Pt((iVert+1) % pPoly->NumVerts());
			}
			else
			{
				pPt1 = &pPoly->m_Unknown_0c[iVert].m_Vec;
				pPt2 = &pPoly->m_Unknown_0c[(iVert+1) % pPoly->m_Unknown_0c.GetSize()].m_Vec;
			}

			PVector vEdgeDir = *pPt2 - *pPt1;
			fEdgeLen = vEdgeDir.Mag();
			if(fEdgeLen < 0.5f)
				continue;

			vEdgeDir *= 1.0f / fEdgeLen;

			bFailed = FALSE;
			bChanged = FALSE;
			for(iTouch=0; iTouch < touching; iTouch++)
			{
				if(bChanged)
					break;

				pTouch = touching[iTouch];

				// Is each of its vertices on the edge's line, and inside the edge?
				nLast = pTouch->NumVerts() - 1;
				bPrevInLine = TJ_IsOnLine(pTouch->Pt(nLast), *pPt1, vEdgeDir, t);

				bCurInLine = TJ_IsOnLine(pTouch->Pt(0), *pPt1, vEdgeDir, t);
				bCurInside = bCurInLine && t > 0.25f && t < (fEdgeLen - 0.25f);

				for(iTouchVert=0; iTouchVert < pTouch->NumVerts(); iTouchVert++)
				{
					pNextPt = &pTouch->Pt((iTouchVert+1) % pTouch->NumVerts());
					bNextInLine = TJ_IsOnLine(*pNextPt, *pPt1, vEdgeDir, t);
					bNextInside = bNextInLine && t > 0.25f && t < (fEdgeLen - 0.25f);

					// A vertex inside the edge, with an edge of its poly along it.
					if(bCurInside && (bPrevInLine || bNextInLine))
					{
						vPt = pTouch->Pt(iTouchVert);

						if(bSplit)
						{
							if(!TJ_FindSplitPlane(pPoly, &vPt, &bFailed, &splitPlane, iVert, (iVert+1) % pPoly->NumVerts()))
							{
								if(bFailed)
								{
									bChanged = TRUE;
									break;
								}
							}
							else if(TJ_SplitPoly(pWorld, &pPoly, &splitPlane, &vPt))
							{
								bChanged = TRUE;
								++nPolysAdded;
								++nSkip;
								break;
							}
						}
						else
						{
							pPoly->m_Unknown_0c.Insert(iVert+1, pTouch->AllVert(iTouchVert));
							pPoly->m_Unknown_0c[iVert+1].m_Vec = vPt;

							fRatio = (vPt - *pPt1).Mag() / (vPt - *pPt2).Mag();
							vN1 = pPoly->m_Unknown_0c[iVert].m_Normal;
							vN2 = pPoly->m_Unknown_0c[(iVert+1) % pPoly->m_Unknown_0c.GetSize()].m_Normal;
							VEC_LERP(pPoly->m_Unknown_0c[iVert+1].m_Normal, vN1, vN2, fRatio);
							pPoly->m_Unknown_0c[iVert+1].m_Normal.Norm();

							bChanged = TRUE;
							++nVertsAdded;
							break;
						}
					}

					bCurInside = bNextInside;
					bPrevInLine = bCurInLine;
					bCurInLine = bNextInLine;
				}
			}

			// No plane to split it on: give up on this poly.
			if(bChanged && bFailed)
			{
				if(nSkip > 100)
				{
					bInvalidGeometry = TRUE;
					vInvalidPt = pPoly->Pt(0);
				}

				break;
			}

			// Split: start again on the second half.
			if(bChanged && bSplit)
			{
				pos = pPoly;
				iVert = (uint32)-1;
			}
		}
	}

	if(bInvalidGeometry)
	{
		DrawStatusText("*** WARNING *** WARNING *** WARNING ***");
		DrawStatusText("** Unable to complete T-Junction processing");
		DrawStatusText("** due to invalid geometry.  Coordinates of");
		DrawStatusText("** first point of current polygon:");
		DrawStatusText("**    %0.4f,%0.4f,%0.4f", VEC_EXPAND(vInvalidPt));
	}

	// The polies that got no vertices don't need their list.
	for(pos=pWorld->m_Polies; pos; )
	{
		pCurPoly = pWorld->m_Polies.GetNext(pos);
		if(pCurPoly->m_Unknown_0c.GetSize() == pCurPoly->NumVerts())
			pCurPoly->m_Unknown_0c.Term();
	}

	pWorld->CalcNodeBoundingBoxes();
	pWorld->SetPolyIndices();
	pWorld->InitPolyVertexIndices();

	if(nPolysAdded)
		DrawStatusTextIfMainWorld("Added %d polys for T-Junctions", nPolysAdded);

	if(nVertsAdded)
		DrawStatusTextIfMainWorld("Added %d verts for T-Junctions", nVertsAdded);

	if(!nPolysAdded && !nVertsAdded)
		DrawStatusTextIfMainWorld("No T-Junctions found");

	if(!g_pGlobs->m_bMakingWorldModel)
		EndProgressStage(0, FALSE);
}
