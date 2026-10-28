// n
// add=1 SetSize2(0, &g_DefAlloc) ov=0
// talon: 0x00425b10..0x004282c0, after parse_world_info: the physics
// blocks ('%d bytes for physics, %d blocks'). Written from the image (no
// source of any version survives: Build 69 has no physics blocks). The
// file's name and every name here are ours.
//
// The physics blocks are a grid over the world model's box (PBlockSize
// from the world info string, or one block for a terrain section). Each
// block keeps a little BSP of its own: the world's BSP with every node
// whose plane cannot touch the block (its box grown by 200 units) cut
// out, saved as 6-byte SaveNodes. Physics-blocker polies
// (SURF_PHYSICSBLOCKER) not in the BSP yet are filtered into it first,
// and the BSP's original node links are put back at the end.
//
// The rest of the range is the template code this module emits, in this
// order: CMoArray<SaveNode>'s destructor and GenList virtuals
// (0x426b80..0x427150), CMoArray<PBNodeBox>'s (0x427150..0x427700),
// CMoArray<CNode>'s (0x427700..0x427cc0), then SetSize2,
// InternalNiceSetSize, Init and BaseNew of the three
// (0x427cc0..0x428302). The vtables: 0x474d9c (SaveNode), 0x474d6c
// (PBNodeBox), 0x474d3c (CNode).

#include "bdefs.h"
#include "processing.h"
#include "preworld.h"
#include "bspgen.h"
#include "node.h"
#include "parse_world_info.h"


// talon: a node of a physics block's BSP as the DAT file saves it (PreWorld.cpp
// has the same struct, local to it: CPreWorld::SavePBlockTable writes
// PBlock::m_pData as an array of them). The first member is the node's
// index in the world's node list (CNode::m_Index, 0x425ba0; PreWorld.cpp
// calls it m_iPlane); the sides are other SaveNodes of the block or
// SAVENODE_IN / SAVENODE_OUT.
#ifndef SAVENODE_IN
	#define SAVENODE_IN		0xFFFF
	#define SAVENODE_OUT	0xFFFE
#endif
struct SaveNode
{
	uint16	m_iPlane;
	uint16	m_Sides[2];
};

// talon: a BSP node's box, the box of its poly (0x426008..0x4261e2): 24
// bytes, BaseNew<PBNodeBox> (0x428290) runs no constructor.
struct PBNodeBox
{
	PVector	m_Min;
	PVector	m_Max;
};


// talon: per node of the world (by CNode::m_Index), whether some corner
// of the block being made is more than 0.1 in front of its plane
// (0x484fd0, set at 0x425fc1 and read by the recursion at 0x425b51).
CMoArray<uint8> *g_pPBNodeFlags;


// talon: 0x425b10. Appends the subtree under pNode to nodes, leaves first,
// and returns the index of its top (0xFFFF for a leaf, or when the subtree
// is all leaves; IN and OUT come back alike). A node whose flag is clear
// is dropped when one of its sides is a leaf: the other side stands for
// it. A leaf side is saved by position: IN behind (m_Sides[BackSide], 0)
// and OUT in front (0x425b9d..0x425bb5).
//
// Near: size-exact, but for the operand order of the add in the inline
// Insert2's GetNumAllocatedElements (0x425c7f: shipped loads m_nElements
// then the cache size, ours the other way round) -- the declaration-count
// kind PLAN.md records for ParseLightDefs. Include order, the function's
// position in the file and every Append spelling leave it.
uint16 MakePBlockNodes_R(CNode *pNode, CMoArray<SaveNode> &nodes)
{
	SaveNode node;

	if(pNode == NODE_IN || pNode == NODE_OUT)
		return 0xFFFF;

	node.m_Sides[0] = MakePBlockNodes_R(pNode->m_Sides[0], nodes);
	node.m_Sides[1] = MakePBlockNodes_R(pNode->m_Sides[1], nodes);

	if(!(*g_pPBNodeFlags)[pNode->m_Index])
	{
		if(node.m_Sides[0] != 0xFFFF)
		{
			if(node.m_Sides[1] == 0xFFFF)
				return node.m_Sides[0];
		}
		else
		{
			if(node.m_Sides[1] == 0xFFFF)
				return 0xFFFF;

			return node.m_Sides[1];
		}
	}

	node.m_iPlane = pNode->m_Index;
	if(node.m_Sides[0] == 0xFFFF)
		node.m_Sides[0] = SAVENODE_IN;
	if(node.m_Sides[1] == 0xFFFF)
		node.m_Sides[1] = SAVENODE_OUT;

	nodes.Append(node);
	return (uint16)(nodes.GetSize() - 1);
}


