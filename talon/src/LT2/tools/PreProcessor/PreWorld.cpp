#include "bdefs.h"

#include "list"
#include "queue"

using namespace std;

#include "preworld.h"
#include "prepoly.h"
#include "processing.h"
#include "prelightmap.h"
#include "bspgen.h"
#include "filemarker.h"
#include "memoryio.h"
#include "compressedio.h"
#include "streamsim.h"
#include "preleaf.h"
#include "node.h"
#include "lightmap_planes.h"
#include "pregeometry.h"
#include "dtxmgr.h"
#include "genltstream.h"
#include "de_memory.h"

// talon: the 2001 PreWorld.cpp (0x430a40..0x43e5a0), from Build 69's with
// the DAT format, light animations, AI volumes, sub-BSPs and physics blocks
// of 2001. The names of what Build 69 lacks are ours.


// A stack of nodes for walking BSP trees without recursion (the name is
// ours; CalcNodeBoundingBoxes_R, 0x439e60, uses it).
static CMoArray<CNode*> g_NodeStack;


class PointStackEl
{
	public:

		void	AddToWorld(CPreWorld *pWorld);

		PReal m_Mag;

		PVector *m_pPoint;

		// All the WORD pointers that depend on this coordinate.
		list<uint32*> m_Attachments;

};

void PointStackEl::AddToWorld(CPreWorld *pWorld)
{
	list<uint32*>::iterator iAttachment;
	PVertex vert;

	vert.PVector::operator=(*m_pPoint);
	pWorld->m_Points.Append(vert);

	for(iAttachment=m_Attachments.begin();
		iAttachment != m_Attachments.end(); iAttachment++)
	{
		*(*iAttachment) = (uint16)pWorld->m_Points.LastI();
	}
}



// ----------------------------------------------------------------------- //
// PreLightFrame.
// ----------------------------------------------------------------------- //

CPreLightFrame::CPreLightFrame()
{
	m_PolyMaps.SetCacheSize(1024);
	m_Unknown_14.SetCacheSize(1024);
}


CPreLightFrame::~CPreLightFrame()
{
	DeleteAndClearArray(m_PolyMaps);
	DeleteAndClearArray(m_Unknown_14);
}



// ----------------------------------------------------------------------- //
// PreLightAnim.
// ----------------------------------------------------------------------- //

CPreLightAnim::CPreLightAnim()
{
	m_Name[0] = 0;
	m_bShadowMap = FALSE;
}


CPreLightAnim::~CPreLightAnim()
{
	DeleteAndClearArray(m_Frames);
}


uint32 CPreLightAnim::CalcFrameDataSize()
{
	uint32 iFrame, iMap, size;
	CPreLightFrame *pFrame;


	size = 0;
	for(iFrame=0; iFrame < m_Frames; iFrame++)
	{
		pFrame = m_Frames[iFrame];

		for(iMap=0; iMap < pFrame->m_PolyMaps; iMap++)
		{
			if(pFrame->m_PolyMaps[iMap])
				size += pFrame->m_PolyMaps[iMap]->m_Data.GetSize();
		}

		for(iMap=0; iMap < pFrame->m_Unknown_14; iMap++)
		{
			if(pFrame->m_Unknown_14[iMap])
				size += pFrame->m_Unknown_14[iMap]->GetSize() * 3;
		}
	}

	return size;
}


// ----------------------------------------------------------------------- //
// DAT file helpers (the names are ours).
// ----------------------------------------------------------------------- //

float ReadFloat(CAbstractIO &file)
{
	float val;

	file >> val;
	return val;
}

LTVector ReadVector(CAbstractIO &file)
{
	float vals[3];

	file >> vals[0];
	file >> vals[1];
	file >> vals[2];

	LTVector vec(vals[0], vals[1], vals[2]);
	return vec;
}

void WriteVector(CAbstractIO &file, LTVector &vec)
{
	file << vec.x << vec.y << vec.z;
}


// ----------------------------------------------------------------------- //
// CPreMainWorld.
// ----------------------------------------------------------------------- //

CPreMainWorld::CPreMainWorld()
{
	m_pInfoString	= "";
	m_Unknown_f0	= 0;
	m_WorldModels.SetCacheSize(1024);
}


CPreMainWorld::~CPreMainWorld()
{
	Term();
}


void CPreMainWorld::Term()
{
	DeleteAndClearArray(m_LightAnims);
	DeleteAndClearArray(m_Objects);

	m_pInfoString = "";
	m_StringHolder.ClearStrings();
	TermGeometry();
}


void CPreMainWorld::TermGeometry()
{
	DeleteAndClearArray(m_WorldModels);

	m_WorldTree.Term();
	RemoveAllLightMaps();
}


// ----------------------------------------------------------------------- //
// The DAT file.
// ----------------------------------------------------------------------- //

// The DAT file's version (the name is ours).
#define CURRENT_DAT_VERSION		70

// The object list (EditRegion's, 0x40ef40 and 0x40f290; the names are ours).
BOOL LoadObjectList(CAbstractIO &file, CMoArray<CBaseEditObj*> &objects);
void SaveObjectList(CAbstractIO &file, CMoArray<CBaseEditObj*> &objects);

static void CopyStream(CAbstractIO &dest, CAbstractIO &src, uint32 endPos);


// 0 if it loaded, 1 if the version is wrong, otherwise corrupt.
uint32 CPreMainWorld::LoadFile(CAbstractIO &file, BOOL bLoadObjects)
{
	CCompressedIO	compressedIO;
	uint32			version, objectDataPos, lightDataPos, dummy, infoLen;
	uint32			nWorldModels, i, worldDataPos, status;
	char			*pInfoString;
	ILTStream		*pStream;


	Term();

	try
	{
		file >> version;
		if(version != CURRENT_DAT_VERSION)
			return 1;

		file >> objectDataPos;
		file >> lightDataPos;
		file >> dummy;
		file >> dummy;
		file >> dummy;
		file >> dummy;
		file >> dummy;
		file >> dummy;
		file >> dummy;
		file >> dummy;

		// The world info string.
		file >> infoLen;
		pInfoString = new char[infoLen+1];
		file.Read(pInfoString, infoLen);
		pInfoString[infoLen] = 0;
		m_pInfoString = m_StringHolder.AddString(pInfoString, TRUE);
		delete pInfoString;

		m_LMGridSize = ReadFloat(file);
		m_PosMin = ReadVector(file);
		m_PosMax = ReadVector(file);

		pStream = streamsim_AbstractIOWrapper(&file);
		if(!pStream)
			return 2;

		status = m_WorldTree.LoadLayout(pStream);
		pStream->Release();
		if(!status)
			return 2;

		// The world models.
		file >> nWorldModels;
		m_WorldModels.SetSizeInit2(nWorldModels, NULL);
		for(i=0; i < nWorldModels; i++)
		{
			m_WorldModels[i] = new CPreWorld(this);

			file >> worldDataPos;
			status = m_WorldModels[i]->LoadFile(file);
			if(status)
			{
				Term();
				return status;
			}
		}

		if(bLoadObjects)
			LoadObjectList(file, m_Objects);
		else
			file.SeekTo(lightDataPos);

		// 0x43178b: the failure path (Term, 2) is laid out before success.
		if(!LoadLightAnims(file))
		{
			Term();
			return 2;
		}

		if(!SetupLeafPolies())
		{
			Term();
			return 2;
		}

		return 0;
	}
	catch(CLithException &exception)
	{
		Term();
		throw exception;
	}
}


// Copies inFile (a DAT file) to outFile with these objects.
BOOL CPreMainWorld::SaveObjectsOnly(CAbstractIO &outFile, CAbstractIO &inFile)
{
	uint32 version, objectDataPos, lightDataPos, dummy, infoLen;
	char *pInfoString;


	inFile >> version;
	inFile >> objectDataPos;
	inFile >> lightDataPos;
	if(version != CURRENT_DAT_VERSION)
		return FALSE;

	inFile >> dummy;
	inFile >> dummy;
	inFile >> dummy;
	inFile >> dummy;
	inFile >> dummy;
	inFile >> dummy;
	inFile >> dummy;
	inFile >> dummy;

	inFile >> infoLen;
	pInfoString = new char[infoLen+1];
	inFile.Read(pInfoString, infoLen);
	delete pInfoString;

	outFile << version;
	CFileMarker objectMarker(outFile, FALSE);
	CFileMarker lightMarker(outFile, FALSE);
	outFile << (uint32)0 << (uint32)0 << (uint32)0 << (uint32)0;
	outFile << (uint32)0 << (uint32)0 << (uint32)0 << (uint32)0;

	outFile << (uint32)strlen(m_pInfoString);
	outFile.Write(m_pInfoString, strlen(m_pInfoString));

	// The world models as they are.
	CopyStream(outFile, inFile, objectDataPos);

	objectMarker.Mark();
	SaveObjectList(outFile, m_Objects);

	// The lighting as it is.
	lightMarker.Mark();
	inFile.SeekTo(lightDataPos);
	CopyStream(outFile, inFile, inFile.GetLen());

	return TRUE;
}


static void CopyStream(CAbstractIO &dest, CAbstractIO &src, uint32 endPos)
{
	uint8 buf[16], theByte;

	while(src.GetCurPos() < endPos-16)
	{
		src.Read(buf, 16);
		dest.Write(buf, 16);
	}

	while(src.GetCurPos() < endPos)
	{
		src.Read(&theByte, 1);
		dest.Write(&theByte, 1);
	}
}


BOOL CPreMainWorld::SaveFile(CAbstractIO &file)
{
	CMemoryIO		memFile;
	uint32			i, nMainWorlds;
	CPreWorld		*pWorld;
	ILTStream		*pStream;
	BOOL			bSaved;


	// The box around the main world models.
	m_PosMin.Init(100000.0f, 100000.0f, 100000.0f);
	m_PosMax.Init(-100000.0f, -100000.0f, -100000.0f);

	nMainWorlds = 0;
	for(i=0; i < m_WorldModels; i++)
	{
		pWorld = m_WorldModels[i];

		pWorld->RemoveUnusedGeometry();
		pWorld->UpdateBoundingBox();

		if(pWorld->m_WorldInfoFlags & (WIF_MAINWORLD | WIF_TERRAIN | WIF_PHYSICSBSP))
		{
			VEC_MIN(m_PosMin, m_PosMin, pWorld->m_PosMin);
			VEC_MAX(m_PosMax, m_PosMax, pWorld->m_PosMax);
			nMainWorlds++;
		}
	}

	if(nMainWorlds == 0)
	{
		m_PosMin.Init();
		m_PosMax.Init();
	}

	memFile.SetCacheSize(65000);

	file << (uint32)CURRENT_DAT_VERSION;
	CFileMarker objectMarker(file, FALSE);
	CFileMarker lightMarker(file, FALSE);
	file << (uint32)0 << (uint32)0 << (uint32)0 << (uint32)0;
	file << (uint32)0 << (uint32)0 << (uint32)0 << (uint32)0;

	file << (uint32)strlen(m_pInfoString);
	file.Write(m_pInfoString, strlen(m_pInfoString));

	file << m_LMGridSize;
	WriteVector(file, m_PosMin);
	WriteVector(file, m_PosMax);

	pStream = streamsim_AbstractIOWrapper(&file);
	if(!pStream)
		return FALSE;

	bSaved = m_WorldTree.SaveLayout(pStream);
	pStream->Release();
	if(!bSaved)
		return FALSE;

	file << m_WorldModels.GetSize();
	for(i=0; i < m_WorldModels; i++)
	{
		CFileMarker worldMarker(file, TRUE);

		if(!m_WorldModels[i]->SaveFile(file))
			return FALSE;
	}

	objectMarker.Mark();
	SaveObjectList(file, m_Objects);

	lightMarker.Mark();
	if(!SaveLightAnims(file))
		return FALSE;

	return TRUE;
}


