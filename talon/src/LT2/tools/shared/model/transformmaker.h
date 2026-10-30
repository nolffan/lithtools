
#ifndef __TRANSFORMMAKER_H__
#define __TRANSFORMMAKER_H__


    #ifndef __LTBASEDEFS_H__
	#include "ltbasedefs.h"
    #endif

    #ifndef __MODEL_H__
	#include "model.h"
    #endif


	class TransformMaker
	{
	public:

						TransformMaker()
						{
							m_pStartMat = LTNULL;
							m_pOutput = LTNULL;
							m_NC = LTNULL;
							m_pNCUser = LTNULL;
							m_hNCObject = LTNULL;
							m_nAnims = 0;
						}

		// Copies m_Anims, m_nAnims, and sets m_pStartMat to GVPStruct::m_BaseTransform.
		void			SetupFromGVPStruct(GVPStruct *pStruct);

		// talon: 2001's returns are LTBOOLs (0x435f98, 0x435fea: `xor eax,eax;
		// inc eax`), and there is no CopyTimes, GetNodeTransform or
		// SetupMovementEncoding: the module (0x435f3d..0x436849) has only the
		// functions below.
		LTBOOL			IsValid();

		LTBOOL			SetupTransforms();


	// Set all these up before calling SetupTransforms or GetNodeTransform.
	public:

		// All the animations.
		AnimTimeRef		m_Anims[MAX_GVP_ANIMS];
		uint32			m_nAnims;

		LTMatrix		*m_pStartMat;	// If null, then identity is used.
		LTMatrix		*m_pOutput;		// Output list.. if null, then Model::m_Transforms is used.
		
		NodeControlFn	m_NC;			// Can be null.
		void			*m_pNCUser;
		HOBJECT			m_hNCObject;

		// If this is set, Recurse() uses this to determine which nodes it recurses into.
		// It starts at m_iCurPath and goes backwards until it reaches index 0.
		uint32			*m_pRecursePath;
		uint32			m_iCurPath;


	// Used internally.
	protected:

		LTBOOL			SetupCall();

		void			InitTransform(uint32 iAnim, uint32 iNode, LTRotation &outQuat, LTVector &outVec);
		void			InitTransformAdditive(uint32 iAnim, uint32 iNode, LTRotation &outQuat, LTVector &outVec);

		// talon: 2001's takes the weight blended in so far and the time ref's
		// m_bNormalize, and returns the weight it used (0x43645f, `ret $0x10`,
		// the result in st(0); Recurse adds it up, 0x4366bb).
		float			BlendTransform(uint32 iAnim, uint32 iNode, float fTotal, LTBOOL bNormalize);

		void			Recurse(uint32 iNode, LTMatrix *pParentT);


	protected:

		// Used as local variables so they don't add to the stack.
		CIRelation		*m_pCurRelation;
		LTMatrix		m_mCurRelation;
		CMoArray<CIRelation>	*m_pNodeRelation;

		LTMatrix		m_mTemp;
		
		LTRotation		m_Quat;
		LTVector		m_vTrans;

		Model			*m_pModel;

		LTMatrix		m_mIdent;

		// talon: 2001's SetupCall looks these up once per call (0x43601b):
		// each time ref's weight sets (NULL when its index is out of range)
		// and animations, at 0x228, 0x248, 0x268 and 0x288.
		WeightSet		*m_pPrevWeightSets[MAX_GVP_ANIMS];
		WeightSet		*m_pCurWeightSets[MAX_GVP_ANIMS];
		ModelAnim		*m_pPrevAnims[MAX_GVP_ANIMS];
		ModelAnim		*m_pCurAnims[MAX_GVP_ANIMS];

	};


#endif

