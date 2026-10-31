//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//

// talon: 2001's DrawBase (0x41ad0a..0x41f523), written from the image in
// its order: the region's brush list and objects, not Build 69's node tree.

#include "bdefs.h"
#include "d3d.h"		// talon: RGB_MAKE, for DrawBase.h's GetDisplayColorD3D
#include "drawbase.h"
#include "regiondoc.h"
#include "regionview.h"
#include "edithelpers.h"
#include "projectbar.h"
#include "geomroutines.h"
#include "ltbeziercurve.h"
#include "texture.h"


// Used to setup z and rhw (0x509f74, 0x509f78).
float g_A;
float g_B;


// ------------------------------------------------------------------------------------ //
// DrawBase.
// ------------------------------------------------------------------------------------ //

// 0x41ad0a
DrawBase::DrawBase(CRegionView *pView)
{
	m_nCookie		= (uint16)-1;
	m_pViewDef		= NULL;
	m_pView			= pView;
}


// 0x41aea9
void DrawBase::Draw3dLine(TLVertex *pVerts)
{
	if(m_pView->ClipLineToFrustum(pVerts))
	{
		m_pViewDef->ProjectPt(pVerts[0].m_Vec, pVerts[0].m_Vec);
		m_pViewDef->ProjectPt(pVerts[1].m_Vec, pVerts[1].m_Vec);

		SetupVertZ(pVerts[0], pVerts[0].m_Vec.z);
		SetupVertZ(pVerts[1], pVerts[1].m_Vec.z);

		DrawLine(pVerts);
	}
}	


// 0x41af3d
void DrawBase::Draw3dLine2(TLVertex &vert1, TLVertex &vert2)
{
	TLVertex verts[2];

	verts[0] = vert1;
	verts[1] = vert2;
	Draw3dLine(verts);
}


// 0x41af6e
void DrawBase::TransformAndDraw3dLine(TLVertex *pVerts)
{
	m_pView->m_Transform.Apply(pVerts[0].m_Vec);
	m_pView->m_Transform.Apply(pVerts[1].m_Vec);
	Draw3dLine(pVerts);
}


// 0x41b00b
void DrawBase::TransformAndDrawLine(const CVector &Pt1, const CVector &Pt2, DWORD lineColor)
{
	TLVertex verts[2];

	m_pView->m_Transform.Apply(Pt1, verts[0].m_Vec);
	m_pView->m_Transform.Apply(Pt2, verts[1].m_Vec);
	verts[0].color = verts[1].color = lineColor;
	
	Draw3dLine(verts);
}


// 0x41b051
void DrawBase::TransformAndDrawLine2(CVector Pt1, CVector Pt2, DWORD lineColor)
{
	TransformAndDrawLine(Pt1, Pt2, lineColor);
}


// 0x41b067
void DrawBase::TransformAndDrawVert(CVector &pos, DWORD borderColor, DWORD fillColor, int nSize)
{
	CVector newPos;

	m_pView->m_Transform.Apply(pos, newPos);
	if(m_pView->InsideFrustum(newPos))
	{
		m_pViewDef->ProjectPt(newPos, newPos);
		DrawVert(newPos, borderColor, fillColor, nSize);
	}
}





// 0x41ad54
void DrawBase::Draw()
{
	float nearZ, farZ;

	// Init this for the z values.
	nearZ = m_pView->ViewDef()->m_NearZ;
	farZ = 16000.0f; 
	nearZ = farZ / (farZ - nearZ);
	g_A = nearZ;
	g_B = -nearZ;

	ZEnable(m_pView->IsPerspectiveViewType());

	m_ViewPos = m_pView->Nav().Pos();

	if(!m_pView->IsWalkThroughMovement() && m_pView->IsPerspectiveViewType())
		m_pView->Nav().UpdateLooking();

	// Draw the edit plane.
	if(m_pView->IsShowGrid())
		DrawEditGrid(&m_pView->EditGrid());

	DrawBrushes();
	DrawObjects();
	DrawPaths();
	DrawBrushVerts();

	if(m_pView->GetEditMode() == BRUSH_EDITMODE && m_pView->GetRegion()->m_Selections > 0)
		DrawSelectedBrushBox();

	DrawSelections();

	// Show marker crosshair...
	if(m_pView->IsShowMarker())
		DrawMarker();

	DrawOrigin();	

	if(m_pView->GetEditMode() == BRUSH_EDITMODE && !m_pView->IsPerspectiveViewType())
		DrawHandles();

	DrawUnknown_b4();
}


// 0x41b0be
BOOL DrawBase::DrawEditGrid(CEditGrid *pGrid)
{
	CReal gridSpacing, majorGridSpacing, scale, widthScale, heightScale;
	DWORD dwGridSpacing, dwMajorGridSpacing;
	DWORD i, count, spacing, mask;
	DVector curPos, endPos, curPosAdd;
	DWORD dwMinorLineColor, dwMajorLineColor;

	if( pGrid->m_DrawSize == 0 )
		return TRUE;

	gridSpacing = (CReal)m_pView->GetGridSpacing();
	majorGridSpacing = LTMAX((CReal)m_pView->GetMajorGridSpacing(), gridSpacing);

	// adjust grid spacing so it isn't drawn too small
	scale = (CReal)m_pView->m_nPixelScale;
	scale = scale * m_pView->ViewDef()->m_Magnify * gridSpacing;
	while ( scale < 3 )
	{	gridSpacing *= 2.0;
		scale *= 2.0;
	}

	dwGridSpacing = (DWORD)gridSpacing;
	dwMajorGridSpacing = (DWORD)majorGridSpacing;


	if ( m_pView->IsParallelViewType() )
	{	// width and height of view ajusted for zoom factor
		widthScale = m_pView->m_ViewDefInfo.m_fProjectHalfWidth / m_pView->m_pViewDef->m_Magnify;
		heightScale = m_pView->m_ViewDefInfo.m_fProjectHalfHeight / m_pView->m_pViewDef->m_Magnify;
	}
	else
	{	// width and height of grid piece to draw
		widthScale = heightScale = pGrid->m_DrawSize;
	}

	// make them multiples of grid, and make sure they'll go to/past the screen edges
	widthScale = majorGridSpacing * ceil(1 + widthScale/majorGridSpacing);
	heightScale = majorGridSpacing * ceil(1 + heightScale/majorGridSpacing);

	spacing = ( dwGridSpacing >= dwMajorGridSpacing ) ? 1 : dwMajorGridSpacing / dwGridSpacing;
	mask = spacing - 1;
	
	// X axis lines.
	curPos = m_pView->Nav().Pos();
	// snap position of view to major grid and kill 3rd dimension
	for ( i=0; i<3; i++ )
	{	curPos[i] = majorGridSpacing * floor(curPos[i]/majorGridSpacing);
		if ( pGrid->Forward()[i] != 0 )
			curPos[i] = 0;
	}
	curPos = curPos - pGrid->Right()*widthScale + pGrid->Up()*heightScale;
	endPos = curPos	+ pGrid->Right()*(2.0*widthScale);
	curPosAdd = -pGrid->Up() * gridSpacing;

	dwMinorLineColor = GetDisplayColorD3D(COptionsDisplay::kColorMinorGrid);
	dwMajorLineColor = GetDisplayColorD3D(COptionsDisplay::kColorMajorGrid);

	count = 2*(DWORD)heightScale / dwGridSpacing + 1;
	for( i=0; i < count; i++ )
	{
		if( (i&mask) != 0)
		{	
			TransformAndDrawLine(curPos, endPos, dwMinorLineColor);
		}
		else
		{	
			TransformAndDrawLine(curPos, endPos, dwMajorLineColor);
		}

		curPos += curPosAdd;
		endPos += curPosAdd;
	}

	// Y axis lines.
	curPos = m_pView->Nav().Pos();
	// snap position of view to grid
	for ( i=0; i<3; i++ )
	{	curPos[i] = majorGridSpacing * floor(curPos[i]/majorGridSpacing);
		if ( pGrid->Forward()[i] != 0 )
			curPos[i] = 0;
	}
	curPos = curPos - pGrid->Right()*(widthScale) + pGrid->Up()*(heightScale);
	endPos = curPos - pGrid->Up()*(2.0*heightScale);
	curPosAdd = pGrid->Right() * gridSpacing;
	
	count = 2*(DWORD)widthScale / dwGridSpacing + 1;
	for( i=0; i < count; i++ )
	{
		if( (i&mask) != 0 )
		{	
			TransformAndDrawLine(curPos, endPos, dwMinorLineColor);
		}
		else
		{	
			TransformAndDrawLine(curPos, endPos, dwMajorLineColor);
		}

		curPos += curPosAdd;
		endPos += curPosAdd;
	}

	return TRUE;
}


