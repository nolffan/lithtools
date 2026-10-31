#ifndef __PS2
#include <windows.h>
#endif
#include "bdefs.h"
#include "transformmaker.h"

// talon: 2001's Recurse multiplies through this (0x494910, .data, just
// ahead of StdLith's g_DefAlloc) rather than assigning the parent first.
static LTMatrix g_mTemp;

// ------------------------------------------------------------------------
// SetFromGVPStruct
// set up from a render packet
// ------------------------------------------------------------------------
void TransformMaker::SetupFromGVPStruct(GVPStruct *pStruct)
{
	uint32 i;

	for(i=0; i < pStruct->m_nAnims; i++)
	{
		m_Anims[i] = pStruct->m_Anims[i];
	}

	m_nAnims = pStruct->m_nAnims;
	m_pStartMat = &pStruct->m_BaseTransform;
	
}


LTBOOL TransformMaker::IsValid()
{
	uint32 i;
	Model *pModel;
	AnimTimeRef *pAnim;

	if(m_nAnims == 0 || m_nAnims > MAX_GVP_ANIMS)
		return LTFALSE;

	pModel = m_Anims[0].m_pModel;
	for(i=0; i < m_nAnims; i++)
	{
		pAnim = &m_Anims[i];

		if(!pAnim->IsValid() || pAnim->m_pModel != pModel)
			return LTFALSE;

		// Make sure the weight set is valid.
		if(i != 0)
		{
			if(pAnim->m_Cur.m_iWeightSet >= pModel->NumWeightSets())
				return LTFALSE;
		}
	}

	return LTTRUE;
}


LTBOOL TransformMaker::SetupTransforms()
{
	if(!SetupCall())
		return LTFALSE;

	Recurse(m_pModel->GetRootNode()->GetNodeIndex(), m_pStartMat);
	return LTTRUE;
}


// ------------------------------------------------------------------------
// SetupCall() (0x43601b)
// talon: 2001's looks up each time ref's weight sets and animations here,
// once, for InitTransform and BlendTransform.
// ------------------------------------------------------------------------
LTBOOL TransformMaker::SetupCall()
{
	uint32 i;
	AnimInfo *pAnim;


	if(!IsValid())
		return LTFALSE;

	m_pRecursePath = LTNULL;
	m_pModel = m_Anims[0].m_pModel;

	for(i=0; i < m_nAnims; i++)
	{
		m_pPrevWeightSets[i] = (m_Anims[i].m_Prev.m_iWeightSet >= m_pModel->NumWeightSets()) ?
			LTNULL : m_pModel->GetWeightSet(m_Anims[i].m_Prev.m_iWeightSet);
		m_pCurWeightSets[i] = (m_Anims[i].m_Cur.m_iWeightSet >= m_pModel->NumWeightSets()) ?
			LTNULL : m_pModel->GetWeightSet(m_Anims[i].m_Cur.m_iWeightSet);

		m_pPrevAnims[i] = m_pModel->GetAnim(m_Anims[i].m_Prev.m_iAnim);
		m_pCurAnims[i] = m_pModel->GetAnim(m_Anims[i].m_Cur.m_iAnim);
	}

	if(!m_pStartMat)
	{
		m_mIdent.Identity();
		m_pStartMat = &m_mIdent;
	}

	if(!m_pOutput)
	{
		m_pOutput = m_pModel->m_Transforms.GetArray();
	}

	pAnim = m_pModel->GetAnimInfo(m_Anims[0].m_Prev.m_iAnim);
	m_pNodeRelation = &pAnim->m_pChildInfo->m_Relation;

	return LTTRUE;
}

