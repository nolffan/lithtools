#include "bdefs.h"
#include "brushtoworld.h"
#include "prepoly.h"
#include "create_world_tree.h"
#include "editpoly.h"
#include "splitpoly.h"

// talon: the 2001 version (0x40a510..0x40b540). Terrain world models are
// subdivided to their own node size, so each poly goes down the tree with
// the world model it came from.

// A poly and its world model (the names are ours).
struct WTPolyRef
{
	CEditPoly		*m_pPoly;
	CWorldModelDef	*m_pWorldModel;
};

PReal g_TerrainNodeSize;
PReal g_WorldNodeSize;

// Subdivision size for areas with terrain polies.
PReal GetTerrainNodeSize(char *pInfoString)
{
	ConParse	parse;
	PReal		ret;

	parse.Init(pInfoString);
	if(parse.ParseFind("TerrainNodeSize", FALSE, 1))
	{
		ret = (PReal)atof(parse.m_Args[1]);
		return DCLAMP(ret, (PReal)1.0, (PReal)20000.0);
	}
	else
	{
		return (PReal)800.0;
	}
}

// Subdivision size for areas with normal world polies.
PReal GetWorldNodeSize(char *pInfoString)
{
	ConParse	parse;
	PReal		ret;

	parse.Init(pInfoString);
	if(parse.ParseFind("WorldNodeSize", FALSE, 1))
	{
		ret = (PReal)atof(parse.m_Args[1]);
		return DCLAMP(ret, (PReal)1.0, (PReal)20000.0);
	}
	else
	{
		return (PReal)600.0;
	}
}

BOOL DoesPolyIntersectNode(CPrePoly *pPoly, WorldTreeNode *pNode)
{
	uint32			i, j;
	LTPlane			thePlane;
	BOOL			bOutside;
	SplitStruct		splitStruct;
	PolySide		side;


	// Trivial reject if all points are outside any of the planes..
	for(i=0; i < NUM_WTPLANES; i++)
	{
		pNode->SetupPlane(i, &thePlane);

		bOutside = TRUE;
		for(j=0; j < pPoly->NumVerts(); j++)
		{
			if(thePlane.DistTo(pPoly->Pt(j)) > 0.0f)
			{
				bOutside = FALSE;
				break;
			}
		}

		if(bOutside)
			return FALSE;
	}

	// Trivial accept..
	for(i=0; i < pPoly->NumVerts(); i++)
	{
		if(base_IsPtInBox(&pPoly->Pt(i), &pNode->m_BBoxMin, &pNode->m_BBoxMax))
		{
			return TRUE;
		}
	}

	// Ok, clip it up.
	CPrePoly *pTemp = pPoly->Clone();
	if(!pTemp)
		return FALSE;

	for(i=0; i < NUM_WTPLANES; i++)
	{
		pNode->SetupPlane(i, &thePlane);

		side = GetPolySide(&thePlane, pTemp, &splitStruct);
		if(side == BackSide)
		{
			return FALSE;
		}
		else if(side == Intersect)
		{
			// Clip.
			CPrePoly *pNewPoly = CreatePoly(CPrePoly, pTemp->NumVerts()+2, FALSE);
			if(!pNewPoly)
				return FALSE;

			SplitPolySide(FrontSide, &thePlane, pTemp, pNewPoly, &splitStruct);
			DeletePoly(pTemp);
			pTemp = pNewPoly;
		}
	}
	DeletePoly(pTemp);

	return TRUE;
}


BOOL CWTCallback(WorldTree *pWorldTree,
	WorldTreeNode *pNode,
	CMoArray<WTPolyRef> *pPolies,
	CMoArray<WTPolyRef> *pInsidePolies)
{
	// Find out which polies are inside this node.
	BOOL bTerrain = FALSE;
	BOOL bMainWorld = FALSE;
	BOOL bInside;
	CPrePoly *pTemp;

	for(uint32 i=0; i < pPolies->GetSize(); i++)
	{
		WTPolyRef &ref = pPolies->Get(i);

		pTemp = EditPolyToPrePoly(ref.m_pPoly);
		if(!pTemp)
			return FALSE;

		bInside = DoesPolyIntersectNode(pTemp, pNode);
		DeletePoly(pTemp);

		if(bInside)
		{
			pInsidePolies->Append(ref);

			if(ref.m_pWorldModel->m_WorldInfoFlags & WIF_TERRAIN)
				bTerrain = TRUE;
			else
				bMainWorld = TRUE;
		}
	}

	// Use the world or the terrain node size to determine
	// if the node should be divided further.
	if(bMainWorld)
	{
		return pNode->m_MinSize > g_WorldNodeSize;
	}
	else if(bTerrain)
	{
		return pNode->m_MinSize > g_TerrainNodeSize;
	}
	else
	{
		// Nothing in here.. definitely don't divide further!
		return FALSE;
	}
}


