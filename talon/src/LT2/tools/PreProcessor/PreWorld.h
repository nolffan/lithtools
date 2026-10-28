//------------------------------------------------------------------
//
//	FILE	  : PreWorld.h
//
//	PURPOSE	  : Defines the CPreWorld class, which is used
//              in the preprocessing phase.
//
//	CREATED	  : 2nd May 1996
//
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------

#ifndef __PREWORLD_H__
#define __PREWORLD_H__


// Includes....
#include "bdefs.h"
#include "node.h"
#include "prepoly.h"
#include "preblockerpoly.h"
#include "editobjects.h"
#include "presurface.h"
#include "de_mainworld.h"

#include <string>
#include <vector>

#define MAINWORLD_MODEL_NAME		"MAIN_WORLD"
#define DEFAULT_CONVEX_THRESHOLD	0.0001f

// Defines....
class CPrePoly;
class CPreLeaf;
class CPreWorld;
class CPreMainWorld;
class CLTANode;
class CLTANodeBuilder;
class CPreLightMap;

// Functions in this module.
void SetNodeIndices(CNodeList &theList);
void CheckNodes(CNode *pRoot);

class CPreLightFrame
{
public:
							CPreLightFrame();
							~CPreLightFrame();
	
	// One lightmap for each poly..
	CMoArray<CPreLightMap*>	m_PolyMaps;

	// talon: 2001; one for each poly too, made with m_PolyMaps by the
	// lightmap maker (0x418b70). What it holds is not known yet.
	CMoArray<CMoArray<uint32>*>	m_Unknown_14;
};


class CPrePolyRef
{
public:
	CPrePolyRef() {}
	CPrePolyRef(const CPrePoly *pPoly) : m_iWorld(pPoly->m_WorldIndex), m_iPoly(pPoly->m_Index) {}
	bool operator==(const CPrePolyRef &cOther) const { return (m_iWorld == cOther.m_iWorld) && (m_iPoly == cOther.m_iPoly); }
	bool operator!=(const CPrePolyRef &cOther) const { return !(*this == cOther); }
	uint32		m_iWorld;
	uint32		m_iPoly;
};


class CPreBlindData
{
public:
	CPreBlindData( uint8* data, uint32 size, uint32 id ) : data(data), size(size), id(id) {}
	~CPreBlindData() { delete [] data; }

	uint8* data;		// dynamically allocated chunk of memory
	uint32 size;		// size of that dynamically allocated chunk of memory
	uint32 id;			// blind data ID for debugging purposes (game code must present a matching ID, otherwise it asserts)
};


class CPreLightAnim
{
public:
								CPreLightAnim();
								~CPreLightAnim();

	uint32						CalcFrameDataSize();

public:
	
	char						m_Name[MAX_LIGHTANIMNAME_LEN];
	CMoArray<CPreLightFrame*>	m_Frames;
	CMoArray<CPrePolyRef>		m_Polies;		// Polies that this LightAnim affects.

	// talon: CreateLightAnim's bShadowMap.
	LTBOOL						m_bShadowMap;
};


struct CPreLightGroup_PolyData
{
	CPreLightGroup_PolyData();
	~CPreLightGroup_PolyData();
	CPreLightGroup_PolyData(const CPreLightGroup_PolyData &cOther);
	CPreLightGroup_PolyData &operator=(const CPreLightGroup_PolyData &cOther);

	CPrePolyRef	m_hPoly;
	typedef std::vector<uint8> TVertexData;
	TVertexData m_aVertexIntensities;
	CPreLightMap *m_pLightmap;
};

class CPreLightGroup
{
public:
	typedef std::vector<CPreLightGroup_PolyData> TPolyDataList;

public:
	const char *GetName() const { return m_sName.c_str(); }
	void SetName(const char *pName) { m_sName = pName; }

	// Normalized color of this light group [0-1]
	const LTVector &GetColor() const { return m_vColor; }
	void SetColor(const LTVector &vColor) { m_vColor = vColor; }

	// The lighting data for each poly affected by this group
	TPolyDataList &GetPolyData() { return m_aPolyData; }
	const TPolyDataList &GetPolyData() const { return m_aPolyData; }

