
// ------------------------------------------------------------------------
// model_load.cpp
// talon: 2001's .abc loader, written from the image (0x42fd4d..0x431e78):
// Build 69's tools have no .abc loader (they read LTA only).  The chunked
// format is the one nolfsrc's engine reads (FORMATS.md: a chunk name, the
// offset of the next chunk, its data); Build 69's runtime model_load.cpp
// (jupiter/src/Engine/runtime/model/src) is the same file three years on
// and gave the names.  The addresses are each function's in the image.
// ------------------------------------------------------------------------
#include "bdefs.h"
#include "model.h"
#include "modelallocations.h"

#include <map>
#include <set>
#include <string>
#include <vector>


// ModelLoadRequest::m_pLoadedSet (+0xc): file names of child models to load
// in place of others (0x430c79..0x430dfc; ModelEditDlg news it).
typedef std::map<std::string, std::set<std::string> > ChildModelRemap;

// talon: the LT2 SDK's ltrotation.h has no stream operators for a rotation;
// the image's are out of line (0x435b08, 0x435b46, after pixelformat), so
// simple_dalloc.cpp defines them.
void LTStream_Read(ILTStream *pStream, LTRotation &quat);


// ------------------------------------------------------------------------
// mod_FindChunk (0x42fd6e)
// Seeks to the named chunk's data.  Each chunk is its name, then the offset
// of the next chunk (-1 for the last).
// ------------------------------------------------------------------------
static LTBOOL mod_FindChunk(ILTStream *pStream, const char *pName)
{
	uint32 i, nextChunk;
	char chunkName[256];

	pStream->SeekTo(0);
	for(i=0; i < 512; i++)
	{
		pStream->ReadString(chunkName, sizeof(chunkName));
		pStream->Read(&nextChunk, sizeof(nextChunk));

		if(strcmp(pName, chunkName) == 0)
			return LTTRUE;

		if(nextChunk == (uint32)-1)
			return LTFALSE;

		pStream->SeekTo(nextChunk);
	}

	return LTFALSE;
}


// 0x42fddf
LTBOOL AnimNode::Load(ILTStream &file)
{
	uint32 i;
	AnimNode *pChild;

	if(!m_KeyFrames.SetSize2(m_pAnim->m_KeyFrames.GetSize(), GetModel()->GetAlloc()))
		return LTFALSE;

	for(i=0; i < m_pAnim->m_KeyFrames.GetSize(); i++)
	{
		file.Read(&m_KeyFrames[i], sizeof(NodeKeyFrame));
	}

	if(!m_Children.SetSizeInit4(m_pNode->m_Children.GetSize(), LTNULL, GetModel()->GetAlloc()))
		return LTFALSE;

	for(i=0; i < m_pNode->m_Children.GetSize(); i++)
	{
		pChild = LNew_2P(GetModel()->GetAlloc(), AnimNode, GetAnim(), this);
		if(!pChild)
			return LTFALSE;

		pChild->m_pNode = m_pNode->m_Children[i];
		if(!pChild->Load(file))
		{
			LDelete(GetModel()->GetAlloc(), pChild);
			DeleteAndClearArray2(m_Children, GetModel()->GetAlloc());
			return LTFALSE;
		}

		m_Children[i] = pChild;
	}

	return LTTRUE;
}


// 0x42ff10
LTBOOL ModelAnim::Load(ILTStream &file)
{
	uint32 i, nKeyFrames;
	AnimKeyFrame *pKeyFrame;

	Term();

	if(!m_pModel->LoadString(file, m_pName))
		return LTFALSE;

	if(m_pModel->GetFileVersion() >= 11)
		file >> m_ModelEditWeightSet;

	if(m_pModel->GetFileVersion() >= 12)
		file >> m_InterpolationMS;

	file >> nKeyFrames;
	if(!m_KeyFrames.SetSize2(nKeyFrames, m_pModel->GetAlloc()))
		return LTFALSE;

	for(i=0; i < nKeyFrames; i++)
	{
		pKeyFrame = &m_KeyFrames[i];

		file >> pKeyFrame->m_Time;
		if(!m_pModel->LoadString(file, pKeyFrame->m_pString))
			return LTFALSE;
	}

	m_pRootNode->m_pNode = m_pModel->GetRootNode();
	if(m_pRootNode->Load(file))
		return PrecalcNodeLists(LTTRUE);

	return LTFALSE;
}