BOOL CPreMainWorld::LoadLightAnims(CAbstractIO &file)
{
	CMoArray<CPrePoly*> polies(0);
	uint32 nPolyRefs, nFrames, nMaps, nAnims, iAnim, iPoly;
	int dataSize, totalData;
	uint16 nAnimPolies, worldIndex, polyIndex, dataLen, iFrame, iMap, iVert;
	uint8 nAnimFrames, nVerts, j;
	CPreLightAnim *pAnim;
	CPreLightFrame *pFrame;
	CPreLightMap *pMap;
	CMoArray<uint32> *pColors;
	uint8 r[256], g[256], b[256];


	SetupPolyArrays();

	file >> nPolyRefs;
	file >> nFrames;
	file >> dataSize;
	file >> nMaps;
	file >> nAnims;

	{
		CPreLightAnim *pNullAnim = NULL;
		if(!m_LightAnims.SetSizeInit(nAnims, pNullAnim) || !AllocateArray(m_LightAnims, (CPreLightAnim*)NULL))
		{
			DeleteAndClearArray(m_LightAnims);
			return FALSE;
		}
	}

	totalData = 0;

	for(iAnim=0; iAnim < m_LightAnims; iAnim++)
	{
		pAnim = m_LightAnims[iAnim];

		if(!file.ReadString(pAnim->m_Name, MAX_LIGHTANIMNAME_LEN))
			return FALSE;

		file.Read(&pAnim->m_bShadowMap, sizeof(pAnim->m_bShadowMap));
		file >> nAnimFrames;
		file >> nAnimPolies;

		if(!pAnim->m_Polies.SetSize(nAnimPolies))
			return FALSE;

		if(!polies.SetSize(pAnim->m_Polies.GetSize()))
			return FALSE;

		for(iPoly=0; iPoly < pAnim->m_Polies; iPoly++)
		{
			file >> worldIndex;
			file >> polyIndex;

			pAnim->m_Polies[iPoly].m_iWorld = worldIndex;
			pAnim->m_Polies[iPoly].m_iPoly = polyIndex;

			polies[iPoly] = GetLMPoly(&pAnim->m_Polies[iPoly]);
			if(!polies[iPoly])
				return FALSE;
		}

		CPreLightFrame *pNullFrame = NULL;
		if(!pAnim->m_Frames.SetSizeInit(nAnimFrames, pNullFrame) || !AllocateArray(pAnim->m_Frames, (CPreLightFrame*)NULL))
		{
			DeleteAndClearArray(pAnim->m_Frames);
			return FALSE;
		}

		for(iFrame=0; iFrame < pAnim->m_Frames; iFrame++)
		{
			pFrame = pAnim->m_Frames[iFrame];

			if(!pFrame->m_PolyMaps.SetSize(pAnim->m_Polies.GetSize()) || !AllocateArray(pFrame->m_PolyMaps, (CPreLightMap*)NULL))
			{
				DeleteAndClearArray(pFrame->m_PolyMaps);
				return FALSE;
			}

			for(iMap=0; iMap < pFrame->m_PolyMaps; iMap++)
			{
				pMap = pFrame->m_PolyMaps[iMap];

				pMap->m_Unknown_08 = pAnim->m_bShadowMap;
				pMap->m_Width = polies[iMap]->m_LMWidth;
				pMap->m_Height = polies[iMap]->m_LMHeight;

				file >> dataLen;
				if(!pMap->m_Data.SetSize(dataLen))
					return FALSE;

				totalData += dataLen;
				if(totalData > dataSize)
					return FALSE;

				file.Read(pMap->m_Data.GetArray(), dataLen);
			}

			// The vertex colors.
			if(!pFrame->m_Unknown_14.SetSize(pAnim->m_Polies.GetSize()) || !AllocateArray(pFrame->m_Unknown_14, (CMoArray<uint32>*)NULL))
			{
				DeleteAndClearArray(pFrame->m_Unknown_14);
				return FALSE;
			}

			for(iVert=0; iVert < pFrame->m_Unknown_14; iVert++)
			{
				pColors = pFrame->m_Unknown_14[iVert];

				file >> nVerts;
				if(!pColors->SetSize(nVerts))
					return FALSE;

				totalData += nVerts * 3;
				if(totalData > dataSize)
					return FALSE;

				for(j=0; j < nVerts; j++)
					file >> r[j];

				for(j=0; j < nVerts; j++)
					file >> g[j];

				for(j=0; j < nVerts; j++)
					file >> b[j];

				for(j=0; j < nVerts; j++)
					(*pColors)[j] = (r[j] << 16) | (g[j] << 8) | b[j];
			}
		}
	}

	return TRUE;
}


// 0x432a40. Near (811 bytes against 813): the vertex-colour loops'
// bound (nVerts, zero-extended) is loaded before the Write of nVerts in
// the shipped code (0x432c7e), after it in ours; and the shipped code
// loads the frame count as a byte (0x432b4c). Retail bug: the frame
// count, the poly count and each poly's vertex count are written as
// uint8, uint16 and uint8, the data for all of them after; more than 255
// frames (or 65535 polies) desyncs the file for LoadLightAnims.
BOOL CPreMainWorld::SaveLightAnims(CAbstractIO &file)
{
	uint32 i, iFrame;
	CPreLightAnim *pAnim;
	CPreLightFrame *pFrame;
	CPreLightMap *pMap;
	CMoArray<uint32> *pColors;
	uint8 nVerts;
	uint32 j;


	if(!CheckLightAnims())
		return FALSE;

	{
		uint32 iAnim, nMaps, dataSize, nFrames, nPolyRefs;

		nMaps = 0;
		dataSize = 0;
		nFrames = 0;
		nPolyRefs = 0;
		for(iAnim=0; iAnim < m_LightAnims; iAnim++)
		{
			pAnim = m_LightAnims[iAnim];

			nPolyRefs += pAnim->m_Polies.GetSize();
			nFrames += pAnim->m_Frames.GetSize();
			dataSize += pAnim->CalcFrameDataSize();
			nMaps += pAnim->m_Polies.GetSize() * pAnim->m_Frames.GetSize();
		}

		file << nPolyRefs;
		file << nFrames;
		file << dataSize;
		file << nMaps;
	}

	file << m_LightAnims.GetSize();

	for(uint32 iAnim=0; iAnim < m_LightAnims; iAnim++)
	{
		pAnim = m_LightAnims[iAnim];

		file.WriteString(pAnim->m_Name);
		// 0x432b4c: one chain, its operands loaded before the first Write.
		file << (uint32)pAnim->m_bShadowMap << (uint8)pAnim->m_Frames.GetSize() << (uint16)pAnim->m_Polies.GetSize();

		for(i=0; i < pAnim->m_Polies; i++)
		{
			file << (uint16)pAnim->m_Polies[i].m_iWorld;
			file << (uint16)pAnim->m_Polies[i].m_iPoly;
		}

		for(iFrame=0; iFrame < pAnim->m_Frames; iFrame++)
		{
			pFrame = pAnim->m_Frames[iFrame];

			for(i=0; i < pFrame->m_PolyMaps; i++)
			{
				pMap = pFrame->m_PolyMaps[i];
				if(pMap)
				{
					file << (uint16)pMap->m_Data.GetSize();
					file.Write(pMap->m_Data.GetArray(), pMap->m_Data.GetSize());
				}
				else
				{
					file << (uint16)0;
				}
			}

			for(i=0; i < pFrame->m_Unknown_14; i++)
			{
				pColors = pFrame->m_Unknown_14[i];
				if(pColors)
				{
					nVerts = (uint8)pColors->GetSize();
					file << nVerts;

					for(j=0; j < nVerts; j++)
						file << (uint8)((*pColors)[j] >> 16);

					for(j=0; j < nVerts; j++)
						file << (uint8)((*pColors)[j] >> 8);

					for(j=0; j < nVerts; j++)
						file << (uint8)(*pColors)[j];
				}
				else
				{
					file << (uint8)0;
				}
			}
		}
	}

	return TRUE;
}


// Fills each world model's poly array from its poly list.
// 0x432d70. Near (200 bytes against 213): the shipped code calls
// m_Unknown_30's _DeleteAndDestroyArray (0x411640) inside the inlined
// SetSize, ours expands it (the inline budget); the rest is registers.
// m_Unknown_30 is a CMoArray<CPrePoly*> in 2001 (CPreWorld's constructor
// stores that vtable, 0x4746b8, at 0x433c19), but retyping it removes
// the CMoArray<void*> instance and moves _$E216 off 0x430a90 (the
// declaration-count flip, guide 09).
void CPreMainWorld::SetupPolyArrays()
{
	uint32 i, iPoly;
	CPreWorld *pWorld;
	GPOS pos;

	for(i=0; i < m_WorldModels; i++)
	{
		pWorld = m_WorldModels[i];

		pWorld->m_Unknown_30.SetSize(pWorld->m_Polies.GetSize());

		iPoly = 0;
		for(pos=pWorld->m_Polies; pos; )
		{
			pWorld->m_Unknown_30[iPoly] = pWorld->m_Polies.GetNext(pos);
			iPoly++;
		}
	}
}


CPrePoly* CPreMainWorld::GetLMPoly(const CPrePolyRef *pRef)
{
	CPreWorld *pWorld;

	if(pRef->m_iWorld >= m_WorldModels.GetSize())
		return NULL;

	pWorld = m_WorldModels[pRef->m_iWorld];
	if(pRef->m_iPoly >= pWorld->m_Unknown_30.GetSize())
		return NULL;

	return (CPrePoly*)pWorld->m_Unknown_30[pRef->m_iPoly];
}


// Whether the light animations fit the world: their polies exist and
// their lightmaps are the polies' sizes (the name is ours).
// 0x432e90. Near (280 bytes against 284): registers (the shipped code
// keeps this in esi, spilled and reloaded inside the poly loop).
BOOL CPreMainWorld::CheckLightAnims()
{
	uint32 iAnim, iFrame, iPoly;
	CPreLightAnim *pAnim;
	CPreLightFrame *pFrame;
	CPreLightMap *pMap;
	CPrePoly *pPoly;

	SetupPolyArrays();

	for(iAnim=0; iAnim < m_LightAnims; iAnim++)
	{
		pAnim = m_LightAnims[iAnim];

		for(iPoly=0; iPoly < pAnim->m_Polies; iPoly++)
		{
			if(pAnim->m_Polies[iPoly].m_iWorld >= m_WorldModels.GetSize())
				return FALSE;

			if(pAnim->m_Polies[iPoly].m_iPoly >= m_WorldModels[pAnim->m_Polies[iPoly].m_iWorld]->m_Polies.GetSize())
				return FALSE;
		}

		for(iFrame=0; iFrame < pAnim->m_Frames; iFrame++)
		{
			pFrame = pAnim->m_Frames[iFrame];

			if(pFrame->m_PolyMaps.GetSize() != pAnim->m_Polies.GetSize())
				return FALSE;

			for(iPoly=0; iPoly < pFrame->m_PolyMaps; iPoly++)
			{
				pMap = pFrame->m_PolyMaps[iPoly];
				if(pMap)
				{
					pPoly = GetLMPoly(&pAnim->m_Polies[iPoly]);
					if(!pPoly)
						return FALSE;

					if(pMap->m_Width != pPoly->m_LMWidth)
						return FALSE;

					if(pMap->m_Height != pPoly->m_LMHeight)
						return FALSE;

					if(pMap->m_Width > 32)
						return FALSE;

					if(pMap->m_Height > 32)
						return FALSE;
				}
			}

			if(pFrame->m_Unknown_14.GetSize() != pAnim->m_Polies.GetSize())
				return FALSE;
		}
	}

	return TRUE;
}


CPreWorld* CPreMainWorld::FindWorldModel(const char *pName)
{
	uint32 i;
	CPreWorld *pModel;

	for(i=0; i < m_WorldModels; i++)
	{
		pModel = m_WorldModels[i];

		if(stricmp(pModel->m_WorldName, pName) == 0)
			return pModel;
	}

	return NULL;
}


// Removes the world models of the AIVolumes and MusicVolumes, and their
// polies from the light animations (the name is ours).
void CPreMainWorld::RemoveAIVolumes()
{
	uint32 i, iWorld, iAnim, iPoly, iFrame;
	CBaseEditObj *pObj;
	const char *pClassName;
	char *pName;
	CPreLightAnim *pAnim;
	CPreLightFrame *pFrame;

	for(i=0; i < m_Objects; i++)
	{
		pObj = m_Objects[i];

		pClassName = pObj->GetClassName();
		pName = pObj->GetName();
		if(pClassName && pName &&
			(stricmp(pClassName, "AIVolume") == 0 || stricmp(pClassName, "MusicVolume") == 0))
		{
			for(iWorld=0; iWorld < m_WorldModels; iWorld++)
			{
				if(stricmp(m_WorldModels[iWorld]->m_WorldName, pName) == 0)
					break;
			}

			if(iWorld < m_WorldModels)
			{
				delete m_WorldModels[iWorld];
				m_WorldModels.Remove(iWorld);

				for(iAnim=0; iAnim < m_LightAnims; iAnim++)
				{
					pAnim = m_LightAnims[iAnim];

					for(iPoly=0; iPoly < pAnim->m_Polies; iPoly++)
					{
						if(pAnim->m_Polies[iPoly].m_iWorld > iWorld)
						{
							pAnim->m_Polies[iPoly].m_iWorld--;
						}
						else if(pAnim->m_Polies[iPoly].m_iWorld == iWorld)
						{
							pAnim->m_Polies.Remove(iPoly);

							for(iFrame=0; iFrame < pAnim->m_Frames; iFrame++)
							{
								pFrame = pAnim->m_Frames[iFrame];

								delete pFrame->m_PolyMaps[iPoly];
								pFrame->m_PolyMaps[iPoly] = NULL;

								delete pFrame->m_Unknown_14[iPoly];
								pFrame->m_Unknown_14[iPoly] = NULL;

								pFrame->m_PolyMaps.Remove(iPoly);
								pFrame->m_Unknown_14.Remove(iPoly);
							}

							iPoly--;
						}
					}
				}
			}
		}
	}
}


void CPreMainWorld::MinimizeSurfaceTCoords()
{
	uint32 i;

	for(i=0; i < m_WorldModels.GetSize(); i++)
	{
		m_WorldModels[i]->MinimizeSurfaceTCoords();
	}
}


uint32 CPreMainWorld::CalcLMDataSize()
{
	uint32 iAnim, count;

	count = 0;
	for(iAnim=0; iAnim < m_LightAnims; iAnim++)
	{
		count += m_LightAnims[iAnim]->CalcFrameDataSize();
	}

	return count;
}


void CPreMainWorld::RemoveAllLightMaps()
{
	DeleteAndClearArray(m_LightAnims);
}


// The lightmaps' size as 32-bit pixels over their compressed size (the
// name is ours).
// 0x433520. Near, size-exact: registers (this in ebp, pFrame spilled).
// Retail bug: with no light animation data the divisor is 0 (0/0, a NaN
// in the statistics); and the uint32 divisor is divided as a signed
// 32-bit integer (fidivl, 0x433609).
PReal CPreMainWorld::CalcLMCompressionRatio()
{
	uint32 iAnim, iFrame, iPoly, uncompressed, compressed;
	CPreLightAnim *pAnim;
	CPreLightFrame *pFrame;
	CPreLightMap *pMap;
	CPrePoly *pPoly;

	SetupPolyArrays();

	compressed = 0;
	uncompressed = 0;
	for(iAnim=0; iAnim < m_LightAnims; iAnim++)
	{
		pAnim = m_LightAnims[iAnim];

		for(iFrame=0; iFrame < pAnim->m_Frames; iFrame++)
		{
			pFrame = pAnim->m_Frames[iFrame];

			if(pFrame->m_PolyMaps.GetSize() == pAnim->m_Polies.GetSize())
			{
				for(iPoly=0; iPoly < pAnim->m_Polies; iPoly++)
				{
					pMap = pFrame->m_PolyMaps[iPoly];
					if(pMap && (pPoly = GetLMPoly(&pAnim->m_Polies[iPoly])))
					{
						uncompressed += pPoly->m_LMHeight * pPoly->m_LMWidth * 4;
						compressed += pMap->m_Data.GetSize();
					}
				}
			}
		}
	}

	return (PReal)uncompressed / (PReal)compressed;
}