// 0x41b5bb
void DrawBase::DrawPolyLines(CEditPoly *pPoly, int xOffset, int yOffset, DWORD lineColor)
{
	float xOffsetFloat, yOffsetFloat;
	TLVertex verts[2];
	WORD *pStart, *pEnd, *pIndex, *pPrevIndex;
	WORD minIndex, maxIndex;
	uint16 *pCookie;
	
	CEditVert *pVerts = pPoly->m_pBrush->m_Points.GetArray();
	xOffsetFloat = (float)xOffset;
	yOffsetFloat = (float)yOffset;

	if(pPoly->NumVerts() >= 2)
	{
		pStart = pPoly->m_Indices.GetArray();
		pEnd = pStart + pPoly->NumVerts();
		pPrevIndex = pEnd - 1;

		for(pIndex = pStart; pIndex < pEnd; pPrevIndex = pIndex, pIndex++)
		{
			if(*pPrevIndex < *pIndex)
			{
				minIndex = *pPrevIndex;
				maxIndex = *pIndex;
			}
			else
			{
				minIndex = *pIndex;
				maxIndex = *pPrevIndex;
			}

			// Each edge once per cookie.
			pCookie = &m_PointToPoint[minIndex * m_nPointsInMap + maxIndex];
			if(*pCookie != m_nCookie)
			{
				*pCookie = m_nCookie;

				verts[0].m_Vec = pVerts[minIndex].m_Transformed;
				verts[1].m_Vec = pVerts[maxIndex].m_Transformed;

				if(m_pView->ClipLineToFrustum(verts))
				{
					m_pViewDef->ProjectPt(verts[0].m_Vec, verts[0].m_Vec);
					m_pViewDef->ProjectPt(verts[1].m_Vec, verts[1].m_Vec);

					SetupVert(&verts[0], verts[0].m_Vec.x + xOffsetFloat, verts[0].m_Vec.y + yOffsetFloat, verts[0].m_Vec.z - 0.001f, lineColor);
					SetupVert(&verts[1], verts[1].m_Vec.x + xOffsetFloat, verts[1].m_Vec.y + yOffsetFloat, verts[1].m_Vec.z - 0.001f, lineColor);

					DrawLine(verts);
				}
			}
		}
	}
}


// talon: stand-in: the 2001 DEdit CPrePoly (part 6's) as 0x41b751 reads
// it: a WORD vertex count at 0x94 and its vertices from 0x9c, 40 bytes
// each, the position first.
struct P1PrePolyVert
{
	CVector		m_Vec;
	uint8		m_Unknown_0c[0x1c];
};

struct P1PrePolyView
{
	uint8			m_Unknown_00[0x94];
	WORD			m_nVerts;
	uint8			m_Unknown_96[6];
	P1PrePolyVert	m_Verts[1];
};


// 0x41b751
void DrawBase::DrawPrePolyLines(CPrePoly *pInPoly, int xOffset, int yOffset, DWORD lineColor)
{
	float xOffsetFloat, yOffsetFloat;
	TLVertex verts[2];
	P1PrePolyView *pPoly = (P1PrePolyView*)pInPoly;
	DWORD i;

	xOffsetFloat = (float)xOffset;
	yOffsetFloat = (float)yOffset;

	if(pPoly->m_nVerts >= 2)
	{
		for(i=0; i < pPoly->m_nVerts; i++)
		{
			m_pView->m_Transform.Apply4x4(pPoly->m_Verts[i].m_Vec, verts[0].m_Vec);
			m_pView->m_Transform.Apply4x4(pPoly->m_Verts[(i+1) % pPoly->m_nVerts].m_Vec, verts[1].m_Vec);

			if(m_pView->ClipLineToFrustum(verts))
			{
				m_pViewDef->ProjectPt(verts[0].m_Vec, verts[0].m_Vec);
				m_pViewDef->ProjectPt(verts[1].m_Vec, verts[1].m_Vec);

				SetupVert(&verts[0], verts[0].m_Vec.x + xOffsetFloat, verts[0].m_Vec.y + yOffsetFloat, verts[0].m_Vec.z, lineColor);
				SetupVert(&verts[1], verts[1].m_Vec.x + xOffsetFloat, verts[1].m_Vec.y + yOffsetFloat, verts[1].m_Vec.z, lineColor);

				DrawLine(verts);
			}
		}
	}
}


// 0x41b8a5
BOOL DrawBase::ClipPolyToPlane(LTPlane *pPlane, TLVertex *pPoints, WORD &pointStart, WORD &pointEnd)
{
	uint32	i, nInputPoints, nOutside;
	uint32	iStart, iEnd;
	CReal	pointDots[100], *pPointDot;
	TLVertex	*pInput, *pOutput, *pCurPoint;
	BOOL	inCodes[100];
	BOOL	*pCurInCode;
	CReal	t;

	// Find out where all the points are.
	nOutside = pointStart;
	pCurInCode = inCodes;
	pPointDot = pointDots;
	pCurPoint = pPoints + pointStart;
	for( i=pointStart; i < pointEnd; i++ )
	{
		*pPointDot = VEC_DOT(pCurPoint->m_Vec, pPlane->m_Normal) - pPlane->m_Dist;
		
		if( *pPointDot > 0.0f )
		{
			*pCurInCode++ = TRUE;
		}
		else
		{
			*pCurInCode++ = FALSE;
			nOutside++;
		}
	
		++pPointDot;
		++pCurPoint;
	}

	// Trivial accept or reject.
	if( nOutside == pointEnd )
		return FALSE;
	else if( nOutside == pointStart )
		return TRUE;

	// Clip...
	pInput	= &pPoints[pointStart];
	pOutput = &pPoints[pointEnd];
	
	nInputPoints = pointEnd - pointStart;
	iStart = nInputPoints - 1;

	for( iEnd=0; iEnd < nInputPoints; iEnd++ )
	{
		if( inCodes[iStart] )
			*pOutput++ = pInput[iStart];

		if( inCodes[iStart] != inCodes[iEnd] )
		{
			TLVertex &v1 = pInput[iStart];	
			TLVertex &v2 = pInput[iEnd];	
			
			t = -pointDots[iStart] / (pointDots[iEnd] - pointDots[iStart]);

			pOutput->m_Vec.x = v1.m_Vec.x + ((v2.m_Vec.x - v1.m_Vec.x) * t);
			pOutput->m_Vec.y = v1.m_Vec.y + ((v2.m_Vec.y - v1.m_Vec.y) * t);
			pOutput->m_Vec.z = v1.m_Vec.z + ((v2.m_Vec.z - v1.m_Vec.z) * t);

			TLVertex::ClipExtra(&v1, &v2, pOutput, t);	
			pOutput++;
		}
		
		iStart = iEnd;
	}

	pointStart	= pointEnd;
	pointEnd	= pOutput - pPoints;

	return TRUE;
}


