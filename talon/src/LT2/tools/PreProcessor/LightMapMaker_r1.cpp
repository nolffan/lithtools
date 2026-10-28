// talon: 0x00413c90..0x00417180, the rows nobody had matched: the box
// portals (LeafPortals.cpp's MakeBoxPortals returns BOOL), the lightmap
// planes' static initializer (lightmap_planes.cpp's g_LMPlanes), and two
// CLightMapMaker members (InitPolyLightmapFill, ThreadLightPoly), rewritten
// from the image, to replace the ones in LeafPortals.cpp, lightmap_planes.cpp
// and LightMapMaker.cpp when they are folded in. (The file name is ours and
// provisional.) The other rows of the range are not functions: 0x413fcc is
// inside 0x413c90, 0x416567 and 0x41656c inside 0x416270, 0x416f66,
// 0x417061 and 0x41706f inside 0x416b50.

#include "bdefs.h"
#include "processing.h"
#include "lightmapmaker.h"
#include "preleaf.h"
#include "lightmap_planes.h"
#include "LightMapMaker_r1_decl.h"


// -------------------------------------------------------------------------------- //
// 0x00413c90 (929 bytes; the rows 0x413fcc and its tail are inside it: 0x413fcc
// cuts the instruction at 0x413fca in half). LeafPortals.cpp's
// MakeBoxPortals, which returns TRUE (0x414027) where ours is void.
// -------------------------------------------------------------------------------- //

// Near, 968 bytes against 929: the inline budget. The shipped code calls
// CPreBasePoly::SetVert (0x414040) for the two loops and the first ten side
// vertices and expands the last six (0x413eae..); ours expands the last
// seven (portals[4]'s second vertex too). Nothing else differs. Building the
// corners as temporaries, an extra CreatePoly wrapper and the three-argument
// SetVert each make it worse or leave it.
BOOL MakeBoxPortals_r1(CMoArray<CPrePortal*> &portals, PVector &vMin, PVector &vMax)
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
		CalcPortalPlane(portals[i]);

	return TRUE;
}




// -------------------------------------------------------------------------------- //
// 0x00415210 (949 bytes): the static initializer of the lightmap planes
// (lightmap_planes.cpp's g_LMPlanes; 0x415200 is its jmp thunk). It matches
// as Build 69 writes it (tools/shared/engine/lightmap_planes.cpp): LMPlane's
// constructor out of line and defined after the table (0x4155d0, which is
// why the image has it after the initializer), so each argument is built in
// a temporary and copied. lightmap_planes.h's constructor is inline, which
// builds them in place (917 bytes). LMPlane_r1 is LMPlane with that
// constructor (the same 0x24 bytes: right, up, normal).
// -------------------------------------------------------------------------------- //

LMPlane_r1 g_LMPlanes_r1[NUM_LM_PLANES] =
{
	LMPlane_r1(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, 0.0f, -1.0f), LTVector(0.0f, 1.0f, 0.0f)),
	LMPlane_r1(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, 0.0f, 1.0f), LTVector(0.0f, -1.0f, 0.0f)),
	LMPlane_r1(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, 1.0f, 0.0f), LTVector(0.0f, 0.0f, 1.0f)),
	LMPlane_r1(LTVector(1.0f, 0.0f, 0.0f), LTVector(0.0f, -1.0f, 0.0f), LTVector(0.0f, 0.0f, -1.0f)),
	LMPlane_r1(LTVector(0.0f, 0.0f, 1.0f), LTVector(0.0f, -1.0f, 0.0f), LTVector(1.0f, 0.0f, 0.0f)),
	LMPlane_r1(LTVector(0.0f, 0.0f, -1.0f), LTVector(0.0f, -1.0f, 0.0f), LTVector(-1.0f, 0.0f, 0.0f))
};


// 0x004155d0.
LMPlane_r1::LMPlane_r1(LTVector vRight, LTVector vUp, LTVector vNormal)
{
	m_Right		= vRight;
	m_Up		= vUp;
	m_Normal	= vNormal;
}


// -------------------------------------------------------------------------------- //
// CLightMapMaker.
// -------------------------------------------------------------------------------- //

// Build 69's ShouldLightmap (a static inline in its LightMapMaker.cpp), with
// the vertices-only test first. The shipped code of both its callers needs
// the helper: written out as one `||` guard (LightMapMaker.cpp's form), VC6
// sinks the return block to the end of the function, where the shipped code
// has it straight after the last test (0x4162c0, 0x416cc8). The names are
// ours.
static inline BOOL ShouldLightmap_r1(CPrePoly *pPoly)
{
	if(g_pGlobs->m_bVerticesOnly)
		return FALSE;

	return (pPoly->GetSurfaceFlags() & SURF_LIGHTMAP) &&
		!(pPoly->GetSurfaceFlags() & SURF_INVISIBLE) &&
		!(pPoly->GetSurfaceFlags() & SURF_SKY);
}