CPreLightAnim* CPreMainWorld::FindLightAnim(const char *pName)
{
	uint32 i;
	CPreLightAnim *pAnim;

	for(i=0; i < m_LightAnims; i++)
	{
		pAnim = m_LightAnims[i];
		if(stricmp(pAnim->m_Name, pName) == 0)
			return pAnim;
	}

	return NULL;
}


void CPreMainWorld::RemoveLightAnim(CPreLightAnim *pAnim)
{
	uint32 index;

	index = m_LightAnims.FindElement(pAnim);
	if(index < m_LightAnims.GetSize())
		m_LightAnims.Remove(index);

	delete pAnim;
}


CPreWorld* CPreMainWorld::GetPhysicsBSP()
{
	GenListPos pos;
	CPreWorld *pTest;

	for(pos=m_WorldModels.GenBegin(); m_WorldModels.GenIsValid(pos); )
	{
		pTest = m_WorldModels.GenGetNext(pos);

		if(pTest->m_WorldInfoFlags & WIF_PHYSICSBSP)
			return pTest;
	}

	return NULL;
}


CPreWorld* CPreMainWorld::GetVisBSP()
{
	GenListPos pos;
	CPreWorld *pTest;

	for(pos=m_WorldModels.GenBegin(); m_WorldModels.GenIsValid(pos); )
	{
		pTest = m_WorldModels.GenGetNext(pos);

		if(pTest->m_WorldInfoFlags & WIF_VISBSP)
			return pTest;
	}

	return NULL;
}


// After loading: points the vis BSP's leaves at their polies, from the
// world model and poly indices in the file (the name is ours).
BOOL CPreMainWorld::SetupLeafPolies()
{
	CMoArray< CMoArray<CPrePoly*> > worldPolies;
	GenListPos pos;
	CPreWorld *pWorld, *pVisBSP;
	CPreLeaf *pLeaf;
	uint32 i, iWorld;
	uint16 *pRef;

	worldPolies.SetSize(m_WorldModels.GenGetSize());

	iWorld = 0;
	for(pos=m_WorldModels.GenBegin(); m_WorldModels.GenIsValid(pos); )
	{
		pWorld = m_WorldModels.GenGetNext(pos);
		worldPolies[iWorld].GenAppendList(pWorld->m_Polies);
		iWorld++;
	}

	pVisBSP = GetVisBSP();
	if(!pVisBSP)
		return FALSE;

	for(pos=pVisBSP->m_Leaves.GenBegin(); pVisBSP->m_Leaves.GenIsValid(pos); )
	{
		pLeaf = pVisBSP->m_Leaves.GenGetNext(pos);

		pLeaf->m_Polies.GenRemoveAll();
		for(i=0; i < pLeaf->m_Unknown_58; i++)
		{
			pRef = (uint16*)&pLeaf->m_Unknown_58[i];
			if(pRef[0] >= worldPolies.GetSize() || pRef[1] >= worldPolies[pRef[0]].GetSize())
				return FALSE;

			pLeaf->m_Polies.GenAppend(worldPolies[pRef[0]][pRef[1]]);
		}

		pLeaf->m_Unknown_58.GenRemoveAll();
	}

	return TRUE;
}


// ----------------------------------------------------------------------- //
// CPreWorld.
// ----------------------------------------------------------------------- //

void PBlockTable::Clear()
{
	m_vBlockSize.Init();
	m_vMin.Init();
	m_nBlocks[0] = m_nBlocks[1] = m_nBlocks[2] = 0;
	m_nBlocksXY = 0;
	m_nBlocksTotal = 0;
	m_pBlocks = NULL;
}


// 0x433af0: the members' constructors (m_Polies's vtable 0x4746b8 stored
// at 0x433b02, m_Nodes's 0x4753ec, m_PBlocks's), then these. Near (112
// bytes against 140): the shipped code expands m_PBlocks's Clear here
// (0x433b3f..0x433b68), so the 2001 Clear was inline, but CPreWorld's
// constructor calls it (0x433c40, the out-of-line copy at 0x433b80), and
// with Clear inline ours expands it there too (382 bytes against 377).
// Clear's spelling, where it is defined and the constructor in the class
// don't move that; CPreWorld's constructor, identical, wins.
CPreSubBsp::CPreSubBsp()
{
	m_RootNode = NODE_OUT;
	m_Unknown_2c = 0;
	m_Unknown_30.Init();
}


CPreWorld::CPreWorld(CPreMainWorld *pMainWorld)
{
	Construct(pMainWorld);
}


void CPreWorld::Construct(CPreMainWorld *pMainWorld)
{
	m_pMainWorld = pMainWorld;
	m_WorldName[0] = 0;
	m_RootNode = NODE_OUT;
	m_StringHolder.SetAllocSize( 600 );
	m_Leaves.SetCacheSize(500);

	m_RootNode = NODE_OUT;
	m_WorldTranslation.Init();

	m_PosMin.Init();
	m_PosMax.Init();

	m_WorldInfoFlags = 0;
}


#undef DrawStatusText



class PointElement
{
	public:

				PointElement() {}
				PointElement(PVector *pVec, uint32 *pIndex)
				{
					m_pPoint = pVec;
					m_pIndex = pIndex;
					m_Mag = m_pPoint->Mag();
				}
		
		bool operator<(const PointElement &other) const
		{
			return m_Mag > other.m_Mag;
		}

		PReal m_Mag;
		PVector	*m_pPoint;
		uint32	*m_pIndex;

};


// Points closer than this are one.
#define POINT_TOLERANCE	((PReal)0.0001)

// talon: a node's index in a DAT file for NODE_IN and NODE_OUT.
#define NODEINDEX_IN	0xFFFFFFFF
#define NODEINDEX_OUT	0xFFFFFFFE

// Surfaces closer than this save as one.
#define SURFACE_EQUAL_EPSILON	0.01f

// talon: a node of the BSP as it is saved (the names are ours;
// PhysicsBlocks.cpp has it too: in PreWorld.h it would cost PreWorld.cpp's
// static CMoArray destructor, 0x430a90, and MakeWorldPVS, 0x43f170, the
// header declaration-count threshold, tools/guides/09).
#define SAVENODE_IN		0xFFFF
#define SAVENODE_OUT	0xFFFE
struct SaveNode
{
	uint16	m_iPlane;
	uint16	m_Sides[2];
};

void CheckNodes_R(NODEREF pNode);
void CheckSaveNodes_R(SaveNode **ppNodes, uint16 iNode);
bool AreSurfacesEqual(CPreSurface *pSurface1, CPreSurface *pSurface2);


// ----------------------------------------------------------------------- //
//      ROUTINE:        Destructor
//      PURPOSE:        
// ----------------------------------------------------------------------- //

CPreWorld::~CPreWorld()
{
	Term();
}



// ----------------------------------------------------------------------- //
//      ROUTINE:        CPreWorld::Term
//      PURPOSE:        Clears out any data in the model.
// ----------------------------------------------------------------------- //

void CPreWorld::Term()
{
	m_WorldName[0] = 0;
	TermGeometry();

	m_StringHolder.ClearStrings();
	m_RootNode = NODE_OUT;
}


// 0x433f60. Near (498 bytes against 516): the shipped code expands
// m_Points.Term's SetSize2 (0x434107), ours calls it (the inline budget;
// Build 69's m_Points.SetSize(0) doesn't move it).
void CPreWorld::TermGeometry()
{
	GPOS pos;
	uint32 i, nBlocks;

	nBlocks = m_PBlocks.m_nBlocks[2] * m_PBlocks.m_nBlocks[1] * m_PBlocks.m_nBlocks[0];
	m_RootNode = NODE_OUT;

	for(i=0; i < nBlocks; i++)
		dfree(m_PBlocks.m_pBlocks[i].m_pData);

	memset(&m_PBlocks, 0, sizeof(m_PBlocks));

	DeleteAndClearArray(m_Leaves);

	GDeleteAndRemoveElements(m_Nodes);
	GDeleteAndRemoveElements(m_Unknown_04);

	for(pos=m_Polies; pos; )
		DeletePoly(m_Polies.GetNext(pos));

	for(pos=m_OriginalBrushPolies; pos; )
		DeletePoly(m_OriginalBrushPolies.GetNext(pos));

	m_Polies.RemoveAll();
	m_OriginalBrushPolies.RemoveAll();

	GDeleteAndRemoveElements( m_Planes );
	GDeleteAndRemoveElements( m_Surfaces );

	m_Points.Term();
	m_TextureNames.Term();
	m_WorldTranslation.Init(0, 0, 0);
}


// 0x434170..0x434572 (the function table's 0x434443, 0x434465 and
// 0x434470 are inside it). Near (1017 bytes against 1026): only the end,
// theQueue's vector destructor, where the shipped code expands
// allocator::deallocate (the null test and __node_alloc's deallocate,
// 0x422060, with the size in bytes) and ours calls it (the inline budget).
void CPreWorld::InitPolyVertexIndices()
{
	priority_queue<PointElement> theQueue;
	list<PointStackEl*> theStack;
	list<PointStackEl*>::iterator iStack;
	GPOS pos;
	uint32 i, nPointsInQueue;
	CPrePoly *pPoly;
	PointElement testElement;
	PointStackEl *pNewStackEl, *pStackEl;
	bool bAddToStack;
	PReal dist;


	m_Points.Term();
	m_Points.SetCacheSize(1000);


	// Sort all the poly points into the queue.
	nPointsInQueue = 0;
	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);

		for(i=0; i < pPoly->NumVerts(); i++)
		{
			pPoly->Index(i) = 0xFFFFFFFF;
			theQueue.push(PointElement(&pPoly->Pt(i), (uint32 *)&pPoly->Index(i)));
			++nPointsInQueue;
		}

		for(i=0; i < pPoly->m_Unknown_0c; i++)
		{
			pPoly->m_Unknown_0c[i].m_Index = 0xFFFFFFFF;
			theQueue.push(PointElement(&pPoly->m_Unknown_0c[i].m_Vec, &pPoly->m_Unknown_0c[i].m_Index));
			++nPointsInQueue;
		}
	}

	// Go over the sorted list, adding necessary ones.
	while(!theQueue.empty())
	{
		testElement = theQueue.top();
		
		bAddToStack = true;
		iStack = theStack.begin();
		while(iStack != theStack.end())
		{
			pStackEl = *iStack;

			if(testElement.m_Mag - pStackEl->m_Mag > POINT_TOLERANCE)
			{
				// We've passed out of range of this stack element.. add it to the world.
				pStackEl->AddToWorld(this);
				iStack = theStack.erase(iStack);
				delete pStackEl;
			}
			else
			{
				// Ok, they're close enough on X, see if their real distance is close enough.
				dist = (*testElement.m_pPoint - *pStackEl->m_pPoint).Mag();
				if(dist < POINT_TOLERANCE)
				{
					// They are close enough, so add the testElement's index to the stackEl.
					pStackEl->m_Attachments.push_back(testElement.m_pIndex);
					bAddToStack = false;
					break;
				}

				++iStack;
			}
		}

		if(bAddToStack)
		{
			pNewStackEl = new PointStackEl;
			pNewStackEl->m_Mag = testElement.m_Mag;
			pNewStackEl->m_pPoint = testElement.m_pPoint;
			pNewStackEl->m_Attachments.push_back(testElement.m_pIndex);
			theStack.push_back(pNewStackEl);
		}

		theQueue.pop();
	}


	// Add the dangling stack elements to the world.
	for(iStack=theStack.begin(); iStack != theStack.end(); iStack++)
	{
		(*iStack)->AddToWorld(this);
		delete (*iStack);
	}
}


// ----------------------------------------------------------------------- //
// talon: helpers of CPreWorld::SaveFile (0x436ad0); the names are ours.
// ----------------------------------------------------------------------- //

// Walks a BSP tree (nothing is left of what it checked in a release
// build; 0x436180).
void CheckNodes_R(NODEREF pNode)
{
	if(pNode == NODE_IN || pNode == NODE_OUT)
		return;

	CheckNodes_R(pNode->m_Sides[0]);
	CheckNodes_R(pNode->m_Sides[1]);
}


// The same for the saved form of the tree: 6-byte nodes, each a plane and
// the indices of its two sides (0x4361b0).
void CheckSaveNodes_R(SaveNode **ppNodes, uint16 iNode)
{
	if(iNode == SAVENODE_OUT || iNode == SAVENODE_IN)
		return;

	SaveNode *pNode = &(*ppNodes)[iNode];

	CheckSaveNodes_R(ppNodes, pNode->m_Sides[0]);
	CheckSaveNodes_R(ppNodes, pNode->m_Sides[1]);
}


// For each poly, the light animations it's in (0x4361f0).
BOOL CPreWorld::GetPolyLightAnims(CMoArray< CMoArray<PolyAnimRef> > &polyAnims, uint32 &nRefs)
{
	uint32 iWorld, iAnim, iRef, iPoly;
	CPreLightAnim *pAnim;
	PolyAnimRef ref;

	nRefs = 0;
	iWorld = m_pMainWorld->m_WorldModels.FindElement(this);
	if(iWorld >= m_pMainWorld->m_WorldModels.GetSize())
		return FALSE;

	if(!polyAnims.SetSize(m_Polies.GetSize()))
		return FALSE;

	for(iAnim=0; iAnim < m_pMainWorld->m_LightAnims.GetSize(); iAnim++)
	{
		pAnim = m_pMainWorld->m_LightAnims[iAnim];

		for(iRef=0; iRef < pAnim->m_Polies.GetSize(); iRef++)
		{
			if(pAnim->m_Polies[iRef].m_iWorld == iWorld)
			{
				iPoly = pAnim->m_Polies[iRef].m_iPoly;
				if(iPoly >= polyAnims.GetSize())
					return FALSE;

				ref.m_iAnim = (uint16)iAnim;
				ref.m_iRef = (uint16)iRef;
				polyAnims[iPoly].Append(ref);
				nRefs++;
			}
		}
	}

	return TRUE;
}


