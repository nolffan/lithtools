// talon: 0x00419e20..0x0041e100: the lighting functions LightMapMaker.cpp's
// drafts don't match yet (texture lights, the lightmap sample positions,
// RemoveExcessPolies, the light groups), to be folded into LightMapMaker.cpp
// in place of its versions. (The file name is ours and provisional.)

#include "bdefs.h"
#include "lightmapmaker.h"
#include "processing.h"
#include "editobjects.h"
#include "iobjectplugin.h"
#include "threads.h"
#include "prelightmap.h"
#include "lightmap_compress.h"
#include "gettextureinfo.h"
#include "geomroutines.h"
#include "node.h"
#include "conparse.h"
#include "genericprop_setup.h"
#include "ltquatbase.h"
#include "parse_world_info.h"
#include "pregeometry.h"
#include "LightMapMaker_r3_decl.h"

// Makes a light for each texture light of each poly's texture: at the
// poly's center, or tiled across it as the texture is (0x419e20..0x41a86f).
//
// talon: near (inline budget). The call sequence is right but for which
// _CVector(x, y, z) constructors are expanded: shipped calls it
// (0x4021c0) at 0x41a049, 0x41a0a6, 0x41a104 (vMin += vTex * fDot, the
// second Pt(j) - vMin), 0x41a1b9 (the Cross) and 0x41a39b (fV's
// vMin - O), and expands the two in the innermost loop's vTest; ours does
// the reverse, and expands GetNext and ~TInfo in the clean-up loop where
// shipped calls them (0x43b250, then the scalar deleting destructor
// 0x4221c0 with 1). The two Dots are written with the temporary as this:
// shipped loads the temporary's components first (0x419fde, 0x41a321).
// Also differs: this in edi (ours esi), the zero register ebx, and the
// x87 order of the first Mag (0x419ee0: (zz + xx) + yy where Mag() is
// (xx + yy) + zz, as the second one at 0x419f2f is).
//
// Retail bug: when GetTextureInfo fails it returns without freeing what
// it put in texInfo (0x419e8d jumps to the epilogue).
// Retail bug: a texture whose plane is perpendicular to the poly's makes
// the tiling divide by zero (N.vNormal at 0x41a60b); the point is then not
// inside the poly and no light is made.
void CLightMapMaker::GetTextureLights()
{
	CGLinkedList<TInfo*> texInfo;
	CPrePoly *pPoly;
	CPreSurface *pSurface;
	TInfo *pInfo;
	TLightDef *pDef;
	PVector vRowStart, vPos, vTest;
	PReal fDot, fU, fV, t;
	uint32 i, j, iDef, iU, iV, nLights;
	BOOL bHit, bAnyHit;
	GPOS pos;


	if(!GetTextureInfo(m_pWorld, texInfo))
	{
		DrawStatusTextIfMainWorld("** Unable to get texture lights.");
		DrawStatusTextIfMainWorld("** (Is project path set?)");
		return;
	}

	nLights = 0;
	for(i=0; i < m_Polies; i++)
	{
		pPoly = m_Polies[i];

		if(pPoly->GetSurface()->m_pTextureName &&
			(pInfo = FindTInfo(texInfo, pPoly->GetSurface()->m_pTextureName)))
		{
			PVector vTexP = pPoly->GetSurface()->m_TextureP / pPoly->GetSurface()->m_TextureP.Mag();
			PVector vTexQ = pPoly->GetSurface()->m_TextureQ / pPoly->GetSurface()->m_TextureQ.Mag();

			// The poly's corner furthest back along the texture vectors.
			PVector vMin = pPoly->Pt(0);
			for(j=1; j < pPoly->NumVerts(); j++)
			{
				fDot = (pPoly->Pt(j) - vMin).Dot(vTexP);
				if(fDot < 0.0f)
					vMin += vTexP * fDot;

				fDot = (pPoly->Pt(j) - vMin).Dot(vTexQ);
				if(fDot < 0.0f)
					vMin += vTexQ * fDot;
			}

			pSurface = pPoly->GetSurface();
			PVector vNormal = pSurface->m_TextureP.Cross(pSurface->m_TextureQ);
			vNormal.Norm();

			fDot = vNormal.Mag();
			if(fDot >= 0.1f && fDot <= 1.1f)
			{
				for(iDef=0; iDef < pInfo->m_LightDefs; iDef++)
				{
					pDef = pInfo->m_LightDefs[iDef];

					if(pDef->m_bCentered)
					{
						AddLightDef(pDef, pPoly->GetCenter(), pPoly);
						nLights++;
					}
					else
					{
						// Tile it across the poly as the texture is tiled.
						pSurface = pPoly->GetSurface();
						fU = (vMin - pSurface->m_TextureO).Dot(pSurface->m_TextureP);
						fV = (vMin - pSurface->m_TextureO).Dot(pSurface->m_TextureQ);

						fU = SnapNumberToGrid(fU, (PReal)pInfo->m_Width) + pDef->m_UOffset;
						fV = SnapNumberToGrid(fV, (PReal)pInfo->m_Height) + pDef->m_VOffset;

						vRowStart = pSurface->InverseP * fU + pSurface->m_TextureO + pSurface->InverseQ * fV;
						PVector vStepU = pSurface->InverseP * (PReal)pInfo->m_Width;
						PVector vStepV = pSurface->InverseQ * (PReal)pInfo->m_Height;

						bAnyHit = FALSE;
						for(iV=0; iV < TEXTURE_LIGHT_ITERATIONS; iV++)
						{
							vPos = vRowStart;
							bHit = FALSE;
							for(iU=0; iU < TEXTURE_LIGHT_ITERATIONS; iU++)
							{
								// Onto the poly's plane, along the texture's normal.
								CPrePlane *pPlane = pPoly->GetPlane();
								t = (pPlane->m_Dist - VEC_DOT(vPos, pPlane->m_Normal)) / VEC_DOT(vNormal, pPlane->m_Normal);
								vTest = vPos + vNormal * t;

								if(InsideConvex(pPoly, vTest))
								{
									AddLightDef(pDef, vTest + pPoly->Normal() * pDef->m_ZOffset, pPoly);
									bHit = TRUE;
									nLights++;
								}
								else if(bHit)
								{
									break;
								}

								vPos += vStepU;
							}

							bAnyHit |= bHit;
							if(bAnyHit && !bHit)
								break;

							vRowStart += vStepV;
						}
					}
				}
			}
		}
	}

	DrawStatusTextIfMainWorld("Created %d texture lights.", nLights);

	for(pos=texInfo; pos; )
		delete texInfo.GetNext(pos);
}