// 0x42fffb
LTBOOL ModelNode::Load(ILTStream &file)
{
	uint32 i, j, nChildren;
	LTMatrix mat;
	ModelNode *pChild;

	if(!m_pModel->LoadString(file, m_pName))
		return LTFALSE;

	file >> m_NodeIndex;
	file >> m_Flags;

	for(i=0; i < 4; i++)
	{
		for(j=0; j < 4; j++)
		{
			file >> mat.m[i][j];
		}
	}

	SetGlobalTransform(mat);

	file >> nChildren;
	if(!m_Children.SetSizeInit4(nChildren, LTNULL, m_pModel->GetAlloc()))
		return LTFALSE;

	for(i=0; i < nChildren; i++)
	{
		pChild = LNew_1P(m_pModel->GetAlloc(), ModelNode, m_pModel);
		if(!pChild)
			return LTFALSE;

		if(!pChild->Load(file))
		{
			LDelete(m_pModel->GetAlloc(), pChild);
			DeleteAndClearArray2(m_Children, m_pModel->GetAlloc());
			return LTFALSE;
		}

		m_Children[i] = pChild;
	}

	return LTTRUE;
}


// 0x430170
LTBOOL AnimInfo::Load(ILTStream &file)
{
	file >> m_vDims.x;
	file >> m_vDims.y;
	file >> m_vDims.z;

	return m_pAnim ? m_pAnim->Load(file) : LTFALSE;
}


// 0x4301b4
LTBOOL ChildInfo::Load(ILTStream &file)
{
	uint32 i;

	if(!m_pParentModel)
		return LTFALSE;

	file >> m_SaveIndex;

	if(!m_Relation.SetSize2(m_pParentModel->NumNodes(), m_pParentModel->GetAlloc()))
		return LTFALSE;

	for(i=0; i < m_pParentModel->NumNodes(); i++)
	{
		LTStream_Read(&file, m_Relation[i].m_Pos);
		LTStream_Read(&file, m_Relation[i].m_Rot);
	}

	return LTTRUE;
}


// 0x43022c
LTBOOL PieceLOD::Load(ILTStream &file, uint32 &iCurWeight)
{
	uint32 i, j, nTris, nVerts, nWeights;
	ModelVert *pVert;
	NewVertexWeight *pWeight;
	uint16 tempWord;

	file >> nTris;
	if(!m_Tris.SetSize2(nTris, m_pModel->GetAlloc()))
		return LTFALSE;

	for(i=0; i < nTris; i++)
	{
		for(j=0; j < 3; j++)
		{
			file >> m_Tris[i].m_UVs[j].tu;
			file >> m_Tris[i].m_UVs[j].tv;
			file >> m_Tris[i].m_Indices[j];
		}
	}

	file >> nVerts;
	if(!m_Verts.SetSize2(nVerts, m_pModel->GetAlloc()))
		return LTFALSE;

	for(i=0; i < nVerts; i++)
	{
		pVert = &m_Verts[i];

		file >> tempWord;
		pVert->m_nWeights = tempWord;
		file >> tempWord;
		pVert->m_iReplacement = tempWord;

		nWeights = pVert->m_nWeights;
		if(iCurWeight + nWeights > m_pModel->m_VertexWeights.GetSize())
			return LTFALSE;

		if(pVert->m_nWeights > 0)
		{
			pVert->m_Weights = &m_pModel->m_VertexWeights[iCurWeight];
			iCurWeight += nWeights;

			for(j=0; j < pVert->m_nWeights; j++)
			{
				pWeight = &pVert->m_Weights[j];

				file >> pWeight->m_iNode;
				file >> pWeight->m_Vec[0];
				file >> pWeight->m_Vec[1];
				file >> pWeight->m_Vec[2];
				file >> pWeight->m_Vec[3];
			}
		}

		file >> pVert->m_Vec.x;
		file >> pVert->m_Vec.y;
		file >> pVert->m_Vec.z;
		file >> pVert->m_Normal.x;
		file >> pVert->m_Normal.y;
		file >> pVert->m_Normal.z;
	}

	return LTTRUE;
}