// 0x00416270 (789 bytes; the rows 0x416567 and 0x41656c are inside its
// min/max loop). Near, 785 bytes against 789: register colouring only. The
// shipped code keeps the surface in eax and its flags in ecx after SetSize
// (0x4162ef), ours the other way round, so in the else arm ours copies
// m_AmbientLight through an `add $0xc8,%ebp` where the shipped code reads
// 0xc8(%ebp).. directly (0x416430). Neither a surface local, the accessor
// spellings (GetSurfaceFlags, GetPlane, m_pSurface), nor the order of the
// plane and ambient statements moves it.
BOOL CLightMapMaker::InitPolyLightmapFill(CPrePoly *pPoly, CFullLightMap *pMap)
{
	uint32 width, height, i;
	PVector fillColor;
	CPrePlane *pPlane;

	width = pPoly->m_LMWidth;
	height = pPoly->m_LMHeight;
	if(m_bOverSample)
	{
		width++;
		height++;
	}

	pMap->m_bAmbientOnly = FALSE;
	if(!ShouldLightmap_r1(pPoly))
		return FALSE;

	pMap->SetSize(width, height);

	if(pPoly->GetSurface()->m_Flags & SURF_DIRECTIONALLIGHT)
	{
		pPlane = pPoly->GetSurface()->m_pPlane;
		fillColor = m_AmbientLight;
		fillColor += CalcSunLightSample(PVector(pPlane->m_Normal.x, pPlane->m_Normal.y, pPlane->m_Normal.z));

		if(fillColor.x < 0.0f)
			fillColor.x = 0.0f;
		else if(fillColor.x > 255.0f)
			fillColor.x = 255.0f;

		if(fillColor.y < 0.0f)
			fillColor.y = 0.0f;
		else if(fillColor.y > 255.0f)
			fillColor.y = 255.0f;

		if(fillColor.z < 0.0f)
			fillColor.z = 0.0f;
		else if(fillColor.z > 255.0f)
			fillColor.z = 255.0f;
	}
	else
	{
		fillColor = m_AmbientLight;
		if(pPoly->GetSurface()->m_Flags & SURF_BRIGHT)
			fillColor.Init(255.0f, 255.0f, 255.0f);
		else
			pMap->m_bAmbientOnly = TRUE;
	}

	pMap->ColorFill(fillColor);

	pMap->m_Min.Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	pMap->m_Max.Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);
	for(i=0; i < pPoly->NumVerts(); i++)
	{
		VEC_MIN(pMap->m_Min, pMap->m_Min, pPoly->Pt(i));
		VEC_MAX(pMap->m_Max, pMap->m_Max, pPoly->Pt(i));
	}

	return TRUE;
}


// 0x00416b50 (1370 bytes; the rows 0x416f66, 0x417061 and 0x41706f are
// inside it). Near, 1371 bytes against 1370: the inline budget. The shipped
// code expands the light loop's GenGetNext and GetNext (0x416e13); ours
// leaves GetNext a call, so pos lives in memory rather than edi, and pPoly
// and iPoly swap ebp and edi throughout. Without ShouldLightmap_r1 GetNext
// is expanded but the return block sinks; AddTail for Append expands far
// more than shipped (the positions constructor, 1282 bytes). The light list
// is walked through a GenListPos, as shipped: GenBegin's copy carries its
// uninitialised m_SubIndex (0x416dff, 0x60(%esp) to 0x58(%esp)).
BOOL CLightMapMaker::ThreadLightPoly(uint32 iPoly)
{
	CPrePoly				*pPoly;
	CFullLightMap			*pLightMap, tempMap;
	CPreLightMap			*pPreLightMap;
	CMoArray<PVector>		positions;
	CLinkedList<CLightDef*>	lights;
	CLightDef				*pLight;
	GenListPos				pos;
	uint32					i;


	pPoly = m_pWorld->GetLMPoly(&m_pCurAnim->m_Polies[iPoly]);
	if(!pPoly)
		return FALSE;

	pPoly->m_nName = iPoly;

	if(!LightPolygon(pPoly))
		return FALSE;

	if(!GetPolyVertexColors(pPoly, *m_pCurFrame->m_Unknown_14[iPoly]))
		return FALSE;

	if(!ShouldLightmap_r1(pPoly))
		return TRUE;

	// Make the full light map.
	pLightMap = m_FullLightMaps[iPoly];

	// Make the output lightmap.
	pPreLightMap = m_pCurFrame->m_PolyMaps[iPoly];
	if(!pPreLightMap)
		return FALSE;

	// Figure out how big this poly's lightmaps are.
	GetPolyLightmapSize(pPoly);

	// Setup the lightmap..
	if(!InitPolyLightmapFill(pPoly, pLightMap))
		return FALSE;

	// Make a list of lights that affect this poly.
	for(i=0; i < m_LightDefs; i++)
	{
		pLight = m_LightDefs[i];
		if(LightTouchesPoly(pLight, pPoly))
			lights.Append(pLight);
	}

	if(lights.GetSize() > 0)
	{
		CalcSamplePositions(pPoly, 3, &positions);

		for(pos=lights.GenBegin(); lights.GenIsValid(pos); )
		{
			pLight = lights.GenGetNext(pos);
			if(!ApplyLightToPoly(pLight, pPoly, &positions))
				return FALSE;
		}
	}

	if(m_bStaticSunLight)
	{
		if(lights.GetSize() == 0)
			CalcSamplePositions(pPoly, 3, &positions);

		if(!ApplySunLightToPoly(pPoly, &positions))
			return FALSE;
	}

	if(m_bOverSample)
	{
		DownSample(pLightMap, &tempMap);
		pLightMap = &tempMap;
	}

	ClampMapValues(pLightMap, pLightMap->m_Width, pLightMap->m_Height);
	ConvertMap(pLightMap, pPreLightMap);
	return TRUE;
}