// ------------------------------------------------------------------------
// InitTransform (0x436115)
//
// ------------------------------------------------------------------------
void TransformMaker::InitTransform(	uint32 iAnim,
									uint32 iNode,
									LTRotation &outQuat,
									LTVector &outVec)
{
	AnimTimeRef *pTimeRef;
	NodeKeyFrame *pKeys[2];
	LTVector *pTrans1, *pTrans2;


	pTimeRef = &m_Anims[iAnim];

	pKeys[0] = &m_pPrevAnims[iAnim]->m_AnimNodes[iNode]->m_KeyFrames[pTimeRef->m_Prev.m_iFrame];
	pKeys[1] = &m_pCurAnims[iAnim]->m_AnimNodes[iNode]->m_KeyFrames[pTimeRef->m_Cur.m_iFrame];

	outQuat.Slerp(pKeys[0]->m_Quaternion, pKeys[1]->m_Quaternion, pTimeRef->m_Percent);
	outVec = pKeys[0]->m_Translation + (pKeys[1]->m_Translation - pKeys[0]->m_Translation) * pTimeRef->m_Percent;

	// Apply the per-animation translation.
	if(m_pModel->IsRootNode(iNode))
	{
		pTrans1 = &m_pModel->GetAnimInfo(pTimeRef->m_Prev.m_iAnim)->m_vTranslation;
		pTrans2 = &m_pModel->GetAnimInfo(pTimeRef->m_Cur.m_iAnim)->m_vTranslation;
		outVec += *pTrans1 + (*pTrans2 - *pTrans1) * pTimeRef->m_Percent;
	}
}


// ------------------------------------------------------------------------
// InitTransformAdditive (0x43627e)
// ------------------------------------------------------------------------
void TransformMaker::InitTransformAdditive( uint32 iAnim,
											uint32 iNode,
											LTRotation &outQuat,
											LTVector &outVec)
{
	AnimTimeRef *pTimeRef;
	NodeKeyFrame *pKeys[2], *pBaseKey;


	pTimeRef = &m_Anims[iAnim];

	pBaseKey = &m_pPrevAnims[iAnim]->m_AnimNodes[iNode]->m_KeyFrames[0];
	pKeys[0] = &pBaseKey[pTimeRef->m_Prev.m_iFrame];
	pKeys[1] = &m_pCurAnims[iAnim]->m_AnimNodes[iNode]->m_KeyFrames[pTimeRef->m_Cur.m_iFrame];

	outQuat.Slerp(pKeys[0]->m_Quaternion, pKeys[1]->m_Quaternion, pTimeRef->m_Percent);
	outVec = pKeys[0]->m_Translation + (pKeys[1]->m_Translation - pKeys[0]->m_Translation) * pTimeRef->m_Percent;

	//frame = base + offset
	//offset = ~base * frame
	outQuat = ~pBaseKey->m_Quaternion * outQuat;
	outVec = outVec - pBaseKey->m_Translation;
}


// ------------------------------------------------------------------------
// BlendTransform (0x43645f)
// talon: 2001's.  The weight is the time ref's previous and current weight
// sets' (lerped by its m_Percent), or, with m_bNormalize, what the anims
// before it leave of 1; it returns the weight it blended with (0 for an
// additive one).  The constants are 1.0f, 2.0f and 0.0f (.rdata 0x474348,
// 0x47434c, 0x474350).
// ------------------------------------------------------------------------
float TransformMaker::BlendTransform(uint32 iAnim, uint32 iNode, float fTotal, LTBOOL bNormalize)
{
	LTRotation qTransform, qTemp;
	LTVector vTransform;
	float fPercent, fPrev, fCur;

	fTotal = (fTotal > 1.0f) ? 1.0f : fTotal;

	if(bNormalize)
	{
		fPercent = 1.0f - fTotal;
	}
	else
	{
		fPrev = m_pPrevWeightSets[iAnim] ? m_pPrevWeightSets[iAnim]->m_Weights[iNode] : 0.0f;
		fCur = m_pCurWeightSets[iAnim] ? m_pCurWeightSets[iAnim]->m_Weights[iNode] : 0.0f;

		if(fPrev == 2.0f)
			fPercent = fCur;
		else if(fCur == 2.0f)
			fPercent = 2.0f;
		else
			fPercent = fPrev + (fCur - fPrev) * m_Anims[iAnim].m_Percent;
	}

	// Can we be lazy?
	if(fPercent == 0.0f)
	{
		return 0.0f;
	}
	else if(fPercent == 2.0f)
	{
		// Add the animation.
		InitTransformAdditive(iAnim, iNode, qTransform, vTransform);

		m_Quat = m_Quat * qTransform;
		m_vTrans = m_vTrans + vTransform;
		return 0.0f;
	}
	else
	{
		InitTransform(iAnim, iNode, qTransform, vTransform);
		qTemp = m_Quat;
		m_Quat.Slerp(qTemp, qTransform, fPercent);
		m_vTrans = m_vTrans + (vTransform - m_vTrans) * fPercent;
		return fPercent;
	}
}