// 0x41bb8d
uint16 DrawBase::InitPointToPointMap(uint16 nPoints)
{
	DWORD nEntries = nPoints * nPoints;

	if(m_PointToPoint.GetSize() < nEntries)
		m_PointToPoint.SetSize(nEntries);

	// Clear the map when the cookie wraps.
	++m_nCookie;
	if(m_nCookie == 0)
	{
		m_nCookie = 1;
		memset(m_PointToPoint.GetArray(), 0, m_PointToPoint.GetSize() * sizeof(uint16));
	}

	m_nPointsInMap = nPoints;
	return m_nCookie;
}


// 0x41bbe2
void DrawBase::GetBoxPoints(const LTVector &min, const LTVector &max, LTVector *pPoints)
{
	pPoints[0].Init( max.x, max.y, max.z );
	pPoints[1].Init( max.x, max.y, min.z );
	pPoints[2].Init( min.x, max.y, min.z );
	pPoints[3].Init( min.x, max.y, max.z );

	pPoints[4].Init( min.x, min.y, min.z );
	pPoints[5].Init( min.x, min.y, max.z );
	pPoints[6].Init( max.x, min.y, max.z );
	pPoints[7].Init( max.x, min.y, min.z );
}


// 0x41bd09
void DrawBase::DrawFlatPoly(CBasePoly *pPoly, COLORREF color)
{
	TLVertex points[128];
	DWORD i;

	for(i=0; i < pPoly->NumVerts(); i++)
	{
		points[i].m_Vec = pPoly->Pt(i);
	}

	DrawFlatPoly2(points, pPoly->NumVerts(), &pPoly->Normal(), color);
}


// 0x41bd77
void DrawBase::DrawBrushes()
{
	CEditRegion *pRegion;
	LPOS pos;
	CEditBrush *pBrush;
	CPreMainWorld *pWorld;
	DWORD i, color, defaultColor;

	pRegion = m_pView->GetRegion();

	if(m_pView->GetShadeMode() == 1 && m_pView->IsPerspectiveViewType())
	{
		// Flat shaded: each poly a colour of its own.
		for(pos=pRegion->m_Brushes.GetHeadPosition(); pos; )
		{
			pBrush = pRegion->m_Brushes.GetNext(pos);

			if(pBrush->m_Unknown_ac && !(pBrush->GetFlags() & NODEFLAG_HIDDEN))
			{
				for(i=0; i < pBrush->m_Polies; i++)
					DrawFlatPoly(pBrush->m_Polies[i], (COLORREF)pBrush->m_Polies[i] * 0x9aeb0);
			}
		}
	}
	else if(m_pView->IsPerspectiveViewType())
	{
		if(m_pView->GetShadeMode() == 2)
		{
			for(pos=pRegion->m_Brushes.GetHeadPosition(); pos; )
			{
				pBrush = pRegion->m_Brushes.GetNext(pos);

				if(pBrush->m_Unknown_ac && !(pBrush->GetFlags() & NODEFLAG_HIDDEN))
				{
					for(i=0; i < pBrush->m_Polies; i++)
						DrawTexturedPoly(pBrush->m_Polies[i]);
				}
			}
		}

		pWorld = m_pView->GetRegionDoc()->m_pLightWorld;
		if(pWorld)
			DrawWorld(pWorld, m_pView->GetShadeMode() != 3);
	}

	DrawDrawingPoly();

	if(m_pView->IsShowWireframe() || m_pView->GetShadeMode() == 0 ||
		m_pView->IsParallelViewType())
	{
		defaultColor = GetDisplayColorD3D(COptionsDisplay::kColorBrushDrawLines);

		for(pos=m_pView->GetRegion()->m_Brushes.GetHeadPosition(); pos; )
		{
			pBrush = m_pView->GetRegion()->m_Brushes.GetNext(pos);

			if(!(pBrush->GetFlags() & (NODEFLAG_SELECTED|NODEFLAG_HIDDEN)))
			{
				InitPointToPointMap((uint16)pBrush->m_Points.GetSize());

				if(pBrush->r == 0xFF && pBrush->g == 0xFF && pBrush->b == 0xFF)
					color = defaultColor;
				else
					color = D3DRGB_255(pBrush->r, pBrush->g, pBrush->b);

				for(i=0; i < pBrush->m_Polies; i++)
					DrawPolyLines(pBrush->m_Polies[i], 0, 0, color);
			}
		}
	}
}


// 0x41bfc7
void DrawBase::DrawBrushVerts()
{
	LPOS pos;
	CEditBrush *pBrush;
	DWORD i;
	BOOL bSelectedOnly;

	if(m_pView->IsShowWireframe() || m_pView->GetShadeMode() == 0 ||
		m_pView->IsParallelViewType())
	{
		bSelectedOnly = FALSE;
		if(GetApp()->GetOptions().GetDisplayOptions()->GetVertexDrawRule() == COptionsDisplay::kVertexDrawSelectedOnly)
			bSelectedOnly = TRUE;

		for(pos=m_pView->GetRegion()->m_Brushes.GetHeadPosition(); pos; )
		{
			pBrush = m_pView->GetRegion()->m_Brushes.GetNext(pos);

			if(!(pBrush->GetFlags() & NODEFLAG_HIDDEN) && 
				(!bSelectedOnly || (pBrush->GetFlags() & NODEFLAG_SELECTED)))
			{
				for(i=0; i < pBrush->m_Polies; i++)
					DrawPolyVerts(pBrush->m_Polies[i], GetDisplayColorD3D(COptionsDisplay::kColorVertexOutline), GetDisplayColorD3D(COptionsDisplay::kColorVertexFill));
			}
		}
	}
}


// 0x41c0aa
void DrawBase::DrawPolyVerts(CEditPoly *pPoly, DWORD lineColor, DWORD fillColor)
{
	WORD i;

	for(i=0; i < pPoly->NumVerts(); i++)
	{
		if(pPoly->Pt(i).m_bInFrustum)
			DrawVert(pPoly->Pt(i).m_Projected, lineColor, fillColor, GetApp()->GetOptions().GetDisplayOptions()->GetVertexSize());
	}
}


// 0x41c118
void DrawBase::DrawDrawingPoly()
{
	WORD i;
	CEditBrush *pBrush;
	CEditVert *pVert;
	BOOL bOld;

	bOld = ZEnable(FALSE);

	if( m_pView->DrawingBrush().m_Points > 0 )
	{
		// Draw the lines.
		pBrush = &m_pView->DrawingBrush();

		for(i=0; i < pBrush->m_Points-1; i++)
		{
			TransformAndDrawLine(pBrush->m_Points[i], pBrush->m_Points[i+1], GetDisplayColorD3D(COptionsDisplay::kColorBrushDrawLines));
		}

		// Draw the points.
		pVert = pBrush->m_Points.GetArray();
		for( i=0; i < pBrush->m_Points; i++ )
		{			
			TransformAndDrawVert(*pVert, GetDisplayColorD3D(COptionsDisplay::kColorVertexOutline), GetDisplayColorD3D(COptionsDisplay::kColorVertexFill), GetApp()->GetOptions().GetDisplayOptions()->GetVertexSize());		
			++pVert;
		}
	}

	ZEnable(bOld);
}


static COLORREF SetPolyVerts(TLVertex *pVerts, 
	CVector &v1, CVector &v2, CVector &v3, CVector &v4, CVector &normal, CVector &baseColor);


