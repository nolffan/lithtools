// talon: Terrain.cpp's helpers of the T-junction fixer (0x447220..0x4479c0),
// which only TJunctions.cpp's FixTJunctions calls (the names by address).

#ifndef __TERRAIN_H__
#define __TERRAIN_H__

	#include "preworld.h"
	#include "splitpoly.h"

	// The functions before 0x4479c0 that only the T-junction code calls
	// (0x447220..0x4479c0, written in Terrain.cpp under these names; cdecl):
	// adds the polies of the BSP under pRoot whose extents overlap pPoly's to
	// touching (recursive);
	void	sub_447220(NODEREF pRoot, CPrePoly *pPoly, CMoArray<CPrePoly*> &touching);
	// splits pPoly in two at pPt, a point on one of its edges, with pStruct
	// the classification of its points ("> 1 edge split!");
	BOOL	sub_4475a0(PVector *pPt, CPrePoly *pPoly, CPrePoly *sides[2], SplitStruct *pStruct);
	// puts pNew1 in place of pOld in the nodes of nodes and adds a node for
	// pNew2 under each (pWorld unused).
	void	sub_447940(CPreWorld *pWorld, CNodeList &nodes, CPrePoly *pOld, CPrePoly *pNew1, CPrePoly *pNew2);

#endif
