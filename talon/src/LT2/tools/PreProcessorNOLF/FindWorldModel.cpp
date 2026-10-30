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
	CMoArray<CWorldModelDef*> &worldModels);
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
	CWorldModelDef *pDef;

	// Recurse and add brushes.. stop when you hit an object.
	CMoArray<CEditBrush*> mainBrushes;
	RecurseAndAddMainBrushes(&pRegion->m_RootNode, mainBrushes);
	if(mainBrushes.GetSize() > 0)
	{
		pDef = new CWorldModelDef;
		pDef->m_Brushes.CopyArray(mainBrushes);
		pDef->m_WorldInfoFlags = WIF_MAINWORLD;
		SAFE_STRCPY(pDef->m_WorldName, MAINWORLD_MODEL_NAME);

		worldModels.Append(pDef);
	}

	// Fill in the worldmodels
	RecurseAndAddWorldModels(pRegion, &pRegion->m_RootNode, worldModels);
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


// talon: NOLF's has no AI volumes (AvP2's bVolumes: only AIVolumes and
// MusicVolumes): its only strings are "Pos" and "Terrain" (0x41cd21).
static void RecurseAndAddWorldModels(
	CEditRegion *pRegion,
	CWorldNode *pNode,
	CMoArray<CWorldModelDef*> &worldModels)
{
	CWorldModelDef *pDef;
	CMoArray<CEditBrush*> brushes;
	CBaseEditObj *pObj;
	brushes.SetCacheSize(4096);

	// talon: through AsObject (0x41cd2a calls it) and its m_TypeName
	// (0x41cdc2), not GetClassName.
	if(pNode->m_Type == Node_Object)
	{
		pObj = pNode->AsObject();

		CVectorProp *pProp = (CVectorProp*)pObj->m_PropList.GetProp("Pos");
		if(pProp)
		{
			GatherWorldModelBrushes(pNode, brushes);
			if(brushes > 0)
			{
				// Make a WorldModel out of the brushes.
				pDef = new CWorldModelDef;

				if(stricmp(pObj->m_TypeName, "Terrain") == 0)
					pDef->m_WorldInfoFlags |= WIF_TERRAIN;
				else
					pDef->m_WorldInfoFlags = WIF_MOVEABLE;

				pDef->m_Brushes.CopyArray(brushes);

				CStringProp *pNameProp = (CStringProp*)pObj->m_PropList.GetProp(g_NameName);
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
	else
	{
		// Recurse into children.
		for(GPOS pos=pNode->m_Children; pos; )
			RecurseAndAddWorldModels(pRegion, pNode->m_Children.GetNext(pos), worldModels);
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
	uint32 brushType;

	brushType = 0;
	if(GetBoolProp(&pBrush->m_PropList, "Solid"))
		brushType |= SURF_SOLID;

	if(GetBoolProp(&pBrush->m_PropList, "Nonexistant"))
		brushType |= SURF_NONEXISTENT;

	if(GetBoolProp(&pBrush->m_PropList, "Invisible"))
		brushType |= SURF_INVISIBLE;

	if(GetBoolProp(&pBrush->m_PropList, "Translucent"))
		brushType |= SURF_TRANSPARENT;

	if(GetBoolProp(&pBrush->m_PropList, "SkyPortal"))
		brushType |= SURF_SKY;

	if(GetBoolProp(&pBrush->m_PropList, "FullyBright"))
		brushType |= SURF_BRIGHT;

	if(GetBoolProp(&pBrush->m_PropList, "FlatShade"))
		brushType |= SURF_FLATSHADE;

	if(GetBoolProp(&pBrush->m_PropList, "GouraudShade"))
		brushType |= SURF_GOURAUDSHADE;

	if(GetBoolProp(&pBrush->m_PropList, "LightMap"))
		brushType |= SURF_LIGHTMAP;

	if(!GetBoolProp(&pBrush->m_PropList, "Subdivide"))
		brushType |= SURF_NOSUBDIV;

	if(GetBoolProp(&pBrush->m_PropList, "HullMaker"))
		brushType |= SURF_HULLMAKER;

	if(GetBoolProp(&pBrush->m_PropList, "AlwaysLightMap"))
		brushType |= SURF_ALWAYSLIGHTMAP;

	if(GetBoolProp(&pBrush->m_PropList, "DirectionalLight"))
		brushType |= SURF_DIRECTIONALLIGHT;

	if(GetBoolProp(&pBrush->m_PropList, "Portal"))
		brushType |= SURF_PORTAL;

	if(GetBoolProp(&pBrush->m_PropList, "SkyPan"))
		brushType |= SURF_PANNINGSKY;

	if(GetBoolProp(&pBrush->m_PropList, "Additive"))
		brushType |= SURF_ADDITIVE;

	if(GetBoolProp(&pBrush->m_PropList, "TimeOfDay"))
		brushType |= SURF_TIMEOFDAY;

	if(GetBoolProp(&pBrush->m_PropList, "TerrainOccluder"))
		brushType |= SURF_TERRAINOCCLUDER;

	// A vis blocker makes hulls too.
	if(GetBoolProp(&pBrush->m_PropList, "VisBlocker"))
		brushType |= SURF_VISBLOCKER | SURF_HULLMAKER;

	if(GetBoolProp(&pBrush->m_PropList, "NotAStep"))
		brushType |= SURF_NOTASTEP;

	// talon: NOLF's has no NoWallWalk or BlockLight surface flags (AvP2's):
	// NotAStep is the last property read (0x41d401).

	// Portals, hull makers and terrain occluders are only for the
	// processor: never drawn or subdivided.
	if(brushType & SURF_PORTAL)
	{
		brushType = SURF_PORTAL | SURF_HULLMAKER | SURF_NOSUBDIV | SURF_INVISIBLE;
	}
	else if(brushType & SURF_HULLMAKER)
	{
		brushType = SURF_HULLMAKER | SURF_NOSUBDIV | SURF_INVISIBLE | (brushType & SURF_VISBLOCKER);
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