// 0x41c1ec
void DrawBase::InternalDrawBox(CVector *pPoints, BOOL bFilled, COLORREF color)
{
	CVector normal, vColor;
	TLVertex verts[4];
	COLORREF drawColor;
	DWORD i;

	vColor.Init(GetRValue(color), GetGValue(color), GetBValue(color));

	if(bFilled && m_pView->IsPerspectiveViewType())
	{
		// Draw the 6 polies.
		drawColor = SetPolyVerts(verts, pPoints[0], pPoints[1], pPoints[5], pPoints[4], normal, vColor);
		DrawFlatPoly2(verts, 4, &normal, drawColor);
		drawColor = SetPolyVerts(verts, pPoints[7], pPoints[6], pPoints[2], pPoints[3], normal, vColor);
		DrawFlatPoly2(verts, 4, &normal, drawColor);
		drawColor = SetPolyVerts(verts, pPoints[0], pPoints[4], pPoints[7], pPoints[3], normal, vColor);
		DrawFlatPoly2(verts, 4, &normal, drawColor);
		drawColor = SetPolyVerts(verts, pPoints[5], pPoints[1], pPoints[2], pPoints[6], normal, vColor);
		DrawFlatPoly2(verts, 4, &normal, drawColor);
		drawColor = SetPolyVerts(verts, pPoints[1], pPoints[0], pPoints[3], pPoints[2], normal, vColor);
		DrawFlatPoly2(verts, 4, &normal, drawColor);
		drawColor = SetPolyVerts(verts, pPoints[4], pPoints[5], pPoints[6], pPoints[7], normal, vColor);
		DrawFlatPoly2(verts, 4, &normal, drawColor);
	}
	else
	{
		for( i=0; i < 4; i++ )
			TransformAndDrawLine(pPoints[i], pPoints[(i==3)?0:(i+1)], color);
		
		for( i=4; i < 8; i++ )
			TransformAndDrawLine(pPoints[i], pPoints[(i==7)?4:(i+1)], color);

		for( i=0; i < 4; i++ )
			TransformAndDrawLine(pPoints[i], pPoints[i+4], color);
	}
}


// 0x41c449 (static: no this), with the static light direction's atexit
// stub at 0x41c5ba.
static COLORREF SetPolyVerts(TLVertex *pVerts, 
	CVector &v1, CVector &v2, CVector &v3, CVector &v4, CVector &normal, CVector &baseColor)
{
	static CVector lightDir(0.371390f, 0.557080f, 0.742780f);
	float dot;
	
	pVerts[0].m_Vec = v1;
	pVerts[1].m_Vec = v2;
	pVerts[2].m_Vec = v3;
	pVerts[3].m_Vec = v4;

	normal = (v3-v1).Cross(v2-v1);
	normal.Norm();

	dot = (normal.Dot(lightDir) + 1.0f) * 0.5f;
	return D3DRGB_255((DWORD)(baseColor.x * dot), (DWORD)(baseColor.y * dot), (DWORD)(baseColor.z * dot));
}




// Used while drawing paths.
class PathInfo
{
public:
	CBaseEditObj	*m_pObj;
	DVector			m_ObjPos;
	DVector			m_PrevTangent;
	DVector			m_NextTangent;
};



void DrawBase::DrawRotatedBox(CVector *pPos, CVector *pDims, CVector &right, CVector &up, CVector &forward, BOOL bFilled, COLORREF color)
{
	CVector points[8];
	DWORD i;

	points[0] = *pPos - right*pDims->x + up*pDims->y + forward*pDims->z;
	points[1] = *pPos + right*pDims->x + up*pDims->y + forward*pDims->z;
	points[2] = *pPos + right*pDims->x - up*pDims->y + forward*pDims->z;
	points[3] = *pPos - right*pDims->x - up*pDims->y + forward*pDims->z;

	points[4] = *pPos - right*pDims->x + up*pDims->y - forward*pDims->z;
	points[5] = *pPos + right*pDims->x + up*pDims->y - forward*pDims->z;
	points[6] = *pPos + right*pDims->x - up*pDims->y - forward*pDims->z;
	points[7] = *pPos - right*pDims->x - up*pDims->y - forward*pDims->z;

	InternalDrawBox(points, bFilled, color);
}


void DrawBase::DrawBox(CVector *pPos, CVector *pDims, BOOL bFilled, COLORREF color)
{
	CVector points[8];

	points[0].Init( pPos->x - pDims->x, pPos->y + pDims->y, pPos->z + pDims->z );
	points[1].Init( pPos->x + pDims->x, pPos->y + pDims->y, pPos->z + pDims->z );
	points[2].Init( pPos->x + pDims->x, pPos->y - pDims->y, pPos->z + pDims->z );
	points[3].Init( pPos->x - pDims->x, pPos->y - pDims->y, pPos->z + pDims->z );

	points[4].Init( pPos->x - pDims->x, pPos->y + pDims->y, pPos->z - pDims->z );
	points[5].Init( pPos->x + pDims->x, pPos->y + pDims->y, pPos->z - pDims->z );
	points[6].Init( pPos->x + pDims->x, pPos->y - pDims->y, pPos->z - pDims->z );
	points[7].Init( pPos->x - pDims->x, pPos->y - pDims->y, pPos->z - pDims->z );

	InternalDrawBox(points, bFilled, color);
}


void DrawBase::DrawCircle(CVector *pCenter, CReal fRadius, UINT nLineSegs, CVector *pForward, CVector *pUp, DWORD color)
{
	CVector vPoints[32];
	DWORD i;
	CMatrix mRot;

	if( nLineSegs > 32 )
		nLineSegs = 32;
	else if( nLineSegs < 8 )
		nLineSegs = 8;

	vPoints[0] = *pUp * fRadius;
	gr_SetupRotationAroundVector(&mRot, *pForward, MATH_CIRCLE / nLineSegs);

	for( i=1; i < nLineSegs; i++ )
	{
		vPoints[i] = vPoints[i-1];
		mRot.Apply( vPoints[i] );
	}

	vPoints[0] += *pCenter;
	for( i = 1; i < nLineSegs; i++ )
	{
		vPoints[i] += *pCenter;
		TransformAndDrawLine(vPoints[i-1], vPoints[(i==nLineSegs-1)?0:(i)], color);
	}
}


// 0x41cef7: the orientation comes as euler angles (gr_GetEulerVectors,
// 0x42c64a), where Build 69 takes a matrix.
void DrawBase::DrawFieldOfView(DVector *pPos, LTVector vAngles,
	float fov, float length, DWORD color)
{
	DVector right, up, forward;
	DVector centerPos, curPos, prevPos;
	DWORD i, nPoints;
	float angle, circleRadius;


	// Convert to radians..
	fov = (fov * MATH_CIRCLE) / 360.0f;

	gr_GetEulerVectors(vAngles, right, up, forward);

	circleRadius = (float)(sin(fov * 0.5f) / cos(fov * 0.5f)) * length;
	centerPos = *pPos + forward * length;
	prevPos = centerPos + right * circleRadius;

	nPoints = 6;
	for(i=1; i <= nPoints; i++)
	{
		angle = ((float)i / nPoints) * MATH_CIRCLE;
		curPos = centerPos + right*((float)cos(angle)*circleRadius) + up*((float)sin(angle)*circleRadius);

		TransformAndDrawLine(prevPos, curPos, color);
		TransformAndDrawLine(*pPos, curPos, color);

		prevPos = curPos;
	}
}