// The positions of the poly's lightmap samples. A sample in a wall moves to
// the closest of nSub*nSub supersamples that is in the clear (0x41c160;
// the function table's 0x41c364 is not a function: inside this one, a call
// target read out of 0x4575dc's bytes).
//
// talon: near (inline budget). The two failures share one exit (0x41c6f2),
// so they are early returns. Shipped calls the _CVector constructor for
// the three in vGridStart's expression (0x41c398, 0x41c3c5, 0x41c3f2) and
// expands every later one, and expands valid's destructor at the last
// return (0x41cce1 frees directly) where the first return calls
// SetSize2(0); ours expands the first ones and calls the later ones
// (dU2, dV2 and vRowPos, the ring hit, the plane). dU2 and dV2 are
// assigned, not initialised (shipped builds them in a temporary at 0x40
// and copies). The conversion of nSub is shared by dU and dV and stored as
// a float (0x41c236); ours keeps it on the x87 stack.
//
// Retail bug: the ring search never looks at the corner (gx+r, gy+r): both
// FindRowSample calls stop short of x = gx+r and both FindColumnSample
// calls of y = gy+r, so of each ring's corners only that one is skipped.
BOOL CLightMapMaker::CalcSamplePositions(CPrePoly *pPoly, uint32 nSub, CMoArray<PVector> *pPositions)
{
	CMoArray<uint32> valid;
	CFullLightMap *pMap;
	CPreSurface *pSurface;
	CPrePlane *pPlane;
	LMMPointQuery_r3 query;
	PVector dU, dV, vGridStart, vRowPos, vPos, vPt, dU2, dV2, *pOut;
	uint32 nSamplesU, nSamplesV, u, v, iRow;
	int gx, gy, r, x, y;


	pMap = m_FullLightMaps[pPoly->m_nName];
	nSamplesU = pMap->m_Width * nSub + 64;
	nSamplesV = pMap->m_Height * nSub + 64;
	if(!valid.SetSize(nSamplesU * nSamplesV))
		return FALSE;

	// The supersample grid, with a border of 32 supersamples.
	pSurface = pPoly->GetSurface();
	dU = pSurface->InverseP * (m_pWorld->m_LMGridSize / (PReal)nSub);
	dV = pSurface->InverseQ * (m_pWorld->m_LMGridSize / (PReal)nSub);
	vGridStart = pPoly->m_PolyO + pSurface->InverseP * m_ShiftU + pSurface->InverseQ * m_ShiftV - dU * 32.0f - dV * 32.0f;

	if(pPoly->m_pWorld->m_bUsingTransform)
	{
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &dU);
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &dV);
		vGridStart = pPoly->m_pWorld->m_mTransform * vGridStart;
	}

	vRowPos = vGridStart;
	iRow = 0;
	for(v=0; v < nSamplesV; v++)
	{
		vPos = vRowPos;
		for(u=0; u < nSamplesU; u++)
		{
			if(m_bClipLight)
			{
				// Which world models' solid space is it in?
				pPlane = pPoly->GetPlane();
				vPt = vPos;
				vPt -= pPlane->m_Normal * (pPlane->m_Normal.Dot(vPos) - pPlane->m_Dist);

				query.m_pMaker = this;
				query.m_nWorlds = 0;
				query.m_pbValid = &valid[iRow + u];
				*query.m_pbValid = TRUE;
				query.m_vPos = vPt;
				m_WorldTree.FindObjectsOnPoint(&vPt, PointInsideCB_41cdf0, &query);

				if(query.m_nWorlds == 0)
					*query.m_pbValid = FALSE;
			}
			else
			{
				valid[iRow + u] = TRUE;
			}

			vPos += dU;
		}

		vRowPos += dV;
		iRow += nSamplesU;
	}

	if(!pPositions->SetSize(pMap->m_Width * pMap->m_Height))
		return FALSE;

	// The samples.
	dU2 = pSurface->InverseP * m_pWorld->m_LMGridSize;
	dV2 = pSurface->InverseQ * m_pWorld->m_LMGridSize;
	vRowPos = pPoly->m_PolyO + pSurface->InverseP * m_ShiftU + pSurface->InverseQ * m_ShiftV;

	if(pPoly->m_pWorld->m_bUsingTransform)
	{
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &dU2);
		MatVMul_InPlace_3x3(&pPoly->m_pWorld->m_mTransform, &dV2);
		vRowPos = pPoly->m_pWorld->m_mTransform * vRowPos;
	}

	for(v=0; v < pMap->m_Height; v++)
	{
		vPos = vRowPos;
		for(u=0; u < pMap->m_Width; u++)
		{
			pOut = &(*pPositions)[v*pMap->m_Width + u];
			*pOut = vPos;

			if(m_bClipLight)
			{
				gx = 32 + u*nSub;
				gy = 32 + v*nSub;
				if(!valid[gy*nSamplesU + gx])
				{
					// The closest valid supersample, ring by ring.
					for(r=1; r < 32; r++)
					{
						if(FindRowSample_41cd10(&valid, gx-r, gx+r, gy-r, nSamplesU, nSamplesV, &x, &y) ||
							FindRowSample_41cd10(&valid, gx-r, gx+r, gy+r, nSamplesU, nSamplesV, &x, &y) ||
							FindColumnSample_41cd80(&valid, gy-r, gy+r, gx-r, nSamplesU, nSamplesV, &x, &y) ||
							FindColumnSample_41cd80(&valid, gy-r, gy+r, gx+r, nSamplesU, nSamplesV, &x, &y))
						{
							*pOut = vGridStart + dU * (PReal)x + dV * (PReal)y;
							break;
						}
					}
				}

				// Onto the poly's plane.
				pPlane = pPoly->GetPlane();
				*pOut -= pPlane->m_Normal * (pPlane->m_Normal.Dot(*pOut) - pPlane->m_Dist);
			}

			vPos += dU2;
		}

		vRowPos += dV2;
	}

	return TRUE;
}


