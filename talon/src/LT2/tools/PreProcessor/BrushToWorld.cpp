// ------------------------------------------------------------------ //
//	FILE	  : BrushToWorld.cpp
//	CREATED	  : February 5, 1997
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
// ------------------------------------------------------------------ //

// Includes....
#include "bdefs.h"
#include "preworld.h"
#include "brushtoworld.h"
#include "processing.h"
#include "splitpoly.h"
#include "proputils.h"


static uint32					g_nSplits;
static CPreWorld				*g_pWorld;
static CMoArray<TempBrush*>		g_Intersecters;
static void						(*g_ProgressCB)(float fPercent);
static CMoArray<TempBrush*>		g_TempBrushes;
static uint32					g_nNonIntersecting;

// Set while a brush is clipped against one that comes before it.
static BOOL						g_bEarlierBrush;


static TempBrush* SetupTempBrush(CPreWorld *pWorld, CEditBrush *pBrush);
static CPrePoly* TempPolyToPrePoly(TempPoly *pPoly);
static void TransferInsideFragments(CMoArray<TempBrush*> &brushes, CPreWorld *pWorld);
static void SortIntersecters(CMoArray<TempBrush*> &brushes);
static void FindIntersecters(CMoArray<TempBrush*> &brushes, CMoArray<TempBrush*> &intersecters);
static void ClearIntersecters(CMoArray<TempBrush*> &brushes);
static void MergeIntersecters(CMoArray<TempBrush*> &brushes);
static void SetBrushIndices(CMoArray<TempBrush*> &brushes);


CPrePoly* EditPolyToPrePoly(CEditPoly *pEditPoly)
{
	CPrePoly *pPrePoly = CreatePoly(CPrePoly, pEditPoly->NumVerts(), FALSE);
	if(!pPrePoly)
		return NULL;

	pPrePoly->m_pSurface = (CPreSurface*)pEditPoly->m_pUser1;

	for(uint32 i=0; i < pEditPoly->NumVerts(); i++)
	{
		CEditVert &pt = pEditPoly->Pt(i);
		pPrePoly->AddVert( PVector(pt.x, pt.y, pt.z) );
	}

	return pPrePoly;
}


BOOL BrushToWorld(
	GenList<CEditBrush*> &brushes,
	CPreWorld *pWorld,
	void (*ProgressCB)(float fPercent))
{
	GenListPos pos;
	CEditBrush *pBrush;
	CPreSurface *pSurface;
	CPrePoly *pPoly;


	g_pWorld = pWorld;
	g_ProgressCB = ProgressCB;
	g_nSplits = 0;
	g_TempBrushes.SetCacheSize(1024);

	CMoArray<TempBrush*> noCSGBrushes;
	noCSGBrushes.SetCacheSize(1024);

	// Create the TempBrushes.
	DrawStatusTextIfMainWorld("Creating temp brushes");
	for(pos=brushes.GenBegin(); brushes.GenIsValid(pos); )
	{
		pBrush = brushes.GenGetNext(pos);

		if(pBrush->m_Polies.GetSize() <= 0)
			continue;

		pSurface = (CPreSurface*)pBrush->m_Polies[0]->m_pUser1;
		if(g_pGlobs->m_bNoSnapNoCSG)
		{
			if(!(pSurface->m_Flags & SURF_HULLMAKER) && !GetBoolProp(&pBrush->m_PropList, "NoSnap"))
				g_TempBrushes.Append(SetupTempBrush(pWorld, pBrush));
			else
				noCSGBrushes.Append(SetupTempBrush(pWorld, pBrush));
		}
		else
		{
			if(pSurface->m_Flags & SURF_HULLMAKER)
			{
				pPoly = EditPolyToPrePoly(pBrush->m_Polies[0]);
				if(pPoly)
					pWorld->m_Polies.Append(pPoly);
			}
			else
			{
				g_TempBrushes.Append(SetupTempBrush(pWorld, pBrush));
			}
		}
	}

	if(g_TempBrushes.GetSize() == 0 && noCSGBrushes.GetSize() == 0)
	{
		DrawStatusText("!! Empty world model BSP found (%s)", pWorld->m_WorldName);
		return pWorld->m_Polies.GetSize() > 0;
	}

	g_nNonIntersecting = 0;
	DrawStatusTextIfMainWorld("Finding intersecters");
	FindIntersecters(g_TempBrushes, g_Intersecters);
	ClearIntersecters(noCSGBrushes);

	DrawStatusTextIfMainWorld("Sorting intersecters");
	SortIntersecters(g_TempBrushes);
	SetBrushIndices(g_TempBrushes);
	SetBrushIndices(noCSGBrushes);

	DrawStatusTextIfMainWorld("Merging intersecters");
	MergeIntersecters(g_TempBrushes);
	MergeIntersecters(noCSGBrushes);
	g_Intersecters.Term();

	DrawStatusTextIfMainWorld("Transferring inside fragments");
	TransferInsideFragments(g_TempBrushes, pWorld);
	TransferInsideFragments(noCSGBrushes, pWorld);
	pWorld->InitPolyVertexIndices();

	DrawStatusTextIfMainWorld("%d BTW poly splits.", g_nSplits);

	uint32 nPolies = pWorld->m_Polies.GetSize();
	DrawStatusTextIfMainWorld("Num Polies: %d.  Joining polies...", nPolies);
	pWorld->JoinPolies(0.0001f);
	DrawStatusTextIfMainWorld("%d polies removed.", nPolies - pWorld->m_Polies.GetSize());
	DrawStatusTextIfMainWorld("%d polies left.", pWorld->m_Polies.GetSize());

	pWorld->SetupSurfaceLMVectors();
	return true;
}