	// The ambient lighting grid data
	struct SLightGrid
	{
		SLightGrid() : m_pSamples(0) {}
		~SLightGrid() { delete[] m_pSamples; }
		SLightGrid(const SLightGrid &cOther) { InternalCopy(cOther); }
		SLightGrid &operator=(const SLightGrid &cOther) 
		{
			if (this == &cOther)
				return *this;
			delete[] m_pSamples;
			InternalCopy(cOther);
			return *this;
		}

		uint32 GetTotalSampleCount() const { return m_vSize.x * m_vSize.y * m_vSize.z; }

		// Offset and size, relative to the main world's light grid
		_CVector<uint32> m_vOffset, m_vSize;
		// Sample data
		uint8 *m_pSamples;

	private:
		void InternalCopy(const SLightGrid &cOther)
		{
			m_vOffset = cOther.m_vOffset;
			m_vSize = cOther.m_vSize;
			if (cOther.m_pSamples)
			{
				m_pSamples = new uint8[GetTotalSampleCount()];
				memcpy(m_pSamples, cOther.m_pSamples, GetTotalSampleCount());
			}
			else
				m_pSamples = 0;
		}
	};
	SLightGrid &GetLightGrid() { return m_cLightGrid; }
	const SLightGrid &GetLightGrid() const { return m_cLightGrid; }

private:
	std::string		m_sName;
	LTVector		m_vColor;

	TPolyDataList	m_aPolyData;
	SLightGrid		m_cLightGrid;
};


// talon (W2): a poly's light animation: the animation's index in the main
// world's m_LightAnims and the poly's in its m_Polies (the names are ours).
struct PolyAnimRef
{
	uint16		m_iAnim;
	uint16		m_iRef;
};

// talon (W2): the physics blocks, a grid of boxes over the world (made
// by the physics module, 0x425d20; the names are ours).
struct PBlock
{
	void		*m_pData;		// W3: a little BSP, m_nNodes SaveNodes.
	uint16		m_nNodes;
	uint16		m_iRoot;
};

struct PBlockTable
{
	PVector		m_vBlockSize;	// talon: the block size (MakePhysicsBlocks, 0x425d20)
	PVector		m_vMin;			// talon: the grid's origin
	uint32		m_nBlocks[3];
	uint32		m_nBlocksXY;
	uint32		m_nBlocksTotal;
	PBlock		*m_pBlocks;

				PBlockTable()	{Clear();}

	// talon, W1: zeroes it (0x433b80; the name is ours).
	void		Clear();
};



// talon (W2): what CPreWorld::m_PolyArray holds in 2001: 0x6c bytes with
// polies and nodes of its own (constructor 0x433af0; LoadSubBsp reads it).
// The names are ours. (W3: laid out from LoadSubBsp and SaveSubBsp; a BSP
// saved after the world's own, with its own physics blocks.)
class CPreSubBsp
{
public:

							CPreSubBsp();

	uint8					m_Unknown_00[4];
	uint32					m_Unknown_04;
	CMoArray<CPrePoly*>		m_Polies;
	NODEREF					m_RootNode;
	CNodeList				m_Nodes;
	uint32					m_Unknown_2c;
	PVector					m_Unknown_30;
	PBlockTable				m_PBlocks;
};


// talon, W3: what CPreWorld::MakePolyEdges fills (the names are ours).
struct PolyEdges
{
					PolyEdges()	{m_pEdges = NULL;}
					// Inline in SaveFile; out of line at 0x437f80 (SaveFile's unwind).
					~PolyEdges()
					{
						uint32 i;

						if(m_pEdges)
						{
							for(i=0; i < m_nPolies; i++)
								delete m_pEdges[i];

							delete m_pEdges;
						}
					}

	uint32			m_nPolies;
	uint16			**m_pEdges;		// Per poly, an edge index per vertex.
};

class CPreWorld
{
public:

						CPreWorld();
						CPreWorld(CPreMainWorld *pMainWorld);
						~CPreWorld();

private:
	
	void				Construct(CPreMainWorld *pMainWorld);


public:

	// Member functions
	void				Term();
	void				TermGeometry();