// Drops, from the current light animation, the polies whose lightmaps are
// black in every frame (0x41cf50..0x41d32a).
//
// talon: near (inline budget). Shipped calls CMoArray<uint32>'s
// destructor for delete pValues (0x41d09c -> 0x41e0d0) where ours expands
// it (and calls _DeleteAndDestroyArray); the Remove2 of m_PolyMaps is called
// (0x447110) and the other two expanded in both. Shipped saves this before
// the pushes and keeps iPoly*4 only in its slot (0x14): a 0x14 frame, ours
// 0x18. Tried without effect: pMap a local or two operator[]s,
// GetArray()[i] for (*pValues)[i].
BOOL CLightMapMaker::RemoveExcessPolies()
{
	uint32 iFrame, iPoly, i;
	BOOL bAllBlack, bNoValues;
	CPreLightFrame *pFrame;
	CMoArray<uint32> *pValues;
	CPreLightMap *pMap;

	if(!m_pCurAnim)
		return FALSE;

	for(iPoly=0; iPoly < m_pCurAnim->m_Polies; iPoly++)
	{
		// Are all the frames black?
		bAllBlack = TRUE;
		for(iFrame=0; iFrame < m_pCurAnim->m_Frames; iFrame++)
		{
			pFrame = m_pCurAnim->m_Frames[iFrame];

			if(pFrame->m_PolyMaps.GetSize() != m_pCurAnim->m_Polies.GetSize())
				return FALSE;

			pMap = pFrame->m_PolyMaps[iPoly];
			if(pMap && !pMap->AllBlack())
			{
				bAllBlack = FALSE;
				break;
			}
		}

		bNoValues = TRUE;
		for(iFrame=0; iFrame < m_pCurAnim->m_Frames; iFrame++)
		{
			pFrame = m_pCurAnim->m_Frames[iFrame];

			if(pFrame->m_Unknown_14.GetSize() != m_pCurAnim->m_Polies.GetSize())
				return FALSE;

			pValues = pFrame->m_Unknown_14[iPoly];
			if(pValues)
			{
				for(i=0; i < pValues->GetSize(); i++)
				{
					if(pValues->GetArray()[i])
					{
						bNoValues = FALSE;
						goto Done;
					}
				}
			}
		}

	Done:;
		if(bAllBlack)
		{
			for(iFrame=0; iFrame < m_pCurAnim->m_Frames; iFrame++)
			{
				pFrame = m_pCurAnim->m_Frames[iFrame];

				delete pFrame->m_PolyMaps[iPoly];
				pFrame->m_PolyMaps[iPoly] = NULL;

				if(bNoValues)
				{
					delete pFrame->m_Unknown_14[iPoly];
					pFrame->m_Unknown_14[iPoly] = NULL;
				}

				if(bNoValues)
				{
					pFrame->m_PolyMaps.Remove(iPoly);
					pFrame->m_Unknown_14.Remove(iPoly);
				}
			}

			if(bNoValues)
			{
				m_pCurAnim->m_Polies.Remove(iPoly);
				--iPoly;
			}
		}
	}

	return TRUE;
}