void DrawBase::DrawArrowLine(DVector *pSrc, DVector *pDest, DWORD color)
{
	DVector dir, vecs[3], up, right, headBase;
	DVector points[4];
	float headSize;
	DWORD i, iPrev;

	dir = *pDest - *pSrc;
	if(dir.Mag() < 0.01f)
		return;

	gr_BuildFrameOfReference(&dir, NULL, &vecs[0], &vecs[1], &vecs[2]);

	TransformAndDrawLine(*pSrc, *pDest, color);

	// Draw the head.
	headSize = 16.0f;
	up = vecs[1] * headSize;
	right = vecs[0] * headSize;
	headBase = *pDest - vecs[2] * headSize;

	points[0] = headBase - right - up;
	points[1] = headBase + right - up;
	points[2] = headBase + right + up;
	points[3] = headBase - right + up;

	iPrev = 3;
	for(i=0; i < 4; i++)
	{
		TransformAndDrawLine(*pDest, points[i], color);
		TransformAndDrawLine(points[iPrev], points[i], color);
		iPrev = i;
	}
}


// 0x41d3d8: one test for the source and the name (0x41d3e8..0x41d3f7), no
// self-test assert.
void DrawBase::DrawLineToObjects(CBaseEditObj *pSrcObject, char *pDestName, DWORD color)
{
	CBaseEditObj *objects[MAX_OBJECTLINK_OBJECTS];
	DWORD i, nObjects;
	CVector vObject;

	if(pSrcObject && pDestName && pDestName[0])
	{
		vObject = pSrcObject->GetPos();

		nObjects = m_pView->m_pRegion->FindObjectsByName(pDestName, objects, MAX_OBJECTLINK_OBJECTS);
		for(i=0; i < nObjects; i++)
		{
			// Make sure that we don't try to draw a line to ourself
			if(objects[i] != pSrcObject)
				DrawArrowLine(&vObject, &objects[i]->GetPos(), color);
		}
	}
}


// 0x41d454..0x41d967 (the function table's 0x41d4f1 row is inside it).
void DrawBase::DrawObject(CBaseEditObj *pObj)
{
	COLORREF	color, dwDimsColor;
	CVector		*pPos;
	CBaseProp	*pProp, *pFOVProp;
	BOOL		bColorFound;
	float		fovLength, fRadius;
	DWORD		i;
	int			nDim;
	LTMatrix	mObjTrans;
	CVector		up, right, forward;
	CVector		vMaxDims;
	CVector		*pDim;
	char		*pString;
	LTVector	*pVec;


	// Initialize the dims if needed
	if(pObj->ShouldSearchForDims())
		pObj->InitDims();

	// Only draw the object if it is in the view
	if(!pObj->m_Vert.m_bInFrustum && !pObj->IsFlagSet(NODEFLAG_SELECTED))
		return;

	pPos = &pObj->GetPos();

	if(pObj->IsFlagSet(NODEFLAG_SELECTED))
		color = GetDisplayColorD3D(COptionsDisplay::kColorSelectedObject);
	else
		color = GetDisplayColorD3D(COptionsDisplay::kColorObject);

	fovLength	= 300.0f;
	pFOVProp	= NULL;
	bColorFound	= FALSE;

	// Search thru the props for color, dims and radius...
	for(i=0; i < pObj->m_PropList.m_Props.GetSize(); i++)
	{
		pProp = pObj->m_PropList.m_Props[i];
		if(!pProp)
			continue;

		// Handle color...
		// There can be only one...
		if(pProp->m_Type == PT_COLOR && !bColorFound)
		{
			bColorFound = TRUE;

			pVec = &((CColorProp *)pProp)->m_Vector;
			color = D3DRGB_255((UINT)pVec->x, (UINT)pVec->y, (UINT)pVec->z);
		}
		// Handle the PF_RADIUS properties...
		// There can be many...
		else if(pProp->m_PropFlags & PF_RADIUS)
		{
			// Only show the radius if object selected...
			if(pObj->IsFlagSet(NODEFLAG_SELECTED) && (pProp->m_Type == PT_STRING || pProp->m_Type == PT_REAL))
			{
				fRadius = 0.0f;

				// Get the radius from the string
				if(pProp->m_Type == PT_STRING)
				{
					pString = ((CStringProp *)pProp)->m_String;
					if(pString && strlen(pString) > 0)
					{
						fRadius = (CReal)atof(pString);
					}
				}
				else if(pProp->m_Type == PT_REAL)
				{
					fRadius = ((CRealProp *)pProp)->m_Value;
				}

				DWORD color=GetDisplayColorD3D(COptionsDisplay::kColorObjectRadius);
				DrawCircle(pPos, fRadius, 32, &m_pView->m_pViewDef->m_Nav.Forward(), &m_pView->m_pViewDef->m_Nav.Up(), color);
			}
		}
		else if(pProp->m_PropFlags & PF_FIELDOFVIEW && pProp->m_Type == PT_REAL)
		{
			pFOVProp = pProp;
		}
		else if(pProp->m_PropFlags & PF_FOVRADIUS)
		{
			// Store this in case it has a FOV.
			fovLength = ((CRealProp*)pProp)->m_Value;
		}
		else if(pObj->IsFlagSet(NODEFLAG_SELECTED) && pProp->m_PropFlags & PF_OBJECTLINK &&
			pProp->m_Type == PT_STRING && ((CStringProp*)pProp)->m_String[0])
		{
			// Draw a line to the object.
			DrawLineToObjects(pObj, ((CStringProp*)pProp)->m_String, GetDisplayColorD3D(COptionsDisplay::kColorObjectLink));
		}
	}


	// Draw the FOV if there is one..
	if(pFOVProp && pObj->IsFlagSet(NODEFLAG_SELECTED))
	{
		DWORD color = GetDisplayColorD3D(COptionsDisplay::kColorObjectFOV);
		DrawFieldOfView(&pObj->GetPos(), pObj->GetOr(),
			((CRealProp*)pFOVProp)->m_Value, fovLength, color);
	}

	gr_SetupMatrixEuler(pObj->GetOr(), mObjTrans.m);
	mObjTrans.GetBasisVectors(&right, &up, &forward);

	// Get the color to draw the dims in
	if(pObj->IsFlagSet(NODEFLAG_SELECTED))
		dwDimsColor = GetDisplayColorD3D(COptionsDisplay::kColorObjectDims);
	else
		dwDimsColor = color;

	// Store the max dims here
	vMaxDims.Init(6.0f, 6.0f, 6.0f);

	if(pObj->GetNumDims() > 0)
	{
		// Draw each dims box
		for(nDim=0; nDim < pObj->GetNumDims(); nDim++)
		{
			// Get the dim
			pDim = pObj->GetDim(nDim);

			// Check the dims flag for this dim
			if(pObj->GetDimsFlags(nDim) & PF_LOCALDIMS)
			{
				DrawRotatedBox(pPos, pDim, right, up, forward, TRUE, dwDimsColor);
			}
			else
			{
				DrawBox(pPos, pDim, TRUE, dwDimsColor);
			}

			// Enlarge the max dims box
			// talon: retail bug: the test keeps the SMALLER of each
			// dimension (0x41d6ea..0x41d76c), as Build 69's does.
			if(fabs(vMaxDims.x) > fabs(pDim->x))
			{
				vMaxDims.x = pDim->x;
			}
			if(fabs(vMaxDims.y) > fabs(pDim->y))
			{
				vMaxDims.y = pDim->y;
			}
			if(fabs(vMaxDims.z) > fabs(pDim->z))
			{
				vMaxDims.z = pDim->z;
			}
		}

		// If there is more than on DIM, then draw the boxes again with an outline only
		if(pObj->GetNumDims() > 1 && pObj->IsFlagSet(NODEFLAG_SELECTED))
		{
			for(nDim=0; nDim < pObj->GetNumDims(); nDim++)
			{
				// Get the dim
				pDim = pObj->GetDim(nDim);

				// Check the dims flag for this dim
				if(pObj->GetDimsFlags(nDim) & PF_LOCALDIMS)
				{
					DrawRotatedBox(pPos, pDim, right, up, forward, FALSE, dwDimsColor);
				}
				else
				{
					DrawBox(pPos, pDim, FALSE, dwDimsColor);
				}
			}
		}
	}
	else
	{
		// Draw the default dims
		DrawBox(pPos, &vMaxDims, TRUE, dwDimsColor);
	}

	// Draw the orientation arrows.
	TransformAndDrawLine2(*pPos, *pPos + up*(5.0f + vMaxDims.y), 0xFF00FF00);
	TransformAndDrawLine2(*pPos, *pPos + right*(5.0f + vMaxDims.x), 0xFFFF0000);
	TransformAndDrawLine2(*pPos, *pPos + forward*(5.0f + vMaxDims.z), 0xFF0000FF);
}


