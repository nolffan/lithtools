// ------------------------------------------------------------------------
// model_save.cpp
// tools
// create binary data from lta.
// talon: 2001's .abc writer, written from the image (0x43208c..0x432cbb):
// the same chunks model_load reads, file version 12.  Build 69's
// model_save.cpp (an LTB-era writer with compressed animation) kept only
// the shape of the Save members; their bodies here are the image's.
// ------------------------------------------------------------------------
#include <windows.h>
#include "stdlith.h"
#include "model.h"

// talon: the module opens with std::ctype<unsigned short>::id's 21-byte
// initializer (0x43208c), which only <xlocale> (via <string>) gives it.
#include <string>
#include "sysstreamsim.h"
#include "modelallocations.h"

#include <vector>

using namespace std ;

// talon: the LT2 SDK's ltrotation.h has no stream operators for a rotation;
// the image's are out of line (0x435b08, 0x435b46, after pixelformat), so
// simple_dalloc.cpp defines them.
void LTStream_Write(ILTStream *pStream, LTRotation &quat);


// ------------------------------------------------------------------------
// mod_StartChunk (0x43236e)
// Ends the previous chunk (writes the new chunk's offset into its "next"
// slot) and starts a new one: its name and a -1 "next" slot.  Returns
// where that slot is.
// ------------------------------------------------------------------------
static uint32 mod_StartChunk(ILTStream *pStream, const char *pName, uint32 prevChunk)
{
	uint32 nextPos;

	if(prevChunk != (uint32)-1)
	{
		uint32 curPos;

		pStream->GetPos(&curPos);
		pStream->SeekTo(prevChunk);
		pStream->WriteVal(curPos);
		pStream->SeekTo(curPos);
	}

	pStream->WriteString((char*)pName);
	pStream->GetPos(&nextPos);
	pStream->WriteVal((uint32)-1);

	return nextPos;
}


// 0x4320ad
LTBOOL AnimNode::Save(ILTStream &file)
{
	uint32 i;

	if(m_Children.GetSize() != m_pNode->m_Children.GetSize())
		return LTFALSE;

	for(i=0; i < m_pAnim->m_KeyFrames.GetSize(); i++)
	{
		file.Write(&m_KeyFrames[i], sizeof(NodeKeyFrame));
	}

	for(i=0; i < m_pNode->m_Children.GetSize(); i++)
	{
		if(!m_Children[i]->Save(file))
			return LTFALSE;
	}

	return LTTRUE;
}


// 0x43211e
LTBOOL ModelAnim::Save(ILTStream &file)
{
	uint32 i;
	AnimKeyFrame *pKeyFrame;

	file.WriteString(m_pName);
	file << m_ModelEditWeightSet;
	file << m_InterpolationMS;

	file.WriteVal(m_KeyFrames.GetSize());
	for(i=0; i < m_KeyFrames; i++)
	{
		pKeyFrame = &m_KeyFrames[i];

		file << pKeyFrame->m_Time;
		file.WriteString(pKeyFrame->m_pString);
	}

	return m_pRootNode->Save(file);
}


// 0x4321a7
LTBOOL ModelNode::Save(ILTStream &file)
{
	uint32 i, j;

	file.WriteString(m_pName);
	file << m_NodeIndex;
	file << m_Flags;

	for(i=0; i < 4; i++)
	{
		for(j=0; j < 4; j++)
		{
			file << m_mGlobalTransform.m[i][j];
		}
	}

	file.WriteVal(m_Children.GetSize());
	for(i=0; i < m_Children; i++)
	{
		if(!m_Children[i]->Save(file))
			return LTFALSE;
	}

	return LTTRUE;
}


// 0x432242
LTBOOL AnimInfo::Save(ILTStream &file)
{
	file << m_vDims.x;
	file << m_vDims.y;
	file << m_vDims.z;

	return m_pAnim ? m_pAnim->Save(file) : LTFALSE;
}


// 0x432286
// talon: 2001's starts its own chunk (chunkPos is the previous one's).
LTBOOL Model::SaveAnimBindings(ILTStream &file, uint32 &chunkPos)
{
	uint32 i, j, nAnims;
	ChildInfo *pInfo;
	AnimInfo *pAnimInfo;

	chunkPos = mod_StartChunk(&file, "AnimBindings", chunkPos);

	for(i=0; i < m_nChildModels; i++)
	{
		pInfo = m_ChildModels[i];

		if(pInfo->m_pModel)
		{
			nAnims = pInfo->m_pModel->CalcNumParentAnims();
			file << nAnims;

			for(j=0; j < nAnims; j++)
			{
				pAnimInfo = &m_Anims[pInfo->m_AnimOffset + j];

				if(pAnimInfo->m_pChildInfo->m_pParentModel == this)
				{
					file.WriteString(pAnimInfo->m_pAnim->GetName());
					LTStream_Write(&file, pAnimInfo->m_vDims);
					LTStream_Write(&file, pAnimInfo->m_vTranslation);
				}
			}
		}
		else
		{
			file.WriteVal((uint32)0);
		}
	}

	return LTTRUE;
}