// Copies the lights under a LightGroup object into its numbered properties
// in the processed world's copy of it (0x41d910..0x41df9d; the function
// table's 0x41de17 is not a function: inside this one, a call target read
// out of 0x47082a's bytes). Called by SetupLightGroups (0x41d790).
//
// talon: near (register allocation), 2 bytes short: shipped keeps
// pos and then the child's name in ebp and the object and then the child in
// esi, ours the other way round, and four scalars in other frame slots
// (shipped: &pObj->m_PropList 0x28, pos 0x2c, pPos 0x34, name 0x38,
// pRadius 0x44). The child's name is copied into Object<n> from the
// property's own string (0x41de3d: ebp, from 0x41dc00), not the std::string.
// Tried without effect: the declaration order, pos assigned before pObj,
// a local for the child's or the object's property list, pChild declared
// in the loop.
void ProcessLightGroup_r3(CWorldNode *pGroup, CPreMainWorld *pWorld)
{
	GPOS pos;
	uint32 iObject, iLight;
	CBaseEditObj *pObj;
	CWorldNode *pChild;
	CBaseProp *pPos, *pColor, *pRadius, *pLightObjects, *pProp;
	char num[8];
	char *pName, *pChildName;

	pName = ((CStringProp*)pGroup->m_PropList.GetProp("Name"))->m_String;
	iObject = FindObjectByName_41dfa0(pName, pWorld);
	if(iObject == 0xFFFFFFFF)
		return;

	pObj = pWorld->m_Objects[iObject];
	pos = pGroup->m_Children;

	static std::string sObject("Object");
	static std::string sPosition("Position");
	static std::string sColor("Color");
	static std::string sRadius("Radius");
	static std::string sModelLight("ModelLight");

	iLight = 1;
	while(pos)
	{
		pChild = pGroup->m_Children.GetNext(pos);

		pChildName = ((CStringProp*)pChild->m_PropList.GetProp("Name"))->m_String;
		std::string name(pChildName);

		pPos = pChild->m_PropList.GetProp("Pos");
		pColor = pChild->m_PropList.GetProp("LightColor");
		if(!pColor)
			pColor = pChild->m_PropList.GetProp("InnerColor");
		pRadius = pChild->m_PropList.GetProp("LightRadius");
		pLightObjects = pChild->m_PropList.GetProp("LightObjects");

		g_LightsByName[name] = NULL;

		std::string objectName = sObject + itoa(iLight, num, 10);
		std::string positionName = sPosition + itoa(iLight, num, 10);
		std::string colorName = sColor + itoa(iLight, num, 10);
		std::string radiusName = sRadius + itoa(iLight, num, 10);
		std::string modelLightName = sModelLight + itoa(iLight, num, 10);

		pProp = pObj->m_PropList.GetProp(objectName.c_str());
		if(pProp)
			strcpy(((CStringProp*)pProp)->m_String, pChildName);

		pProp = pObj->m_PropList.GetProp(positionName.c_str());
		if(pProp && pPos)
			((CVectorProp*)pProp)->m_Vector = ((CVectorProp*)pPos)->m_Vector;

		pProp = pObj->m_PropList.GetProp(colorName.c_str());
		if(pProp && pColor)
			((CColorProp*)pProp)->m_Vector = ((CColorProp*)pColor)->m_Vector;

		pProp = pObj->m_PropList.GetProp(radiusName.c_str());
		if(pProp && pRadius)
			((CRealProp*)pProp)->m_Value = ((CRealProp*)pRadius)->m_Value;

		pProp = pObj->m_PropList.GetProp(modelLightName.c_str());
		if(pProp && pLightObjects)
			((CBoolProp*)pProp)->m_Value = ((CBoolProp*)pLightObjects)->m_Value;

		iLight++;
	}
}



// 0x41e090 is _CVector<float>::FVec (ltvector.h) out of line: called only
// from ApplyLightToPoly (0x417566, 0x417bb7, LightMapMaker_r2.cpp's range),
// where the inline budget is spent. It is emitted when that function calls
// it rather than expanding it; nothing to write here (an explicit
// instantiation of the inline member emits nothing with this compiler).
