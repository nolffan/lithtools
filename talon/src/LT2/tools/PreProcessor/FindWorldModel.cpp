#include "bdefs.h"
#include "editpoly.h"
#include "editregion.h"
#include "findworldmodel.h"
#include "de_world.h"
#include "editobjects.h"
#include "processing.h"
#include "preworld.h"
#include "proputils.h"

// talon: the 2001 version (0x411670..0x411f60), written from the image.
// Build 69's asks object.lto which classes are world models; this one
// makes one of every object with brushes under it, and reads a brush's
// surface flags from its bool properties, not from Type and Lighting.

static void RecurseAndAddMainBrushes(CWorldNode *pNode, CMoArray<CEditBrush*> &brushes);
static void RecurseAndAddWorldModels(CEditRegion *pRegion, CWorldNode *pNode,
	CMoArray<CWorldModelDef*> &worldModels, BOOL bVolumes);
static void GatherWorldModelBrushes(CWorldNode *pNode, CMoArray<CEditBrush*> &brushes);
static uint32 GetRealProp(CPropList *pList, char *pName, DWORD defaultValue);


char GetBoolProp(CPropList *pList, const char *pName)
{
	CBoolProp *pProp;

	if(pProp = (CBoolProp*)pList->GetMatchingProp(pName, PT_BOOL))
		return pProp->m_Value;

	return false;
}


void FindWorldModels(CEditRegion *pRegion, CMoArray<CWorldModelDef*> &worldModels)
{
	// Recurse and add brushes.. stop when you hit an object.
	CMoArray<CEditBrush*> mainBrushes;
	RecurseAndAddMainBrushes(&pRegion->m_RootNode, mainBrushes);
	if(mainBrushes.GetSize() > 0)
	{
		CWorldModelDef *pDef = new CWorldModelDef;
		pDef->m_Brushes.CopyArray(mainBrushes);
		pDef->m_WorldInfoFlags = WIF_MAINWORLD;
		SAFE_STRCPY(pDef->m_WorldName, MAINWORLD_MODEL_NAME);

		worldModels.Append(pDef);
	}

	// Fill in the worldmodels
	RecurseAndAddWorldModels(pRegion, &pRegion->m_RootNode, worldModels, FALSE);
}


static void RecurseAndAddMainBrushes(CWorldNode *pNode, CMoArray<CEditBrush*> &brushes)
{
	GPOS pos;

	if((pNode->GetType() == Node_Brush) &&
		(!(pNode->m_Flags & NODEFLAG_HIDDEN) || !g_pGlobs->m_bIgnoreHidden))
	{
		if(!(GetBrushType(pNode->AsBrush()) & SURF_NONEXISTENT))
			brushes.Append(pNode->AsBrush());
	}
	else if(pNode->GetType() == Node_Object)
	{
		return;
	}

	// Go into children.
	for(pos=pNode->m_Children; pos; )
		RecurseAndAddMainBrushes(pNode->m_Children.GetNext(pos), brushes);
}


// With bVolumes, only AIVolumes and MusicVolumes.
static void RecurseAndAddWorldModels(
	CEditRegion *pRegion,
	CWorldNode *pNode,
	CMoArray<CWorldModelDef*> &worldModels,
	BOOL bVolumes)
{
	CWorldModelDef *pDef;
	CMoArray<CEditBrush*> brushes;
	brushes.SetCacheSize(4096);

	if(pNode->m_Type == Node_Object)
	{
		if(!bVolumes ||
			stricmp(pNode->GetClassName(), "AIVolume") == 0 ||
			stricmp(pNode->GetClassName(), "MusicVolume") == 0)
		{
			CVectorProp *pProp = (CVectorProp*)pNode->m_PropList.GetProp("Pos");
			if(pProp)
			{
				GatherWorldModelBrushes(pNode, brushes);
				if(brushes > 0)
				{
					// Make a WorldModel out of the brushes.
					pDef = new CWorldModelDef;

					if(stricmp(pNode->GetClassName(), "Terrain") == 0)
						pDef->m_WorldInfoFlags |= WIF_TERRAIN;
					else
						pDef->m_WorldInfoFlags = WIF_MOVEABLE;

					pDef->m_Brushes.CopyArray(brushes);

					CStringProp *pNameProp = (CStringProp*)pNode->m_PropList.GetProp(g_NameName);
					if(!pNameProp)
					{
						DrawStatusText("ERROR: world model without a Name property found.");
						return;
					}

					SAFE_STRCPY(pDef->m_WorldName, pNameProp->m_String);

					worldModels.Append(pDef);
					brushes.SetSize(0);
				}
			}
		}
	}
	else
	{
		// Recurse into children.
		for(GPOS pos=pNode->m_Children; pos; )
			RecurseAndAddWorldModels(pRegion, pNode->m_Children.GetNext(pos), worldModels, bVolumes);
	}
}


static void GatherWorldModelBrushes(CWorldNode *pNode, CMoArray<CEditBrush*> &brushes)
{
	GPOS pos;

	if((pNode->GetType() == Node_Brush) &&
		(!(pNode->m_Flags & NODEFLAG_HIDDEN) || !g_pGlobs->m_bIgnoreHidden) &&
		!(GetBrushType(pNode->AsBrush()) & SURF_NONEXISTENT))
	{
		brushes.Append(pNode->AsBrush());
	}

	for(pos=pNode->m_Children; pos; )
		GatherWorldModelBrushes(pNode->m_Children.GetNext(pos), brushes);
}