// talon: 0x425d20. Makes pWorld->m_PBlocks. bOneBlock (a terrain section,
// Terrain.cpp) makes one block over the world's box; otherwise the blocks
// are PBlockSize (pInfoString) on a side. bInitVertexIndices calls
// CPreWorld::InitPolyVertexIndices at the end (MakeBsps.cpp passes TRUE).
//
// Near (guide 11, structurally right): callseq 44 of 45 calls agree; the
// one left is the saveNodes.Term() at 0x4267eb, which the shipped build
// expands one level less (it calls _DeleteAndDestroyArray, 0x411640;
// ours expands that and calls BaseDelete). The inline budget decides the
// first three arrays' Init (expanded at 0x425d4b..0x425e15, D/E/F call
// Init) and that Term together; the spellings here are the ones that get
// the three Inits (VEC_* macros add budget, operators spend it; the
// center's dot product and the corner test as VEC_DOT, vHalf by
// operator*, vBlockMax by operator+ whose temporary the shipped code
// copies at 0x426424). Every spelling of the Term tried (SetSize(0),
// SetSize2(0, &g_DefAlloc), RemoveAll, GenRemoveAll) gives it the same
// depth or loses the Inits. The rest is register and frame allocation:
// the frame is 0xf8 shipped, 8 bytes less in ours, and the commutative
// operand order of the center's dot product (y, z, x shipped) follows
// the slots (measured: no parenthesisation moves it).
void MakePhysicsBlocks_425d20(CPreWorld *pWorld, BOOL bOneBlock, char *pInfoString, BOOL bInitVertexIndices)
{
	CMoArray<PVector>	corners;		// 0x34 (vtable 0x474510)
	CMoArray<SaveNode>	saveNodes;		// 0x80 (0x474d9c)
	CMoArray<PBNodeBox>	nodeBoxes;		// 0x4c (0x474d6c)
	CMoArray<uint8>		nodeFlags;		// 0xb0 (0x4746e8)
	CMoArray<CNode>		savedNodes;		// 0x9c (0x474d3c)
	CMoArray<CNode*>	nodeStack;		// 0xc4 (0x474590)
	GPOS pos;
	CNode *pNode;
	CPrePoly *pPoly;
	PBNodeBox *pBox;
	PBlock *pBlock;
	uint32 i, j;
	int x, y, z;
	int iStack;
	uint32 maxNodes, totalNodes;
	float fBlockSize, fDist, fRadius;
	PVector vMin, vMax, vBlockSize, vBlockMin, vBlockMax, vCenter, vDiff, vHalf;


	// Keep the BSP's links (0x425ec4..0x425f1c): filtering the physics
	// blockers in below changes them.
	savedNodes.SetSize(pWorld->m_Nodes.GetSize());
	pos = pWorld->m_Nodes;
	for(i=0; i < savedNodes; i++)
	{
		pNode = pWorld->m_Nodes.GetNext(pos);
		savedNodes[i].m_Sides[0] = pNode->m_Sides[0];
		savedNodes[i].m_Sides[1] = pNode->m_Sides[1];
	}

	// Physics blockers not in the BSP yet go in (0x425f1e..0x425f76).
	for(pos=pWorld->m_Polies; pos; )
	{
		pPoly = pWorld->m_Polies.GetNext(pos);

		if(pPoly->GetSurfaceFlags() & SURF_PHYSICSBLOCKER)
		{
			if(!NodeWithPoly(pWorld, pPoly))
				FilterPolyIntoBSP(pWorld, pWorld->m_RootNode, pPoly, FILTER_ALWAYSADD, NULL);
		}
	}

	maxNodes = 0;
	totalNodes = 0;
	SetNodeIndices(pWorld->m_Nodes);

	nodeFlags.SetSize(pWorld->m_Nodes.GetSize());
	g_pPBNodeFlags = &nodeFlags;

	// Each node's box, the box of its poly (0x425fce..0x4261f3).
	nodeBoxes.SetSize(pWorld->m_Nodes.GetSize());
	for(pos=pWorld->m_Nodes; pos; )
	{
		pNode = pWorld->m_Nodes.GetNext(pos);

		if(pNode->m_pPoly)
		{
			VEC_SET(nodeBoxes[pNode->m_Index].m_Min, MAX_CREAL, MAX_CREAL, MAX_CREAL);
			VEC_SET(nodeBoxes[pNode->m_Index].m_Max, -MAX_CREAL, -MAX_CREAL, -MAX_CREAL);

			for(j=0; j < pNode->m_pPoly->NumVerts(); j++)
			{
				VEC_MIN(nodeBoxes[pNode->m_Index].m_Min, nodeBoxes[pNode->m_Index].m_Min, pNode->m_pPoly->Pt(j));
				VEC_MAX(nodeBoxes[pNode->m_Index].m_Max, nodeBoxes[pNode->m_Index].m_Max, pNode->m_pPoly->Pt(j));
			}
		}
	}

	// The grid (0x4261f9..0x426334).
	fBlockSize = ParsePBlockSize(pInfoString);
	if(bOneBlock)
	{
		pWorld->m_PBlocks.m_nBlocks[0] = pWorld->m_PBlocks.m_nBlocks[1] = pWorld->m_PBlocks.m_nBlocks[2] = 1;
		pWorld->GetBoundingBox(&vMin, &vMax);
		VEC_SUB(vBlockSize, vMax, vMin);
	}
	else
	{
		vBlockSize.Init(fBlockSize, fBlockSize, fBlockSize);
		pWorld->GetBoundingBox(&vMin, &vMax);
		pWorld->m_PBlocks.m_nBlocks[0] = (int)((vMax.x - vMin.x) / fBlockSize) + 1;
		pWorld->m_PBlocks.m_nBlocks[1] = (int)((vMax.y - vMin.y) / fBlockSize) + 1;
		pWorld->m_PBlocks.m_nBlocks[2] = (int)((vMax.z - vMin.z) / fBlockSize) + 1;
	}

	VEC_COPY(pWorld->m_PBlocks.m_vMin, vBlockSize);
	VEC_COPY(pWorld->m_PBlocks.m_Unknown_0c, vMin);
	pWorld->m_PBlocks.m_nBlocksXY = pWorld->m_PBlocks.m_nBlocks[0] * pWorld->m_PBlocks.m_nBlocks[1];
	pWorld->m_PBlocks.m_nBlocksTotal = pWorld->m_PBlocks.m_nBlocks[2] * pWorld->m_PBlocks.m_nBlocks[0] * pWorld->m_PBlocks.m_nBlocks[1];
	pWorld->m_PBlocks.m_pBlocks = (PBlock*)dalloc_z(pWorld->m_PBlocks.m_nBlocksTotal * sizeof(PBlock));

	nodeStack.SetSize(0);
	corners.SetSize(8);

	for(x=0; x < (int)pWorld->m_PBlocks.m_nBlocks[0]; x++)
	{
		for(y=0; y < (int)pWorld->m_PBlocks.m_nBlocks[1]; y++)
		{
			for(z=0; z < (int)pWorld->m_PBlocks.m_nBlocks[2]; z++)
			{
				// The block's box, grown by 200 (0x4263aa..0x42649d).
				vBlockMin.Init(
					vMin.x + x * vBlockSize.x,
					vMin.y + y * vBlockSize.y,
					vMin.z + z * vBlockSize.z);
				vBlockMax = vBlockMin + vBlockSize;

				pBlock = &pWorld->m_PBlocks.m_pBlocks[x + y*pWorld->m_PBlocks.m_nBlocks[0] + z*pWorld->m_PBlocks.m_nBlocksXY];

				vBlockMin.x -= 200.0f;
				vBlockMin.y -= 200.0f;
				vBlockMin.z -= 200.0f;
				vBlockMax.x += 200.0f;
				vBlockMax.y += 200.0f;
				vBlockMax.z += 200.0f;

				// Its corners (0x42649d..0x42659d).
				VEC_SET(corners[0], vBlockMin.x, vBlockMin.y, vBlockMin.z);
				VEC_SET(corners[1], vBlockMin.x, vBlockMax.y, vBlockMin.z);
				VEC_SET(corners[2], vBlockMax.x, vBlockMin.y, vBlockMin.z);
				VEC_SET(corners[3], vBlockMax.x, vBlockMax.y, vBlockMin.z);
				VEC_SET(corners[4], vBlockMin.x, vBlockMin.y, vBlockMax.z);
				VEC_SET(corners[5], vBlockMin.x, vBlockMax.y, vBlockMax.z);
				VEC_SET(corners[6], vBlockMax.x, vBlockMin.y, vBlockMax.z);
				VEC_SET(corners[7], vBlockMax.x, vBlockMax.y, vBlockMax.z);

				// Its sphere (0x4265a0..0x42666a).
				VEC_SUB(vDiff, vBlockMax, vBlockMin);
				vHalf = vDiff * 0.5f;
				VEC_ADD(vCenter, vBlockMin, vHalf);
				fRadius = VEC_MAG(vDiff) * 0.5f;

				// Walk the BSP down from the root, both sides where the
				// sphere straddles a plane, flagging the solid nodes whose
				// box overlaps the block and whose plane has a corner of the
				// block in front (0x42664a..0x4267e6).
				memset(nodeFlags.GetArray(), 0, nodeFlags.GetSize());
				iStack = 0;
				pNode = pWorld->m_RootNode;
				for(;;)
				{
					while(pNode == NODE_IN || pNode == NODE_OUT)
					{
						if(iStack == 0)
							goto Done;

						pNode = nodeStack[--iStack];
					}

					fDist = VEC_DOT(pNode->m_pPlane->m_Normal, vCenter) - pNode->m_pPlane->m_Dist;

					if(pNode->m_pPoly->GetSurfaceFlags() & SURF_SOLID)
					{
						pBox = &nodeBoxes[pNode->m_Index];
						if(pBox->m_Min.x < vBlockMax.x && pBox->m_Min.y < vBlockMax.y && pBox->m_Min.z < vBlockMax.z &&
							pBox->m_Max.x > vBlockMin.x && pBox->m_Max.y > vBlockMin.y && pBox->m_Max.z > vBlockMin.z)
						{
							for(i=0; i < corners; i++)
							{
								if(VEC_DOT(pNode->m_pPlane->m_Normal, corners[i]) - pNode->m_pPlane->m_Dist > 0.1)
								{
									nodeFlags[pNode->m_Index] = 1;
									break;
								}
							}
						}
					}

					if(fDist > fRadius)
					{
						pNode = pNode->m_Sides[FrontSide];
					}
					else if(fDist < -fRadius)
					{
						pNode = pNode->m_Sides[BackSide];
					}
					else
					{
						if(iStack+1 >= (int)nodeStack.GetSize())
							nodeStack.Append(pNode->m_Sides[BackSide]);

						nodeStack[iStack] = pNode->m_Sides[BackSide];
						pNode = pNode->m_Sides[FrontSide];
						iStack++;
					}
				}

Done:;

				// The block's BSP (0x4267eb..0x4268c1).
				saveNodes.Term();
				saveNodes.SetCacheSize(30);
				pBlock->m_iRoot = MakePBlockNodes_R(pWorld->m_RootNode, saveNodes);

				if(saveNodes.GetSize() > maxNodes)
					maxNodes = saveNodes.GetSize();
				totalNodes += saveNodes.GetSize();

				pBlock->m_pData = dalloc(saveNodes.GetSize() * sizeof(SaveNode));
				memcpy(pBlock->m_pData, saveNodes.GetArray(), saveNodes.GetSize() * sizeof(SaveNode));
				pBlock->m_nNodes = (uint16)saveNodes.GetSize();
			}
		}
	}

	// Put the BSP's links back (0x426901..0x426954).
	pos = pWorld->m_Nodes;
	for(i=0; i < savedNodes; i++)
	{
		pNode = pWorld->m_Nodes.GetNext(pos);
		pNode->m_Sides[0] = savedNodes[i].m_Sides[0];
		pNode->m_Sides[1] = savedNodes[i].m_Sides[1];
	}

	if(bInitVertexIndices)
		pWorld->InitPolyVertexIndices();

	DrawStatusTextIfMainWorld("%d bytes for physics, %d blocks (%d average nodes per block, %d max)",
		totalNodes*sizeof(SaveNode) + pWorld->m_PBlocks.m_nBlocksTotal*sizeof(PBlock),
		pWorld->m_PBlocks.m_nBlocksTotal,
		totalNodes / pWorld->m_PBlocks.m_nBlocksTotal,
		maxNodes);
}