// 0x41d9a8
void DrawBase::DrawObjects()
{
	DWORD i;

	if(m_pView->IsShowObjects())
	{
		for(i=0; i < m_pView->m_pRegion->m_Objects.GetSize(); i++)
		{
			if(!m_pView->m_pRegion->m_Objects[i]->IsFlagSet(NODEFLAG_HIDDEN))
				DrawObject(m_pView->m_pRegion->m_Objects[i]);
		}
	}
}


void DrawBase::DrawPaths()
{
	uint32		i, iPath, nSubDiv, curIndex, count;
	CWorldNode	*pNode;
	CWorldNode	*pChild;
	CVector		prevPos, curPos;
	GPOS		pos;
	char		*pName;
	CVectorProp *pProp;
	float		t, mul;
	BOOL		bBezier;
	PathInfo	tempPath;
	uint32		dwColor;
	uint32		dwColorComp[3];
	float		endDiff[3];
	uint32		segColor[3];
	static		CMoArray<PathInfo> sortedList;

	LTVector curveCurPos, curvePrevPos;
	LTVector prevCtrl, curCtrl;
	LTVector objPos, tempCtrl, curTangent, prevTangent;

	for(iPath=0; iPath < m_pView->m_pRegion->m_PathNodes; iPath++)
	{
		pNode = m_pView->m_pRegion->m_PathNodes[iPath];

		if (pNode->IsFlagSet(NODEFLAG_HIDDEN))
			continue;


		// Make sure our list is large enough.
		if(sortedList.GetSize() < pNode->m_Children.GetSize())
		{
			if(!sortedList.SetSize(pNode->m_Children.GetSize()))
				return;
		}

		// Figure out what the tangents are.
		count = 0;
		bBezier = FALSE;
		for(pos=pNode->m_Children; pos; )
		{
			pChild = pNode->m_Children.GetNext(pos);

			if(pChild->GetType() == Node_Object)
			{
				sortedList[count].m_pObj = pChild->AsObject();
				sortedList[count].m_ObjPos = pChild->GetPos();
				sortedList[count].m_PrevTangent.Init();
				sortedList[count].m_NextTangent.Init();

				if(pProp = (CVectorProp*)pChild->m_PropList.GetPropByFlagsAndType(PF_BEZIERPREVTANGENT, PT_VECTOR))
				{
					sortedList[count].m_PrevTangent = pProp->m_Vector;
					bBezier = TRUE;
				}

				if(pProp = (CVectorProp*)pChild->m_PropList.GetPropByFlagsAndType(PF_BEZIERNEXTTANGENT, PT_VECTOR))
				{
					sortedList[count].m_NextTangent = pProp->m_Vector;
					bBezier = TRUE;
				}

				count++;
			}
		}


		// Sort by name.
		if(count < 2)
			continue;

		for(curIndex=0; curIndex < (count-1); curIndex++)
		{
			if(strcmp(sortedList[curIndex].m_pObj->GetName(), sortedList[curIndex+1].m_pObj->GetName()) == 1)
			{
				tempPath = sortedList[curIndex];
				sortedList[curIndex] = sortedList[curIndex+1];
				sortedList[curIndex+1] = tempPath;

				if(curIndex == 0)
					curIndex--;
				else
					curIndex -= 2;
			}
		}

		dwColor = GetDisplayColorD3D(COptionsDisplay::kColorPath);
		dwColorComp[0] = D3DRGB_GETR(dwColor);
		dwColorComp[1] = D3DRGB_GETG(dwColor);
		dwColorComp[2] = D3DRGB_GETB(dwColor);
		endDiff[0] = (float)(0xFF - dwColorComp[0]);
		endDiff[1] = (float)(0xFF - dwColorComp[1]);
		endDiff[2] = (float)(0xFF - dwColorComp[2]);

		for(curIndex=0; curIndex < count; curIndex++)
		{
			if(curIndex == 0)
			{
				prevPos = sortedList[curIndex].m_ObjPos;
				prevTangent = sortedList[curIndex].m_NextTangent;
			}
			else
			{
				curPos = sortedList[curIndex].m_ObjPos;

				if(bBezier)
				{
					curTangent = sortedList[curIndex].m_PrevTangent;

					// Draw from (prevPos,prevTangent) to (curPos,curTangent).
					curvePrevPos = prevPos;
					prevCtrl = prevPos + prevTangent;
					curCtrl = curPos + curTangent;

					nSubDiv = 25;
					mul = 1.0f / nSubDiv;
					for(i=1; i <= nSubDiv; i++)
					{
						t = (float)i * mul;

						segColor[0] = dwColorComp[0] + (DWORD)(endDiff[0] * t);
						segColor[1] = dwColorComp[1] + (DWORD)(endDiff[1] * t);
						segColor[2] = dwColorComp[2] + (DWORD)(endDiff[2] * t);

						Bezier_Evaluate(curveCurPos, prevPos, prevCtrl, curCtrl, curPos, t);
						TransformAndDrawLine(curvePrevPos, curveCurPos, D3DRGB_255(segColor[0], segColor[1], segColor[2]));

						curvePrevPos = curveCurPos;
					}

					prevTangent = sortedList[curIndex].m_NextTangent;
				}
				else
				{
					TransformAndDrawLine(prevPos, curPos, dwColor);
				}

				prevPos = curPos;
			}
		}


		// Draw the tangents (this comes last so they overlap the lines).
		if(bBezier)
		{
			for(i=0; i < count; i++)
			{
				pChild = sortedList[i].m_pObj;

				if(!(pChild->IsFlagSet(NODEFLAG_SELECTED)))
					continue;

				tempCtrl = sortedList[i].m_ObjPos + sortedList[i].m_PrevTangent;
				DrawArrowLine(&sortedList[i].m_ObjPos, &tempCtrl, dwColor);

				tempCtrl = sortedList[i].m_ObjPos + sortedList[i].m_NextTangent;
				DrawArrowLine(&sortedList[i].m_ObjPos, &tempCtrl, D3DRGB_255(0xFF, 0xFF, 0xFF));
			}
		}
	}
}