// 0x403010. talon: near (240 against 243 bytes): the same code under a
// register rotation (pRet in ebp, shipped esi) and the box's y and z stored
// through pRet where shipped stores x through pMin; the three bytes are
// ebp's disp8. Spelling the box as pRet->m_Min, assigning pMin after the
// Inits and moving the declarations don't move it.
static TempBrush* SetupTempBrush(CPreWorld *pWorld, CEditBrush *pBrush)
{
	TempBrush *pRet;
	GPOS pos;
	TempPoly *pPoly;
	PVector *pMin, *pMax;

	pRet = MakeTempBrush(pBrush);

	for(pos=pRet->m_Polies; pos; )
	{
		pPoly = pRet->m_Polies.GetNext(pos);
		pPoly->m_pOriginalPoly = TempPolyToPrePoly(pPoly);
		pWorld->m_OriginalBrushPolies.Append(pPoly->m_pOriginalPoly);
	}

	// Get the bounding box.
	pMin = &pRet->m_Min;
	pMax = &pRet->m_Max;
	pMin->Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	pMax->Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);
	for(pos=pRet->m_Polies; pos; )
	{
		pPoly = pRet->m_Polies.GetNext(pos);
		pPoly->ExtendBounds(*pMin, *pMax);
	}

	*pMin -= 1.0f;
	*pMax += 1.0f;
	return pRet;
}


static CPrePoly* TempPolyToPrePoly(TempPoly *pPoly)
{
	CPrePoly *pRet;

	pRet = CreatePoly(CPrePoly, pPoly->NumVerts(), TRUE);
	memcpy(pRet->VertList(), pPoly->VertList(), sizeof(CPolyVert) * pPoly->NumVerts());
	pRet->m_pSurface = pPoly->m_pSurface;
	return pRet;
}


