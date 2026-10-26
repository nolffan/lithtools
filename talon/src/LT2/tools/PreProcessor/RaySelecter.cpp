// talon: a module of its own in the image, 0x004450c0..0x00446170, after
// Processing and ProcessorOptions, written from the image (no source of it
// survives; Build 69 has none of RaySelect). Three parts, in image order:
//
//   0x4450c0              RaySelect: selects the brushes the RaySelecter
//                         objects' rays hit (the name ours);
//   0x445330..0x4458c0    ReplaceTextures, Build 69's replacetextures.cpp
//                         as it was in 2001 (one texture a surface);
//   0x445b80..0x446164    the terrain's surface merging (MakeTerrain calls
//                         it, 0x446714; the names ours).
//
// Between the last two, 0x4458d0..0x445b75 is not this module's: a
// CMoArray<T*> destructor (0x4458d0, a folded COMDAT already matched) and
// streamsim.cpp's streamsim_Open (0x445910), SSFile's methods
// (0x445960..0x445b40, its vtable 0x475c44) and streamsim_AbstractIOWrapper
// (0x445b50, SSAbstractIOWrapper's vtable 0x475664).

#include "bdefs.h"
#include "editregion.h"
#include "editpoly.h"
#include "editobjects.h"
#include "processing.h"
#include "preworld.h"
#include "findworldmodel.h"
#include "pregeometry.h"
#include "geomroutines.h"


// talon: the 2001 CEditPoly has one texture; what this module reads of it
// (the class is laid out nowhere yet). Its texture name is at 0x74
// (ReplaceTexturesInWorld compares it with the surface's and adds it to the
// world's string holder, 0x445625, 0x445677); 0x84 is OR'd into the
// surface's flags with the brush type, and compared with them (0x445663,
// 0x4456ab): what it is is not known.
class CEditPoly2001_RS : public CBasePoly
{
public:
	uint8			m_Unknown_2c[0x74 - 0x2c];
	const char		*m_pTextureName;
	uint8			m_Unknown_78[0x84 - 0x78];
	uint32			m_Unknown_84;
};

#define EDITPOLY2001(pPoly)	((CEditPoly2001_RS*)(pPoly))


// ------------------------------------------------------------------------ //
// RaySelect (0x4450c0).
// ------------------------------------------------------------------------ //

// talon: each RaySelecter object casts a ray from its position along its
// Rotation's forward vector at every world model's polies; the brush of
// the last poly it hits (0.1 to 1000000 along it) is selected. Returns
// how many were selected -- of the last world model only: the count is
// cleared for each one (0x4450e7), a retail bug kept. In 2001 a
// CPreSurface's m_pBrush is the CEditPoly it came from (CEditPoly::m_pBrush,
// 0x28, at 0x4452c7).
//
// Near: size-exact; the three products of dir * t come out as dir.x * t
// where the shipped code has t * dir.x (0x44524f..0x445267), the operand
// order of a commutative fmul. VEC_MULSCALAR, a named temporary, a
// constructed vector, (dir * t) + pos and the declaration order don't move
// it. The plane through m_pSurface->m_pPlane each time and Dot() are what
// give both dot products the shipped z, y, x order (a CPrePlane* local or
// VEC_DOT give x, z, y; -DIST_TO_PLANE the order but an fchs).
uint32 DoRaySelect(CPreMainWorld *pMainWorld, CEditRegion *pRegion)
{
	float			t;
	uint32			i, j, nSelected;
	CPreWorld		*pWorld;
	CBaseEditObj	*pObj;
	CBaseProp		*pProp;
	PVector			pos, dir, right, up, forward, testPt;
	CPrePoly		*pPoly, *pSelPoly;
	GPOS			polyPos;

	for(i=0; i < pMainWorld->m_WorldModels; i++)
	{
		nSelected = 0;
		pWorld = pMainWorld->m_WorldModels[i];

		for(j=0; j < pMainWorld->m_Objects; j++)
		{
			pObj = pMainWorld->m_Objects[j];

			if(strcmp(pObj->GetClassName(), "RaySelecter") == 0)
			{
				VEC_COPY(pos, pObj->GetPos());

				pProp = pObj->m_PropList.GetProp("Rotation");
				if(pProp)
				{
					gr_GetEulerVectors(((CRotationProp*)pProp)->GetEulerAngles(), right, up, forward);
					dir = forward;

					pSelPoly = NULL;
					for(polyPos=pWorld->m_Polies; polyPos; )
					{
						pPoly = pWorld->m_Polies.GetNext(polyPos);

						t = (pPoly->m_pSurface->m_pPlane->m_Dist - pPoly->m_pSurface->m_pPlane->m_Normal.Dot(pos)) /
							pPoly->m_pSurface->m_pPlane->m_Normal.Dot(dir);
						if(t > 0.1f && t < 1000000.0f)
						{
							testPt = pos + dir * t;
							if(InsideConvex(pPoly, testPt))
								pSelPoly = pPoly;
						}
					}

					if(pSelPoly)
					{
						pRegion->SelectNode(((CEditPoly*)pSelPoly->m_pSurface->m_pBrush)->m_pBrush);
						nSelected++;
					}
				}
			}
		}
	}

	return nSelected;
}