// 0x4323da
LTBOOL ChildInfo::Save(ILTStream &file)
{
	uint32 i;
	CIRelation defRelation;
	LTRotation *pRot;

	if(!m_pParentModel)
		return LTFALSE;

	file << m_SaveIndex;

	for(i=0; i < m_pParentModel->NumNodes(); i++)
	{
		if(i < m_Relation.GetSize())
		{
			LTStream_Write(&file, m_Relation[i].m_Pos);
			pRot = &m_Relation[i].m_Rot;
		}
		else
		{
			defRelation.m_Pos.Init();
			defRelation.m_Rot.Init();
			LTStream_Write(&file, defRelation.m_Pos);
			pRot = &defRelation.m_Rot;
		}

		LTStream_Write(&file, *pRot);
	}

	return LTTRUE;
}


// 0x43247d
LTBOOL PieceLOD::Save(ILTStream &file)
{
	uint32 i, j;
	ModelVert *pVert;

	file.WriteVal(m_Tris.GetSize());
	for(i=0; i < m_Tris; i++)
	{
		for(j=0; j < 3; j++)
		{
			file << m_Tris[i].m_UVs[j].tu;
			file << m_Tris[i].m_UVs[j].tv;
			file << m_Tris[i].m_Indices[j];
		}
	}

	file.WriteVal(m_Verts.GetSize());
	for(i=0; i < m_Verts; i++)
	{
		pVert = &m_Verts[i];

		file.WriteVal(pVert->m_nWeights);
		file.WriteVal(pVert->m_iReplacement);

		for(j=0; j < pVert->m_nWeights; j++)
		{
			file << pVert->m_Weights[j].m_iNode;
			file << pVert->m_Weights[j].m_Vec[0];
			file << pVert->m_Weights[j].m_Vec[1];
			file << pVert->m_Weights[j].m_Vec[2];
			file << pVert->m_Weights[j].m_Vec[3];
		}

		file << pVert->m_Vec.x;
		file << pVert->m_Vec.y;
		file << pVert->m_Vec.z;
		file << pVert->m_Normal.x;
		file << pVert->m_Normal.y;
		file << pVert->m_Normal.z;
	}

	return LTTRUE;
}


// 0x43263f
LTBOOL ModelPiece::Save(ILTStream &file)
{
	uint32 i;

	if(NumLODs() != m_pModel->NumLODs())
		return LTFALSE;

	file.WriteVal((uint16)m_iTexture);
	file << m_SpecularPower;
	file << m_SpecularScale;
	file << m_LODWeight;

	// No old LOD info.
	file.WriteVal((uint16)0);
	file.WriteString(m_Name);

	if(!PieceLOD::Save(file))
		return LTFALSE;

	for(i=0; i < m_LODs; i++)
	{
		if(!m_LODs[i].Save(file))
			return LTFALSE;
	}

	return LTTRUE;
}


// 0x4326f5
LTBOOL WeightSet::Save(ILTStream &file)
{
	uint32 i;

	file.WriteString(m_Name);

	file.WriteVal(m_Weights.GetSize());
	for(i=0; i < m_Weights; i++)
	{
		file << m_Weights[i];
	}

	return LTTRUE;
}


// 0x432742
LTBOOL Model::SaveSockets(ILTStream &file)
{
	uint32 i;
	ModelSocket *pSocket;

	file.WriteVal(m_Sockets.GetSize());
	for(i=0; i < m_Sockets; i++)
	{
		pSocket = m_Sockets[i];

		file << pSocket->m_iNode;
		file.WriteString(pSocket->m_Name);
		LTStream_Write(&file, pSocket->m_Rot);
		LTStream_Write(&file, pSocket->m_Pos);
	}

	return LTTRUE;
}


// 0x4327c1
LTBOOL Model::SaveWeightSets(ILTStream &file)
{
	uint32 i;

	file.WriteVal(m_WeightSets.GetSize());
	for(i=0; i < m_WeightSets; i++)
	{
		if(!m_WeightSets[i]->Save(file))
			return LTFALSE;
	}

	return LTTRUE;
}