// Moves what's left of each brush into the world, giving each original
// poly's vertices the average normal of the brush's planes that touch them.
static void TransferInsideFragments(CMoArray<TempBrush*> &brushes, CPreWorld *pWorld)
{
	CMoArray<PVector> unused;
	uint32 i, iVert, k;
	TempBrush *pBrush;
	GPOS pos, testPos;
	TempPoly *pPoly, *pTest;
	CPrePoly *pPrePoly, *pOrig;
	PVector vNormal, vPt;

	unused.SetCacheSize(1000);

	for(i=0; i < brushes; i++)
	{
		pBrush = brushes[i];
		unused.SetSize(0);

		for(pos=pBrush->m_Polies; pos; )
		{
			pPoly = pBrush->m_Polies.GetNext(pos);

			pPrePoly = TempPolyToPrePoly(pPoly);
			pPrePoly->m_pOriginalBrushPoly = pPoly->m_pOriginalPoly;

			pOrig = pPoly->m_pOriginalPoly;
			for(iVert=0; iVert < pOrig->NumVerts(); iVert++)
			{
				vNormal.Init(0.0f, 0.0f, 0.0f);
				vPt = pOrig->Pt(iVert);

				for(testPos=pBrush->m_Polies; testPos; )
				{
					pTest = pBrush->m_Polies.GetNext(testPos);
					for(k=0; k < pTest->NumVerts(); k++)
					{
						if((vPt - pTest->Pt(k)).MagSqr() < 0.1f)
						{
							vNormal += pTest->Normal();
							break;
						}
					}
				}

				vNormal.Norm();
				pOrig->PtNormal(iVert) = vNormal;
			}

			pWorld->m_Polies.Append(pPrePoly);
		}

		// Free the brush.
		for(pos=pBrush->m_Polies; pos; )
		{
			pPoly = pBrush->m_Polies.GetNext(pos);
			DeletePoly(pPoly);
		}
		pBrush->m_Polies.RemoveAll();
		free(pBrush);
	}

	brushes.Term();
}


// Brushes with the most intersecters go first.
static void SortIntersecters(CMoArray<TempBrush*> &brushes)
{
	uint32 i;
	BOOL bSorted;
	TempBrush *pTemp;

	if(brushes.GetSize() == 0)
		return;

	do
	{
		bSorted = TRUE;
		for(i=0; i < brushes.GetSize()-1; i++)
		{
			if(brushes[i]->m_nIntersecters < brushes[i+1]->m_nIntersecters)
			{
				pTemp = brushes[i];
				brushes[i] = brushes[i+1];
				bSorted = FALSE;
				brushes[i+1] = pTemp;
			}
		}
	} while(!bSorted);
}


static BOOL DoBrushesIntersect(TempBrush *pBrush1, TempBrush *pBrush2);

static void FindIntersecters(CMoArray<TempBrush*> &brushes, CMoArray<TempBrush*> &intersecters)
{
	uint32 i, j, iCur;
	TempBrush *pBrush, *pOther;

	intersecters.Term();
	intersecters.SetCacheSize(50000);

	for(i=0; i < brushes; i++)
	{
		pBrush = brushes[i];
		pBrush->m_nIntersecters = 0;

		if(pBrush->m_Polies.GetSize() < 4)
			continue;

		for(j=0; j < brushes; j++)
		{
			if(i == j)
				continue;

			pOther = brushes[j];
			if(pOther->m_Polies.GetSize() < 4)
				continue;

			if(pBrush->m_Min <= pOther->m_Max && pBrush->m_Max >= pOther->m_Min)
			{
				if(DoBrushesIntersect(pBrush, pOther))
				{
					intersecters.Append(pOther);
					pBrush->m_nIntersecters++;
				}
				else
				{
					g_nNonIntersecting++;
				}
			}
		}
	}

	// Point each brush at its part of the array.
	iCur = 0;
	for(i=0; i < brushes; i++)
	{
		if(brushes[i]->m_nIntersecters > 0)
		{
			brushes[i]->m_Intersecters = &intersecters[iCur];
			iCur += brushes[i]->m_nIntersecters;
		}
	}
}


