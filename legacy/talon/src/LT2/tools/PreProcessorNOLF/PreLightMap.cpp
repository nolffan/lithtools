#include "bdefs.h"
#include "preworld.h"
#include "pregeometry.h"
#include "preleaf.h"
#include "prelightmap.h"
#include "lightmap_compress.h"
#include "node.h"
#include "editobjects.h"
#include "brushtoworld.h"
#include "splitpoly.h"
#include "editbrush.h"
#include "editpoly.h"
#include "findworldmodel.h"
#include "proputils.h"
#include "processing.h"

// talon: 0x43ee40..0x43f2b0 in the image, in its order (CMoArray<uint8>'s
// destructor, 0x43ee00, is this object's template code); Term is inline
// (no copy of its own in the image; Compress* expand it).

CPreLightMap::CPreLightMap()
{
	m_Width = m_Height = 0;
}




void CPreLightMap::Term()
{
	m_Width = m_Height = 0;
	m_Data.Term();
}


// talon: NOLF's lightmaps are 16 bits a pixel: AllBlack (0x43eec0) checks
// width*height*2 bytes of a 2048-byte buffer, and CompressLightMap's buffer
// is 2048 bytes too (0x43ef53, sub $0x828).
BOOL CPreLightMap::AllBlack()
{
	uint16 tempData[LIGHTMAP_MAX_TOTAL_PIXELS];
	uint8 *pBytes;
	uint32 i, nBytes;

	if(m_Unknown_08)
	{
		if(!DecompressShadowMap((uint8*)tempData))
			return FALSE;

		nBytes = m_Width * m_Height;
	}
	else
	{
		if(!Decompress(tempData))
			return FALSE;

		nBytes = m_Width * m_Height * 2;
	}

	// talon: near (138 bytes against 140): the shipped loop's second
	// (unrolled) test reads through a register holding pBytes, ours
	// through the frame.
	pBytes = (uint8*)tempData;
	for(i=0; i < nBytes; i++)
	{
		if(pBytes[i] != 0)
			return FALSE;
	}

	return TRUE;
}


BOOL CPreLightMap::CompressLightMap(uint32 width, uint32 height, uint16 *pData)
{
	uint16 compressedData[LIGHTMAP_MAX_TOTAL_PIXELS];
	uint32 dataLen;

	Term();

	if(!CompressLMData(pData, width, height, compressedData, dataLen))
		return FALSE;

	if(!m_Data.SetSize(dataLen))
		return FALSE;

	memcpy(m_Data.GetArray(), compressedData, dataLen);
	m_Unknown_08 = FALSE;
	m_Width = width;
	m_Height = height;
	return TRUE;
}




// talon: near (0x43f0a0, size-exact): the inlined m_Data.SetSize adds the
// cache size to dataLen in the other order (add %eax,%esi; the shipped
// code, add %esi,%eax). icl 4.0 gives one of CompressLightMap and this the
// shipped order and the other not, depending on what else the object
// holds (with 32-bit pixels and an inline Term it was the other way round),
// not on either function's statement or declaration order.
BOOL CPreLightMap::CompressShadowMap(uint32 width, uint32 height, uint16 *pData)
{
	uint8 compressedData[LIGHTMAP_MAX_TOTAL_PIXELS];
	uint32 dataLen;

	Term();

	if(!::CompressShadowMap(pData, width, height, compressedData, dataLen))
		return FALSE;

	if(!m_Data.SetSize(dataLen))
		return FALSE;

	memcpy(m_Data.GetArray(), compressedData, dataLen);
	m_Height = height;
	m_Width = width;
	m_Unknown_08 = TRUE;
	return TRUE;
}




void CFullLightMap::ColorFill( CVector color )
{
	PVector *pCur, *pEnd;
	uint32 y;

	pCur = m_Map.GetArray();
	for(y=0; y < m_Height; y++)
	{
		pEnd = pCur + m_Width;
		while(pCur < pEnd)
		{
			*pCur = color;
			pCur++;
		}
	}
}


BOOL CPreLightMap::DecompressShadowMap(uint8 *pData)
{
	return ::DecompressShadowMap(m_Data.GetArray(), m_Data.GetSize(), pData);
}


BOOL CPreLightMap::Decompress(uint16 *pData)
{
	return DecompressLMData((uint16*)m_Data.GetArray(), m_Data.GetSize(), pData);
}