	// Gets a world vertex given a poly index.
	PVertex&			PolyPt(CPrePoly *pPoly, uint32 i);

	// Sets up all the vertex indices for CPrePolies.
	void				InitPolyVertexIndices();
	
	void				RemoveUnusedGeometry();
	void				RemoveUnusedPlanes();
	void				RemoveUnusedSurfaces();
	BOOL				RemoveUnusedPoints();

	uint32				TagUsedSurfaces();

	// talon (W2): helpers of SaveFile (0x4361f0, 0x436400; the names are ours).
	BOOL				GetPolyLightAnims(CMoArray< CMoArray<PolyAnimRef> > &polyAnims, uint32 &nRefs);
	void				GetUniqueSurfaces(CMoArray<CPreSurface*> &surfaces, CMoArray<uint32> &surfaceMap);
	void				SetupIndexMap( CMoByteArray &pointsUsed, CMoWordArray &indexMap, uint32 &nPointsUsed );

	CPrePlane*			FindOrAddPlane(PVector normal, PReal dist);
	CPrePlane*			AddPlane(PVector &normal, PReal dist);

	void				SetPolyIndices();
	void				SetPlaneIndices();

	// talon, W1 (called by CPreMainWorld's): a world model's part of the DAT
	// file (0x434630, 0x436ad0; the names are ours). LoadFile returns 0 if
	// it loaded.
	uint32				LoadFile(CAbstractIO &file);

	// talon (W2): reads one of m_PolyArray's (0x437fc0, thiscall; the name
	// is ours). polies is LoadFile's poly-by-index array.
	BOOL				LoadSubBsp(CAbstractIO &file, CMoArray<CPrePoly*> &polies, CPreSubBsp *pBsp);
	BOOL				SaveFile(CAbstractIO &file);

	// talon, W3: m_Leaves[i]->m_Index = i (0x4394d0; the name is ours).
	void				SetLeafIndices();

	// talon, W3: each poly's edges as indices into the world's edges, each
	// edge once; returns the number of edges (0x439050; the name is ours).
	uint32				MakePolyEdges(PolyEdges &edges);
	uint16				FindEdge(CMoArray<struct EdgeHashEntry*> &edgeHash, uint32 iPt1, uint32 iPt2);	// 0x4392a0
	
	void				GetTextureNames( CMoArray<const char*> &texNames );
	uint16				FindTextureName( const char *pName, CMoArray<const char*> &texNames );
	
	void				SetupSurfaceTextureVectors();
	void				SetupSurfaceLMVectors();
	void				MinimizeSurfaceTCoords();

	void				GetBoundingBox(PVector *pMin, PVector *pMax);
	void				UpdateBoundingBox();
	// talon: 0x43a290, the bounding boxes of the BSP nodes from the root
	// down (name provisional).
	// talon, W3: the box under pNode; sets the nodes' spheres (0x439e60).
	BOOL				CalcNodeBoundingBoxes_R(NODEREF pNode, PVector &vMin, PVector &vMax);
	void				CalcNodeBoundingBoxes();
	// talon: 0x43ab10 and 0x43a2b0 (names provisional).
	uint32				NumNodes();
	void				TermLeaves();
	
	int					GetTreeDepth();

	// talon, P: counts for the processor's stats (0x43aad0, 0x43ab60,
	// 0x43abc0; the names are ours).
	uint32				NumDetailNodes();
	uint32				CalcVisListMemory();
	uint32				NumVisibleLeaves();

	void				RemoveAllNodes();

// Polygon joining stuff.
public:

	void				JoinPolyList(CGLinkedList<CPrePoly*> &polies, PReal convexThreshold=DEFAULT_CONVEX_THRESHOLD);
	void				JoinPolies(PReal convexThreshold);
	
	CPrePoly*			JoinPolygons(	CPrePoly *pPoly1, CPrePoly *pPoly2, 
										uint16 poly1v1, uint16 poly2v1, 
										uint16 poly1v2, uint16 poly2v2);


// Accessors
public:

	CNode*				GetNode(NODEREF node) {ASSERT(IsValidNode(node)); return node;}
	

public:

	uint32				NumPolyVerts();