// The used surfaces, those that are the same as one before them left out,
// and for each surface the index of the one it's saved as (BAD_INDEX if
// it isn't used; 0x436400). Near (567 bytes against 566): registers
// (this, surfaceMap and pSurface rotate through ebx, ebp and edi).
void CPreWorld::GetUniqueSurfaces(CMoArray<CPreSurface*> &surfaces, CMoArray<uint32> &surfaceMap)
{
	GPOS pos;
	CPreSurface *pSurface;
	uint32 i;

	surfaces.SetCacheSize(TagUsedSurfaces());
	surfaceMap.SetCacheSize(1024);

	for(pos=m_Surfaces; pos; )
	{
		pSurface = m_Surfaces.GetNext(pos);

		i = BAD_INDEX;
		if(pSurface->m_bUsed)
		{
			for(i=0; i < surfaces.GetSize(); i++)
			{
				if(AreSurfacesEqual(pSurface, surfaces[i]))
					goto Found;
			}

			i = surfaces.GetSize();
			surfaces.Append(pSurface);
		}

	Found:;

		surfaceMap.Append(i);
	}
}


static inline BOOL AreVectorsClose(const PVector &v1, const PVector &v2)
{
	return LTDIFF(v1.x, v2.x) <= SURFACE_EQUAL_EPSILON &&
		LTDIFF(v1.y, v2.y) <= SURFACE_EQUAL_EPSILON &&
		LTDIFF(v1.z, v2.z) <= SURFACE_EQUAL_EPSILON;
}

// Whether two surfaces can be saved as one (0x436640).
bool AreSurfacesEqual(CPreSurface *pSurface1, CPreSurface *pSurface2)
{
	BOOL bEffect1, bEffect2;
	uint32 portal1;

	if(pSurface1->m_Flags != pSurface2->m_Flags)
		return false;

	bEffect2 = pSurface2->m_Unknown_a0 && pSurface2->m_Unknown_a4;
	bEffect1 = pSurface1->m_Unknown_a0 && pSurface1->m_Unknown_a4;
	if(bEffect2 != bEffect1)
		return false;

	if(pSurface2->m_TextureFlags != pSurface1->m_TextureFlags)
		return false;

	portal1 = pSurface1->m_Flags & SURF_PORTAL;
	if((pSurface2->m_Flags & SURF_PORTAL) != portal1)
		return false;

	if(stricmp(pSurface2->m_pTextureName, pSurface1->m_pTextureName) != 0)
		return false;

	if(portal1)
	{
		if(!AreVectorsClose(pSurface1->m_TextureO, pSurface2->m_TextureO) ||
			!AreVectorsClose(pSurface1->P, pSurface2->P) ||
			!AreVectorsClose(pSurface1->Q, pSurface2->Q) ||
			!AreVectorsClose(pSurface1->m_pPlane->m_Normal, pSurface2->m_pPlane->m_Normal) ||
			fabs(pSurface1->m_pPlane->m_Dist - pSurface2->m_pPlane->m_Dist) >= 0.01)
		{
			return false;
		}
	}

	if(bEffect1)
	{
		if(!AreVectorsClose(pSurface1->m_TextureO, pSurface2->m_TextureO) ||
			!pSurface1->P.Equals(pSurface2->P, 0.01f) ||
			!pSurface1->Q.Equals(pSurface2->Q, 0.01f) ||
			!pSurface1->m_pPlane->m_Normal.Equals(pSurface2->m_pPlane->m_Normal, 0.01f) ||
			fabs(pSurface1->m_pPlane->m_Dist - pSurface2->m_pPlane->m_Dist) >= 0.01 ||
			strcmp((char*)pSurface1->m_Unknown_a0, (char*)pSurface2->m_Unknown_a0) != 0 ||
			strcmp((char*)pSurface1->m_Unknown_a4, (char*)pSurface2->m_Unknown_a4) != 0)
		{
			return false;
		}
	}

	return true;
}


// ----------------------------------------------------------------------- //
// talon: CPreWorld::LoadFile (0x434630).
// ----------------------------------------------------------------------- //


// What m_Unknown_04 holds, as LoadFile fills it (the names are ours; VisLeaves.h
// has it too).
struct PreSurfaceUser : public CGLLNode
{
	CPreSurface		*m_pSurface;
	const char		*m_pName;
	uint32			m_Index;
};


uint32 CPreWorld::LoadFile(CAbstractIO &file)
{
	CMoArray<CNode*>		nodes;
	CMoArray<CPrePlane*>	planes;
	CMoArray<CPrePoly*>		polies;
	CMoArray<CPreSurface*>	surfaces;
	CMoArray<CPrePoly*>		newPolies;
	uint32					dummy, i, j;
	uint32					nPoints, nPlanes, nSurfaces, nPortals, nPolies, nLeaves, nNodes, nTextures;
	uint32					nVisListSize, nLeafListSize, nUnknown1, nUnknown2, nUnknown3, nUnknown4;
	uint32					iSurface, iPlane, iPoly, iFront, iBack, iRoot, nSubBsps, value, nPolyRefs;
	uint16					nLeafLists, word, listSize, iTexture, iLeaf, nSkip, skip1, skip2, index;
	uint8					nVerts, nExtraVerts, byteVal, r, g, b;
	PVector					vec, vMin, vMax, vO, vP, vQ;
	PReal					dist;
	char					textureName[256], effectName[256], *pChar;
	GPOS					pos;
	CNode					*pNode;
	CPrePoly				*pPoly;
	CPreLeaf				*pLeaf;
	CPreLeafList			*pList;
	CPrePlane				*pPlane;
	CPreSurface				*pSurface, *pSrcSurface;
	PreSurfaceUser		*pUser;


	// Header.
	file >> dummy;
	file >> dummy;
	file >> dummy;
	file >> dummy;
	file >> dummy;
	file >> dummy;
	file >> dummy;
	file >> dummy;

	file >> m_WorldInfoFlags;
	file >> nUnknown1;
	file.ReadString(m_WorldName, MAX_WORLDNAME_LEN);

	file >> nPoints;
	file >> nPlanes;
	file >> nSurfaces;
	file >> nPortals;
	file >> nPolies;
	file >> nLeaves;
	file >> nVisListSize;
	file >> nLeafListSize;
	file >> nUnknown2;
	file >> nNodes;
	file >> nUnknown3;
	file >> nUnknown4;
	file >> vMin.x;
	file >> vMin.y;
	file >> vMin.z;
	file >> vMax.x;
	file >> vMax.y;
	file >> vMax.z;
	m_WorldTranslation = ReadVector(file);

	file >> dummy;
	file >> nTextures;

	// Texture names.
	m_TextureNames.SetSize(nTextures);
	for(i=0; i < nTextures; i++)
	{
		pChar = textureName - 1;
		do
		{
			pChar++;
			file.Read(pChar, 1);
		}
		while(*pChar);

		m_TextureNames[i] = m_StringHolder.AddString(textureName, TRUE);
	}

	// Nodes (linked up at the end).
	nodes.SetSize(nNodes);
	for(i=0; i < nNodes; i++)
		nodes[i] = new CNode;

	// Polies.
	newPolies.SetSize(nPolies);
	for(i=0; i < nPolies; i++)
	{
		file >> nVerts;
		file >> nExtraVerts;

		pPoly = CreatePoly(CPrePoly, nVerts, TRUE);
		pPoly->m_Unknown_0c.SetSize(nExtraVerts);

		newPolies[i] = pPoly;
		m_Polies.AddTail(pPoly);
	}

	// Leaves.
	m_Leaves.SetSize(nLeaves);
	for(i=0; i < m_Leaves.GetSize(); i++)
	{
		pLeaf = new CPreLeaf;
		m_Leaves[i] = pLeaf;

		file >> nLeafLists;
		if(nLeafLists == 0xFFFF)
		{
			file >> pLeaf->m_Unknown_80;
		}
		else
		{
			for(j=0; j < nLeafLists; j++)
			{
				pList = new CPreLeafList;
				pLeaf->m_Unknown_6c.Append(pList);

				file >> word;
				pList->m_Unknown_14 = word;

				file >> listSize;
				pList->m_Data.SetSize(listSize);
				file.Read(pList->m_Data.GetArray(), listSize);
			}
		}

		file >> nPolyRefs;
		pLeaf->m_Unknown_58.SetSize(nPolyRefs);
		for(j=0; j < nPolyRefs; j++)
		{
			file >> pLeaf->m_Unknown_58[j].m_Unknown_00;
			file >> pLeaf->m_Unknown_58[j].m_Unknown_02;
		}

		file >> value;
		pLeaf->m_Unknown_9c = value;
	}

	// Planes.
	planes.SetSize(nPlanes);
	for(i=0; i < nPlanes; i++)
	{
		pPlane = new CPrePlane;

		file >> vec.x;
		file >> vec.y;
		file >> vec.z;
		file >> dist;

		pPlane->m_Normal.Init(vec.x, vec.y, vec.z);
		pPlane->m_Dist = dist;

		planes[i] = pPlane;
		m_Planes.AddTail(pPlane);
	}

	// Surfaces.
	surfaces.SetSize(nSurfaces);
	for(i=0; i < nSurfaces; i++)
	{
		pSurface = new CPreSurface;

		file >> vec.x;
		file >> vec.y;
		file >> vec.z;
		pSurface->m_TextureO.Init(vec.x, vec.y, vec.z);

		file >> vec.x;
		file >> vec.y;
		file >> vec.z;
		pSurface->m_TextureP = vec;

		file >> vec.x;
		file >> vec.y;
		file >> vec.z;
		pSurface->m_TextureQ = vec;

		file >> iTexture;
		if(iTexture >= m_TextureNames.GetSize())
			goto Error;

		pSurface->m_pTextureName = m_TextureNames[iTexture];
		pSurface->m_pPlane = NULL;

		file >> pSurface->m_Flags;
		file >> byteVal;
		pSurface->m_bDetail = byteVal;
		file >> pSurface->m_Ambient[0];
		file >> pSurface->m_Ambient[1];
		file >> pSurface->m_Ambient[2];

		file >> byteVal;
		if(byteVal == 1)
		{
			file.ReadString(effectName, 255);
			pSurface->m_Unknown_a0 = (uint32)m_StringHolder.AddString(effectName, TRUE);
			file.ReadString(effectName, 255);
			pSurface->m_Unknown_a4 = (uint32)m_StringHolder.AddString(effectName, TRUE);
		}

		surfaces[i] = pSurface;
		file >> pSurface->m_TextureFlags;
		m_Surfaces.AddTail(pSurface);
	}

	// Points.
	m_Points.SetSize(nPoints);
	for(i=0; i < m_Points.GetSize(); i++)
	{
		file >> vec.x;
		file >> vec.y;
		file >> vec.z;
		m_Points[i].Init(vec.x, vec.y, vec.z);
	}

	// Each poly's own surface and vertices.
	polies.SetSize(m_Polies.GetSize());
	i = 0;
	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);
		polies[i++] = pPoly;

		pPoly->m_PolyO = ReadVector(file);
		file >> pPoly->m_LMWidth;
		file >> pPoly->m_LMHeight;

		file >> nSkip;
		while(nSkip)
		{
			nSkip--;
			file >> skip1;
			file >> skip2;
		}

		file >> iSurface;
		if(iSurface >= surfaces.GetSize())
			goto Error;

		file >> iPlane;
		if(iPlane >= m_Planes.GetSize())
			goto Error;

		pSrcSurface = surfaces[iSurface];

		file >> vO.x;
		file >> vO.y;
		file >> vO.z;
		file >> vP.x;
		file >> vP.y;
		file >> vP.z;
		file >> vQ.x;
		file >> vQ.y;
		file >> vQ.z;

		for(j=0; j < pPoly->NumVerts(); j++)
		{
			file >> index;
			pPoly->Vert(j).m_Index = index;
			file >> r;
			file >> g;
			file >> b;
			pPoly->Vert(j).m_Color.x = (float)r;
			pPoly->Vert(j).m_Color.y = (float)g;
			pPoly->Vert(j).m_Color.z = (float)b;
		}

		for(j=0; j < pPoly->m_Unknown_0c.GetSize(); j++)
		{
			file >> index;
			pPoly->m_Unknown_0c[j].m_Index = index;
			file >> r;
			file >> g;
			file >> b;
			pPoly->m_Unknown_0c[j].m_Color.x = (float)r;
			pPoly->m_Unknown_0c[j].m_Color.y = (float)g;
			pPoly->m_Unknown_0c[j].m_Color.z = (float)b;
		}

		pSurface = new CPreSurface;
		pPoly->m_pSurface = pSurface;

		pSurface->m_TextureO = vO;
		pSurface->m_TextureP = vP;
		pSurface->m_TextureQ = vQ;
		pSurface->m_pTextureName = pSrcSurface->m_pTextureName;
		pSurface->m_Flags = pSrcSurface->m_Flags;
		pSurface->m_pPlane = planes[iPlane];

		GetLMPlaneVectors(FindLMPlane(pSurface->m_pPlane->m_Normal),
			pSurface->m_pPlane->m_Normal, pSurface->P, pSurface->Q);

		pSurface->m_InverseTextureP = pSurface->m_TextureP.Inverse();
		pSurface->m_InverseTextureQ = pSurface->m_TextureQ.Inverse();
		pSurface->InverseP = pSurface->P.Inverse();
		pSurface->InverseQ = pSurface->Q.Inverse();

		pSurface->m_bDetail = pSrcSurface->m_bDetail;
		pSurface->m_Ambient[0] = pSrcSurface->m_Ambient[0];
		pSurface->m_Ambient[1] = pSrcSurface->m_Ambient[1];
		pSurface->m_Ambient[2] = pSrcSurface->m_Ambient[2];
		pSurface->m_Unknown_a0 = pSrcSurface->m_Unknown_a0;
		pSurface->m_Unknown_a4 = pSrcSurface->m_Unknown_a4;
		pSurface->m_TextureFlags = pSrcSurface->m_TextureFlags;

		m_Surfaces.AddTail(pSurface);
	}

	// Link up the nodes.
	for(i=0; i < nNodes; i++)
	{
		pNode = nodes[i];
		m_Nodes.AddTail(pNode);

		file >> iPoly;
		pNode->m_pPoly = polies[iPoly];
		pNode->m_pPlane = pNode->m_pPoly->m_pSurface->m_pPlane;

		file >> iLeaf;
		pNode->m_pLeaf = (iLeaf == 0xFFFF) ? NULL : m_Leaves[iLeaf];

		file >> iFront;
		file >> iBack;
		pNode->m_Sides[0] = (iFront == NODEINDEX_IN) ? NODE_IN : ((iFront == NODEINDEX_OUT) ? NODE_OUT : nodes[iFront]);
		pNode->m_Sides[1] = (iBack == NODEINDEX_IN) ? NODE_IN : ((iBack == NODEINDEX_OUT) ? NODE_OUT : nodes[iBack]);
	}

	// What m_Unknown_04 holds.
	for(i=0; i < nPortals; i++)
	{
		pUser = new PreSurfaceUser;
		pUser->m_Index = i;

		file.ReadString(effectName, 256);
		pUser->m_pName = m_StringHolder.AddString(effectName, TRUE);

		file >> dummy;
		file >> iPoly;
		if(iPoly >= m_Polies.GetSize())
			goto Error;

		pUser->m_pSurface = polies[iPoly]->m_pSurface;

		file >> word;
		if(word >= m_Surfaces.GetSize())
			goto Error;

		pUser->m_pSurface->m_Unknown_80 = (uint32)pUser;

		file >> vec.x;
		file >> vec.y;
		file >> vec.z;
		file >> vec.x;
		file >> vec.y;
		file >> vec.z;

		m_Unknown_04.AddTail(pUser);
	}

	if(!LoadPBlockTable(file, &m_PBlocks))
		goto Error;

	// The poly vertices' positions.
	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);

		for(j=0; j < pPoly->NumVerts(); j++)
			pPoly->Pt(j) = m_Points[pPoly->Vert(j).m_Index];

		for(j=0; j < pPoly->m_Unknown_0c.GetSize(); j++)
			pPoly->m_Unknown_0c[j].m_Vec = m_Points[pPoly->m_Unknown_0c[j].m_Index];
	}

	file >> iRoot;
	m_RootNode = (iRoot == NODEINDEX_IN) ? NODE_IN : ((iRoot == NODEINDEX_OUT) ? NODE_OUT : nodes[iRoot]);

	// The sub-BSPs.
	file >> nSubBsps;
	if(!m_PolyArray.SetSize(nSubBsps))
		return 2;

	for(i=0; i < m_PolyArray.GetSize(); i++)
		m_PolyArray[i] = NULL;

	for(i=0; i < nSubBsps; i++)
	{
		m_PolyArray[i] = new CPreSubBsp;
		if(!m_PolyArray[i])
			return 2;

		if(!LoadSubBsp(file, polies, m_PolyArray[i]))
			return 2;
	}

	return 0;