BOOL CreateNodes_R(	WorldTree *pWorldTree, WorldTreeNode *pNode,
					PVector *pMin, PVector *pMax, uint32 depth, CMoArray<WTPolyRef> *pPolies)
{
	WorldTreeNode			*pChild;
	uint32					i;
	CMoArray<WTPolyRef>		insidePolies;


	pNode->TermChildren();

	++pWorldTree->m_nNodes;
	if(depth > pWorldTree->m_nDepth)
	{
		pWorldTree->m_nDepth = depth;
	}

	// Setup..
	pNode->SetBBox(*pMin, *pMax);

	// It'll avoid making a node if we're past MAX_NODE_LEVEL, but it would have to
	// be a ridiculously large tree to have > 16 levels (4^16 or 4294967296 nodes
	// would be in the tree...)
	ASSERT(depth < MAX_NODE_LEVEL);

	// Subdivide?
	insidePolies.SetCacheSize(pPolies->GetSize() / 2); // For speed...
	if(depth < MAX_NODE_LEVEL &&
		CWTCallback(pWorldTree, pNode, pPolies, &insidePolies))
	{
		if(!pNode->Subdivide())
			return FALSE;

		// Recurse
		for(i=0; i < MAX_WTNODE_CHILDREN; i++)
		{
			pChild = pNode->m_ChildrenA[i];

			if(!CreateNodes_R(pWorldTree, pChild,
				&pChild->m_BBoxMin, &pChild->m_BBoxMax, depth+1, &insidePolies))
			{
				pNode->Term();
				return FALSE;
			}
		}
	}

	return TRUE;
}


BOOL CreateWorldTree(WorldTree *pWorldTree,
	CMoArray<CWorldModelDef*> &worldModels, char *pInfoString)
{
	uint32					i, iPoly, iOutPoly, iBrush;
	CMoArray<WTPolyRef>		rootPolies;
	CEditBrush				*pBrush;
	CWorldModelDef			*pWorldModel;
	PVector					minBox, maxBox;
	CEditPoly				*pPoly;



	// Figure out node sizes.
	g_TerrainNodeSize = GetTerrainNodeSize(pInfoString);
	g_WorldNodeSize = GetWorldNodeSize(pInfoString);

	pWorldTree->Term();

	// Build the initial poly list.
	for(i=0; i < worldModels.GetSize(); i++)
	{
		pWorldModel = worldModels[i];

		if(!(pWorldModel->m_WorldInfoFlags & WIF_MAINWORLD))
			continue;

		// Add all of its polies.
		iOutPoly = rootPolies.GetSize();
		rootPolies.Fast_NiceSetSize(rootPolies.GetSize() + pWorldModel->CalcNumPolies());
		for(iBrush=0; iBrush < pWorldModel->m_Brushes; iBrush++)
		{
			pBrush = pWorldModel->m_Brushes[iBrush];

			for(iPoly=0; iPoly < pBrush->m_Polies; iPoly++)
			{
				pPoly = pBrush->m_Polies[iPoly];
				WTPolyRef *pRef = &rootPolies[iOutPoly];
				pRef->m_pPoly = pPoly;
				pRef->m_pWorldModel = pWorldModel;
				iOutPoly++;
			}
		}
	}

	// Figure out the extents for the root node.
	// (Some messed up worlds have 0 polies on the root so the extents need to be 0).
	if(rootPolies.GetSize() > 0)
	{
		minBox.Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
		maxBox.Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);

		for(iPoly=0; iPoly < rootPolies; iPoly++)
		{
			pPoly = rootPolies[iPoly].m_pPoly;

			for(i=0; i < pPoly->NumVerts(); i++)
			{
				VEC_MIN(minBox, minBox, pPoly->Pt(i));
				VEC_MAX(maxBox, maxBox, pPoly->Pt(i));
			}
		}
	}
	else
	{
		minBox.Init();
		maxBox.Init();
	}

	return CreateNodes_R(pWorldTree, &pWorldTree->m_RootNode,
		&minBox, &maxBox, 0, &rootPolies);
}
