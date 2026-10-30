#include "bdefs.h"
#include "lightmap_compress.h"


// talon: NOLF's (0x422040..0x422270): 16-bit pixels, the run flag the high
// bit of the pixel. AvP2's (written from its image) is the same scheme on
// 32-bit pixels.
LTBOOL CompressLMData(uint16 *pData, uint32 width, uint32 height,
	uint16 *pOut, uint32 &outLen)
{
	uint32 i, iRun, nPixels, runLen;
	uint16 color;
	uint8 *pOutPos;


	// Make sure we'll have room.
	if(width > LIGHTMAP_MAX_PIXELS || height > LIGHTMAP_MAX_PIXELS)
		return LTFALSE;

	nPixels = width * height;
	pOutPos = (uint8*)pOut;
	for(i=0; i < nPixels; i++)
	{
		color = pData[i];

		// How long a run starts here?
		for(iRun=i+1; iRun < nPixels; iRun++)
		{
			if(pData[iRun] != color)
				break;
		}

		runLen = iRun - i;
		color &= 0x7FFF;
		if(runLen >= 2)
		{
			if(runLen >= 256)
				runLen = 255;

			*((uint16*)pOutPos) = color | 0x8000;
			pOutPos[2] = (uint8)runLen;
			pOutPos += 3;
			i += runLen - 1;
		}
		else
		{
			*((uint16*)pOutPos) = color;
			pOutPos += 2;
		}
	}

	outLen = pOutPos - (uint8*)pOut;
	return LTTRUE;
}


LTBOOL DecompressLMData(uint16 *pCompressed, uint32 dataLen, uint16 *pOut)
{
	uint8 *pCur, *pEnd;
	uint16 color;
	uint8 runLen;
	int nLeft;


	nLeft = LIGHTMAP_MAX_TOTAL_PIXELS;
	pCur = (uint8*)pCompressed;
	pEnd = pCur + dataLen;
	while(pCur < pEnd)
	{
		color = *((uint16*)pCur);
		pCur += 2;

		if(color & 0x8000)
		{
			runLen = *pCur;
			color &= 0x7FFF;
			pCur++;
		}
		else
		{
			runLen = 1;
		}

		nLeft -= runLen;
		if(nLeft < 0)
			return LTFALSE;

		while(runLen)
		{
			runLen--;
			*pOut = color;
			pOut++;
		}
	}

	return LTTRUE;
}


LTBOOL CompressShadowMap(uint16 *pData, uint32 width, uint32 height,
	uint8 *pOut, uint32 &outLen)
{
	uint32 runLength, iPixel, nPixels;
	LTBOOL bOn;
	uint8 *pOutPos;


	// Make sure we'll have room.
	if(width > LIGHTMAP_MAX_PIXELS || height > LIGHTMAP_MAX_PIXELS)
		return LTFALSE;

	pOutPos = pOut;
	nPixels = width * height;
	bOn = LTFALSE;
	runLength = 0;

	for(iPixel=0; iPixel < nPixels; iPixel++)
	{
		if((pData[iPixel] != 0) == bOn)
		{
			++runLength;
			if(runLength == 255)
			{
				*pOutPos = (uint8)runLength;
				pOutPos++;

				bOn = !bOn;
				runLength = 0;
			}
		}
		else
		{
			*pOutPos = (uint8)runLength;
			pOutPos++;
			bOn = !bOn;
			runLength = 1;
		}
	}

	if(runLength)
	{
		*pOutPos = (uint8)runLength;
		pOutPos++;
	}

	outLen = pOutPos - pOut;
	return LTTRUE;
}


LTBOOL DecompressShadowMap(uint8 *pCompressed, uint32 dataLen, uint8 *pOutData)
{
	uint32 runLength;
	uint8 *pOut, *pEndOut;
	LTBOOL bOn;
	uint8 theValue;
	uint8 values[2];


	values[0] = 0;
	values[1] = 0xFF;

	pOut = pOutData;
	pEndOut = pOut + LIGHTMAP_MAX_TOTAL_PIXELS;
	bOn = LTFALSE;
	while(dataLen)
	{
		dataLen--;
		runLength = *pCompressed;
		++pCompressed;

		if((pOut+runLength) > pEndOut)
			return LTFALSE;

		theValue = values[bOn];
		while(runLength)
		{
			runLength--;
			*pOut = theValue;
			pOut++;
		}

		bOn = !bOn;
	}

	return LTTRUE;
}