// 0x41e108: in brush mode the selected brushes' edges; in geometry mode
// those too, then the tagged polies, the immediate edge, the tagged verts
// and the immediate vert.
void DrawBase::DrawSelections()
{
	DWORD i, k;
	CWorldNode *pNode;
	CEditBrush *pBrush;
	CEditPoly *pPoly;
	GenListPos vertPos;
	GenList<CVertRef> *pTaggedVerts;

	if(m_pView->GetEditMode() == BRUSH_EDITMODE)
	{
		for(i=0; i < m_pView->m_pRegion->m_Selections.GetSize(); i++)
		{
			pNode = m_pView->m_pRegion->m_Selections[i];
			if(pNode->GetType() == Node_Brush)
			{
				pBrush = pNode->AsBrush();

				InitPointToPointMap((uint16)pBrush->m_Points.GetSize());
				for(k=0; k < pBrush->m_Polies.GetSize(); k++)
				{
					Unknown_28(pBrush->m_Polies[k]);
					DrawPolyLines(pBrush->m_Polies[k], 1, 1, GetDisplayColorD3D(COptionsDisplay::kColorSelectedBrush));
				}
			}
		}
	}
	else if(m_pView->GetEditMode() == GEOMETRY_EDITMODE)
	{
		for(i=0; i < m_pView->m_pRegion->m_Selections.GetSize(); i++)
		{
			pNode = m_pView->m_pRegion->m_Selections[i];
			if(pNode->GetType() == Node_Brush)
			{
				pBrush = pNode->AsBrush();

				InitPointToPointMap((uint16)pBrush->m_Points.GetSize());
				for(k=0; k < pBrush->m_Polies.GetSize(); k++)
				{
					Unknown_28(pBrush->m_Polies[k]);
					DrawPolyLines(pBrush->m_Polies[k], 1, 1, GetDisplayColorD3D(COptionsDisplay::kColorSelectedBrush));
				}
			}
		}

		// Draw the tagged polygons.
		CPolyRefArray &taggedPolies = m_pView->TaggedPolies();
		for(i=0; i < taggedPolies.GetSize(); i++)
		{
			if(taggedPolies[i].IsValid())
			{
				pPoly = taggedPolies[i]();

				Unknown_28(pPoly);
				InitPointToPointMap((uint16)pPoly->m_pBrush->m_Points.GetSize());
				DrawPolyLines(pPoly, 1, 1, GetDisplayColorD3D(COptionsDisplay::kColorSelectedFace));
				DrawPolyVerts(pPoly, GetDisplayColorD3D(COptionsDisplay::kColorVertexOutline), GetDisplayColorD3D(COptionsDisplay::kColorVertexFill));
			}
		}

		// Draw the immediate edge.
		if(m_pView->IEdge().IsValid())
		{
			TransformAndDrawLine(m_pView->IEdge().Vert1(), m_pView->IEdge().Vert2(), GetDisplayColorD3D(COptionsDisplay::kColorSelectedEdge));
		}

		// Draw tagged verts.
		pTaggedVerts = &m_pView->TaggedVerts();
		for(vertPos=pTaggedVerts->GenBegin(); pTaggedVerts->GenIsValid(vertPos); )
		{
			CEditVert &vert = pTaggedVerts->GenGetNext(vertPos)();

			if(vert.m_bInFrustum)
			{
				TransformAndDrawVert(vert, GetDisplayColorD3D(COptionsDisplay::kColorTaggedVerticeOutline), GetDisplayColorD3D(COptionsDisplay::kColorTaggedVerticeFill),
					GetApp()->GetOptions().GetDisplayOptions()->GetVertexSize());
			}
		}

		// Draw the immediate vert.
		if(m_pView->IVert().IsValid())
		{
			CEditVert &vert = m_pView->IVert()();
			if(vert.m_bInFrustum)
			{
				DrawVert(vert.m_Projected, GetDisplayColorD3D(COptionsDisplay::kColorImmediateVertice),
					GetDisplayColorD3D(COptionsDisplay::kColorImmediateVertice), GetApp()->GetOptions().GetDisplayOptions()->GetVertexSize());
			}
		}
	}
}


void DrawBase::DrawSelectedBrushBox()
{
	CVector &min = m_pView->GetRegionDoc()->m_SelectionMin;
	CVector &max = m_pView->GetRegionDoc()->m_SelectionMax;

	CVector points[8];
	DWORD i, lineColor;
	TLVertex tVerts[8];


	lineColor = GetDisplayColorD3D(COptionsDisplay::kColorBoundingBox);

	// Setup the points.
	GetBoxPoints(min, max, points);
	CMatrix &mTransform = m_pView->m_Transform;
	for( i=0; i < 8; i++ )
	{
		mTransform.Apply(points[i], tVerts[i].m_Vec);
		tVerts[i].color = lineColor;
	}

	// Draw the lines.
	Draw3dLine2(tVerts[0], tVerts[1]);
	Draw3dLine2(tVerts[1], tVerts[2]);
	Draw3dLine2(tVerts[2], tVerts[3]);
	Draw3dLine2(tVerts[3], tVerts[0]);

	Draw3dLine2(tVerts[4], tVerts[5]);
	Draw3dLine2(tVerts[5], tVerts[6]);
	Draw3dLine2(tVerts[6], tVerts[7]);
	Draw3dLine2(tVerts[7], tVerts[4]);

	Draw3dLine2(tVerts[4], tVerts[2]);
	Draw3dLine2(tVerts[7], tVerts[1]);
	Draw3dLine2(tVerts[5], tVerts[3]);
	Draw3dLine2(tVerts[6], tVerts[0]);
}


// 0x41e561: no handles for nothing selected or for one selection that
// isn't a brush (0x41e579..0x41e591).
void DrawBase::DrawHandles()
{
	CEditRegion *pRegion = m_pView->m_pRegion;

	if(pRegion->m_Selections.GetSize() <= 0)
		return;

	if(pRegion->m_Selections.GetSize() == 1 && pRegion->m_Selections[0]->GetType() != Node_Brush)
		return;

	// Draw the move handles.
	DWORD lineColor = GetDisplayColorD3D(COptionsDisplay::kColorHandleOutline);
	DWORD fillColor = GetDisplayColorD3D(COptionsDisplay::kColorHandleFill);

	CVector &boxMiddle = m_pView->m_BoxMiddle;
	CVector &boxHalf = m_pView->m_BoxHalf;

	CVector &min = m_pView->GetRegionDoc()->m_SelectionMin;
	CVector &max = m_pView->GetRegionDoc()->m_SelectionMax;

	boxMiddle = min + (max - min) * 0.5f;
	boxHalf = (max - min) * 0.5f;

	// X plane
	m_pView->m_BoxHandles[0].Init(boxMiddle.x+boxHalf.x, boxMiddle.y,           boxMiddle.z );
	m_pView->m_BoxHandles[1].Init(boxMiddle.x-boxHalf.x, boxMiddle.y,           boxMiddle.z );

	// Y plane
	m_pView->m_BoxHandles[2].Init(boxMiddle.x,           boxMiddle.y+boxHalf.y, boxMiddle.z );
	m_pView->m_BoxHandles[3].Init(boxMiddle.x,           boxMiddle.y-boxHalf.y, boxMiddle.z );

	// Z plane
	m_pView->m_BoxHandles[4].Init(boxMiddle.x,           boxMiddle.y,           boxMiddle.z+boxHalf.z );
	m_pView->m_BoxHandles[5].Init(boxMiddle.x,           boxMiddle.y,           boxMiddle.z-boxHalf.z );

	// XY handles
	m_pView->m_BoxHandles[6].Init(boxMiddle.x+boxHalf.x, boxMiddle.y+boxHalf.y, boxMiddle.z );
	m_pView->m_BoxHandles[7].Init(boxMiddle.x-boxHalf.x, boxMiddle.y+boxHalf.y, boxMiddle.z );
	m_pView->m_BoxHandles[8].Init(boxMiddle.x+boxHalf.x, boxMiddle.y-boxHalf.y, boxMiddle.z );
	m_pView->m_BoxHandles[9].Init(boxMiddle.x-boxHalf.x, boxMiddle.y-boxHalf.y, boxMiddle.z );

	// XZ handles
	m_pView->m_BoxHandles[10].Init(boxMiddle.x+boxHalf.x, boxMiddle.y, boxMiddle.z+boxHalf.z );
	m_pView->m_BoxHandles[11].Init(boxMiddle.x-boxHalf.x, boxMiddle.y, boxMiddle.z+boxHalf.z );
	m_pView->m_BoxHandles[12].Init(boxMiddle.x+boxHalf.x, boxMiddle.y, boxMiddle.z-boxHalf.z);
	m_pView->m_BoxHandles[13].Init(boxMiddle.x-boxHalf.x, boxMiddle.y, boxMiddle.z-boxHalf.z);

	// YZ handles
	m_pView->m_BoxHandles[14].Init(boxMiddle.x, boxMiddle.y+boxHalf.y, boxMiddle.z+boxHalf.z );
	m_pView->m_BoxHandles[15].Init(boxMiddle.x, boxMiddle.y-boxHalf.y, boxMiddle.z+boxHalf.z );
	m_pView->m_BoxHandles[16].Init(boxMiddle.x, boxMiddle.y+boxHalf.y, boxMiddle.z-boxHalf.z);
	m_pView->m_BoxHandles[17].Init(boxMiddle.x, boxMiddle.y-boxHalf.y, boxMiddle.z-boxHalf.z);

	int i;
	for( i=0; i < NUM_BOX_HANDLES; i++ )
	{
		m_pView->m_HandlesInFrustum[i] =
			m_pView->TransformAndProjectInFrustum(m_pView->m_BoxHandles[i], m_pView->m_HandlePos[i]);

		if(m_pView->m_HandlesInFrustum[i] )
		{
			TransformAndDrawVert(m_pView->m_BoxHandles[i], lineColor, fillColor, GetApp()->GetOptions().GetDisplayOptions()->GetHandleSize());
		}
	}
}