Error:
	return 2;
}


void CPreWorld::RemoveUnusedGeometry()
{
	RemoveUnusedSurfaces();
	RemoveUnusedPlanes();
	RemoveUnusedPoints();
}


void CPreWorld::RemoveUnusedSurfaces()
{
	GPOS pos;
	CPreSurface *pSurface;

	BuildSurfacePolyLists(m_Surfaces, m_Polies);

	for(pos=m_Surfaces; pos; )
	{
		pSurface = m_Surfaces.GetNext(pos);

		if(pSurface->m_PolyList.GetSize() == 0)
		{
			m_Surfaces.RemoveAt(pSurface);
			delete pSurface;
		}
	}

	TermSurfacePolyLists(m_Surfaces);
}


// talon: the 2001 CPreSurface's destructor is here, out of line (0x438a50,
// called from RemoveUnusedSurfaces at 0x438a24): m_PolyList's RemoveAll.
CPreSurface::~CPreSurface()
{
}


void CPreWorld::RemoveUnusedPlanes()
{
	GPOS pos, oldPos, surfacePos;
	CPrePlane *pPlane;


	// First clear the high bit on all the planes.
	for(pos=m_Planes; pos; )
		m_Planes.GetNext(pos)->ClearHighBit();


	// Now mark the used planes.
	for(surfacePos=m_Surfaces; surfacePos; )
	{
		m_Surfaces.GetNext(surfacePos)->m_pPlane->SetHighBit();
	}


	// Remove unmarked ones.
	for(pos=m_Planes; pos; )
	{
		oldPos = pos;
		pPlane = m_Planes.GetNext(pos);

		if(!pPlane->GetHighBit())
		{
			m_Planes.RemoveAt(oldPos);
			delete pPlane;
		}
	}
}


BOOL CPreWorld::RemoveUnusedPoints()
{
	GPOS pos;
	PVertexArray newPoints;
	CMoArray<uint32> indexRename;
	uint32 i, k, iVert;
	CPrePoly *pPoly;

	newPoints.SetCacheSize(1024);

	if(!indexRename.SetSize(m_Points.GetSize()))
		return FALSE;

	for(i=0; i < indexRename; i++)
	{
		indexRename[i] = i;
	}

	// Mark which points are used.
	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);

		for( k=0; k < pPoly->NumVerts(); k++ )
		{
			iVert = pPoly->Index(k);
			if(indexRename[iVert] == iVert)
			{
				// Ok, make a new output vert.
				newPoints.Append(m_Points[iVert]);
				indexRename[iVert] = newPoints.LastI();
			}
		}

		for( k=0; k < pPoly->m_Unknown_0c.GetSize(); k++ )
		{
			iVert = pPoly->m_Unknown_0c[k].m_Index;
			if(indexRename[iVert] == iVert)
			{
				newPoints.Append(m_Points[iVert]);
				indexRename[iVert] = newPoints.LastI();
			}
		}
	}

	// Replace the array and remap indices.
	m_Points.CopyArray(newPoints);

	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);

		for( k=0; k < pPoly->NumVerts(); k++ )
		{
			pPoly->Index(k) = indexRename[pPoly->Index(k)];
		}

		for( k=0; k < pPoly->m_Unknown_0c.GetSize(); k++ )
		{
			pPoly->m_Unknown_0c[k].m_Index = indexRename[pPoly->m_Unknown_0c[k].m_Index];
		}
	}

	return TRUE;
}


// ----------------------------------------------------------------------- //
// Tags the surfaces that are actually being used.
// Returns the number of surfaces being used.
// ----------------------------------------------------------------------- //

uint32 CPreWorld::TagUsedSurfaces()
{
	GPOS		pos;
	uint32		nUsed;
	CPreSurface	*pSurface;

	for(pos=m_Surfaces.GetHeadPosition(); pos; )
		m_Surfaces.GetNext(pos)->m_bUsed = FALSE;

	for(pos=m_Polies; pos; )
		m_Polies.GetNext(pos)->GetSurface()->m_bUsed = TRUE;

	for(pos=m_Unknown_04; pos; )
		((PreSurfaceUser*)m_Unknown_04.GetNext(pos))->m_pSurface->m_bUsed = TRUE;

	nUsed = 0;
	for(pos=m_Surfaces.GetHeadPosition(); pos; )
	{
		pSurface = m_Surfaces.GetNext(pos);

		pSurface->m_UseIndex = (uint16)nUsed;
		nUsed += pSurface->m_bUsed;
	}

	return nUsed;
}



// ----------------------------------------------------------------------- //
//      ROUTINE:        CPreWorld::SetupIndexMap
//      PURPOSE:        Helps optimize the saving of BSP files by marking
//                      which points are used.
// ----------------------------------------------------------------------- //

void CPreWorld::SetupIndexMap( CMoByteArray &pointsUsed, CMoWordArray &indexMap, uint32 &nPointsUsed )
{
	uint32 i, k;
	uint16 subtract;
	CPrePoly *pPoly;
	GPOS pos;


	// Init the structures.
	pointsUsed.SetSize( m_Points.GetSize() );
	indexMap.SetSize( m_Points.GetSize() );

	memset( pointsUsed.GetArray(), 0, m_Points.GetSize() );

	for( i=0; i < indexMap; i++ )
		indexMap[i] = (uint16)i;


	// Mark which points are used.
	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);

		for( k=0; k < pPoly->NumVerts(); k++ )
			pointsUsed[pPoly->Index(k)] = TRUE;

		for( k=0; k < pPoly->m_Unknown_0c.GetSize(); k++ )
			pointsUsed[pPoly->m_Unknown_0c[k].m_Index] = TRUE;
	}


	// Decrement index map stuff.
	subtract = 0;
	nPointsUsed = 0;

	for( i=0; i < pointsUsed; i++ )
	{
		if( pointsUsed[i] )
			++nPointsUsed;
		else
			subtract++;

		indexMap[i] -= subtract;
	}
}


// ----------------------------------------------------------------------- //
// The poly edges the DAT file stores: each poly's edges, as indices into
// a list of the world's edges, each edge once (the names are ours).
// ----------------------------------------------------------------------- //

struct EdgeHashEntry
{
	uint32			m_iPt1, m_iPt2;		// m_iPt1 < m_iPt2.
	uint16			m_Index;
	EdgeHashEntry	*m_pNext;
};


// 0x439050. Near (551 bytes against 589): edgeHash's constructor. The
// shipped code calls Clear (0x420650) and expands Init (Term's SetSize2,
// _InitArray 0x422440, SetSize's SetSize2); ours expands Clear and calls
// Init. The rest is registers.
uint32 CPreWorld::MakePolyEdges(PolyEdges &edges)
{
	CMoArray<EdgeHashEntry*> edgeHash;
	EdgeHashEntry *pEntry, *pNext;
	GPOS pos;
	CPrePoly *pPoly;
	uint16 *pPolyEdges;
	uint32 i, iPoly, iPt1, iPt2, nEdges;

	nEdges = m_Polies.GetSize();
	edges.m_nPolies = nEdges;
	edges.m_pEdges = new uint16*[nEdges];

	nEdges = 0;
	edgeHash.SetSize(m_Points.GetSize());
	memset(edgeHash.GetArray(), 0, sizeof(EdgeHashEntry*)*edgeHash.GetSize());

	iPoly = 0;
	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);

		pPolyEdges = edges.m_pEdges[iPoly] = new uint16[pPoly->NumVerts()];
		for(i=0; i < pPoly->NumVerts(); i++)
		{
			iPt1 = pPoly->Index(i);
			iPt2 = pPoly->NextIndex(i);

			*pPolyEdges = FindEdge(edgeHash, iPt1, iPt2);
			if(*pPolyEdges == 0xFFFF)
			{
				pEntry = new EdgeHashEntry;
				pEntry->m_iPt1 = LTMIN(iPt1, iPt2);
				pEntry->m_iPt2 = LTMAX(iPt1, iPt2);
				pEntry->m_Index = (uint16)nEdges;
				pEntry->m_pNext = edgeHash[pEntry->m_iPt1];
				edgeHash[pEntry->m_iPt1] = pEntry;

				*pPolyEdges = (uint16)nEdges;
				nEdges++;
			}

			pPolyEdges++;
		}

		iPoly++;
	}

	for(i=0; i < edgeHash; i++)
	{
		for(pEntry=edgeHash[i]; pEntry; pEntry=pNext)
		{
			pNext = pEntry->m_pNext;
			delete pEntry;
		}
	}

	return nEdges;
}