// 0x403720. talon: near (868 against 748 bytes), the inline budget: the
// shipped code calls the _CVector constructor in the edge loop's
// operator- and Cross (0x4021c0), Norm (0x4044b0) and both Dots
// (0x4021e0), where ours expands them all. The rest (the touching test,
// IsDetail on pPoly2 first, the final DistTo loop) is the shipped shape.
static BOOL DoBrushesIntersect(TempBrush *pBrush1, TempBrush *pBrush2)
{
	GPOS pos1, pos2;
	TempPoly *pPoly1, *pPoly2;
	CPrePlane *pPlane1, *pPlane2;
	BOOL bOutside;
	uint32 i, k;
	PVector vEdge, vEdgeNormal;
	PReal dist;

	// The brushes intersect unless one of pBrush1's planes has all of pBrush2 in front of it.
	for(pos1=pBrush1->m_Polies; pos1; )
	{
		pPoly1 = pBrush1->m_Polies.GetNext(pos1);
		pPlane1 = pPoly1->GetPlane();

		bOutside = TRUE;
		for(pos2=pBrush2->m_Polies; pos2 && bOutside; )
		{
			pPoly2 = pBrush2->m_Polies.GetNext(pos2);
			pPlane2 = pPoly2->GetPlane();

			// Faces touching, one detail and one not?
			if(pPlane1->m_Normal.Dot(pPlane2->m_Normal) < -0.9 && pPlane2->m_Dist + pPlane1->m_Dist < 0.1)
			{
				if(pPoly1->GetSurface()->IsDetail() != pPoly2->GetSurface()->IsDetail())
				{
					// If pPoly1 is inside pPoly2's edges, they only touch.
					for(i=0; i < pPoly2->NumVerts(); i++)
					{
						vEdge = pPoly2->NextPt(i) - pPoly2->Pt(i);
						vEdgeNormal = vEdge.Cross(pPoly2->Normal());
						vEdgeNormal.Norm();
						dist = vEdgeNormal.Dot(pPoly2->Pt(i));

						for(k=0; k < pPoly1->NumVerts(); k++)
						{
							if(vEdgeNormal.Dot(pPoly1->Pt(k)) - dist > 0.1)
								goto NotInside;
						}
					}

					return FALSE;
				}
			}

NotInside:;

			for(i=0; i < pPoly2->NumVerts(); i++)
			{
				if(pPlane1->DistTo(pPoly2->Pt(i)) <= 0.1)
				{
					bOutside = FALSE;
					break;
				}
			}
		}

		if(bOutside)
			return FALSE;
	}

	return TRUE;
}


static void ClearIntersecters(CMoArray<TempBrush*> &brushes)
{
	uint32 i;

	for(i=0; i < brushes; i++)
		brushes[i]->m_nIntersecters = 0;
}


static TempNode* BuildBrushTree(TempBrush *pBrush);
static void ClipBrush(TempBrush *pBrush, TempNode *pTree);

static void MergeIntersecters(CMoArray<TempBrush*> &brushes)
{
	uint16 i, j, iOther;
	TempBrush *pBrush;

	for(i=0; i < (uint16)brushes.GetSize(); i++)
	{
		pBrush = brushes[i];
		pBrush->m_pTree = BuildBrushTree(pBrush);
	}

	for(i=0; i < (uint16)brushes.GetSize(); i++)
	{
		pBrush = brushes[i];
		for(j=0; j < pBrush->m_nIntersecters; j++)
		{
			iOther = pBrush->m_Intersecters[j]->m_Index;
			g_bEarlierBrush = iOther < i;
			ClipBrush(brushes[iOther], pBrush->m_pTree);
		}

		if(g_ProgressCB)
			g_ProgressCB((float)i / brushes.GetSize());
	}
}


static TempNode* BuildBrushTree(TempBrush *pBrush)
{
	uint32 i, j;
	GPOS pos, testPos;
	TempPoly *pPoly;
	TempNode *pNode, **ppPrev;

	ppPrev = NULL;
	pos = pBrush->m_Polies;
	for(i=0; i < pBrush->m_Polies.GetSize(); i++)
	{
		pPoly = pBrush->m_Polies.GetNext(pos);
		pNode = &pBrush->m_Nodes[i];

		// Only one node per plane.
		testPos = pBrush->m_Polies;
		for(j=0; j < i; j++)
		{
			if(pPoly->GetPlane() == pBrush->m_Polies.GetNext(testPos)->GetPlane())
				goto NextPoly;
		}

		if(ppPrev)
			*ppPrev = pNode;

		ppPrev = &pNode->m_Sides[BackSide];
		pNode->m_Sides[FrontSide] = TEMPNODE_IN;
		pNode->m_pPlane = pPoly->GetPlane();

NextPoly:;
	}

	if(ppPrev)
		*ppPrev = TEMPNODE_OUT;

	return pBrush->m_Nodes;
}