	// talon, W3: counts for the DAT file (0x43aba0, 0x43abf0; the names
	// are ours).
	uint32				CalcVisListCount();
	uint32				NumLeafPolies();

	// talon, W3: the box of the polies on pUser's surface (0x43ac10), and
	// each leaf's sphere around its portals (0x43ad20). The names are ours.
	void				GetSurfaceBoundingBox(struct PreSurfaceUser *pUser, PVector *pMin, PVector *pMax);
	void				CalcLeafSpheres();

	// talon, W3: the rest of the DAT file's (0x4384a0, 0x4386d0, 0x438830;
	// the names are ours).
	void				SaveSubBsp(CAbstractIO &file, CPreSubBsp *pBsp);
	BOOL				LoadPBlockTable(CAbstractIO &file, PBlockTable *pTable);
	void				SavePBlockTable(CAbstractIO &file, PBlockTable *pTable);

	void				FindTextureOrigins();

	void				SetPolyWorldIndices();

	CPrePoly *			FindPolyByIndex(uint32 nIndex);

public:

	// Parent world...
	CPreMainWorld				*m_pMainWorld;

	// The 2001 class (its constructor, 0x433bb0) has a list, two arrays
	// and a 48-byte structure that Build 69's does not; their types and
	// use are not known yet.
	CGLinkedList<void*>			m_Unknown_04;

	// Description of the world.
	CGLinkedList<CPrePoly*>		m_Polies;

	// The leaves hull making found (CPreLeaf::m_Index indexes it).
	CMoArray<CPreLeaf*>			m_Leaves;
	CMoArray<void*>				m_Unknown_30;

	// This list is setup in BrushToWorld then emptied after BSP generation.
	CGLinkedList<CPrePoly*>		m_OriginalBrushPolies;

	// talon: the physics blocks (W2).
	PBlockTable					m_PBlocks;

	CGLinkedList<CPrePlane*>	m_Planes;
	CGLinkedList<CPreSurface*>	m_Surfaces;
	CNodeList					m_Nodes;

	// Bounding box enclosing all geometry.
	PVector						m_PosMin, m_PosMax;

	// Objects..
	PVertexArray				m_Points;

	// The root for the world's BSP tree.
	NODEREF						m_RootNode;

	CMoArray<const char*>		m_TextureNames;

	uint32						m_Unknown_e8;
	
	// This holds all the names for stuff in the world -- mainly texture names.
	CStringHolder				m_StringHolder;
	
	// The model's name.
	char						m_WorldName[MAX_WORLDNAME_LEN];

	// When you call SaveWorld(), this is set.
	uint32						m_nPointsSaved;

	// This world's translation.  This is used for centering world models.
	PVector						m_WorldTranslation;

	uint32						m_WorldInfoFlags;

	// Extra poly array for indexed look-ups
	// talon (W2): sub-BSPs in 2001 (LoadFile news them).
	CMoArray<CPreSubBsp*>		m_PolyArray;
};


// This is the container for all the world models in a world.
class CPreMainWorld
{
public:

						CPreMainWorld();
						~CPreMainWorld();
	
	void				Term();
	void				TermGeometry();

 
	// Get a poly given a PrePolyRef.
	CPrePoly*				GetLMPoly(const CPrePolyRef *pRef);
	const CPrePoly*			GetLMPoly(const CPrePolyRef *pRef) const;

	CPreWorld*				FindWorldModel(const char *pName);

	// These just call thru to all their worlds.
	void					MinimizeSurfaceTCoords();
	
	uint32					CalcLMDataSize();
	void					RemoveAllLightMaps();

	// talon: fills each world model's poly array (0x30) from its poly
	// list (0x432d70; the name is ours).
	void					SetupPolyArrays();

	void					RemoveLightAnim(CPreLightAnim *pAnim);

	// talon: the light animation named pName, or NULL (0x433620).
	CPreLightAnim*			FindLightAnim(const char *pName);

	// talon, W1: whether the light animations fit the world (0x432e90).
	BOOL					CheckLightAnims();

	// talon, W1: removes the AIVolumes' and MusicVolumes' world models
	// (0x433000).
	void					RemoveAIVolumes();

