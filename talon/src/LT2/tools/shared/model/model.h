

#ifndef __MODEL_H__
#define __MODEL_H__
#pragma warning (disable:4786)

    #ifndef __BDEFS_H__
    #include "bdefs.h"
    #endif

    #ifndef __QUATERNION_H__
    #include "ltrotation.h"
    #endif

    #ifndef __LTANIMTRACKER_H__
    #include "ltanimtracker.h"
    #endif

    #ifndef __MODELALLOCATIONS_H__
    #include "modelallocations.h"
    #endif

	// talon: no STL here in 2001: <string> (or anything else that brings in
	// <xlocale>) would give every file that includes this header the
	// ctype<unsigned short>::id initializer, and model.cpp's module
	// (0x429636) and transformmaker's (0x435f3d) have none.

	// How the vertex weighting works:
	// M = skeleton transform for the given frame
	// r = vertex->skeleton relation for this vertex (vertex->skeleton)
	// v = original (global) vertex position for this vertex
	// w = envelope weight for this vertex (vertex->skeleton)
	// offset = ((M * r) - v) * w
	// offset = (w*(M * r) - w*v)
	// a = w * r  (for each vertex)...
	// b = w * v
	// offset = M * a - b
	// The full equation, with 3 bones
	// n = v + (M1 * a1 - b1) + (M2 * a2 - b2) + (M3 * a3 - b3)
	// s = v - b1 - b2 - b3
	// n = s + M1*a1 + M2*a2 + M3*a3



	//------------------------------------------------------------------ //
	// Defines.
	//------------------------------------------------------------------ //

	#define PIECELOD_BASE			0

	#define MAX_GVP_ANIMS			8

	#define MAX_PIECENAME_LEN		32
	#define MAX_SOCKETNAME_LEN		16
	#define MAX_CHILD_MODELS		16	// talon: 2001 (InitChildInfo 0x42c4e6)
	#define MAX_WEIGHTSETNAME_LEN	16

	#define MAX_PIECE_TEXTURES		4

	#define MODEL_FILE_VERSION		25

	// Model node types.
	#define MNODE_REMOVABLE		(1<<0)	// This node can be removed.
	#define MNODE_ROTATIONONLY	(1<<1)	// Only use rotation info from animation data.

	// Key types.
	#define KEYTYPE_POSITION	0
	#define KEYTYPE_CALLBACK	1

	#define VC_NOTUSED	((uint32)0xFFFFFFFF)
	#define SN_NOTUSED	((uint32)0xFFFFFFFF)

	#define NODEPARENT_NONE		0xFFFFFFFF
	
	#define MODELFLAG_TOUCHED	(1<<0)	// Used by client.
	#define MODELHEADER_SIZE	8
	
	class ModelNode;
	class ModelAnim;
	class AnimKeyFrame;
	class Model;
	class ModelLoadRequest;
	class SharedTexture;
	class TransformMaker;
	class ModelPiece;
	class PieceLOD ;
	
	typedef unsigned char KeyType;
	typedef void (*KeyCallback)(void *pTracker, AnimKeyFrame *pFrame);


	// pRequest is a load request with everything setup except m_pFile.
	// The filename specified only has to be looked for in the same directory that the parent model is in.
	// ppModel is what you should fill in with the model you loaded.
	// Returns LT_OK if the model loaded ok.
	// Returns LT_NOCHANGE if it didn't load the model but it's not a fatal error.
	// Any other return signals an error and means to stop loading.  It will return the given error.
	typedef LTRESULT (*LoadChildFn)(ModelLoadRequest *pRequest, Model **pModel);

	// Default implementation.. just returns LT_NOCHANGE.
	LTRESULT DefaultLoadChildFn(ModelLoadRequest *pRequest, Model **pModel);

	
	// ------------------------------------------------------------------------
	// GVPStruct 
	// render/animation set up structure. Sets up what is sent down to renderer.
	// ------------------------------------------------------------------------
	class GVPStruct
	{
	public:
					GVPStruct()
					{
						m_Vertices = NULL;
						m_VertexStride = 0;
						Mat_Identity(&m_BaseTransform);
						m_nAnims = 1;
						m_iLOD = PIECELOD_BASE;
					}

		
		// What frame we're asking for.
		AnimTimeRef	m_Anims[MAX_GVP_ANIMS];
		uint32		m_nAnims;

		// Here is where the vertices go.  There must be room in here for all the vertices in the model.
		// The vertices are output in order of the m_FlatNodeList nodes (preorder traversal).
		// m_DestVertices points at the first LTVector.  m_VertexStride tells how many
		// bytes are in between each vertex.
		void		*m_Vertices;
		uint32		m_VertexStride;

		// Global transform applied to all verts.
		LTMatrix		m_BaseTransform;

		// What LOD to get the vertex positions for.
		// talon: 2001 (gl_modelrender's reads, 0x403c64): m_iLOD at 0x16c is
		// the last member; Build 69's m_CurrentLODDist isn't there.
		uint32		m_iLOD;
	};


	// ------------------------------------------------------------------------
	// ModelLoadRequest
	// ------------------------------------------------------------------------
	// talon: 0x24 bytes in 2001 (the inline constructor at 0x417678, part
	// 4's DoLoadModel 0x417194): m_pLoadedSet at 0xc and LTBOOL flags.
	class ModelLoadRequest
	{
	public:
		
							ModelLoadRequest()
							{
								m_pFilename = NULL;
								m_pFile = NULL;
								m_LoadChildFn = DefaultLoadChildFn;
								m_pLoadFnUserData = NULL;
								m_bLoadChildModels = TRUE;
								
								m_bTreesValid = TRUE;
								m_bSaveCountValid = TRUE;
								m_bAllChildrenLoaded = TRUE;
								m_pLoadedSet = NULL;
							}

		LTBOOL				IsValid() {return m_pFilename && m_pFile && m_LoadChildFn;}


	// Stuff to be set for Model::Load.
	public:
		
		// The file to load from..
		ILTStream				*m_pFile;
		
		// The model loader will call this when it wants a child model.
		LoadChildFn			m_LoadChildFn;
		void				*m_pLoadFnUserData;

		// talon: a container the caller news before Model::Load (0x417194;
		// the name is part 4's). Its type is the caller's and the loader's to
		// settle: model.h can't name it, since a map of strings would need
		// <string> and model.cpp and transformmaker have no ctype initializer.
		void				*m_pLoadedSet;


	// Stuff automatically set for m_LoadChildFn.
	public:

		// When m_LoadChildFn is called, this is set to the filename it wants.
		char				*m_pFilename;

		// Don't touch this.  The model loader uses it so it doesn't recursively load children.
		LTBOOL				m_bLoadChildModels;


	// Return info.
	public:

		// If this is FALSE, there was an error loading a ChildInfo tree.  The model 
		// still loaded but at least one of the ChildInfos has an invalid tree (and can't
		// be used unless it is rebuilt with SetupChildNodes).
		LTBOOL				m_bTreesValid;

		// If this is FALSE, then one of the child models is out of date.
		LTBOOL				m_bSaveCountValid;

		// If this is FALSE, then one or more child models couldn't load.
		LTBOOL				m_bAllChildrenLoaded;
	};

	// ------------------------------------------------------------------------
	// ModelString
	// Cell or node in model string list
	// ------------------------------------------------------------------------
	struct ModelString_t
	{
		uint32					m_AllocSize;
		struct ModelString_t	*m_pNext;
		char					m_String[1];
	} ;
	// backwards compat
	typedef ModelString_t ModelString;

	// ------------------------------------------------------------------------
	//  ModelSphere
	// ------------------------------------------------------------------------
	struct ModelSphere_t
	{
		LTVector	m_Center;
		float	m_Radius;
	} ;
	// backwards compat
	typedef ModelSphere_t ModelSphere;

	// ------------------------------------------------------------------------
	// NewVertexWeight
	// xyz in iNode's local space + weight + matrix 
	// a vertex to matrix association. for doing skeletal deformation
	// ------------------------------------------------------------------------
	class NewVertexWeight
	{
	public:
				NewVertexWeight();
	public:
		float	m_Vec[4];
		uint32	m_iNode;
	};

	// ------------------------------------------------------------------------
	// CDefVertexLst
	// This is a frame of animation for vertex animation
	// (for now all vals are floats)
	// ------------------------------------------------------------------------
	class CDefVertexLst {
	public :

		CDefVertexLst();
		CDefVertexLst(LAlloc *alloc):m_pAlloc(alloc) {;}
		CDefVertexLst( int size, LAlloc *alloc );
		~CDefVertexLst();

		// pre set the size of the array
		void setSize( int size , LAlloc *alloc );
		
		// size of the list
		int  size()                                  { return m_vVertexList.GetSize();}
		// memory size of data.
		int mem_size() 								 { return m_vVertexList.GetSize() * sizeof(defVert) ; }
		
		// get value at index (copy of data )
		void getValue( int index, float val[3] ) ;   
		
		// get value at index (pointer to actual data )
		// the value may not be continuous, so allways use getValue to get the next elem.
		float* getValue( int index );
		
		// kludge/ accessor to raw data.
		float* getArray()                  { return (float*)m_vVertexList.GetArray(); }
		
		// add values at the end of the list
		void append( float val[3] );
		void append( float x, float y, float z);

	private :
		// internal type 
		struct defVert {
			float pos[3];
		};
		
		// data array
		CMoArray<defVert, NoCache> m_vVertexList ;
		LAlloc                    *m_pAlloc ;
	};
	
	// ------------------------------------------------------------------------
	// ModelVert
	// Model Vertex. encompasses weight values base pos, and normal
	// ------------------------------------------------------------------------
	class ModelVert
	{
	public:
							ModelVert();

		NewVertexWeight*	AddWeight();


	public:

		NewVertexWeight	*m_Weights;
		uint16			m_nWeights;
		uint16			m_iReplacement; // This tells what vertex in the next LOD model
										// this vertex ends up collapsing into.  This is
										// used while transitioning the model between LODs.
										// NOTE: this is not saved.  It is only used by the
										// LOD generation code.  Xbox code was using it, but
										// it is effectively dead.

		// Vertex position.
		LTVector			m_Vec;

		// The vertex normal.
		LTVector			m_Normal;
	};


	typedef struct UVPair_t
	{
		float tu, tv;
	} UVPair;


	// talon: 0x20 bytes in 2001 (the indices, a pad, the UVs at 8:
	// gl_modelrender 0x404d86, model_cleanup 0x42fa40): no m_Normals or
	// m_Colors.
	class ModelTri
	{
	public:
		
		void			operator=(const ModelTri &other)
		{
			uint32 i;
			for(i=0; i < 3; i++)
			{
				m_Indices[i] = other.m_Indices[i];
				m_UVs[i] = other.m_UVs[i];
			}
		}
		
		unsigned short	m_Indices[3];
	
		UVPair			m_UVs[3];
	};

	// ------------------------------------------------------------------------
	// NodeKeyFrame
	// data for AnimNode animation data 
	// ------------------------------------------------------------------------
	// talon: 0x1c bytes in 2001 (the 0x1c stride at 0x429647; transformmaker
	// 0x436115): no m_Time, no m_pDefVertexLst, no constructor.
	class NodeKeyFrame
	{
	public:
		// talon: no default constructor in 2001: CMoArray's BaseNew for it
		// only allocates (0x41219d, which Insert2 0x42e87d calls).
		void		operator=(const NodeKeyFrame &other)
		{
			m_Translation = other.m_Translation;
			m_Quaternion = other.m_Quaternion;
		}

		void		ConvertToMatrix(LTMatrix &mat)
		{
			m_Quaternion.ConvertToMatrix(mat);
			mat.m[0][3] = m_Translation.x;
			mat.m[1][3] = m_Translation.y;
			mat.m[2][3] = m_Translation.z;
		}

		void		ConvertFromMatrix(LTMatrix &mat)
		{
			m_Quaternion.ConvertFromMatrix(mat);
			m_Translation.x = mat.m[0][3];
			m_Translation.y = mat.m[1][3];
			m_Translation.z = mat.m[2][3];
		}

		void		Identity()
		{
			m_Quaternion.Init();
			m_Translation.Init();
		}

		LTVector		m_Translation;
		LTRotation		m_Quaternion;
	};

	// ------------------------------------------------------------------------
	// AnimNode
	// represents a collection of positions/rotations in time associated with 
	// a model node.
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------
	// AnimNode
	// talon: the 2001 layout, 0x30 bytes (0x429a41, 0x429af5, the 0x30 new
	// at 0x429b99): vtable, m_pNode 4, m_KeyFrames 8, m_pParentNode 0x18,
	// m_Children 0x1c, m_pAnim 0x2c. No vertex animation (m_iPiece) and no
	// m_LastKF. The virtuals are in Build 69's order (SetAnim is slot 2,
	// 0x429f4a).
	// ------------------------------------------------------------------------
	class AnimNode
	{
	friend class ModelAnim;

	public:

							AnimNode(ModelAnim *pAnim, AnimNode *pParent);
		virtual				~AnimNode();

		void				Term();
		void				Clear();

		uint32				NumChildren() {return m_Children.GetSize();}
		AnimNode*			GetChild(uint32 index) {return m_Children[index];}

		Model*				GetModel();
		LAlloc*				GetAlloc();

		// Fills in ModelAnim::m_AnimNodes.
		LTBOOL				FillAnimNodeList(uint32 &curNodeIndex);

		// Sets m_pNode and recurses (returns FALSE if the trees don't match).
		LTBOOL				SetNodePointers(ModelNode *pNode);

		uint32				NumKeyFrames();	

	// Functions used by ModelEdit.
	public:

		LTBOOL				CopyKeyFrame(uint32 iKeyFrame, AnimNode *pDest);
		LTBOOL				PasteKeyFrame(AnimNode *pSrc, uint32 iBefore);
		LTBOOL				RemoveKeyFrame(uint32 iKeyFrame);

		// talon: 2001's (0x429dd3; the name is ours): keyframe iDest = the
		// interpolation of iFrom and iTo at t.
		LTBOOL				InterpolateKeyFrame(uint32 iDest, uint32 iFrom, uint32 iTo, float t);

		// talon: load (model_load) and save (model_save), part 7's.
		LTBOOL				Load(ILTStream &file);	// 0x42fddf
		LTBOOL				Save(ILTStream &file);	// 0x4320ad

	// Overridables.
	public:

		virtual ModelAnim*	GetAnim() {return m_pAnim;}
		virtual void		SetAnim(ModelAnim *pAnim);
		virtual AnimNode*	AllocateSameKind(ModelAnim *pAnim, AnimNode *pParent);

	public:

		// The ModelNode this AnimNode modifies.
		ModelNode			*m_pNode;

		// Keyframes.
		CMoArray<NodeKeyFrame,NoCache>	m_KeyFrames;

		AnimNode			*m_pParentNode;
		CMoArray<AnimNode*,NoCache>	m_Children;

	private:
							AnimNode();

		ModelAnim			*m_pAnim;
	};

	
	// ------------------------------------------------------------------------
	// AnimKeyFrame
	// time -> value pair 
	// value is call back and/or string associated with time.
	// ------------------------------------------------------------------------
	class AnimKeyFrame
	{
	public:
						AnimKeyFrame();
		
		uint32	m_Time;

		// A string of information about this key..
		char*			m_pString;
		
		KeyType			m_KeyType;
		KeyCallback		m_Callback;
		
		// User data for the callback.
		void			*m_pUser1;
	};

	// ------------------------------------------------------------------------
	// ModelAnim
	// Collection of animations associated with a model.
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------
	// ModelAnim
	// talon: the 2001 layout, 0x5c bytes (0x429ef4, the 0x5c new at
	// 0x42a06f): vtable, m_AnimNodes 4, m_KeyFrames 8, m_ModelEditWeightSet
	// 0x18 (-1), m_InterpolationMS 0x1c (200), m_pModel 0x20, m_pName 0x24,
	// m_DefaultRootNode 0x28, m_pRootNode 0x58. No compression type and no
	// LUT buffer.
	// ------------------------------------------------------------------------
	class ModelAnim
	{
	friend class Model;

	public:

						ModelAnim(Model *pModel);
		virtual			~ModelAnim();

		void			Term();

		inline uint32	NumKeyframes()	{return m_KeyFrames.GetSize();}

		AnimNode*		GetRootNode()					{return m_pRootNode;}
		void			SetRootNode(AnimNode *pNode);

		char*			GetName()	{return m_pName;}
		void			SetName(const char *pName);

		Model*			GetModel() {return m_pModel;}
		LAlloc*			GetAlloc();

		uint32			NumKeyFrames() {return m_KeyFrames.GetSize();}

		// Builds m_AnimNodes.
		LTBOOL			PrecalcNodeLists(LTBOOL bRebuild=TRUE);

		// Get the time of the last keyframe.
		uint32			GetAnimTime();

	// Functions used by ModelEdit.
	public:

		// Copy and cut a keyframe into a new animation.
		ModelAnim*	CopyKeyFrame(uint32 iKeyFrame);
		ModelAnim*	CutKeyFrame(uint32 iKeyFrame);

		LTBOOL			PasteKeyFrame(ModelAnim *pFrame, uint32 iBefore);
		LTBOOL			RemoveKeyFrame(uint32 iKeyFrame);

		// talon: 2001's (0x42a213; the name is ours): the keyframe times, then
		// AnimNode::InterpolateKeyFrame over the tree.
		LTBOOL			InterpolateKeyFrame(uint32 iDest, uint32 iFrom, uint32 iTo, float t);

		// talon: load (model_load) and save (model_save), part 7's.
		LTBOOL			Load(ILTStream &file);	// 0x42ff10
		LTBOOL			Save(ILTStream &file);	// 0x43211e

		// Copies pSrc through a memory stream (0x432c40; the name is ours).
		LTBOOL			CopyAnim(ModelAnim *pSrc);

	// Overridables.
	public:

		virtual void	SetModel(Model *pModel);
		virtual ModelAnim*	AllocateSameKind(Model *pModel);

	private:

		LTBOOL			BuildAnimNodeList(LTBOOL bRebuild=TRUE);

	public:

		// AnimNodes for each ModelNode.
		AnimNode		**m_AnimNodes;

		// The keyframes.
		CMoArray<AnimKeyFrame, NoCache>	m_KeyFrames;

		// Used by ModelEdit to remember the weight set.
		uint32			m_ModelEditWeightSet;

		// How long it takes to interpolate into this animation.
		uint32			m_InterpolationMS;

	private:

		void			DeleteRootNode();

	private:

		Model			*m_pModel;
		char			*m_pName;

		AnimNode		m_DefaultRootNode; // Default root node.
		AnimNode		*m_pRootNode;
	};


	// Used to represent position/rotation for some stuff.
	// basic node transform
	// Used to represent position/rotation for some stuff.
	// talon: 2001's is 0x1c bytes, a position and a rotation (ModelSocket's
	// constructor 0x42a9b2; CIRelation, transformmaker 0x43674e): no
	// m_Scale.
	class ModelTransform
	{
	public:

		void		ConvertToMatrix(LTMatrix &mat)
		{
			m_Rot.ConvertToMatrix(mat);
			mat.SetTranslation(m_Pos);
		}

		void		ConvertFromMatrix(LTMatrix &mat)
		{
			m_Rot.ConvertFromMatrix(mat);
			mat.GetTranslation(m_Pos);
		}

		LTVector		m_Pos;
		LTRotation		m_Rot;
	};


	// Just a position/rotation.
	class CIRelation : public ModelTransform
	{
	public:

	};


	// A (skeletal) Model contains one ChildInfo for each child model it is bound to.
	// The ChildModel structure contains the mapping from the child's skeleton
	// to the parent's geometry along with the weights from the skeleton to the parent.
	// NOTE: the parent itself has one of these to itself because it is used for all
	//       skeletal animation.
	// A (skeletal) Model contains one ChildInfo for each child model it is bound to.
	// The ChildModel structure contains the mapping from the child's skeleton
	// to the parent's geometry along with the weights from the skeleton to the parent.
	// NOTE: the parent itself has one of these to itself because it is used for all
	//       skeletal animation.
	// talon: 0x30 bytes in 2001 (the 0x30 stride of BaseDelete, 0x42e84c;
	// the constructor 0x429942), the flags LTBOOLs.
	class ChildInfo
	{
	public:

						ChildInfo();
						~ChildInfo();

		void			Term();

		uint32			GetSaveIndex()	{return m_SaveIndex;}

		// talon: load (model_load) and save (model_save), part 7's.
		LTBOOL			Load(ILTStream &file);	// 0x4301b4
		LTBOOL			Save(ILTStream &file);	// 0x4323da

		// Before depending on the bounding radius, this should be called to make sure it's valid.
		LTBOOL			IsBoundRadiusValid() {return m_bBoundRadiusValid;}

		LAlloc*			GetAlloc();

	public:

		// Relation matrices... used because the original skeleton points can
		// be moved for different parent models.
		CMoArray<CIRelation>	m_Relation;

		// Tells if the animation bounding radii are valid.
		LTBOOL			m_bBoundRadiusValid;

		// How far into the parent's m_Anims list this model's anims start.
		uint32			m_AnimOffset;

		// This is what it uses while loading to do its callback to load the child model.
		char			*m_pFilename;

		// All models have a # that's incremented each time they're saved.  This is saved
		// when the ChildModel is first created and if it's wrong (the child model file
		// has been changed), the animation dims must be recalculated.
		uint32			m_SaveIndex;

		// The parent model.
		Model			*m_pParentModel;

		// The child model itself.  This will be NULL if the child model couldn't be loaded.
		Model			*m_pModel;

		// Set to FALSE if there is an error while loading the model.
		LTBOOL			m_bTreeValid;
	};



	// The Model::m_Anims list contains these.
	class AnimInfo
	{
	public:
					AnimInfo();
		
		// Returns the model this animation comes from.
		// Returns NULL if something's wrong.
		Model*		GetAnimOwner();

		// talon: load (model_load) and save (model_save), part 7's.
		LTBOOL		Load(ILTStream &file);	// 0x430170
		LTBOOL		Save(ILTStream &file);	// 0x432242

		// Returns TRUE if this animation comes from the parent model (ie: if it
		// can be edited in ModelEdit).
		LTBOOL		IsParentModel()	{return m_pChildInfo->m_pParentModel == m_pChildInfo->m_pModel;}

	public:
		
		ModelAnim		*m_pAnim;
		ChildInfo		*m_pChildInfo;


	// Note: the data in here is stored in the parent model for each child model animation.
	// (So if there's a base model and it has 2 parent models, each parent model has separate
	// user dims and translation).
	public:

		// USER dimensions.. settable in ModelEdit.
		LTVector			m_vDims;
		
		// Translation for this animation.
		LTVector			m_vTranslation;
	};


	class ModelSocket : public ModelTransform
	{
	public:
					ModelSocket();
		
		char*		GetName()	{return m_Name;}
		void		SetName(const char *pName);


	public:
		char		m_Name[MAX_SOCKETNAME_LEN];
		uint32		m_iNode;	// Which node it's attached to.
		
		// Only used by modeledit..
		Model		*m_pAttachment;
	};

	// ------------------------------------------------------------------------
	// ModelNode
	// hierarchical organization of model. coordinate system for model
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------
	// ModelNode
	// hierarchical organization of model. coordinate system for model
	// talon: the 2001 layout, 0xb4 bytes (0x42a3f2, 0x42a49e, the 0xb4 new
	// at 0x42a4fb): vtable, m_vOffsetFromParent 4, m_NodeIndex 0x10,
	// m_Flags 0x12, m_ScratchData 0x14, m_Children 0x18, m_iParentNode
	// 0x28, the two transforms 0x2c and 0x6c, m_pModel 0xac, m_pName 0xb0.
	// No eval state, no OBB.
	// ------------------------------------------------------------------------
	class ModelNode
	{
	friend class Model;

	public:

						ModelNode(Model *pModel);
		virtual			~ModelNode();

		// Change all references to vertex iFrom to iTo.  The piece is
		// being modified and this is the node's chance to update things.
		virtual void	ChangeVertexReferences(PieceLOD *pPiece, uint32 iFrom, uint32 iTo) {}

		void			Term();
		void			Clear();

		uint32			NumChildren()			{return m_Children.GetSize();}
		ModelNode*		GetChild(uint32 index)	{return m_Children[index];}

		uint32			GetNodeIndex()			{return m_NodeIndex;}
		uint32			GetParentNodeIndex()	{return m_iParentNode;}

		char*			GetName()		{return m_pName;}
		void			SetName(char *pName);

		Model*			GetModel() {return m_pModel;}
		LAlloc*			GetAlloc();

		uint32&			ScratchData() {return m_ScratchData;}

		// Calculates the number of nodes including this one.
		uint32			CalcNumNodes();

		// Fills in the node list.
		LTBOOL			FillNodeList(uint32 &curNodeIndex);

		// Sets the parent node index for this node's children.
		void			SetParent_R(uint32 iNode);

		// talon: load (model_load) and save (model_save), part 7's.
		LTBOOL			Load(ILTStream &file);	// 0x42fffb
		LTBOOL			Save(ILTStream &file);	// 0x4321a7

		inline LTMatrix&		GetGlobalTransform()	{return m_mGlobalTransform;}
		inline LTMatrix&		GetInvGlobalTransform()	{return m_mInvGlobalTransform;}
		LTMatrix				GetLocalTransform() ;

		inline void			SetGlobalTransform(LTMatrix mMat)
		{
			m_mGlobalTransform = mMat;
			m_mInvGlobalTransform = ~mMat;
		}

	// Overridables.
	public:

		virtual ModelNode*	AllocateSameKind(Model *pModel);
		virtual void		SetModel(Model *pModel);

	public:

		// Offset from parent (in parent's space).
		LTVector			m_vOffsetFromParent;

		// Index into Model::m_FlatNodeList.
		uint16				m_NodeIndex;

		// Node flags (MNODE_).
		uint8				m_Flags;

		// Can be used for whatever.
		uint32				m_ScratchData;

		// Child nodes.
		CMoArray<ModelNode*, NoCache>	m_Children;

		// Parent node index (NODEPARENT_NONE for the root).
		uint32			m_iParentNode;

	private:
						ModelNode();

		// The global transform and its inverse (setup by ModelEdit).
		LTMatrix			m_mGlobalTransform;
		LTMatrix			m_mInvGlobalTransform;

		Model			*m_pModel;
		char			*m_pName;
	};

	// ------------------------------------------------------------------------
	// PieceLOD
	// Level of Detail geometry and renderstyle. 
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------
	// PieceLOD
	// talon: 2001's, 0x24 bytes (0x42a60b..0x42a694; model_cleanup
	// 0x42f950): the vertices at 0, the triangles at 0x10 and the model at
	// 0x20, and nothing else: no textures, render style or name.  A
	// ModelPiece is its own base LOD.
	// ------------------------------------------------------------------------
	class PieceLOD
	{
	public:

							PieceLOD(Model *pModel);
							PieceLOD();
							~PieceLOD();

		void				Init(Model *pModel);

		LAlloc*				GetAlloc();
		Model*				GetModel();

		uint32				NumVerts()		{return m_Verts.GetSize();}

		// talon: load (model_load) and save (model_save), part 7's.
		LTBOOL				Load(ILTStream &file, uint32 &iCurWeight);	// 0x43022c
		LTBOOL				Save(ILTStream &file);						// 0x43247d

	public:

		// Tris and vertices.
		CMoArray<ModelVert, NoCache>	m_Verts;
		CMoArray<ModelTri, NoCache>		m_Tris;

		// Point back to parent model.
		Model							*m_pModel;
	};


	// ------------------------------------------------------------------------
	// ModelPiece
	// model piece is a container for a collection of lod stages.
	// each lod stage is a render-object.
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------
	// ModelPiece
	// talon: 2001's, 0x6c bytes (0x42a712, 0x42a767): a PieceLOD itself
	// (its base LOD, at 0), the texture index 0x24, m_VertOffset 0x28, the
	// specular power and scale 0x2c and 0x30 (5 and 1), the other LODs
	// 0x34, m_LODWeight 0x44 (1), m_Name 0x48, m_pModel 0x68.  GetLOD(0) is
	// the piece, GetLOD(i) m_LODs[i-1] (gl_modelrender, model_cleanup
	// 0x42fb03).
	// ------------------------------------------------------------------------
	class ModelPiece : public PieceLOD
	{
	public:						

						ModelPiece(Model *pModel);
						~ModelPiece();

		void			Term();

		// The number of LODs, the base one included.
		inline uint32	NumLODs()	{return m_LODs.GetSize() + 1;}

		// If iLOD is PIECELOD_BASE, then it just returns the piece's LOD, otherwise
		// it returns the correct LOD.
		// (0x432dd3, 0x42fa68: NULL past the end; the dec, 0x42185c).
		PieceLOD*		GetLOD(uint32 iLOD)
		{
			if(iLOD == 0)
				return this;
			iLOD--;
			if(iLOD < m_LODs.GetSize())
				return &m_LODs[iLOD];
			return NULL;
		}

		float			GetLODWeight() {return m_LODWeight;}
		void			SetLODWeight(float fWeight) {m_LODWeight = fWeight;}

	public:

		char*			GetName()		{return m_Name;}
		void			SetName(const char *pName);

		Model*			GetModel() { return m_pModel ;}
		LAlloc*			GetAlloc();

	public:

		// Copy the UV coordinates.
		// Returns FALSE if the geometries on the two models don't match.
		LTBOOL			CopyUVs(ModelPiece *pSrc);

		LTBOOL			ReplaceVertex(uint32 iVert, uint32 iReplaceWith);

		// talon: load (model_load) and save (model_save), part 7's.
		LTBOOL			Load(ILTStream &file, uint32 &iCurWeight);	// 0x43044c
		LTBOOL			Save(ILTStream &file);						// 0x43263f

	public:

		// The texture this piece uses.
		uint32			m_iTexture;	// talon: unsigned: SortPiecesByTexture compares it so (0x417b9f)

		// The base index of its vertices if all the nodes were laid out in a linear list
		// (in the order of m_FlatNodeList).
		uint32			m_VertOffset;

		float			m_SpecularPower;
		float			m_SpecularScale;

		// The other LODs (the piece itself is LOD 0).  Use GetLOD.
		CMoArray<PieceLOD, NoCache>	m_LODs;

		// used to determine collapse priority for automatically generated lods.
		float			m_LODWeight;

		char			m_Name[MAX_PIECENAME_LEN];

	private:

		Model*			m_pModel ;
	};


	// Weight sets are created in order to tell the engine how to blend animations together.
	// There is one weight for each node.
	class WeightSet
	{
	public:
						WeightSet(Model *pModel);
						~WeightSet();

		LTBOOL			InitWeights(uint32 nWeights);	// talon: LTBOOL (0x4283c9)
	
		// talon: load (model_load) and save (model_save), part 7's.
		LTBOOL			Load(ILTStream &file);	// 0x430563
		LTBOOL			Save(ILTStream &file);	// 0x4326f5

		char*			GetName()	{return m_Name;}
		void			SetName(const char *pName);

		inline Model*	GetModel()	{return m_pModel;}
		LAlloc*			GetAlloc();

	public:
		
		char			m_Name[MAX_WEIGHTSETNAME_LEN];
		CMoArray<float>	m_Weights;
		Model			*m_pModel;
	};


	class LODInfo
	{
	public:

					LODInfo()
					{
						m_Dist = 0.0f;
					}

	public:
		// Desired distance to use this LOD.
		float		m_Dist;
	};


	// All a model's strings are stored in here, since there are 
	// tons of redundant strings (like "").
	class ModelStringList
	{
	public:

						ModelStringList(LAlloc *pAlloc=&g_DefAlloc);
						~ModelStringList();

		// Free all strings.
		void			Term();

		// Add a new string.  Checks for duplicates.
		char*			AddString(const char *pStr);

		// Change the allocator.  You can NOT change it if strings have already been
		// allocated into here.
		LTBOOL			SetAlloc(LAlloc *pAlloc);


	protected:

		inline LAlloc*	GetAlloc()	{return m_pAlloc;}


	public:

		ModelString		*m_StringList;


	protected:

		LAlloc			*m_pAlloc;
	};

	// ------------------------------------------------------------------------
	// CModelCallbackList
	// talon: Model's member at 0x14 (8 bytes), from its own module between
	// modelallocations and pixelformat (0x433084..0x43311c). The names are
	// ours: no source names it. A list of registrations, each with a list
	// of callbacks {fn, next}; Notify calls every fn(this, reg, a1, a2).
	// ------------------------------------------------------------------------
	class CModelCallbackList
	{
	public:

						CModelCallbackList();			// 0x4330a5: Init(0)
						~CModelCallbackList();			// 0x4330b3

		LTBOOL			Init(void *pUser);				// 0x4330b8
		LTRESULT		Term();							// 0x4330c8: Notify(0, &1)
		LTRESULT		Notify(uint32 a1, void *a2);	// 0x4330e0

	public:

		void			*m_pHead;
		void			*m_pUser;
	};


	// ------------------------------------------------------------------------
	// Model
	// Container that holds geometric, material and animation information. 
	// The data contained represents data for character models as well as props 
	// and things.
	// ------------------------------------------------------------------------
	class Model
	{
	public:
						Model(LAlloc *pAlloc=&g_DefAlloc, LAlloc *pDefAlloc=&g_DefAlloc);
		virtual			~Model();

		void			Term(LTBOOL bDeleteChildModels=FALSE);

		// Deletes all the animations.
		void			TermAnims();

		// Deletes and removes references to the child models.
		void			TermChildModels(LTBOOL bDeleteChildModels);

	
	public:

		// The save index is stored in the child model.
		uint32			GetSaveIndex() {return m_SelfChildModel.m_SaveIndex;}
		void			SetSaveIndex(uint32 index) {m_SelfChildModel.m_SaveIndex = index;}
		
		// Access the root node.
		ModelNode*		GetRootNode() {return m_pRootNode;}
		void			SetRootNode(ModelNode *pNode);

		// The way the lists are built, the root node is always node 0.
		LTBOOL			IsRootNode(uint32 iNode) {return iNode == 0;}

		// Get the allocators this model uses.
		inline LAlloc*	GetAlloc()		{return m_pAlloc;}
		inline LAlloc*	GetDefAlloc()	{return m_pDefAlloc;}

		uint32			NumAnims()	{return m_Anims.GetSize();}
		
		// Just gets the anim, skips past the AnimInfo.
		ModelAnim*		GetAnim(uint32 index) {return m_Anims[index].m_pAnim;}

		AnimInfo*		GetAnimInfo(uint32 index) {return &m_Anims[index];}

		// Asserts and returns NULL if the index is invalid.
		ModelAnim*		GetAnimCheck(uint32 index)
		{
			if(index >= m_Anims.GetSize())
			{
				ASSERT(FALSE);
				return NULL;
			}
			return m_Anims[index].m_pAnim;
		}
		
		// Gets the anim, but returns NULL if the animation's parent 
		ModelAnim*		GetAnimIfOwner(uint32 index)
		{
			if(m_Anims[index].GetAnimOwner() == this)
				return m_Anims[index].m_pAnim;
			else
				return NULL;
		}


		// Weight sets..
		uint32			NumWeightSets()			{return m_WeightSets.GetSize();}
		WeightSet*		GetWeightSet(uint32 i)	{return m_WeightSets[i];}
		LTBOOL			AddWeightSet(WeightSet *pSet);
		void			RemoveWeightSet(uint32 index);


		uint32			GetTotalNumVerts()	{return m_nTotalVerts;}		
		uint32			GetTotalNumTris()	{return m_nTotalTris;}		
		
		// The model's LODs: the base one (m_BaseLODInfo) and m_LODs.
		uint32			NumLODs()	{return m_LODs.GetSize() + 1;}

		// ------------------------------------------------------------------------
		// NODES 

		// Precalculate the lists of nodes (and figures out how many there are).
		// This should be called whenever a node is added or removed.
		LTBOOL			PrecalcNodeLists(LTBOOL bRebuild=TRUE);

		ModelNode*		FindNode(const char *pName, uint32 *index=NULL);
		// talon: the count is m_Transforms' (0x78: BuildFlatNodeList
		// 0x42b136, transformmaker), the array m_FlatNodeList's.
		uint32			NumNodes() {return m_Transforms.GetSize();}
		ModelNode*		GetNode(uint32 i) {return m_FlatNodeList[i];}

		// This call gets the transform for node "i" after anim has been applied. So this is the current Global position of that node.
		LTMatrix*		GetTransform(uint32 i) {return &m_Transforms[i];}
		
		// Find the parent of a given node.  Starts recursing at pRoot.
		// Fills in index if it's non-NULL.
		ModelNode*		FindParent(ModelNode *pNode, ModelNode *pRoot, uint32 *index=NULL);

		// Removes rotation from this node.  Adjusts its child nodes so they animate the same.
		// UnrotateNode just calls UnrotateAnimNode for all the animations on this node.
		void			UnrotateAnimNode(ModelAnim *pAnim, AnimNode *pAnimNode);
		void			UnrotateNode(ModelNode *pNode);


		// Removes the node from the tree.  Only works on null nodes currently.
		// NOTE: this function overwrites ModelNode::m_pSoftElem.
		LTBOOL			RemoveNode(ModelNode *pNode);

		// Calls RemoveNode on all the removable nodes.
		LTBOOL			RemoveRemovableNodes();

		// ------------------------------------------------------------------------
		// PIECES 

		uint32			NumPieces() {return m_Pieces.GetSize();}
		ModelPiece*		GetPiece(uint32 i) {return m_Pieces[i];}
		ModelPiece*		FindPiece(const char *pName, uint32 *index=NULL); 
		WeightSet*		FindWeightSet(const char *pName, uint32 *index=NULL);

		// Swap the two pieces.
		LTBOOL			SwapPieces(uint32 iPiece1, uint32 iPiece2);
	
		// Add a string to the model's list.
		char*			AddString(const char* pStr);

		// Find an animation by name.
		ModelAnim*		FindAnim(char *pName, uint32 *index=NULL, AnimInfo **ppInfo=NULL);

		// Find an animation with the given model.
		AnimInfo*		FindAnimInfo(char *pAnimName, Model *pOwner, uint32 *index=NULL);

		// Returns the index of the given anim info.
		LTBOOL			GetAnimInfoIndex(AnimInfo *pInfo, uint32 &index);

		// Add an animation to the list.  It adds it in the appropriate position given
		// the ChildInfo.
		AnimInfo*		AddAnim(ModelAnim *pAnim, ChildInfo *pInfo);

		// Add an animation to this model (just calls AddAnim with m_SelfChildModel).
		AnimInfo*		AddAnimToSelf(ModelAnim *pAnim) {return AddAnim(pAnim, &m_SelfChildModel);}

		// Creates a single-frame animation with identity translations and rotations
		// using the vertex positions from the model's geometry.
		// This does NOT support deformation nodes currently (returns FALSE if there are any).
		LTBOOL			AddDummyAnimation(char *pName);

		// Load a model (.abc): 0x4308ab (model_load).  Returns LT_OK or an error.
		LTRESULT		Load(ModelLoadRequest *pRequest);

		// talon: load (model_load) and save (model_save), part 7's.
		LTBOOL			LoadString(ILTStream &file, char* &pStr);					// 0x4305cc
		LTBOOL			LoadAnimBindings(ModelLoadRequest *pRequest, ILTStream &file, LTBOOL bAllowUpdates);	// 0x430613 (the flag is unread)
		LTBOOL			LoadSockets(ILTStream &file);								// 0x430722
		LTBOOL			LoadWeightSets(ILTStream &file);							// 0x430807
		LTBOOL			LoadHeader(ILTStream &file, ModelAllocations &allocs);	// 0x431233

		LTBOOL			SaveAnimBindings(ILTStream &file, uint32 &chunkPos);		// 0x432286
		LTBOOL			SaveSockets(ILTStream &file);								// 0x432742
		LTBOOL			SaveWeightSets(ILTStream &file);							// 0x4327c1
		LTBOOL			SaveToStream(ILTStream &file);	// 0x43280b (the name is ours)
		LTBOOL			Save(ILTStream &file);			// 0x432bb7: ForceAnimOrder, then SaveToStream

		// Copies pSrc through a memory stream (0x432be4; the name is ours).
		LTBOOL			CopyModel(Model *pSrc);

		// Copy the UV coordinates.
		// Returns FALSE if the geometries on the two models don't match.
		LTBOOL			CopyUVs(Model *pSrc);

		// Calculates the total number of vertices in the model.
		uint32			CalcNumTris(uint32 iLOD = PIECELOD_BASE);
		uint32			CalcNumVerts();

		// Calculates the total number of vertex weights (ie: how expensive it is to 
		// weight all the vertices in the model).
		uint32			CalcNumVertexWeights();

		// Figures out how many animations are in the child models.
		uint32			CalcNumChildModelAnims(LTBOOL bIncludeSelf);

		// Returns how many animations in our m_Anims list come from this model.
		uint32			CalcNumParentAnims();

		// Parses the command string.
		void			ParseCommandString();

		// Sets the fade range (the distances and their squares, 0x42c0e2).
		void			SetFadeRange(float fMin, float fMax);

		// Access the filename.
		const char*		GetFilename();
		LTBOOL			SetFilename(const char *pFilename);
		void			FreeFilename(); // Free m_pFilename.
		

	// Functions to access child models.
	// NOTE: there is ALWAYS at least one child model and the first child model in the 
	// list is always m_SelfChildModel.
	public:

		ChildInfo*		GetSelfChildModel() {return m_ChildModels[0];}

		uint32			NumChildModels() {return m_nChildModels;}
		ChildInfo*		GetChildModel(uint32 index) {ASSERT(index < m_nChildModels); return m_ChildModels[index];}
		
		// Returns if the child model's tree is valid.
		LTBOOL			VerifyChildModelTree(Model *pChild, ModelNode* &pErrNode);

		// Setup the relation between the models.  If bScaleSkeleton is TRUE, then it 
		// scales the skeleton to match the other model's skeleton.
		LTBOOL			SetupChildModelRelation( ChildInfo *pInfo, 
												 Model *pChild, 
												 LTBOOL bScaleSkeleton=FALSE);
		
		// Used by ModelEdit.. shouldn't be used at runtime.  Adds a new ChildModel
		// with the specified info.
		ChildInfo*		AddChildModel( Model *pChild, char *pFilename, 
										char *pErrStr, 
										LTBOOL bScaleSkeleton);
		// Creates a reference to a child model, no actually child model is assigned. 
		// This message should only be used by tools that just need the child model filename and 
		// params.
		ChildInfo*		AddChildModelRef( char *pFilename, char *errStr );

		// Remove the specified child model.
		LTBOOL			RemoveChildModel(uint32 index);


		// Just initializes the model and sets our m_ChildModel[index] = pChildModel.
		LTBOOL			InitChildInfo(uint32 index, ChildInfo *pChildModel, Model *pModel, char *pFilename);
		
		// Find child models, first one by pointer reference.
		ChildInfo*		FindChildModel(Model *pModel);
		ChildInfo*		FindChildModelByFilename(char *pFilename, uint32 *index=NULL); // Case sensitive.

		// Make a unique animation name.
		LTBOOL			MakeUniqueAnimName(char *pBase, char *pName);

		// Sets ModelNode::m_vOffsetFromParent (and calls it on the children).
		void			SetNodeParentOffsets();


	public:

		// This is the 'reference' function for how to get the vertex positions on a given 
		// frame of animation.
		LTBOOL			GetVertexPositions(GVPStruct *pStruct, LTBOOL bSetupTransforms=TRUE, LTBOOL bWeight=TRUE);

		// Fix for a bug in ModelEdit where it sometimes saves animations in an order
		// different from animation bindings.
		LTBOOL			ForceAnimOrder();


	private:

		// These are used during precalculation.
		LTBOOL			BuildTransformList(LTBOOL bRebuild=TRUE);
		LTBOOL			BuildFlatNodeList(LTBOOL bRebuild=TRUE);


	// talon: the 2001 layout, 0x2c8 bytes (the constructor 0x42a9fd, which
	// adds 0x2c8 to g_ModelMemory; ParseCommandString 0x42bceb for the
	// command-string fields; model_load for m_FileVersion 0x208).  The
	// flags are LTBOOLs.
	private:

		char			*m_pFilename;

	public:

		LTLink			m_Link;

		// talon: 0x14, eight bytes (part 7's, 0x433084..0x43311c): a list
		// of callbacks run when the model goes (the destructor's first call,
		// 0x42ae02).
		CModelCallbackList	m_Callbacks;

		unsigned long	m_FileID;		// Used by server.
		unsigned long	m_Flags;		// MODELFLAG flags above.  Used by various things.

	// Node info.
	public:

		// Flat node list (in the order they come in the tree).
		CMoArray<ModelNode*, NoCache>	m_FlatNodeList;

		// Model pieces.
		CMoArray<ModelPiece*, NoCache>	m_Pieces;

		CMoArray<WeightSet*, NoCache>	m_WeightSets;

		CMoArray<NewVertexWeight, NoCache>	m_VertexWeights;

		// Calculated in PrecalcNodeLists.
		uint32			m_nTotalVerts;
		uint32			m_nTotalTris;

		// How many DWORDs a bit per node takes.
		unsigned long	m_nDWordNodes;

		// The transforms, used in GetVertexPositions.
		CMoArray<LTMatrix, NoCache>	m_Transforms;

		// The command string.
		char			*m_CommandString;

		ModelStringList	m_StringList;

		// The model's LODs past the base one.
		CMoArray<LODInfo>	m_LODs;
		LODInfo			m_BaseLODInfo;

	// Command string stuff (ParseCommandString).
	public:

		float			m_GlobalRadius;		// Global radius.. MUST enclose the whole model.
		float			m_VisRadius;		// VisRadius
		LTBOOL			m_bNoAnimation;		// NoAnimation

		// FadeRangeMin, FadeRangeMax (and their squares).
		float			m_FadeRangeMin, m_FadeRangeMinSqr;
		float			m_FadeRangeMax, m_FadeRangeMaxSqr;

		// talon: 0xc0, zeroed by the constructor and read by nothing in
		// the model code.
		uint32			m_Unknown0c0;

		// FadeSpriteSize.
		float			m_FadeSpriteSizeX, m_FadeSpriteSizeY;

		float			m_AmbientLight;
		float			m_DirLight;

		LTBOOL			m_bShadowEnable;
		float			m_fShadowProjectLength;
		float			m_fShadowLightDist;
		float			m_fShadowSizeX;
		float			m_fShadowSizeY;
		LTVector		m_vShadowCenterOffset;

		// NormalRef: m_mNormalRef is the INVERSE of the first frame of
		// m_iNormalRefAnim.
		uint32			m_iNormalRefNode;
		uint32			m_iNormalRefAnim;
		LTMatrix		m_mNormalRef;
		LTBOOL			m_bNormalRef;

		// FovXOffset, FovYOffset.
		LTBOOL			m_bFOVOffset;
		float			m_xFOVOffset;
		float			m_yFOVOffset;

		LTBOOL			m_bSpecularEnable;
		LTBOOL			m_bRigid;


	// Sockets.
	public:

		uint32			NumSockets() {return m_Sockets.GetSize();}
		ModelSocket*	GetSocket(uint32 index) {return m_Sockets[index];}
		ModelSocket*	FindSocket(const char *pName, uint32 *index=NULL);

		LTBOOL			AddSocket(ModelSocket *pSocket);
		LTBOOL			RemoveSocket(uint32 index);
		
		// Gets the socket's transform, assumes the transforms for the model are already setup.
		LTBOOL			GetSocketTransform(ModelSocket *pSocket, LTMatrix *pSocketTransform);
		
		CMoArray<ModelSocket*,NoCache>	m_Sockets;
		

	// Other...
	public:

		LTBOOL			AnyVertsWeightedToNode(uint32 iNode);

	
	// Animation data.
	public:

		// Note: with skeletal models, some of these anims may not be from this model,
		// it should never assume that they are.
		CMoArray<AnimInfo, NoCache>		m_Anims;

		
	// Misc.
	public:

		// m_pAlloc = who's doing our BLOCK allocations.  Most allocations can go through 
		// here, but some must go thru m_pDefAlloc if we can't predict ahead of time how
		// many allocations there will be.
		LAlloc							*m_pAlloc;
		LAlloc							*m_pDefAlloc;

		// At runtime, models are loaded using this.
		LAllocSimpleBlock				m_BlockAlloc;

		// talon: 0x190, zeroed last by the constructor (0x42ac79) and set to
		// 1 by model_load (0x430daf).
		uint32							m_Unknown190;

		// What version of the file did we come from?
		inline uint32	GetFileVersion()	{return m_FileVersion;}
		// Set the file version. This should only be used by an importer.
		inline void     SetFileVersion( uint32 vers ) { m_FileVersion = vers ; }

	// Child model list.
	private:

		// The list of child models.
		// The model itself (m_SelfChildModel) is ALWAYS first.
		ChildInfo*						m_ChildModels[MAX_CHILD_MODELS];
		uint32							m_nChildModels;
		
		// Saves an allocation.
		ChildInfo						m_SelfChildModel;

	
	private:

		// Set when it starts reading so it can track what file version it's reading in.
		uint32							m_FileVersion;

	public:

		// How many parent models reference this one as a child (AddChildModel
		// 0x42c38c, InitChildInfo 0x42c510; TermChildModels deletes a child
		// whose count drops to 0, 0x42af7d).
		uint32 m_RefCount ;
		LTBOOL IsFreeable() { return m_RefCount == 0 ; }
		uint32 AddRef()   { return m_RefCount++ ; }
		uint32 DecRef()   { return m_RefCount-- ; }

	private:
		
		// Default root node.. setup so it can be subclassed.
		ModelNode		m_DefaultRootNode;
		ModelNode		*m_pRootNode;
	};



	//------------------------------------------------------------------ //
	// Functions.
	//------------------------------------------------------------------ //

	// This is used to find tokens in node and socket names.  Tokens start with _z.
	// Returns a pointer to the first character in the token.  So if the token you're
	// looking for is "Tex" and it finds _zTex, it'll return "Tex".
	// The comparison is case-insensitive.
	// Current tokens:
	// _zN  = don't remove this node
	// _zT0	= texture 0...
	char* model_FindExporterToken(char *pName, char *pToken);


	//------------------------------------------------------------------ //
	// Inlines.
	//------------------------------------------------------------------ //

	inline LAlloc* ModelAnim::GetAlloc()
	{
		return GetModel()->GetAlloc();
	}

	inline LAlloc* ModelNode::GetAlloc()
	{
		return GetModel()->GetAlloc();
	}

	// ------------------------------------------------------------------------
	//	GetLocalTransform()
	// ------------------------------------------------------------------------
	inline LTMatrix	ModelNode::GetLocalTransform() 
	{ 
		if( m_iParentNode != NODEPARENT_NONE && m_pModel != NULL)
		{
			LTMatrix mat ;
			mat.Identity();
			ModelNode *parent = m_pModel->GetNode( m_iParentNode );
			if( parent!=NULL )
				mat = parent->GetInvGlobalTransform() * m_mGlobalTransform  ;
			else 
				mat = m_mGlobalTransform ;
			return mat;
		}
		return m_mGlobalTransform ;
	}
	
	inline LAlloc* AnimNode::GetAlloc()
	{
		return GetModel()->GetAlloc();
	}

	inline LAlloc* PieceLOD::GetAlloc()
	{
		return m_pModel->GetAlloc();
	}

	inline Model* PieceLOD::GetModel()
	{
		return m_pModel;
	}

	inline LAlloc* ModelPiece::GetAlloc()
	{
		return GetModel()->GetAlloc();
	}

	inline LAlloc* WeightSet::GetAlloc()
	{
		return GetModel()->GetAlloc();
	}

	inline LAlloc* ChildInfo::GetAlloc()
	{
		return m_pParentModel->GetAlloc();
	}

	//inline LODInfo* Model::GetLOD(uint32 i)
	//{
		//if(i == 0)
			//return &m_BaseLODInfo;
//
		//--i;
		//if(i < m_LODs.GetSize())
			//return &m_LODs[i];
//
		//ASSERT(FALSE);
		//return NULL;
	//}


#endif  // __MODEL_H__