// 0x43044c
LTBOOL ModelPiece::Load(ILTStream &file, uint32 &iCurWeight)
{
	uint32 i, nLODs;
	uint16 tempWord, nOldLODs;
	float dummy;

	file >> tempWord;
	m_iTexture = tempWord;

	file >> m_SpecularPower;
	file >> m_SpecularScale;

	if(m_pModel->GetFileVersion() >= 10)
		file >> m_LODWeight;

	// Old LOD info, skipped.
	file >> nOldLODs;
	for(i=0; i < nOldLODs; i++)
		file >> dummy;

	nLODs = m_pModel->m_LODs.GetSize();
	if(!m_LODs.SetSize2(nLODs, m_pModel->GetAlloc()))
		return LTFALSE;

	for(i=0; i < nLODs; i++)
		m_LODs[i].Init(m_pModel);

	if(file.ReadString(m_Name, sizeof(m_Name)) != LT_OK)
		return LTFALSE;

	if(!PieceLOD::Load(file, iCurWeight))
		return LTFALSE;

	for(i=0; i < nLODs; i++)
	{
		if(!m_LODs[i].Load(file, iCurWeight))
			return LTFALSE;
	}

	return LTTRUE;
}


// 0x430563
LTBOOL WeightSet::Load(ILTStream &file)
{
	uint32 i, nWeights;

	if(file.ReadString(m_Name, sizeof(m_Name)) != LT_OK)
		return LTFALSE;

	file >> nWeights;
	if(!m_Weights.SetSize2(nWeights, m_pModel->GetAlloc()))
		return LTFALSE;

	for(i=0; i < nWeights; i++)
	{
		file >> m_Weights[i];
	}

	return LTTRUE;
}


// 0x4305cc
LTBOOL Model::LoadString(ILTStream &file, char* &pStr)
{
	char tempStr[4096];

	if(file.ReadString(tempStr, sizeof(tempStr)) != LT_OK)
		return LTFALSE;

	pStr = AddString(tempStr);
	return LTTRUE;
}


// 0x430613
// talon: 2001's reads the dims and drops them, and keeps the translation.
LTBOOL Model::LoadAnimBindings(ModelLoadRequest *pRequest, ILTStream &file, LTBOOL bAllowUpdates)
{
	uint32 i, j, nAnimBindings, testIndex;
	ChildInfo *pInfo;
	AnimInfo *pAnimInfo;
	char animName[256];
	LTVector dims, trans;

	if(!mod_FindChunk(&file, "AnimBindings"))
		return LTFALSE;

	for(i=0; i < NumChildModels(); i++)
	{
		pInfo = m_ChildModels[i];

		file >> nAnimBindings;

		// If the save index is wrong or the number of anims is wrong then the
		// bounding radii need to be marked as out of date.
		if(pInfo->m_pModel)
		{
			if(nAnimBindings != pInfo->m_pModel->CalcNumParentAnims() ||
				pInfo->m_SaveIndex != pInfo->m_pModel->GetSaveIndex())
			{
				pRequest->m_bSaveCountValid = LTFALSE;
			}
		}

		for(j=0; j < nAnimBindings; j++)
		{
			if(file.ReadString(animName, sizeof(animName)) != LT_OK)
				return LTFALSE;

			LTStream_Read(&file, dims);
			LTStream_Read(&file, trans);

			// The model didn't exist..
			if(!pInfo->m_pModel)
				continue;

			pAnimInfo = FindAnimInfo(animName, pInfo->m_pModel, &testIndex);
			if(pAnimInfo)
			{
				pAnimInfo->m_vTranslation = trans;
			}
		}
	}

	return LTTRUE;
}