	// talon, W1: the lightmaps' size over their compressed size (0x433520).
	PReal					CalcLMCompressionRatio();

	// Get the physics and visibility BSPs. Just searches for a CPreWorld with
	// WIF_PHYSICSBSP or WIF_VISBSP.
	CPreWorld*				GetPhysicsBSP();
	const CPreWorld*		GetPhysicsBSP() const;
	CPreWorld*				GetVisBSP();							// talon, W1: 0x433810

	//goes through all world models, and removes all unused geometry
	//associated with the world model. This will also update the bounding
	//boxes of the world models if bUpdateBoundingBoxes is true;
	void					RemoveAllUnusedGeometry(bool bUpdateBoundingBoxes = true);


	//updates the bounding box for the main world model. This assumes that all the 
	//preworlds have up to date bounding boxes.
	void					UpdateBoundingBox();

	// Find the index for a world
	uint32					GetWorldIndex(const CPreWorld *pWorld) const;

	// Get a world by index
	CPreWorld*				GetWorld(uint32 nIndex) { ASSERT(nIndex < m_WorldModels.GetSize()); return m_WorldModels[nIndex]; }
	const CPreWorld*		GetWorld(uint32 nIndex) const { ASSERT(nIndex < m_WorldModels.GetSize()); return m_WorldModels[nIndex]; }

	// Collect the blocker polys from the world
	// Returns the number of polys found
	uint32					CollectBlockerPolys();

	// collect the particle blocker polys from the world and return the number found
	uint32					CollectParticleBlockerPolys();

	// talon: the lightmap grid size (the lighting reads it through this).
	PReal					GetLMGridSize() const	{return m_LMGridSize;}

	// talon, W1: the DAT file (0x431470, 0x431c00, 0x431820; the names are
	// ours). LoadFile returns 0 if it loaded, 1 for a wrong version, 2 if
	// it is corrupt (or a world model's error). SaveObjectsOnly copies
	// inFile, a DAT file, to outFile with these objects.
	virtual uint32			LoadFile(CAbstractIO &file, BOOL bLoadObjects);
	virtual BOOL			SaveFile(CAbstractIO &file);
	virtual BOOL			SaveObjectsOnly(CAbstractIO &outFile, CAbstractIO &inFile);

	// talon, W1: the light animations' part of it (0x432380, 0x432a40), and
	// the vis BSP's leaves' polies after loading (0x433880).
	BOOL					LoadLightAnims(CAbstractIO &file);
	BOOL					SaveLightAnims(CAbstractIO &file);
	BOOL					SetupLeafPolies();

public:

	// talon: the 2001 layout, from its constructor (0x431060, inlined in
	// DoProcessing). m_Unknown_* are members whose use is not known yet.
	// All the light animations (including the base one).
	CMoArray<CPreLightAnim*>		m_LightAnims;

	// No objects are stored in here, but this is created and maintained 
	// so at runtime it knows how to create the WorldTree.
	WorldTree						m_WorldTree;

	CMoArray<CBaseEditObj*>			m_Objects;
	
	// Terrain and WorldModel BSPs.
	CMoArray<CPreWorld*>			m_WorldModels;

	char							*m_pInfoString;

	// Box enclosing all geometry.
	PVector							m_PosMin, m_PosMax;

	CStringHolder					m_StringHolder;

	uint32							m_Unknown_f0;

	// The lightmap grid size: texture pixels to a lightmap pixel.
	PReal							m_LMGridSize;
};

// Builds poly lists for poly groups that 'look' like surfaces.
void BuildSurfacePolyLists(CGLinkedList<CPreSurface*> &surfaces, CGLinkedList<CPrePoly*> &polies);
void TermSurfacePolyLists(CGLinkedList<CPreSurface*> &surfaces);

//utility functions for conversion between nodes and indices
// talon, W3: inline in 2001.
inline uint32 NodeToIndex(NODEREF node)
{
	if(node == NODE_IN)
		return (uint32)-1;
	else if(node == NODE_OUT)
		return (uint32)-2;
	else
		return node->m_Index;
}


// ------------------------------------------------------------------------ //
// Inlines.
// ------------------------------------------------------------------------ //





#endif