// ------------------------------------------------------------------------
// Recurse (0x43661f)
// talon: 2001's.  The first time ref's weight comes from its weight sets
// (unless its m_Percent is 2), the rest add theirs as they blend; each
// node's transform is the parent times the animated one times the child
// model relation, and the last child is the loop's next node, not a call
// (the image's `movl $1,0x13c(%ebx)` store is m_iCurPath's, as in the
// engine's twin, nolfsrc transform.cpp's ProcessNode).
// ------------------------------------------------------------------------
void TransformMaker::Recurse(uint32 iNode, LTMatrix *pParentT)
{
	uint32 i, nLast;
	LTMatrix *pMyGlobal;
	ModelNode *pNode;
	CIRelation *pRelation;
	float fTotal, fPrev;

	for(;;)
	{
		pMyGlobal = &m_pOutput[iNode];

		// Apply animation data (first one inits, the rest are blended in).
		InitTransform(0, iNode, m_Quat, m_vTrans);

		if(m_pPrevWeightSets[0] && m_pCurWeightSets[0] && m_Anims[0].m_Percent != 2.0f)
		{
			fPrev = m_pPrevWeightSets[0]->m_Weights[iNode];
			fTotal = fPrev + (m_pCurWeightSets[0]->m_Weights[iNode] - fPrev) * m_Anims[0].m_Percent;
		}
		else
		{
			fTotal = 0.0f;
		}

		for(i=1; i < m_nAnims; i++)
		{
			fTotal += BlendTransform(i, iNode, fTotal, m_Anims[i].m_bNormalize);
		}

		pNode = m_pModel->GetNode(iNode);

		m_Quat.ConvertToMatrix(m_mTemp);

		// Use the offset from the parent if this node only uses rotation data
		// from the animation.
		if(pNode->m_Flags & MNODE_ROTATIONONLY)
		{
			m_mTemp.SetTranslation(pNode->m_vOffsetFromParent);
		}
		else
		{
			m_mTemp.SetTranslation(m_vTrans);
		}

		MatMul(&g_mTemp, pParentT, &m_mTemp);

		m_pCurRelation = &(*m_pNodeRelation)[iNode];
		pRelation = m_pCurRelation;
		pRelation->m_Rot.ConvertToMatrix(m_mCurRelation);
		m_mCurRelation.SetTranslation(pRelation->m_Pos);

		MatMul(pMyGlobal, &g_mTemp, &m_mCurRelation);

		if(m_NC)
			m_NC(m_hNCObject, iNode, pMyGlobal, m_pNCUser);

		// Do the children..
		if(m_pRecursePath)
		{
			if(m_iCurPath > 0)
			{
				m_iCurPath--;
				pParentT = pMyGlobal;
				iNode = m_pRecursePath[m_iCurPath];
			}
			else
				return;
		}
		else
		{
			nLast = pNode->NumChildren();
			if(!nLast)
				return;
			nLast--;

			for(i=0; i < nLast; i++)
			{
				Recurse(pNode->m_Children[i]->GetNodeIndex(), pMyGlobal);
			}

			m_iCurPath = 1;
			iNode = pNode->m_Children[nLast]->GetNodeIndex();
			pParentT = pMyGlobal;
		}
	}
}