// 0x430722
LTBOOL Model::LoadSockets(ILTStream &file)
{
	uint32 i, nSockets;
	ModelSocket *pSocket;

	if(!mod_FindChunk(&file, "Sockets"))
		return LTFALSE;

	file >> nSockets;
	if(!m_Sockets.SetSizeInit4(nSockets, LTNULL, GetAlloc()))
		return LTFALSE;

	for(i=0; i < nSockets; i++)
	{
		pSocket = LNew(GetAlloc(), ModelSocket);
		if(!pSocket)
			return LTFALSE;

		m_Sockets[i] = pSocket;

		file >> pSocket->m_iNode;
		if(pSocket->m_iNode >= NumNodes())
			return LTFALSE;

		if(file.ReadString(pSocket->m_Name, sizeof(pSocket->m_Name)) != LT_OK)
			return LTFALSE;

		LTStream_Read(&file, pSocket->m_Rot);
		LTStream_Read(&file, pSocket->m_Pos);
	}

	return LTTRUE;
}


// 0x430807
LTBOOL Model::LoadWeightSets(ILTStream &file)
{
	uint32 i, nSets;
	WeightSet *pSet;

	DeleteAndClearArray2(m_WeightSets, GetAlloc());

	file >> nSets;
	if(!m_WeightSets.SetSizeInit4(nSets, LTNULL, GetAlloc()))
		return LTFALSE;

	for(i=0; i < m_WeightSets; i++)
	{
		pSet = m_WeightSets[i] = LNew_1P(GetAlloc(), WeightSet, this);
		if(!pSet || !pSet->Load(file))
		{
			DeleteAndClearArray2(m_WeightSets, GetAlloc());
			return LTFALSE;
		}
	}

	return LTTRUE;
}