// 0x41ea57: segment i goes from point i to its 80% mark in one colour and
// on to point i+1 in the complement; every point gets a vertex of size 2.
void DrawBase::DrawUnknown_b4()
{
	CEditRegion *pRegion;
	DWORD i, lineColor, vertColor;
	CVector vMid, *pCur, *pNext;

	pRegion = m_pView->m_pRegion;
	if(pRegion)
	{
		lineColor = GetDisplayColorD3D(COptionsDisplay::kColorLeakFinder);
		vertColor = GetDisplayColorD3D(COptionsDisplay::kColorVertexFill);

		for(i=0; i < pRegion->m_Unknown_b4.GetSize(); i++)
		{
			if(i >= pRegion->m_Unknown_b4.GetSize()-1)
				break;

			pCur = &pRegion->m_Unknown_b4[i];
			pNext = &pRegion->m_Unknown_b4[i+1];
			vMid = *pCur + (*pNext - *pCur) * 0.8f;

			TransformAndDrawLine(*pCur, vMid, lineColor);
			TransformAndDrawLine(vMid, *pNext, ~lineColor);
		}

		for(i=0; i < pRegion->m_Unknown_b4.GetSize(); i++)
		{
			TransformAndDrawVert(pRegion->m_Unknown_b4[i], vertColor, vertColor, 2);
		}
	}
}


void DrawBase::DrawOrigin()
{
	TLVertex	verts[2];
	LTVector	originPos;
	DWORD		nColor;

	originPos = m_pView->m_pRegion->m_vMarker;
	nColor = GetDisplayColorD3D(COptionsDisplay::kColorOrigin);

	verts[0].color = verts[1].color = nColor;

	verts[0].m_Vec.Init(originPos.x - ORIGIN_LINE_SIZE, originPos.y, originPos.z);
	verts[1].m_Vec.Init(originPos.x + ORIGIN_LINE_SIZE, originPos.y, originPos.z);
	TransformAndDraw3dLine(verts);

	verts[0].m_Vec.Init(originPos.x, originPos.y - ORIGIN_LINE_SIZE, originPos.z);
	verts[1].m_Vec.Init(originPos.x, originPos.y + ORIGIN_LINE_SIZE, originPos.z);
	TransformAndDraw3dLine(verts);

	verts[0].m_Vec.Init(originPos.x, originPos.y, originPos.z - ORIGIN_LINE_SIZE);
	verts[1].m_Vec.Init(originPos.x, originPos.y, originPos.z + ORIGIN_LINE_SIZE);
	TransformAndDraw3dLine(verts);
}


void DrawBase::DrawMarker()
{
	TLVertex	verts[2];
	CReal		fLength, widthScale, heightScale;
	LTVector	center;
	DWORD		nColor = GetDisplayColorD3D(COptionsDisplay::kColorMarker);
	int i;

	verts[0].color = verts[1].color = nColor;

	if(m_pView->IsParallelViewType())
	{
		// width and height of view ajusted for zoom factor
		widthScale = m_pView->m_ViewDefInfo.m_fProjectHalfWidth / m_pView->m_pViewDef->m_Magnify;
		heightScale = m_pView->m_ViewDefInfo.m_fProjectHalfHeight / m_pView->m_pViewDef->m_Magnify;
		fLength = ( widthScale >= heightScale ) ? widthScale : heightScale;
		center = m_pView->m_pViewDef->m_Nav.Pos();
	}
	else
	{	fLength = m_pView->m_pViewDef->m_Grid.m_DrawSize / 2.0f;
		center = m_pView->m_pRegion->m_vMarker;
	}

	for ( i = 0; i < 3; i++ )
	{
		verts[0].m_Vec = verts[1].m_Vec = m_pView->m_pRegion->m_vMarker;

		verts[0].m_Vec[i] = verts[1].m_Vec[i] = center[i];
		verts[0].m_Vec[i] -= fLength;
		verts[1].m_Vec[i] += fLength;

		TransformAndDraw3dLine(verts);
	}
}


// 0x41eda4
BOOL DrawBase::InitFrame()
{
	CBaseEditObj *pObj;
	CEditBrush *pBrush;
	DWORD i;
	LPOS pos;
	CEditRegion *pRegion;

	pRegion = m_pView->m_pRegion;
	if(!m_pView || !pRegion)
		return FALSE;

	m_pViewDef = m_pView->m_pViewDef;

	// Transform and project the brushes' points.
	for(pos=pRegion->m_Brushes; pos; )
	{
		pBrush = pRegion->m_Brushes.GetNext(pos);

		// talon: brush+0xb0 (0x41edfa, 0x41ee8d) is m_Unknown_ac (the
		// processor's 0xac): the brush's in-view flag, which the
		// constructor sets to 1.
		pBrush->m_Unknown_ac = FALSE;
		for(i=0; i < pBrush->m_Points.GetSize(); i++)
		{
			CEditVert &vert = pBrush->m_Points[i];

			m_pView->m_Transform.Apply4x4(vert, vert.m_Transformed);
			m_pViewDef->ProjectPt(vert.m_Transformed, vert.m_Projected);
			vert.m_bInFrustum = m_pView->InsideFrustum(vert.m_Transformed);

			if(m_pView->IsParallelViewType() ||
				(vert.m_Transformed.z >= m_pView->m_pViewDef->m_NearZ && vert.m_Transformed.z <= m_pView->m_pViewDef->m_FarZ))
			{
				pBrush->m_Unknown_ac = TRUE;
			}
		}
	}

	// And the objects' positions.
	for(i=0; i < pRegion->m_Objects.GetSize(); i++)
	{
		pObj = pRegion->m_Objects[i];

		m_pView->m_Transform.Apply4x4(pObj->GetPos(), pObj->m_Vert.m_Transformed);
		m_pViewDef->ProjectPt(pObj->m_Vert.m_Transformed, pObj->m_Vert.m_Projected);
		pObj->m_Vert.m_bInFrustum = m_pView->InsideFrustum(pObj->m_Vert.m_Transformed);
	}

	return TRUE;
}