// 0x43280b (the name is ours: Save's body)
LTBOOL Model::SaveToStream(ILTStream &file)
{
	uint32 i, chunkPos, nodesPos;
	ModelAllocations allocs;
	ChildInfo *pInfo;
	AnimInfo *pAnimInfo;

	// Our save index changes with each save.
	m_SelfChildModel.m_SaveIndex++;

	chunkPos = mod_StartChunk(&file, "Header", (uint32)-1);
	file.WriteVal((uint32)12);

	if(!allocs.InitFromModel(this) || !allocs.Save(file))
		return LTFALSE;

	file.WriteString(m_CommandString);
	file << m_GlobalRadius;

	// The LOD distances, after 62 reserved bytes.
	file.WriteVal((uint16)m_LODs.GetSize());
	for(i=0; i < 62; i++)
		file.WriteVal((uint8)0);

	for(i=0; i < m_LODs; i++)
		file << m_LODs[i].m_Dist;

	// Pieces.
	chunkPos = mod_StartChunk(&file, "Pieces", chunkPos);
	file.WriteVal(CalcNumVertexWeights());
	file.WriteVal(m_Pieces.GetSize());
	for(i=0; i < m_Pieces; i++)
	{
		if(!m_Pieces[i]->Save(file))
			return LTFALSE;
	}

	// Nodes.
	nodesPos = mod_StartChunk(&file, "Nodes", chunkPos);
	if(!GetRootNode()->Save(file) || !SaveWeightSets(file))
		return LTFALSE;

	// Child models.
	chunkPos = mod_StartChunk(&file, "ChildModels", nodesPos);
	file.WriteVal(m_nChildModels);
	for(i=0; i < m_nChildModels; i++)
	{
		pInfo = m_ChildModels[i];

		if(i != 0)
			file.WriteString(pInfo->m_pFilename);

		if(!pInfo->Save(file))
			return LTFALSE;
	}

	// Animations.
	chunkPos = mod_StartChunk(&file, "Animation", chunkPos);
	file.WriteVal(CalcNumParentAnims());
	for(i=0; i < m_Anims; i++)
	{
		pAnimInfo = &m_Anims[i];

		if(pAnimInfo->GetAnimOwner() == this && !pAnimInfo->Save(file))
			return LTFALSE;
	}

	// Sockets and animation bindings.
	chunkPos = mod_StartChunk(&file, "Sockets", chunkPos);
	if(!SaveSockets(file) || !SaveAnimBindings(file, chunkPos))
		return LTFALSE;

	return file.ErrorStatus() == LT_OK;
}


// 0x432a9a
// Fix for a bug in ModelEdit where it sometimes saves animations in an order
// different from animation bindings.
LTBOOL Model::ForceAnimOrder()
{
	CMoArray<AnimInfo,NoCache> newAnimList;
	uint32 i, iOut;

	if(!newAnimList.SetSize(m_Anims.GetSize()))
		return LTFALSE;

	// Make sure our animations are the first in the list.
	iOut = 0;
	for(i=0; i < m_Anims; i++)
	{
		if(m_Anims[i].IsParentModel())
		{
			newAnimList[iOut] = m_Anims[i];
			iOut++;
		}
	}

	for(i=0; i < m_Anims; i++)
	{
		if(!m_Anims[i].IsParentModel())
		{
			newAnimList[iOut] = m_Anims[i];
			iOut++;
		}
	}

	return m_Anims.CopyArray(newAnimList);
}


// 0x432bb7
LTBOOL Model::Save(ILTStream &file)
{
	ModelLoadRequest request;

	ForceAnimOrder();
	return SaveToStream(file) != LTFALSE;
}


// 0x432be4 (the name is ours)
// Copies pSrc into this model by saving it to memory and loading it back.
LTBOOL Model::CopyModel(Model *pSrc)
{
	ModelLoadRequest request;
	LTBOOL bRet;

	bRet = LTFALSE;
	request.m_pFile = streamsim_OpenMemStream(0xFFFF);
	if(request.m_pFile)
	{
		if(pSrc->Save(*request.m_pFile))
		{
			request.m_pFile->SeekTo(0);
			if(Load(&request) == LT_OK)
				bRet = LTTRUE;
		}

		request.m_pFile->Release();
	}

	return bRet;
}


// 0x432c40 (the name is ours)
// Copies pSrc into this animation the same way.
LTBOOL ModelAnim::CopyAnim(ModelAnim *pSrc)
{
	ILTStream *pStream;
	LTBOOL bRet;

	bRet = LTFALSE;
	pStream = streamsim_OpenMemStream(0xFFFF);
	if(pStream)
	{
		if(pSrc->Save(*pStream))
		{
			pStream->SeekTo(0);
			if(Load(*pStream))
				bRet = LTTRUE;
		}

		pStream->Release();
	}

	return bRet;
}