uint16 CPreWorld::FindEdge(CMoArray<EdgeHashEntry*> &edgeHash, uint32 iPt1, uint32 iPt2)
{
	EdgeHashEntry *pEntry;
	uint32 iMin, iMax;

	iMin = LTMIN(iPt1, iPt2);
	iMax = LTMAX(iPt1, iPt2);

	for(pEntry=edgeHash[iMin]; pEntry; pEntry=pEntry->m_pNext)
	{
		if(pEntry->m_iPt2 == iMax)
			return pEntry->m_Index;
	}

	return 0xFFFF;
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CPreWorld::FindOrAddPlane
//      PURPOSE:        Looks for the plane and if it doesn't exist, adds it.
// ----------------------------------------------------------------------- //

CPrePlane* CPreWorld::FindOrAddPlane(PVector normal, PReal dist)
{
	CPrePlane *pTestPlane;
	GPOS pos;


	for( pos=m_Planes.GetHeadPosition(); pos; )
	{
		pTestPlane = m_Planes.GetNext( pos );
		
		if( pTestPlane->m_Normal.x == normal.x &&
			pTestPlane->m_Normal.y == normal.y &&
			pTestPlane->m_Normal.z == normal.z &&
			pTestPlane->m_Dist == dist)
		{
			return pTestPlane;
		}
	}

	// Couldn't find it .. create a new one.
	return AddPlane(normal, dist);
}


CPrePlane* CPreWorld::AddPlane(PVector &normal, PReal dist)
{
	CPrePlane *pPlane;

	pPlane = new CPrePlane;
	pPlane->m_Normal = normal;
	pPlane->m_Dist = dist;
	m_Planes.AddTail( pPlane );	

	return pPlane;
}


void CPreWorld::SetPolyIndices()
{
	uint32 i;
	GPOS pos;

	// talon: the poly's index is at 0x78 (m_nName here) in 2001.
	i = 0;
	for(pos=m_Polies; pos; )
	{
		m_Polies.GetNext(pos)->m_nName = i;
		++i;
	}
}


void CPreWorld::SetPlaneIndices()
{
	uint32 i;
	GPOS pos;

	// Give each plane its index.
	i = 0;
	for(pos=m_Planes; pos; )
	{
		m_Planes.GetNext(pos)->SetIndex(i);
		i++;
	}				  	
}


void CPreWorld::SetLeafIndices()
{
	uint16 i;

	for(i=0; i < m_Leaves.GetSize(); i++)
		m_Leaves[i]->m_Index = i;
}


void CPreWorld::GetTextureNames( CMoArray<const char*> &texNames )
{
	uint32			i;
	bool			bFound;
	GPOS			pos;
	CPreSurface		*pSurf;


	// Make the list of texture names.
	texNames.SetSize(0);
	for( pos=m_Surfaces.GetHeadPosition(); pos; )
	{
		pSurf = m_Surfaces.GetNext( pos );

		bFound = false;
		for( i=0; i < texNames; i++ )
		{
			if( texNames[i] == pSurf->m_pTextureName )
			{
				bFound = true;
				break;
			}
		}

		if( !bFound )
			texNames.Append( pSurf->m_pTextureName );
	}
}


WORD CPreWorld::FindTextureName( const char *pName, CMoArray<const char*> &texNames )
{
	if( !pName )
		return (WORD)-1;

	for( uint32 i=0; i < texNames; i++ )
		if( texNames[i] == pName )
			return (WORD)i;

	return (WORD)-1;
}


void SetupSurface(CPreSurface *pSurface)
{
	// Make sure TextureP and TextureQ are valid.
	// NOTE: this probably isn't necessary since geometry is never affected
	// by the texture plane.
	if(pSurface->m_TextureP.Mag() < 0.0001f || pSurface->m_TextureP.Mag() > 100000.0f || 
		pSurface->m_TextureQ.Mag() < 0.0001f || pSurface->m_TextureQ.Mag() > 100000.0f)
	{
		GetPerpendicularVector(&pSurface->m_pPlane->m_Normal, NULL, &pSurface->m_TextureP);
		pSurface->m_TextureP.Norm();
		pSurface->m_TextureQ = pSurface->m_TextureP.Cross(pSurface->m_pPlane->m_Normal);
	}
	
	// Setup the inverses..
	pSurface->m_InverseTextureP = pSurface->m_TextureP.Inverse();
	pSurface->m_InverseTextureQ = pSurface->m_TextureQ.Inverse();
}


// Adjusts pSurface->O so the texture coordinates on the specified poly have the
// smallest possible value.
void MinimizeTCoordValues(CPreSurface *pSurface, CPrePoly *pPoly)
{
	uint32 iUV, iPt;
	int nBlockPositions, iBlock, iBestBlock;
	PReal fMin, fMax, fCoord, fSum, fBestSum;
	PVector *pTexVec, *pInvTexVec, vSurfaceO;


	// Minimize U and V separately.
	for(iUV=0; iUV < 2; iUV++)
	{
		if(iUV == 0)
		{
			pTexVec		= &pSurface->m_TextureP;
			pInvTexVec	= &pSurface->m_InverseTextureP;
		}
		else
		{
			pTexVec		= &pSurface->m_TextureQ;
			pInvTexVec	= &pSurface->m_InverseTextureQ;
		}

		// Calculate the min and max of the texture coordinate values.
		fMin = 100000.0f;
		fMax = -100000.0f;
		for(iPt=0; iPt < pPoly->NumVerts(); iPt++)
		{
			fCoord = pTexVec->Dot(pPoly->Pt(iPt) - pSurface->m_TextureO);
			fMin = DMIN(fMin, fCoord);
			fMax = DMAX(fMax, fCoord);
		}

		// Determine how many block positions we should test (using MAX_DTX_SIZE).
		nBlockPositions = (int)((PReal)DMAX(fabs(fMax), fabs(fMin)) / MAX_DTX_SIZE + 0.5f);
		
		// Make sure it's not insane..
		nBlockPositions = DCLAMP(nBlockPositions, 0, 500);

		// Test all the blocks to see which is best.
		iBestBlock = 0;
		for(iBlock=-nBlockPositions; iBlock <= nBlockPositions; iBlock++)
		{
			vSurfaceO = pSurface->m_TextureO + *pInvTexVec * (PReal)(iBlock * MAX_DTX_SIZE);

			// See how far away the numbers are.
			fSum = 0.0f;
			for(iPt=0; iPt < pPoly->NumVerts(); iPt++)
			{
				fSum += (PReal)fabs(pTexVec->Dot(pPoly->Pt(iPt) - vSurfaceO));
			}

			if(iBlock == -nBlockPositions || fSum < fBestSum)
			{
				iBestBlock = iBlock;
				fBestSum = fSum;
			}
		}

		// Adjust by the specified amount.
		if(iBestBlock != 0)
		{
			pSurface->m_TextureO += *pInvTexVec * (PReal)(iBestBlock * MAX_DTX_SIZE);
		}
	}
}


// This function takes the surfaces, which should have their TextureP and TextureQ set,
// and sets up the P and Q (lightmap) coordinate system.  It also sets up the inverse
// texture vectors for everything.
void CPreWorld::SetupSurfaceTextureVectors()
{
	GPOS pos;
	CPreSurface *pSurface;


	for(pos=m_Surfaces; pos; )
	{
		pSurface = m_Surfaces.GetNext(pos);
		
		SetupSurface(pSurface);
	}

	SetupSurfaceLMVectors();
}


void CPreWorld::SetupSurfaceLMVectors()
{
	GPOS pos;
	CPreSurface *pSurface;


	for(pos=m_Surfaces; pos; )
	{
		pSurface = m_Surfaces.GetNext(pos);
		
		pSurface->SetupLMVectors();
	}
}


void CPreWorld::MinimizeSurfaceTCoords()
{
	GPOS pos;
	CPreSurface *pSurface;


	BuildSurfacePolyLists(m_Surfaces, m_Polies);

	for(pos=m_Surfaces; pos; )
	{
		pSurface = m_Surfaces.GetNext(pos);
		
		// Certain hardware accelerators don't like large numbers so we seek to
		// minimize the texture coordinate values here.
		if(pSurface->m_PolyList.GetSize() > 0)
		{
			MinimizeTCoordValues(pSurface, pSurface->m_PolyList.GetHead());
		}
	}

	TermSurfacePolyLists(m_Surfaces);
}


void CPreWorld::GetBoundingBox(PVector *pMin, PVector *pMax)
{
	GPOS pos;
	CPrePoly *pPoly;
	PVector vec;
	uint32 i;

	pMin->Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	pMax->Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);
	
	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);

		for(i=0; i < pPoly->NumVerts(); i++)
		{
			vec = pPoly->Pt(i);
			
			if(vec.x < pMin->x) pMin->x = vec.x;
			if(vec.y < pMin->y) pMin->y = vec.y;
			if(vec.z < pMin->z) pMin->z = vec.z;

			if(vec.x > pMax->x) pMax->x = vec.x;
			if(vec.y > pMax->y) pMax->y = vec.y;
			if(vec.z > pMax->z) pMax->z = vec.z;
		}
	}
}


void CPreWorld::UpdateBoundingBox()
{
	GetBoundingBox(&m_PosMin, &m_PosMax);
}


static CPreWorld *g_pRecursionWorld;
static int g_MaxRecursionDepth;
static void RecurseAndGetTreeDepth(NODEREF iRoot, int depth);

int CPreWorld::GetTreeDepth()
{
	g_pRecursionWorld = this;
	g_MaxRecursionDepth = 0;
	RecurseAndGetTreeDepth(m_RootNode, 1);
	return g_MaxRecursionDepth;
}


static void RecurseAndGetTreeDepth(NODEREF iRoot, int depth)
{
	if(!IsValidNode(iRoot))
		return;	

	if(depth > g_MaxRecursionDepth)
		g_MaxRecursionDepth = depth;

	RecurseAndGetTreeDepth(g_pRecursionWorld->GetNode(iRoot)->m_Sides[0], depth+1);
	RecurseAndGetTreeDepth(g_pRecursionWorld->GetNode(iRoot)->m_Sides[1], depth+1);
}


// ----------------------------------------------------------------------- //
// Node and leaf spheres.
// ----------------------------------------------------------------------- //

// The box of the polies under pNode, into vMin and vMax; FALSE if pNode
// isn't a node. Sets the nodes' spheres on the way up (0x439e60..0x43a286;
// the function table's 0x43a1d8 is inside it; the name is ours). Near (1056
// bytes against 1062): in the radius loop the shipped code keeps nStack
// in memory and fRadius's address in ebp, and copies vCenter once per node
// (0x43a0f3); ours keeps nStack in ebp and copies vCenter per vertex.
BOOL CPreWorld::CalcNodeBoundingBoxes_R(NODEREF pNode, PVector &vMin, PVector &vMax)
{
	PVector vChildMin, vChildMax, vPt;
	CPrePoly *pPoly;
	uint32 i;
	int nStack;
	PReal fDist;

	nStack = 0;
	vMin.Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	vMax.Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);

	if(!IsValidNode(pNode))
		return FALSE;

	pPoly = pNode->m_pPoly;

	CalcNodeBoundingBoxes_R(pNode->m_Sides[0], vMin, vMax);
	if(CalcNodeBoundingBoxes_R(pNode->m_Sides[1], vChildMin, vChildMax))
	{
		if(vChildMin.x < vMin.x) vMin.x = vChildMin.x;
		if(vChildMin.y < vMin.y) vMin.y = vChildMin.y;
		if(vChildMin.z < vMin.z) vMin.z = vChildMin.z;

		if(vChildMax.x > vMax.x) vMax.x = vChildMax.x;
		if(vChildMax.y > vMax.y) vMax.y = vChildMax.y;
		if(vChildMax.z > vMax.z) vMax.z = vChildMax.z;
	}

	for(i=0; i < pPoly->NumVerts(); i++)
	{
		vPt = pPoly->Pt(i);

		if(vPt.x < vMin.x) vMin.x = vPt.x;
		if(vPt.y < vMin.y) vMin.y = vPt.y;
		if(vPt.z < vMin.z) vMin.z = vPt.z;

		if(vPt.x > vMax.x) vMax.x = vPt.x;
		if(vPt.y > vMax.y) vMax.y = vPt.y;
		if(vPt.z > vMax.z) vMax.z = vPt.z;
	}

	PReal &fRadius = pNode->m_fRadius;
	pNode->m_vCenter = vMin + (vMax - vMin) * 0.5f;
	PVector &vCenter = pNode->m_vCenter;
	fRadius = -1.0f;

	// The radius: the farthest vertex of the subtree's polies.
	for(;;)
	{
		while(!IsValidNode(pNode))
		{
			if(nStack == 0)
				goto Done;

			pNode = g_NodeStack[--nStack];
		}

		pPoly = pNode->m_pPoly;
		for(i=0; i < pPoly->NumVerts(); i++)
		{
			fDist = (pPoly->Pt(i) - vCenter).Mag();
			if(fDist > fRadius)
				fRadius = fDist;
		}

		if(nStack < (int)g_NodeStack.GetSize())
			g_NodeStack[nStack] = pNode->m_Sides[0];
		else
			g_NodeStack.Append(pNode->m_Sides[0]);

		nStack++;
		pNode = pNode->m_Sides[1];
	}

Done:
	fRadius += 1.0f;
	return TRUE;
}


void CPreWorld::CalcNodeBoundingBoxes()
{
	PVector vMin, vMax;

	CalcNodeBoundingBoxes_R(m_RootNode, vMin, vMax);
}


void CPreWorld::TermLeaves()
{
	GPOS pos;

	DeleteAndClearArray(m_Leaves);

	for(pos=m_Nodes; pos; )
		m_Nodes.GetNext(pos)->m_pLeaf = NULL;
}