// ------------------------------------------------------------------------
// Model::Load (0x4308ab)
// talon: 2001's.  Returns LT_INVALIDPARAMS for a bad request, LT_OK, or
// the step it failed at (1..0x1d, after Term()).  Retail bugs kept: a
// header it can't take returns 0 (LT_OK) (0x430914), and a failed m_LODs
// SetSize returns 0 without even a Term (0x430986).
// talon: near (2379 bytes against 2372, the same calls in the same order,
// checked instruction by instruction).  The image expands push_back and
// calls vector::insert(iterator, const T&) (0x431366) at the six adds; cl
// 9044 here either calls push_back or, written as insert(end(), x),
// expands that and calls insert(iterator, n, x).  The error code sits in
// pRequest's top byte (0xb(%ebp)) in the image, in its own slot here.
// ------------------------------------------------------------------------
LTRESULT Model::Load(ModelLoadRequest *pRequest)
{
	uint32 i, j, curOffset, nChildModels, nToLoad;
	uint32 nAnims, nPieces, nVertexWeights, iCurWeight;
	uint16 nLODs;
	uint8 dummy;
	uint8 error_val;
	LTRESULT dResult;
	Model *pModel;
	ModelAnim *pAnim;
	ModelPiece *pPiece;
	ChildInfo *pInfo, *pChildModel;
	char *pFilename, *pName;
	ChildModelRemap *pRemap;
	ModelLoadRequest myLoadRequest;
	ModelAllocations allocs;

	// Make sure the request is valid.
	if(!pRequest->m_pFile ||
		(pRequest->m_bLoadChildModels && !pRequest->m_LoadChildFn))
	{
		return LT_INVALIDPARAMS;
	}

	Term();

	ILTStream &file = *pRequest->m_pFile;

	if(!LoadHeader(file, allocs))
		{error_val = 0; goto Error;}

	if(!LoadString(file, m_CommandString))
		{error_val = 1; goto Error;}

	file >> m_GlobalRadius;

	// The LOD distances, after 62 reserved bytes.
	file >> nLODs;
	for(i=0; i < 62; i++)
		file >> dummy;

	if(!m_LODs.SetSize2(nLODs, GetAlloc()))
		return LTFALSE;

	for(i=0; i < nLODs; i++)
		file >> m_LODs[i].m_Dist;

	// Pieces.
	if(!mod_FindChunk(&file, "Pieces"))
		{error_val = 2; goto Error;}

	file >> nVertexWeights;
	if(!m_VertexWeights.SetSize2(nVertexWeights, GetAlloc()))
		{error_val = 3; goto Error;}

	file >> nPieces;
	if(!m_Pieces.SetSizeInit4(nPieces, LTNULL, GetAlloc()))
		{error_val = 4; goto Error;}

	iCurWeight = 0;
	for(i=0; i < nPieces; i++)
	{
		pPiece = LNew_1P(GetAlloc(), ModelPiece, this);
		if(!pPiece)
		{
			Term();
			error_val = 5;
			goto Error;
		}

		m_Pieces[i] = pPiece;
		if(!pPiece->Load(file, iCurWeight))
			{error_val = 6; goto Error;}
	}

	// Nodes.
	if(!mod_FindChunk(&file, "Nodes"))
		{error_val = 7; goto Error;}

	if(!GetRootNode()->Load(file))
		{error_val = 8; goto Error;}

	if(!LoadWeightSets(file))
		{error_val = 9; goto Error;}

	// (need the node lists for the name searches it does in here).
	if(!PrecalcNodeLists(LTTRUE))
		{error_val = 10; goto Error;}

	// Child models.
	if(!mod_FindChunk(&file, "ChildModels"))
		{error_val = 11; goto Error;}

	file >> nChildModels;

	// Must have at least one (for the model itself).
	if(nChildModels == 0 || nChildModels > MAX_CHILD_MODELS)
		{error_val = 12; goto Error;}

	{
		std::vector<char*> filenames;
		std::vector<char*> loadNames;
		std::vector<ChildInfo*> infos;
		std::vector<ChildInfo*> loadInfos;

		pRemap = (ChildModelRemap*)pRequest->m_pLoadedSet;
		nToLoad = 0;

		// Add ourselves as the first child model.
		if(!InitChildInfo(0, &m_SelfChildModel, this, "SELF"))
			{error_val = 13; goto ChildError;}

		if(!m_SelfChildModel.Load(file))
			{error_val = 14; goto ChildError;}

		InitChildInfo(0, &m_SelfChildModel, this, "SELF");

		// Read the child model names and their ChildInfos.
		for(i=0; i < nChildModels-1; i++)
		{
			pFilename = LTNULL;
			if(!LoadString(file, pFilename))
				{error_val = 15; goto ChildError;}

			filenames.insert(filenames.end(), pFilename);

			pInfo = new ChildInfo;
			pInfo->m_pParentModel = this;
			if(!pInfo->Load(file))
				{error_val = 16; goto ChildError;}

			infos.insert(infos.end(), pInfo);
		}

		// Remap the names the caller asks for.
		for(i=0; i < nChildModels-1; i++)
		{
			pName = filenames[i];
			pInfo = infos[i];

			if(pRemap && pRemap->find(std::string(pName)) != pRemap->end())
			{
				std::set<std::string>::iterator it = (*pRemap)[std::string(pName)].begin();
				(*pRemap)[std::string(pName)];

				m_Unknown190 = 1;
				pName = AddString(it->c_str());
				loadNames.insert(loadNames.end(), pName);
				loadInfos.insert(loadInfos.end(), pInfo);
				nToLoad++;
				it++;
			}
			else
			{
				loadNames.insert(loadNames.end(), pName);
				loadInfos.insert(loadInfos.end(), pInfo);
				nToLoad++;
			}
		}

		// Load them.
		for(i=0; i < nToLoad; i++)
		{
			myLoadRequest.m_pFile = LTNULL;
			myLoadRequest.m_pFilename = loadNames[i];
			myLoadRequest.m_LoadChildFn = pRequest->m_LoadChildFn;
			myLoadRequest.m_pLoadFnUserData = pRequest->m_pLoadFnUserData;
			myLoadRequest.m_bLoadChildModels = LTFALSE;  //  Recursive loading is NOT supported.

			pModel = LTNULL;

			if(pRequest->m_bLoadChildModels)
				dResult = pRequest->m_LoadChildFn(&myLoadRequest, &pModel);
			else
				dResult = LT_NOCHANGE; // (don't load it)

			if(dResult != LT_OK && dResult != LT_NOCHANGE)
				{error_val = 0x15; goto ChildError;}

			if(!pModel)
			{
				pRequest->m_bAllChildrenLoaded = LTFALSE;
			}
			else if(pModel->NumChildModels() > 1)
			{
				// Make sure the child model doesn't have any child models.
				error_val = 0x12;
				goto ChildError;
			}

			pChildModel = LNew(GetAlloc(), ChildInfo);
			if(!pChildModel)
				{error_val = 0x13; goto ChildError;}

			*pChildModel = *loadInfos[i];

			if(!InitChildInfo(i+1, pChildModel, pModel, myLoadRequest.m_pFilename))
				{error_val = 0x14; goto ChildError;}

			m_nChildModels++;

			pChildModel->m_bTreeValid = LTFALSE;
			if(pChildModel->m_pModel)
			{
				ModelNode *pErrNode;
				pChildModel->m_bTreeValid = VerifyChildModelTree(pChildModel->m_pModel, pErrNode);
			}
		}

		for(i=0; i < infos.size(); i++)
		{
			delete infos[i];
		}

		goto ChildDone;

ChildError:
		;
	}
	goto Error;

ChildDone:
	// Load animations.
	if(!mod_FindChunk(&file, "Animation"))
		{error_val = 0x16; goto Error;}

	file >> nAnims;
	if(!m_Anims.SetSize2(nAnims + CalcNumChildModelAnims(LTFALSE), GetDefAlloc()))
		{error_val = 0x17; goto Error;}

	for(i=0; i < nAnims; i++)
	{
		pAnim = LNew_1P(GetAlloc(), ModelAnim, this);
		if(!pAnim)
			{error_val = 0x18; goto Error;}

		m_Anims[i].m_pAnim = pAnim;
		m_Anims[i].m_pChildInfo = &m_SelfChildModel;

		if(!m_Anims[i].Load(file))
		{
			LDelete(GetAlloc(), pAnim);
			error_val = 0x19;
			goto Error;
		}
	}

	// Add the anims from child models to our list.
	curOffset = nAnims;
	for(i=0; i < m_nChildModels; i++)
	{
		pChildModel = m_ChildModels[i];

		if(pChildModel->m_pModel && pChildModel->m_pModel != this)
		{
			pChildModel->m_AnimOffset = curOffset;

			for(j=0; j < pChildModel->m_pModel->m_Anims; j++)
			{
				m_Anims[curOffset] = pChildModel->m_pModel->m_Anims[j];
				m_Anims[curOffset].m_pChildInfo = pChildModel;
				curOffset++;
			}
		}
	}

	// Load the animation bindings that exist.
	if(!LoadAnimBindings(pRequest, file, LTTRUE))
		{error_val = 0x1a; goto Error;}

	// Load sockets.
	if(!LoadSockets(file))
		{error_val = 0x1b; goto Error;}

	if(file.ErrorStatus() != LT_OK)
		{error_val = 0x1c; goto Error;}

	if(!PrecalcNodeLists(LTFALSE))
		{error_val = 0x1d; goto Error;}

	ParseCommandString();
	SetNodeParentOffsets();

	return LT_OK;

// Error conditions go here.
Error:
	Term();
	return error_val;
}


// 0x431233
// talon: near; the image has the first `return LTFALSE` right after the
// chunk test, ours shares the one at the end (same instructions otherwise).
LTBOOL Model::LoadHeader(ILTStream &file, ModelAllocations &allocs)
{
	if(!mod_FindChunk(&file, "Header"))
		return LTFALSE;

	file >> m_FileVersion;

	if(m_FileVersion == 9 || m_FileVersion == 10 ||
		m_FileVersion == 11 || m_FileVersion == 12)
	{
		return allocs.Load(file) != LTFALSE;
	}

	return LTFALSE;
}
