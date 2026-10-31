#pragma warning (disable:4786)

#include "bdefs.h"
#include "modelallocations.h"

// talon: the module opens with std::ctype<unsigned short>::id's 21-byte
// initializer (0x432cbb), which only <xlocale> (via <string>) gives it.
#include <string>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
ModelAllocations::ModelAllocations()
{
	Clear();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void ModelAllocations::Clear()
{
	m_nKeyFrames = 0;
	m_nParentAnims = 0;
	m_nNodes = 0;
	m_nPieces = 0;
	m_nChildModels = 0;
	m_nTris = 0;
	m_nVerts = 0;
	m_nVertexWeights = 0;
	m_nLODs = 0;
	m_nSockets = 0;
	m_nWeightSets = 0;
	m_nStrings = 0;
	m_StringLengths = 0;
}


//-----------------------------------------------------------------------------
// InitFromModel (0x432d11)
// calculate the size of the model.
// talon: 2001's.  The LOD count is the model's (it gives up on a model with
// none, before clearing), and the strings are the animations', their
// keyframes', the nodes', the command string and the child model file names.
//-----------------------------------------------------------------------------
LTBOOL ModelAllocations::InitFromModel(Model *pModel)
{
	uint32 iAnim, iPiece, iLOD, iKey, iNode, iChild;
	ModelAnim *pAnim;
	ModelPiece *pPiece;
	ModelString *pString;
	PieceLOD *pLOD;
	ModelStringList tempStringList;
	
	if(pModel->NumLODs() < 1)
		return LTFALSE;

	Clear();

	m_nParentAnims = pModel->CalcNumParentAnims();
	m_nNodes = pModel->NumNodes();
	m_nPieces = pModel->NumPieces();
	m_nChildModels = pModel->NumChildModels();
	m_nLODs = pModel->NumLODs();
	m_nSockets = pModel->NumSockets();
	m_nWeightSets = pModel->NumWeightSets();
	m_nVertexWeights = pModel->CalcNumVertexWeights();
	
	// Count keyframes.
	for(iAnim=0; iAnim < m_nParentAnims; iAnim++)
	{
		pAnim = pModel->GetAnim(iAnim);
		m_nKeyFrames += pAnim->NumKeyFrames();
	}

	// Count geometry stuff.
	for(iPiece=0; iPiece < pModel->NumPieces(); iPiece++)
	{
		pPiece = pModel->GetPiece(iPiece);
		
		for(iLOD=0; iLOD < pPiece->NumLODs(); iLOD++)
		{
			pLOD = pPiece->GetLOD(iLOD);

			m_nTris += pLOD->m_Tris.GetSize();
			m_nVerts += pLOD->m_Verts.GetSize();
		}
	}

	// Count strings.
	for(iAnim=0; iAnim < m_nParentAnims; iAnim++)
	{
		pAnim = pModel->GetAnim(iAnim);
		
		tempStringList.AddString(pAnim->GetName());

		for(iKey=0; iKey < pAnim->NumKeyFrames(); iKey++)
			tempStringList.AddString(pAnim->m_KeyFrames[iKey].m_pString);
	}

	for(iNode=0; iNode < pModel->NumNodes(); iNode++)
		tempStringList.AddString(pModel->GetNode(iNode)->GetName());

	tempStringList.AddString(pModel->m_CommandString);

	for(iChild=1; iChild < pModel->NumChildModels(); iChild++)
		tempStringList.AddString(pModel->GetChildModel(iChild)->m_pFilename);

	// Tally it up...
	for(pString=tempStringList.m_StringList; pString; pString=pString->m_pNext)
	{
		++m_nStrings;
		m_StringLengths += strlen(pString->m_String);
	}

	return LTTRUE;
}


LTBOOL ModelAllocations::Load(ILTStream &str)
{
	str >> m_nKeyFrames;
	str >> m_nParentAnims;
	str >> m_nNodes;
	str >> m_nPieces;
	str >> m_nChildModels;
	str >> m_nTris;
	str >> m_nVerts;
	str >> m_nVertexWeights;
	str >> m_nLODs;
	str >> m_nSockets;
	str >> m_nWeightSets;
	str >> m_nStrings;
	str >> m_StringLengths;

	return str.ErrorStatus() == LT_OK;
}


LTBOOL ModelAllocations::Save(ILTStream &str)
{
	str << m_nKeyFrames;
	str << m_nParentAnims;
	str << m_nNodes;
	str << m_nPieces;
	str << m_nChildModels;
	str << m_nTris;
	str << m_nVerts;
	str << m_nVertexWeights;
	str << m_nLODs;
	str << m_nSockets;
	str << m_nWeightSets;
	str << m_nStrings;
	str << m_StringLengths;

	return str.ErrorStatus() == LT_OK;
}