void CPreWorld::RemoveAllNodes()
{
	GDeleteAndRemoveElements(m_Nodes);
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CEditBrush::JoinPolies
//      PURPOSE:        Joins as many polies in the current plane as possible.
// ----------------------------------------------------------------------- //

static void AddToConnectivity(CPrePoly *pPoly, CMoArray<PrePolyArray> &pointConnectivity);


CPrePoly* FindPolyWithEdge2(CMoArray<CPrePoly*> &polies, 
	CPrePoly *pReject,
	CPreSurface *pSurface,
	uint32 iPt1,
	uint32 iPt2,
	WORD &pt1Index, 
	WORD &pt2Index)
{
	uint32 i, iPoly, nextI;
	CPrePoly *pPoly;


	for(iPoly=0; iPoly < polies; iPoly++)
	{
		pPoly = polies[iPoly];

		if(pPoly->GetPostRemove() || pPoly == pReject || pPoly->m_pSurface != pSurface)
			continue;

		for(i=0; i < pPoly->NumVerts(); i++)
		{
			nextI = (i+1) % pPoly->NumVerts();

			if( (pPoly->Index(i)==iPt2) && (pPoly->Index(nextI)==iPt1) )
			{
				pt1Index = (WORD)nextI;
				pt2Index = (WORD)i;
				return pPoly;
			}
			else if( (pPoly->Index(nextI)==iPt2) && (pPoly->Index(i)==iPt1) )
			{
				pt1Index = (WORD)i;
				pt2Index = (WORD)nextI;
				return pPoly;
			}
		}
	}

	return NULL;
}


void CPreWorld::JoinPolies(PReal convexThreshold)
{
	JoinPolyList(m_Polies, convexThreshold);
}


void CPreWorld::JoinPolyList(CGLinkedList<CPrePoly*> &polies, PReal convexThreshold)
{
	uint32 edge, nRemoved;
	CPrePoly *pPoly, *pOther;
	CPrePoly *pJoined;
	CMoArray<PrePolyArray> pointConnectivity;
	BOOL bAnyJoined;
	WORD otherIndex1, otherIndex2;
	GPOS pos;

	
	nRemoved = 0;
	pointConnectivity.SetSize(m_Points.GetSize());


	// Mark them all as non-removed.
	// Add them to the point connectivity thing.
	for(pos=polies; pos; )
	{
		pPoly = polies.GetNext(pos);

		pPoly->SetPostRemove(FALSE);
		AddToConnectivity(pPoly, pointConnectivity);
	}
	

	do
	{
		bAnyJoined = FALSE;

		for(pos=polies; pos; )
		{
			pPoly = polies.GetNext(pos);
			
			if(pPoly->GetPostRemove())
				continue;

			// For each edge, try to find the same edge on another polygon.
			for(edge=0; edge < pPoly->NumVerts(); edge++)
			{
				if(
					(pOther = FindPolyWithEdge2(pointConnectivity[pPoly->Index(edge)], pPoly, pPoly->m_pSurface, 
						pPoly->Index(edge), pPoly->NextIndex(edge), otherIndex1, otherIndex2)) ||
					
					(pOther = FindPolyWithEdge2(pointConnectivity[pPoly->NextIndex(edge)], pPoly, pPoly->m_pSurface, 
						pPoly->Index(edge), pPoly->NextIndex(edge), otherIndex1, otherIndex2))
				)
				{
					if((pPoly->NumVerts() + pOther->NumVerts() - 2) < MAX_WORLDPOLY_VERTS)
					{
						// Ok, join them.
						pJoined = JoinPolygons(pPoly, pOther, edge, otherIndex1, 
							(edge+1)%pPoly->NumVerts(), otherIndex2);

						if(pJoined)
						{
							if(pJoined->PostJoinFixup(convexThreshold))
							{
								pPoly->SetPostRemove(TRUE);
								pOther->SetPostRemove(TRUE);
								++nRemoved;
								polies.Append(pJoined);
								AddToConnectivity(pJoined, pointConnectivity);

								// Start the loops over.
								bAnyJoined = TRUE;
								break;
							}
							else
							{
								DeletePoly(pJoined);
							}
						}
					}
				}
			}
		}
	} while(bAnyJoined);

	// Remove all the polies that got marked.
	for(pos=polies; pos; )
	{
		pPoly = polies.GetNext(pos);

		if(pPoly->GetPostRemove())
		{
			polies.RemoveAt(pPoly);
			DeletePoly(pPoly);
		}
	}
}


static void AddToConnectivity(CPrePoly *pPoly, 
	CMoArray<PrePolyArray> &pointConnectivity)
{
	uint32 i;

	for(i=0; i < pPoly->NumVerts(); i++)
	{
		pointConnectivity[pPoly->Index(i)].Append(pPoly);
	}
}


// ----------------------------------------------------------------------- //
//      ROUTINE:        CPreWorld::JoinPolygons
//      PURPOSE:        Joins the two polygons, leaving the originals intact.
// ----------------------------------------------------------------------- //

CPrePoly* CPreWorld::JoinPolygons(CPrePoly *pPoly1, CPrePoly *pPoly2, 
	WORD poly1v1, WORD poly2v1, WORD poly1v2, WORD poly2v2)
{
	CPrePoly *pNew;
	uint32 i, i1, i2, nNewPolyVerts, end;


	i1 = poly2v1;
	i2 = poly2v2;

	// Make sure they came in the right order.
	if(i2 == ((i1+1)%pPoly2->NumVerts()))
	{
		return NULL;
	}

	
	// Join the two.
	nNewPolyVerts = (pPoly1->NumVerts() + pPoly2->NumVerts()) - 2;
	pNew = CreatePoly(CPrePoly, nNewPolyVerts, FALSE);
	
	pNew->m_pSurface = pPoly1->m_pSurface;
	pNew->m_pOriginalBrushPoly = pPoly1->m_pOriginalBrushPoly;
	
	// Add poly1's vertices, except for the joined two.all of poly1's vertices.
	i = (poly1v2+1) % pPoly1->NumVerts();
	do
	{
		pNew->AddVert(pPoly1->Pt(i));
		pNew->Index(pNew->NumVerts()-1) = pPoly1->Index(i);
		i = (i+1) % pPoly1->NumVerts();
	} while(i != poly1v1);
	

	// Add all of poly2's vertices.
	i = end = poly2v1;
	do
	{
		pNew->AddVert(pPoly2->Pt(i));
		pNew->Index(pNew->NumVerts()-1) = pPoly2->Index(i);
		i = (i+1) % pPoly2->NumVerts();
	} while(i != end);

	return pNew;
}


// ----------------------------------------------------------------------- //
//
//      ROUTINE:        CPreWorld counting functions
//
// ----------------------------------------------------------------------- //

// The nodes whose polies are detail (0x43aad0; the name is ours).
uint32 CPreWorld::NumDetailNodes()
{
	uint32 num=0;
	GPOS pos;

	for(pos=m_Nodes; pos; )
	{
		if(m_Nodes.GetNext(pos)->m_pPoly->m_pSurface->IsDetail())
			++num;
	}

	return num;
}


uint32 CPreWorld::NumNodes()
{
	return m_Nodes.GetSize();
}


uint32 CPreWorld::NumPolyVerts()
{
	uint32 num=0;
	GPOS pos;

	for(pos=m_Polies; pos; )
		num += m_Polies.GetNext(pos)->NumVerts();

	return num;
}


// The leaves' visibility data (0x43ab60; the name is ours).
uint32 CPreWorld::CalcVisListMemory()
{
	uint32 i, j, size;
	CPreLeaf *pLeaf;

	size = 0;
	for(i=0; i < m_Leaves; i++)
	{
		pLeaf = m_Leaves[i];

		for(j=0; j < pLeaf->m_Unknown_6c; j++)
			size += pLeaf->m_Unknown_6c[j]->GetSize();
	}

	return size;
}


// (0x43aba0; the name is ours.)
uint32 CPreWorld::CalcVisListCount()
{
	uint32 i, count;

	count = 0;
	for(i=0; i < m_Leaves; i++)
		count += m_Leaves[i]->m_Unknown_6c.GetSize();

	return count;
}


// (0x43abc0; the name is ours.)
uint32 CPreWorld::NumVisibleLeaves()
{
	uint32 i, count;

	count = 0;
	for(i=0; i < m_Leaves; i++)
	{
		if(m_Leaves[i]->m_Flags & 1)
			++count;
	}

	return count;
}


// (0x43abf0; the name is ours.)
uint32 CPreWorld::NumLeafPolies()
{
	uint32 i, count;

	count = 0;
	for(i=0; i < m_Leaves; i++)
		count += m_Leaves[i]->m_Polies.GetSize();

	return count;
}


// The box of the polies on pUser's surface (0x43ac10; the name is ours).
void CPreWorld::GetSurfaceBoundingBox(PreSurfaceUser *pUser, PVector *pMin, PVector *pMax)
{
	GPOS pos;
	CPrePoly *pPoly;
	uint32 i;

	pMin->Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
	VEC_NEGATE(*pMax, *pMin);

	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);

		if(pPoly->m_pSurface == pUser->m_pSurface)
		{
			for(i=0; i < pPoly->NumVerts(); i++)
			{
				if(pPoly->Pt(i).x < pMin->x) pMin->x = pPoly->Pt(i).x;
				if(pPoly->Pt(i).y < pMin->y) pMin->y = pPoly->Pt(i).y;
				if(pPoly->Pt(i).z < pMin->z) pMin->z = pPoly->Pt(i).z;

				if(pPoly->Pt(i).x > pMax->x) pMax->x = pPoly->Pt(i).x;
				if(pPoly->Pt(i).y > pMax->y) pMax->y = pPoly->Pt(i).y;
				if(pPoly->Pt(i).z > pMax->z) pMax->z = pPoly->Pt(i).z;
			}
		}
	}
}


// Each leaf's sphere, around its portals (0x43ad20; the name is ours).
void CPreWorld::CalcLeafSpheres()
{
	uint32 i, j, k;
	CPreLeaf *pLeaf;
	CPrePortal *pPortal;
	PVector vMin, vMax;

	for(i=0; i < m_Leaves; i++)
	{
		pLeaf = m_Leaves[i];

		vMin.Init((PReal)MAX_CREAL, (PReal)MAX_CREAL, (PReal)MAX_CREAL);
		vMax.Init((PReal)-MAX_CREAL, (PReal)-MAX_CREAL, (PReal)-MAX_CREAL);

		for(j=0; j < pLeaf->m_Portals; j++)
		{
			pPortal = pLeaf->m_Portals[j];

			for(k=0; k < pPortal->NumVerts(); k++)
			{
				VEC_MIN(vMin, vMin, pPortal->Pt(k));
				VEC_MAX(vMax, vMax, pPortal->Pt(k));
			}
		}

		pLeaf->m_vCenter = vMin + (vMax - vMin) * 0.5f;
		pLeaf->m_fRadius = ((vMax - vMin) * 0.5f).Mag();
	}
}


void CPreWorld::FindTextureOrigins()
{
	GPOS pos;
	CPrePoly *pPoly;

	// Make polygon origins.
	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);
		pPoly->FindTextureOrigin(NULL, TRUE, m_pMainWorld->GetLMGridSize());
	}
}


void CPreWorld::SetPolyWorldIndices()
{
	GPOS pos;
	uint32 myIndex;

	myIndex = m_pMainWorld->m_WorldModels.FindElement(this);
	if(myIndex < m_pMainWorld->m_WorldModels.GetSize())
	{
		for(pos=m_Polies; pos; )
			m_Polies.GetNext(pos)->m_Index = myIndex;
	}
}


// ----------------------------------------------------------------------- //
// An ILTStream on a CAbstractIO, so the physics data can be read and
// written with ILTStream's operators (the name is ours).
// ----------------------------------------------------------------------- //

class CAbstractIOLTStream : public CGenLTStream
{
public:

						CAbstractIOLTStream(CAbstractIO *pFile)
						{
							m_pFile = pFile;
							m_bError = FALSE;
						}

	virtual void		Release()
	{
		delete this;
	}

	virtual LTRESULT	Read(void *pData, uint32 size)
	{
		if(m_pFile->Read(pData, size))
			return LT_OK;

		m_bError = TRUE;
		return LT_ERROR;
	}

	virtual LTRESULT	ErrorStatus()
	{
		return m_bError ? LT_ERROR : LT_OK;
	}

	virtual LTRESULT	SeekTo(uint32 offset)
	{
		if(m_pFile->SeekTo(offset))
			return LT_OK;

		m_bError = TRUE;
		return LT_ERROR;
	}

	virtual LTRESULT	GetPos(uint32 *offset)
	{
		*offset = m_pFile->GetCurPos();
		return LT_OK;
	}

	virtual LTRESULT	GetLen(uint32 *len)
	{
		*len = m_pFile->GetLen();
		return LT_OK;
	}

	virtual LTRESULT	Write(const void *pData, uint32 size)
	{
		if(m_pFile->Write((void*)pData, size))
			return LT_OK;

		m_bError = TRUE;
		return LT_ERROR;
	}

public:

	CAbstractIO		*m_pFile;
	BOOL			m_bError;
};


// ----------------------------------------------------------------------- //
// The physics blocks.
// ----------------------------------------------------------------------- //

BOOL CPreWorld::LoadPBlockTable(CAbstractIO &file, PBlockTable *pTable)
{
	int i;

	file >> pTable->m_nBlocks[0];
	file >> pTable->m_nBlocks[1];
	file >> pTable->m_nBlocks[2];
	pTable->m_nBlocksXY = pTable->m_nBlocks[0] * pTable->m_nBlocks[1];
	pTable->m_nBlocksTotal = pTable->m_nBlocks[2] * pTable->m_nBlocks[0] * pTable->m_nBlocks[1];

	pTable->m_pBlocks = (PBlock*)dalloc_z(pTable->m_nBlocksTotal * sizeof(PBlock));
	if(!pTable->m_pBlocks && (int)pTable->m_nBlocksTotal > 0)
	{
		pTable->m_vBlockSize.Init();
		pTable->m_vMin.Init();
		pTable->m_nBlocks[2] = 0;
		pTable->m_nBlocksXY = 0;
		pTable->m_nBlocksTotal = 0;
		pTable->m_pBlocks = NULL;
		pTable->m_nBlocks[0] = 0;
		pTable->m_nBlocks[1] = 0;
		return FALSE;
	}

	file >> pTable->m_vBlockSize.x;
	file >> pTable->m_vBlockSize.y;
	file >> pTable->m_vBlockSize.z;
	file >> pTable->m_vMin.x;
	file >> pTable->m_vMin.y;
	file >> pTable->m_vMin.z;

	// 0x4387b9: the block is indexed afresh for each use.
	for(i=0; i < (int)pTable->m_nBlocksTotal; i++)
	{
		file >> pTable->m_pBlocks[i].m_nNodes;
		file >> pTable->m_pBlocks[i].m_iRoot;
		pTable->m_pBlocks[i].m_pData = dalloc(pTable->m_pBlocks[i].m_nNodes * sizeof(SaveNode));
		file.Read(pTable->m_pBlocks[i].m_pData, pTable->m_pBlocks[i].m_nNodes * sizeof(SaveNode));
	}

	return TRUE;
}


void CPreWorld::SavePBlockTable(CAbstractIO &file, PBlockTable *pTable)
{
	int i;

	file << pTable->m_nBlocks[0] << pTable->m_nBlocks[1] << pTable->m_nBlocks[2];
	pTable->m_nBlocksXY = pTable->m_nBlocks[0] * pTable->m_nBlocks[1];
	pTable->m_nBlocksTotal = pTable->m_nBlocks[2] * pTable->m_nBlocks[0] * pTable->m_nBlocks[1];

	file << pTable->m_vBlockSize.x << pTable->m_vBlockSize.y << pTable->m_vBlockSize.z;
	file << pTable->m_vMin.x << pTable->m_vMin.y << pTable->m_vMin.z;

	// 0x43891b: the block is indexed afresh for each use.
	for(i=0; i < (int)pTable->m_nBlocksTotal; i++)
	{
		CheckSaveNodes_R((SaveNode**)&pTable->m_pBlocks[i].m_pData, pTable->m_pBlocks[i].m_iRoot);
		file << pTable->m_pBlocks[i].m_nNodes;
		file << pTable->m_pBlocks[i].m_iRoot;
		file.Write(pTable->m_pBlocks[i].m_pData, pTable->m_pBlocks[i].m_nNodes * sizeof(SaveNode));
	}
}


// ----------------------------------------------------------------------- //
// The sub-BSPs.
// ----------------------------------------------------------------------- //