void FindAIVolumes(CEditRegion *pRegion, CMoArray<CWorldModelDef*> &worldModels)
{
	RecurseAndAddWorldModels(pRegion, &pRegion->m_RootNode, worldModels, TRUE);
}


// ------------------------------------------------------------------------------------- //
// CWorldModelDef.
// ------------------------------------------------------------------------------------- //

CWorldModelDef::CWorldModelDef()
{
	m_WorldInfoFlags = 0;
	m_WorldName[0] = 0;
	m_Brushes.SetCacheSize(1024);
}


uint32 CWorldModelDef::CalcNumPolies()
{
	DWORD i, total;

	total = 0;
	for(i=0; i < m_Brushes.GetSize(); i++)
	{
		total += m_Brushes[i]->m_Polies.GetSize();
	}

	return total;
}


uint32 GetBrushType(CEditBrush *pBrush)
{
	CPropList *pList;
	uint32 brushType;

	pList = &pBrush->m_PropList;

	brushType = 0;
	if(GetBoolProp(pList, "Solid"))
		brushType |= SURF_SOLID;

	if(GetBoolProp(pList, "Nonexistant"))
		brushType |= SURF_NONEXISTENT;

	if(GetBoolProp(pList, "Invisible"))
		brushType |= SURF_INVISIBLE;

	if(GetBoolProp(pList, "Translucent"))
		brushType |= SURF_TRANSPARENT;

	if(GetBoolProp(pList, "SkyPortal"))
		brushType |= SURF_SKY;

	if(GetBoolProp(pList, "FullyBright"))
		brushType |= SURF_BRIGHT;

	if(GetBoolProp(pList, "FlatShade"))
		brushType |= SURF_FLATSHADE;

	if(GetBoolProp(pList, "GouraudShade"))
		brushType |= SURF_GOURAUDSHADE;

	if(GetBoolProp(pList, "LightMap"))
		brushType |= SURF_LIGHTMAP;

	if(!GetBoolProp(pList, "Subdivide"))
		brushType |= SURF_NOSUBDIV;

	if(GetBoolProp(pList, "HullMaker"))
		brushType |= SURF_HULLMAKER;

	if(GetBoolProp(pList, "AlwaysLightMap"))
		brushType |= SURF_ALWAYSLIGHTMAP;

	if(GetBoolProp(pList, "DirectionalLight"))
		brushType |= SURF_DIRECTIONALLIGHT;

	if(GetBoolProp(pList, "Portal"))
		brushType |= SURF_PORTAL;

	if(GetBoolProp(pList, "SkyPan"))
		brushType |= SURF_PANNINGSKY;

	if(GetBoolProp(pList, "Additive"))
		brushType |= SURF_ADDITIVE;

	if(GetBoolProp(pList, "TimeOfDay"))
		brushType |= SURF_TIMEOFDAY;

	if(GetBoolProp(pList, "TerrainOccluder"))
		brushType |= SURF_TERRAINOCCLUDER;

	// A vis blocker makes hulls too.
	if(GetBoolProp(pList, "VisBlocker"))
		brushType |= SURF_VISBLOCKER | SURF_HULLMAKER;

	if(GetBoolProp(pList, "NotAStep"))
		brushType |= SURF_NOTASTEP;

	if(GetBoolProp(pList, "NoWallWalk"))
		brushType |= SURF_NOWALLWALK;

	if(!GetBoolProp(pList, "BlockLight"))
		brushType |= SURF_NOBLOCKLIGHT;

	// Portals, hull makers and terrain occluders are only for the
	// processor: never drawn or subdivided.
	if(brushType & SURF_PORTAL)
	{
		brushType = SURF_PORTAL | SURF_HULLMAKER | SURF_NOSUBDIV | SURF_INVISIBLE;
	}
	else if(brushType & SURF_HULLMAKER)
	{
		brushType = (brushType & SURF_VISBLOCKER) | SURF_HULLMAKER | SURF_NOSUBDIV | SURF_INVISIBLE;
	}
	else if(brushType & SURF_TERRAINOCCLUDER)
	{
		brushType = SURF_TERRAINOCCLUDER | SURF_NOSUBDIV | SURF_INVISIBLE;
	}

	return brushType;
}


static uint32 GetRealProp(CPropList *pList, char *pName, DWORD defaultValue)
{
	CBaseProp	*pProp;

	pProp = pList->GetProp(pName);
	if(!pProp)
		return defaultValue;

	if(pProp->m_Type == PT_LONGINT || pProp->m_Type == PT_REAL)
	{
		return (DWORD)((CRealProp*)pProp)->m_Value;
	}
	else
	{
		return defaultValue;
	}
}


uint32 GetDetailLevel(CEditBrush *pBrush)
{
	DWORD	val = GetRealProp(&pBrush->m_PropList, "DetailLevel", 0);

	if(val > MAX_SURFACE_DETAIL_LEVEL)
		val = MAX_SURFACE_DETAIL_LEVEL;

	return val;
}
