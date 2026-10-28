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

// talon: 0x42dc00..0x42f580 in the image.

CPrePoly::CPrePoly()
{
	m_PPFlags				= 0;
	m_pReplacement			= NULL;
	m_pOriginalBrushPoly	= NULL;
	m_Unknown_24			= 0;
	m_Unknown_20			= 0;
	m_LMWidth = m_LMHeight	= 0;
}


CPrePoly::~CPrePoly()
{
	Term();
}


void CPrePoly::Term()
{
}


// talon: near, 538 bytes against 541 (0x42dd50): the edge plane's
// m_Dist and the DistTo sums are scheduled differently (shipped keeps the
// normal's z on the stack and sums z, y, x; ours starts with y), the
// instructions otherwise the same. Build 69's source as it is; DistTo
// spelled out, an LTPlane or a bare normal and distance, the references
// reordered or a reference to Pt(i) each make it worse.
void CPrePoly::RemoveCollinearVertices()
{
	CPrePlane edgePlane;
	PReal dot;

	for(uint32 i = 0; i < NumVerts(); i++)
	{
		PVector &prev = PrevPt(i);
		PVector &next = NextPt(i);

		edgePlane.m_Normal = (next - prev).Cross(Normal());
		edgePlane.m_Normal.Norm();
		edgePlane.m_Dist = edgePlane.m_Normal.Dot(prev);

		dot = edgePlane.DistTo(Pt(i));
		if(fabs(dot) < 0.0001f)
		{
			RemoveVert(i);
			--i;
		}
	}
}


BOOL CPrePoly::PostJoinFixup(PReal convexThreshold)
{
	RemoveCollinearVertices();

	if(!IsConvex(this, convexThreshold))
		return FALSE;

	if(NumVerts() < 3)
		return FALSE;

	return TRUE;
}


// talon: near, 802 bytes against 808 (0x42e200): the inline budget. The
// shipped code calls _CVector's constructor (0x4021c0) for all four
// corners' last + and ours expands the first one. Normal() through
// GetPlane(), VEC_MULSCALAR, Vert(0).m_Vec, vCenter set later or declared
// first, and CreatePoly moved down do not change it.
CPrePortal* CPrePoly::MakePortal(PReal fSize)
{
	CPrePortal *pRet;
	PVector right, up, forward, vCenter;

	pRet = CreatePoly(CPrePortal, 4, TRUE);

	BuildFrameOfReference((PVector*)&Normal(), NULL, &right, &up, &forward);
	vCenter = Pt(0);

	VEC_NORM(right);
	VEC_NORM(up);
	right *= fSize;
	up *= fSize;

	pRet->Pt(0) = vCenter + (-right - up);
	pRet->Pt(1) = (right - up) + vCenter;
	pRet->Pt(2) = (up + right) + vCenter;
	pRet->Pt(3) = (-right + up) + vCenter;

	pRet->m_Plane = *GetPlane();
	return pRet;
}


void AdjustForVec(CPrePoly *pPoly, const PVector &dir, const PVector &unused, float lmGridSize)
{
	PReal dot;
	PVector normDir;

	normDir = dir;
	normDir.Norm();

	dot = (PReal)fmod((PReal)normDir.Dot(pPoly->m_PolyO), (PReal)lmGridSize);
	if(dot > 0.0f)
		pPoly->m_PolyO -= normDir * dot;
	else
		pPoly->m_PolyO -= normDir * (dot+lmGridSize);
}


void CPrePoly::FindTextureOrigin(PVector *pPolyO, BOOL bAdjust, float lmGridSize)
{
	PReal dot;
	PVector cur = Pt(0);
	PVector unitP, unitQ;

	unitP = P();
	unitP.Norm();

	unitQ = Q();
	unitQ.Norm();

	// Get the main origin.
	for(uint32 i = 1; i < NumVerts(); i++ )
	{
		dot = unitP.Dot( Pt(i) - cur );
		if(dot < 0)
			cur += unitP * dot;

		dot = unitQ.Dot( Pt(i) - cur );
		if(dot < 0)
			cur += unitQ * dot;
	}

	if(pPolyO)
		*pPolyO = cur;
	else
		m_PolyO = cur;

	// Align it to a LIGHTMAP_GRID_SIZE_I-pixel grid to help avoid edges showing up.
	if(bAdjust)
	{
		AdjustForVec(this, P(), InverseP(), lmGridSize);
		AdjustForVec(this, Q(), InverseQ(), lmGridSize);
	}
}


void CPrePoly::CalculateTextureSize(BOOL bCapSizes,
	float maxLightmapSize, float lmGridSize,
	uint16 &xPixels, uint16 &yPixels)
{
	PReal dot;
	PReal maxX, maxY;
	int bErr;
	uint16 wGridSize;

	maxX = maxY = (PReal)-MAX_CREAL;
	for(uint32 i = 0; i < NumVerts(); i++ )
	{
		dot = m_pSurface->P.Dot( Pt(i) - PolyO() );
		if( dot > maxX )
			maxX = dot;

		dot = m_pSurface->Q.Dot( Pt(i) - PolyO() );
		if( dot > maxY )
			maxY = dot;
	}

	xPixels = (uint16)ceil(maxX);
	yPixels = (uint16)ceil(maxY);

	if(bCapSizes)
	{
		if(xPixels > (lmGridSize*maxLightmapSize))
		{
			bErr = 1;
		}

		if(yPixels > (lmGridSize*maxLightmapSize))
		{
			bErr = 1;
		}
	}

	wGridSize = (uint16)lmGridSize;
	if(xPixels % wGridSize)
		xPixels = xPixels + (wGridSize - (xPixels % wGridSize));

	if(yPixels % wGridSize)
		yPixels = yPixels + (wGridSize - (yPixels % wGridSize));
}


CPrePoly* CPrePoly::Clone() const
{
	CPrePoly *pRet;

	pRet = CreatePoly(CPrePoly, NumVerts(), FALSE);
	if(!pRet)
		return NULL;

	pRet->CopySplitAttributes(this);

	for(uint32 i = 0; i < NumVerts(); i++)
	{
		pRet->AddVert(Pt(i));
		pRet->PtNormal(i) = PtNormal(i);
	}

	return pRet;
}