// ------------------------------------------------------------------------ //
// ReplaceTextures (0x445330..0x4458c0): Build 69's replacetextures.cpp,
// with one texture a surface.
// ------------------------------------------------------------------------ //

class CTempSurface
{
	public:

		CEditBrush *m_pBrush;
		CEditPoly *m_pPoly;
		CPrePlane m_Plane;
};


// 0x445330.
BOOL IsPolyInsideSurface(CTempSurface *pSurface, CPrePoly *pPoly)
{
	DWORD i, j;
	CEditPoly *pEditPoly;
	CPrePlane edgePlane;
	PVector vTemp, testPt;

	pEditPoly = pSurface->m_pPoly;
	for(i=0; i < pEditPoly->NumVerts(); i++)
	{
		VEC_SUB(vTemp, pEditPoly->NextPt(i), pEditPoly->Pt(i));
		VEC_CROSS(edgePlane.m_Normal, vTemp, pEditPoly->Normal());
		edgePlane.m_Normal.Norm();
		edgePlane.m_Dist = VEC_DOT(edgePlane.m_Normal, pEditPoly->Pt(i));

		for(j=0; j < pPoly->NumVerts(); j++)
		{
			testPt = pPoly->Pt(j);

			if(DIST_TO_PLANE(testPt, edgePlane) < -0.1f)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}


// 0x4454f0.
BOOL MatchSurface(CTempSurface *pSurface, CPrePoly *pPoly)
{
	CPrePlane *pPlane, diffPlane;

	pPlane = pPoly->m_pSurface->m_pPlane;

	// Are the planes the same?
	diffPlane.m_Normal.x = (PReal)fabs(pPlane->m_Normal.x - pSurface->m_Plane.m_Normal.x);
	diffPlane.m_Normal.y = (PReal)fabs(pPlane->m_Normal.y - pSurface->m_Plane.m_Normal.y);
	diffPlane.m_Normal.z = (PReal)fabs(pPlane->m_Normal.z - pSurface->m_Plane.m_Normal.z);
	diffPlane.m_Dist = (PReal)fabs(pPlane->m_Dist - pSurface->m_Plane.m_Dist);

	if(diffPlane.m_Normal.x < 0.01f && diffPlane.m_Normal.y < 0.01f &&
		diffPlane.m_Normal.z < 0.01f && diffPlane.m_Dist < 0.01f)
	{
		return IsPolyInsideSurface(pSurface, pPoly);
	}
	else
	{
		return FALSE;
	}
}


// 0x445590.
CTempSurface* MatchSurface(CMoArray<CTempSurface*> &surfaces, CPrePoly *pPoly)
{
	DWORD i;

	for(i=0; i < surfaces.GetSize(); i++)
	{
		if(MatchSurface(surfaces[i], pPoly))
			return surfaces[i];
	}

	return NULL;
}


// 0x4455d0. talon: in 2001 a surface has one texture, and its flags count
// as a difference too.
int ReplaceTexturesInWorld(CMoArray<CTempSurface*> &surfaces, CPreWorld *pWorld)
{
	int nReplaced;
	GPOS pos;
	CPrePoly *pPoly;
	CTempSurface *pTempSurface;


	nReplaced = 0;

	// Go thru each poly in the world.
	for(pos=pWorld->m_Polies; pos; )
	{
		pPoly = pWorld->m_Polies.GetNext(pos);
		pTempSurface = MatchSurface(surfaces, pPoly);
		if(pTempSurface)
		{
			if(strcmp(EDITPOLY2001(pTempSurface->m_pPoly)->m_pTextureName, pPoly->m_pSurface->m_pTextureName) != 0 ||
				EDITPOLY2001(pTempSurface->m_pPoly)->m_Unknown_84 != pPoly->m_pSurface->m_Flags)
			{
				pPoly->m_pSurface->m_pTextureName = pWorld->m_StringHolder.AddString(EDITPOLY2001(pTempSurface->m_pPoly)->m_pTextureName);
				pPoly->m_pSurface->m_Flags = (WORD)GetBrushType(pTempSurface->m_pBrush) | EDITPOLY2001(pTempSurface->m_pPoly)->m_Unknown_84;

				if(stricmp(pPoly->m_pSurface->m_pTextureName, "textures\\invisible.dtx") == 0)
					pPoly->m_pSurface->m_Flags |= SURF_INVISIBLE;

				++nReplaced;
			}
		}
	}

	return nReplaced;
}


// 0x445700.
int ReplaceTextures(CEditRegion *pRegion, CPreMainWorld *pWorld)
{
	int nReplaced;
	DWORD i, iPoly;
	CEditBrush *pBrush;
	CMoArray<CTempSurface*> surfaces;
	CTempSurface *pTempSurface;
	LPOS brushPos;


	// Build the surface list.
	surfaces.SetCacheSize(3000);
	for(brushPos=pRegion->m_Brushes; brushPos; )
	{
		pBrush = pRegion->m_Brushes.GetNext(brushPos);

		for(iPoly=0; iPoly < pBrush->m_Polies; iPoly++)
		{
			pTempSurface = new CTempSurface;
			pTempSurface->m_pPoly = pBrush->m_Polies[iPoly];
			pTempSurface->m_pBrush = pBrush;
			pTempSurface->m_Plane = (LTPlane)pBrush->m_Polies[iPoly]->m_Plane;

			surfaces.Append(pTempSurface);
		}
	}

	nReplaced = 0;
	for(i=0; i < pWorld->m_WorldModels; i++)
	{
		nReplaced += ReplaceTexturesInWorld(surfaces, pWorld->m_WorldModels[i]);
	}

	DeleteAndClearArray(surfaces);
	return nReplaced;
}


// ------------------------------------------------------------------------ //
// The terrain's surface merging (0x445b80..0x446164; the names are ours).
// ------------------------------------------------------------------------ //

#define NUM_SURFACE_HASH_BUCKETS	512

// 0x446080: the bucket of a surface: its plane, texture vectors and
// texture name (the multipliers -10000 at 0x475c7c, 10000 at 0x475c78).
uint32 GetSurfaceHash_RS(CPreSurface *pSurface)
{
	return ((uint32)pSurface->m_pTextureName +
		(int)(pSurface->m_pPlane->m_Normal.x * 10000.0f) +
		(int)(pSurface->m_pPlane->m_Normal.y * 10000.0f) +
		(int)(pSurface->m_pPlane->m_Normal.z * 10000.0f) +
		(int)(pSurface->m_TextureO.x * 10000.0f) +
		(int)(pSurface->m_TextureO.y * 10000.0f) +
		(int)(pSurface->m_TextureO.z * 10000.0f) +
		(int)(pSurface->P.x * 10000.0f) +
		(int)(pSurface->P.y * 10000.0f) +
		(int)(pSurface->P.z * 10000.0f) +
		(int)(pSurface->Q.x * 10000.0f) +
		(int)(pSurface->Q.y * 10000.0f) +
		(int)(pSurface->Q.z * 10000.0f)) & (NUM_SURFACE_HASH_BUCKETS-1);
}


// 0x445e10: whether two surfaces are the same (0.0001 at 0x47559c, as a
// double at 0x475c70).
BOOL AreSurfacesSame_RS(CPreSurface *pSurface1, CPreSurface *pSurface2)
{
	return pSurface1->m_TextureO.Equals(pSurface2->m_TextureO, 0.0001f) &&
		pSurface1->P.Equals(pSurface2->P, 0.0001f) &&
		pSurface1->Q.Equals(pSurface2->Q, 0.0001f) &&
		pSurface1->m_pPlane->m_Normal.Equals(pSurface2->m_pPlane->m_Normal, 0.0001f) &&
		fabs(pSurface1->m_pPlane->m_Dist - pSurface2->m_pPlane->m_Dist) < 0.0001 &&
		pSurface1->m_pTextureName == pSurface2->m_pTextureName &&
		pSurface1->m_Flags == pSurface2->m_Flags;
}


// 0x445b80: gives the polies of surfaces that are the same one surface.
// The polies are hashed by surface into 512 lists (each with a cache of
// twice the polies over 512, 0x445bd9); then each list's poly at i is
// taken out (0x445c67), and every poly left in the list with a surface the
// same as its surface gets it and is taken out too. (Taking i out and then
// going on to i+1 skips a poly each time: retail.)
//
// Near: 649 bytes shipped, 661 ours, the same instructions but for
// registers: the shipped code keeps j in ebp across the inlined Remove2
// (its copy loop needs one register, 0x445d08) and ours gives j a stack
// home (its copy loop takes two). Declaration order, nested ifs, an int j,
// no pTest, GetAt, Remove2(j, &g_DefAlloc) and a pointer loop over the
// lists don't move it; Remove(j--) is further off.
void MergeSurfaces_RS(CPreWorld *pWorld)
{
	CMoArray<CPrePoly*>	hash[NUM_SURFACE_HASH_BUCKETS];
	CMoArray<CPrePoly*>	*pList;
	CPrePoly			*pPoly, *pTest;
	CPreSurface			*pSurface;
	GPOS				pos;
	uint32				i, j, k;

	for(i=0; i < NUM_SURFACE_HASH_BUCKETS; i++)
		hash[i].SetCacheSize(pWorld->m_Polies.GetSize() * 2 / NUM_SURFACE_HASH_BUCKETS);

	for(pos=pWorld->m_Polies; pos; )
	{
		pPoly = pWorld->m_Polies.GetNext(pos);
		hash[GetSurfaceHash_RS(pPoly->m_pSurface)].Append(pPoly);
	}

	for(k=0; k < NUM_SURFACE_HASH_BUCKETS; k++)
	{
		pList = &hash[k];

		for(i=0; i < pList->GetSize(); i++)
		{
			pPoly = (*pList)[i];
			pSurface = pPoly->m_pSurface;
			pList->Remove(i);

			for(j=0; j < pList->GetSize(); j++)
			{
				pTest = (*pList)[j];
				if(pTest->m_pSurface != pSurface && AreSurfacesSame_RS(pSurface, pTest->m_pSurface))
				{
					pTest->m_pSurface = pSurface;
					pList->Remove(j);
					j--;
				}
			}
		}
	}
}