static void ClipPoly(TempBrush *pBrush, TempPoly *pPoly, TempNode *pNode);

static void ClipBrush(TempBrush *pBrush, TempNode *pTree)
{
	uint32 i, nPolies;
	GPOS pos;
	TempPoly *pPoly;

	pos = pBrush->m_Polies;
	nPolies = pBrush->m_Polies.GetSize();
	for(i=0; i < nPolies; i++)
	{
		pPoly = pBrush->m_Polies.GetNext(pos);
		ClipPoly(pBrush, pPoly, pTree);
	}

	pos = pBrush->m_Polies;
	nPolies = pBrush->m_Polies.GetSize();
	for(i=0; i < nPolies; i++)
	{
		pPoly = pBrush->m_Polies.GetNext(pos);
		pPoly->m_pOriginalPoly->SetPPFlags(pPoly->m_pOriginalPoly->GetPPFlags() & ~PP_COPLANAR);
	}
}


// talon: the name is ours. ClipPoly's front test goes through an inline
// helper: one more expansion after the split's DeletePoly spends the
// inline budget (spent last site first) so that DeletePoly stays
// `push 0; call ??_GTempPoly` (0x403ed3) as shipped, while the later
// one expands to ~TempPoly. The back test is written out: through the
// helper it compiles 8 bytes short.
inline BOOL IsTempLeaf(TempNode *pNode)
{
	return pNode == TEMPNODE_IN || pNode == TEMPNODE_OUT;
}

static void ClipPoly(TempBrush *pBrush, TempPoly *pPoly, TempNode *pNode)
{
	TempPoly *sides[2];
	SplitStruct splitStruct;
	PolySide side;

	for(;;)
	{
		sides[0] = sides[1] = NULL;

		side = GetPolySide(pNode->m_pPlane, pPoly, &splitStruct);
		if(side == Intersect)
		{
			++g_nSplits;
			SplitPoly(pNode->m_pPlane, pPoly, sides, &splitStruct);
			sides[0]->m_Unknown_0c = sides[1]->m_Unknown_0c = pPoly->m_Unknown_0c;

			pBrush->m_Polies.AddTail(sides[0]);
			pBrush->m_Polies.AddTail(sides[1]);
			pBrush->m_Polies.RemoveAt(pPoly);
			DeletePoly(pPoly);
		}
		else
		{
			// A face coplanar with a face of an earlier brush goes.
			if(g_bEarlierBrush && splitStruct.m_nIntersect == pPoly->NumVerts() && side == FrontSide)
			{
				side = BackSide;
				pPoly->m_pOriginalPoly->SetPPFlags(pPoly->m_pOriginalPoly->GetPPFlags() | PP_COPLANAR);
			}

			sides[side] = pPoly;
		}

		if(sides[BackSide])
		{
			if(pNode->m_Sides[BackSide] == TEMPNODE_IN || pNode->m_Sides[BackSide] == TEMPNODE_OUT)
			{
				// It's inside the brush.
				if(sides[BackSide]->m_pOriginalPoly->GetPPFlags() & PP_COPLANAR)
					sides[BackSide]->m_pOriginalPoly->SetPPFlags(sides[BackSide]->m_pOriginalPoly->GetPPFlags() | PP_REMOVED);

				pBrush->m_Polies.RemoveAt(sides[BackSide]);
				DeletePoly(sides[BackSide]);
			}
			else if(sides[FrontSide])
			{
				ClipPoly(pBrush, sides[BackSide], pNode->m_Sides[BackSide]);
			}
			else
			{
				pPoly = sides[BackSide];
				pNode = pNode->m_Sides[BackSide];
				continue;
			}
		}

		if(!sides[FrontSide] || IsTempLeaf(pNode->m_Sides[FrontSide]))
			return;

		pPoly = sides[FrontSide];
		pNode = pNode->m_Sides[FrontSide];
	}
}


static void SetBrushIndices(CMoArray<TempBrush*> &brushes)
{
	uint32 i;

	for(i=0; i < brushes; i++)
		brushes[i]->m_Index = (uint16)i;
}