static inline NODEREF IndexToNode(uint32 index, CMoArray<CNode*> &nodes)
{
	if(index == (uint32)-1)
		return NODE_IN;
	else if(index == (uint32)-2)
		return NODE_OUT;
	else
		return nodes[index];
}


BOOL CPreWorld::LoadSubBsp(CAbstractIO &file, CMoArray<CPrePoly*> &polies, CPreSubBsp *pBsp)
{
	CMoArray<CNode*> nodes;
	CNode *pNode, *pNullNode;
	CPrePoly *pNullPoly;
	uint32 i, nNodes, index, polyIndex;

	pNullNode = NULL;
	file >> nNodes;
	if(!nodes.SetSizeInit(nNodes, pNullNode))
		return FALSE;

	for(i=0; i < nodes; i++)
	{
		nodes[i] = new CNode;
		if(!nodes[i])
		{
			DeleteAndClearArray(nodes);
			return FALSE;
		}
	}

	for(i=0; i < nodes; i++)
	{
		pNode = nodes[i];

		file >> polyIndex;
		if(polyIndex >= polies.GetSize())
		{
			DeleteAndClearArray(nodes);
			return FALSE;
		}

		pNode->m_pPoly = polies[polyIndex];

		file >> index;
		pNode->m_Sides[0] = IndexToNode(index, nodes);

		file >> index;
		pNode->m_Sides[1] = IndexToNode(index, nodes);
	}

	file >> index;
	pBsp->m_RootNode = IndexToNode(index, nodes);

	pBsp->m_Nodes.Term();
	for(i=0; i < nodes; i++)
		pBsp->m_Nodes.AddTail(nodes[i]);

	file >> index;
	pNullPoly = NULL;
	if(!pBsp->m_Polies.SetSizeInit(index, pNullPoly))
		return FALSE;

	for(i=0; i < index; i++)
	{
		file >> polyIndex;
		if(polyIndex >= polies.GetSize())
			return FALSE;

		pBsp->m_Polies[i] = polies[polyIndex];
	}

	if(!LoadPBlockTable(file, &pBsp->m_PBlocks))
		return FALSE;

	CAbstractIOLTStream(&file) >> pBsp->m_Unknown_00[0] >> pBsp->m_Unknown_00[1] >>
		pBsp->m_Unknown_00[2] >> pBsp->m_Unknown_00[3] >> pBsp->m_Unknown_04;
	CAbstractIOLTStream(&file) >> pBsp->m_Unknown_30;

	return TRUE;
}


void CPreWorld::SaveSubBsp(CAbstractIO &file, CPreSubBsp *pBsp)
{
	GPOS pos;
	CNode *pNode;
	uint32 i;

	SetNodeIndices(pBsp->m_Nodes);

	file << pBsp->m_Nodes.GetSize();
	for(pos=pBsp->m_Nodes; pos; )
	{
		pNode = pBsp->m_Nodes.GetNext(pos);

		file << pNode->m_pPoly->m_nName;
		file << NodeToIndex(pNode->m_Sides[0]);
		file << NodeToIndex(pNode->m_Sides[1]);
	}

	file << NodeToIndex(pBsp->m_RootNode);

	file << pBsp->m_Polies.GetSize();
	for(i=0; i < pBsp->m_Polies; i++)
		file << pBsp->m_Polies[i]->m_nName;

	SavePBlockTable(file, &pBsp->m_PBlocks);

	CAbstractIOLTStream(&file) << pBsp->m_Unknown_00[0] << pBsp->m_Unknown_00[1] <<
		pBsp->m_Unknown_00[2] << pBsp->m_Unknown_00[3] << pBsp->m_Unknown_04;
	CAbstractIOLTStream(&file) << pBsp->m_Unknown_30;
}


// The world in the 2001 .dat form (0x436ad0).
BOOL CPreWorld::SaveFile(CAbstractIO &file)
{
	CMoArray<CPrePoly*> unused;
	CMoByteArray pointsUsed;
	CMoWordArray indexMap;
	PolyEdges edges;
	CMoArray< CMoArray<PolyAnimRef> > polyAnims;
	CMoArray<PolyAnimRef> *pAnims;
	uint32 i, j, iPoly, nPointsUsed, nSurfaces, nRefs, nameLen;
	GPOS pos;
	LPOS lPos;
	CPrePoly *pPoly, *pFound;
	CPrePlane *pPlane;
	CPreSurface *pSurface;
	CPreLeaf *pLeaf;
	CPreLeafList *pList;
	CNode *pNode;
	PreSurfaceUser *pUser;
	CPolyVert *pVert;
	PVector vMin, vMax;
	uint8 r, g, b;

	// Filled in later.
	file << (uint32)0 << (uint32)0 << (uint32)0 << (uint32)0;
	file << (uint32)0 << (uint32)0 << (uint32)0 << (uint32)0;

	file << m_WorldInfoFlags;
	file << GetTreeDepth();

	m_Unknown_e8 = MakePolyEdges(edges);

	m_TextureNames.Term();
	m_TextureNames.SetCacheSize(100);
	GetTextureNames(m_TextureNames);

	SetNodeIndices(m_Nodes);
	CheckNodes_R(m_RootNode);

	SetupIndexMap(pointsUsed, indexMap, nPointsUsed);
	m_nPointsSaved = nPointsUsed;

	file.WriteString(m_WorldName);
	file << nPointsUsed;
	file << m_Planes.GetSize();

	CMoArray<CPreSurface*> surfaces;
	CMoArray<uint32> surfaceMap;

	GetUniqueSurfaces(surfaces, surfaceMap);
	nSurfaces = surfaces.GetSize();
	file << nSurfaces;

	file << m_Unknown_04.GetSize();
	file << m_Polies.GetSize();
	file << m_Leaves.GetSize();
	file << NumPolyVerts();
	file << CalcVisListMemory();
	file << CalcVisListCount();
	file << m_Nodes.GetSize();
	file << NumLeafPolies();

	if(!GetPolyLightAnims(polyAnims, nRefs))
		return FALSE;

	file << nRefs;

	GetBoundingBox(&vMin, &vMax);
	file << vMin.x << vMin.y << vMin.z;
	file << vMax.x << vMax.y << vMax.z;
	file << m_WorldTranslation.x << m_WorldTranslation.y << m_WorldTranslation.z;

	// The texture names.
	nameLen = 0;
	for(i=0; i < m_TextureNames; i++)
		nameLen += strlen(m_TextureNames[i]) + 1;

	file << nameLen;
	file << m_TextureNames.GetSize();
	for(i=0; i < m_TextureNames; i++)
		file.Write((void*)m_TextureNames[i], strlen(m_TextureNames[i]) + 1);

	SetPolyWorldIndices();
	SetPolyIndices();
	SetPlaneIndices();
	SetLeafIndices();

	// The polies' vertex counts.
	for(pos=m_Polies; pos; )
	{
		file << (uint8)m_Polies.GetAt(pos)->NumVerts();
		file << (uint8)m_Polies.GetNext(pos)->m_Unknown_0c.GetSize();
	}

	// The leaves.
	for(i=0; i < m_Leaves; i++)
	{
		pLeaf = m_Leaves[i];

		if(pLeaf->m_Unknown_80 == 0xFFFF)
		{
			file << (uint16)pLeaf->m_Unknown_6c.GetSize();
			for(j=0; j < pLeaf->m_Unknown_6c; j++)
			{
				pList = pLeaf->m_Unknown_6c[j];

				file << (uint16)pList->m_Unknown_14;
				file << (uint16)pList->m_Data.GetSize();
				file.Write(pList->m_Data.GetArray(), pList->m_Data.GetSize());
			}
		}
		else
		{
			file << (uint16)0xFFFF;
			file << pLeaf->m_Unknown_80;
		}

		file << pLeaf->m_Polies.GetSize();
		for(lPos=pLeaf->m_Polies; lPos; )
		{
			pPoly = pLeaf->m_Polies.GetNext(lPos);

			file << (uint16)pPoly->m_Index;
			file << (uint16)pPoly->m_nName;
		}

		file << pLeaf->m_Unknown_9c;
	}

	// The planes.
	for(pos=m_Planes; pos; )
	{
		pPlane = m_Planes.GetNext(pos);

		file << pPlane->m_Normal.x;
		file << pPlane->m_Normal.y;
		file << pPlane->m_Normal.z;
		file << pPlane->m_Dist;
	}

	// The surfaces.
	for(i=0; i < nSurfaces; i++)
	{
		pSurface = surfaces[i];

		if(pSurface->m_bUsed)
		{
			file << pSurface->m_TextureO.x;
			file << pSurface->m_TextureO.y;
			file << pSurface->m_TextureO.z;
			file << pSurface->m_TextureP.x;
			file << pSurface->m_TextureP.y;
			file << pSurface->m_TextureP.z;
			file << pSurface->m_TextureQ.x;
			file << pSurface->m_TextureQ.y;
			file << pSurface->m_TextureQ.z;

			file << FindTextureName(pSurface->m_pTextureName, m_TextureNames);
			file << pSurface->m_Flags;
			file << pSurface->m_bDetail;
			file << pSurface->m_Ambient[0];
			file << pSurface->m_Ambient[1];
			file << pSurface->m_Ambient[2];

			if(pSurface->m_Unknown_a0 && pSurface->m_Unknown_a4)
			{
				file << (uint8)1;
				file.WriteString((char*)pSurface->m_Unknown_a0);
				file.WriteString((char*)pSurface->m_Unknown_a4);
			}
			else
			{
				file << (uint8)0;
			}

			file << pSurface->m_TextureFlags;
		}
	}

	// The points.
	for(i=0; i < m_Points; i++)
	{
		if(pointsUsed[i])
		{
			file << m_Points[i].x;
			file << m_Points[i].y;
			file << m_Points[i].z;
		}
	}

	// The polies.
	iPoly = 0;
	for(pos=m_Polies; pos; )
	{
		pPoly = m_Polies.GetNext(pos);

		file << pPoly->m_PolyO.x;
		file << pPoly->m_PolyO.y;
		file << pPoly->m_PolyO.z;
		file << pPoly->m_LMWidth;
		file << pPoly->m_LMHeight;

		pAnims = &polyAnims[iPoly];
		file << (uint16)pAnims->GetSize();
		for(j=0; j < *pAnims; j++)
		{
			file << (*pAnims)[j].m_iAnim;
			file << (*pAnims)[j].m_iRef;
		}

		file << surfaceMap[pPoly->m_pSurface->m_UseIndex];
		file << pPoly->m_pSurface->m_pPlane->GetIndex();

		file << pPoly->m_pSurface->m_TextureO.x;
		file << pPoly->m_pSurface->m_TextureO.y;
		file << pPoly->m_pSurface->m_TextureO.z;
		file << pPoly->m_pSurface->m_TextureP.x;
		file << pPoly->m_pSurface->m_TextureP.y;
		file << pPoly->m_pSurface->m_TextureP.z;
		file << pPoly->m_pSurface->m_TextureQ.x;
		file << pPoly->m_pSurface->m_TextureQ.y;
		file << pPoly->m_pSurface->m_TextureQ.z;

		for(j=0; j < pPoly->NumVerts(); j++)
		{
			pVert = &pPoly->Vert(j);

			file << indexMap[pVert->m_Index];
			r = (uint8)pVert->m_Color.x;
			g = (uint8)pVert->m_Color.y;
			b = (uint8)pVert->m_Color.z;
			file << r;
			file << g;
			file << b;
		}

		for(j=0; j < pPoly->m_Unknown_0c; j++)
		{
			pVert = &pPoly->m_Unknown_0c[j];

			file << indexMap[pVert->m_Index];
			r = (uint8)pVert->m_Color.x;
			g = (uint8)pVert->m_Color.y;
			b = (uint8)pVert->m_Color.z;
			file << r;
			file << g;
			file << b;
		}

		iPoly++;
	}

	// The nodes.
	for(pos=m_Nodes; pos; )
	{
		pNode = m_Nodes.GetNext(pos);

		file << pNode->m_pPoly->m_nName;
		file << (uint16)(pNode->m_pLeaf ? pNode->m_pLeaf->m_Index : 0xFFFF);
		file << NodeToIndex(pNode->m_Sides[0]);
		file << NodeToIndex(pNode->m_Sides[1]);
	}

	// What m_Unknown_04 holds.
	for(pos=m_Unknown_04; pos; )
	{
		pUser = (PreSurfaceUser*)m_Unknown_04.GetNext(pos);

		file.WriteString((char*)pUser->m_pName);
		file << (uint32)0;

		pFound = NULL;
		for(GPOS polyPos=m_Polies; polyPos; )
		{
			pPoly = m_Polies.GetNext(polyPos);
			if(pPoly->m_pSurface == pUser->m_pSurface)
			{
				pFound = pPoly;
				break;
			}
		}

		file << pFound->m_nName;
		file << (uint16)surfaceMap[pUser->m_pSurface->m_UseIndex];

		PVector vBoxMin, vBoxMax;
		GetSurfaceBoundingBox(pUser, &vBoxMin, &vBoxMax);
		PVector vCenter = vBoxMin + (vBoxMax - vBoxMin) * 0.5f;
		PVector vDims = vBoxMax - vCenter;

		file << vCenter.x;
		file << vCenter.y;
		file << vCenter.z;
		file << vDims.x;
		file << vDims.y;
		file << vDims.z;
	}

	SavePBlockTable(file, &m_PBlocks);
	file << NodeToIndex(m_RootNode);

	file << m_PolyArray.GetSize();
	for(i=0; i < m_PolyArray; i++)
		SaveSubBsp(file, m_PolyArray[i]);

	return TRUE;
}


// talon: 0x43b3c0..0x43e5a0 holds no hand-written PreWorld code: it is
// the template and inline instances the PreWorld object emits (CMoArray and
// CGLinkedList Gen* virtuals, SetSize2/Insert2/Remove2/CopyArray2,
// DeleteAndClearArray2, STLport list/vector/heap pieces, _CVector inlines).
// They come with the code that uses them. PreWorld ends at
// 0x43e5a0.
