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

// talon: 0x42d7f0..0x42dc00 in the image.

CPreLightMap::CPreLightMap()
{
	m_Width = m_Height = 0;
}


void CPreLightMap::Term()
{
	m_Width = m_Height = 0;
	m_Data.Term();
}


BOOL CPreLightMap::AllBlack()
{
	uint32 tempData[LIGHTMAP_MAX_TOTAL_PIXELS];
	uint8 *pBytes;
	uint32 i, nBytes;

	pBytes = (uint8*)tempData;
	if(m_Unknown_08)
	{
		if(!DecompressShadowMap(pBytes))
			return FALSE;

		nBytes = m_Width * m_Height;
	}
	else
	{
		if(!Decompress(tempData))
			return FALSE;

		nBytes = m_Width * m_Height * 4;
	}

	for(i=0; i < nBytes; i++)
	{
		if(pBytes[i] != 0)
			return FALSE;
	}

	return TRUE;
}


BOOL CPreLightMap::CompressLightMap(uint32 width, uint32 height, uint32 *pData)
{
	uint32 compressedData[LIGHTMAP_MAX_TOTAL_PIXELS];
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


BOOL CPreLightMap::Decompress(uint32 *pData)
{
	return DecompressLMData((uint32*)m_Data.GetArray(), m_Data.GetSize(), pData);
}


BOOL CPreLightMap::CompressShadowMap(uint32 width, uint32 height, uint32 *pData)
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


BOOL CPreLightMap::DecompressShadowMap(uint8 *pData)
{
	return ::DecompressShadowMap(m_Data.GetArray(), m_Data.GetSize(), pData);
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
