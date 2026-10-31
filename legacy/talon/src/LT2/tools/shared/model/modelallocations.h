#ifndef __MODELALLOCATIONS_H__
#define __MODELALLOCATIONS_H__

class ModelAllocations;

#ifndef __MODEL_H__
#include "model.h"
#endif



// --------------------------------------------------------------------------
// ModelAllocations
// Utility class that counts the size of a model.
// (The engine needs to know how big a model is going to be.)
// --------------------------------------------------------------------------
	class ModelAllocations
	{
	public:

					ModelAllocations();

		// Clear everything.
		void		Clear();
		
		// Tally up everything in the model.
		LTBOOL		InitFromModel(Model *pModel);
		
		// Load/save.
		// talon: 2001's (0x432f16, 0x432fcd): thirteen counts, read and
		// written in order; no vertex animation or animation data sizes.
		LTBOOL		Load(ILTStream &str);
		LTBOOL		Save(ILTStream &str);

	public:

		// Note that the AnimInfos aren't handled in here because it can't pre-store
		// how many AnimInfos it will need to allocate.
		
		uint32		m_nKeyFrames;		// Number of keyframes.
		uint32		m_nParentAnims;		// Number of animations (that come from us).
		uint32		m_nNodes;			// Number of nodes.
		uint32		m_nPieces;			// Number of pieces.
		uint32		m_nChildModels;		// Number of child models (including the self child model).
		uint32		m_nTris;			// Number of triangles.
		uint32		m_nVerts;			// Number of vertices.
		uint32		m_nVertexWeights;	// Number of vertex weights.
		uint32		m_nLODs;			// Number of LODs.
		uint32		m_nSockets;			// Number of sockets.
		uint32		m_nWeightSets;		// Number of weight sets.

		uint32		m_nStrings;			// How many strings we're allocating.
		uint32		m_StringLengths;	// Sum of all string lengths (not including null terminator).
		
	};


#endif



