#include "bdefs.h"
#include "lightmap_compress.h"


LTBOOL CompressLMData(uint32 *pData, uint32 width, uint32 height,
	uint32 *pOut, uint32 &outLen)
{
	uint32 i, iRun, nPixels, runLen, color;
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
		color &= 0x7FFFFFFF;
		if(runLen >= 2)
		{
			if(runLen >= 256)
				runLen = 255;

			i += runLen - 1;
			*((uint32*)pOutPos) = color | 0x80000000;
			pOutPos += 4;
			*pOutPos = (uint8)runLen;
			pOutPos++;
		}
		else
		{
			*((uint32*)pOutPos) = color;
			pOutPos += 4;
		}
	}

	outLen = pOutPos - (uint8*)pOut;
	return LTTRUE;
}


LTBOOL DecompressLMData(uint32 *pCompressed, uint32 dataLen, uint32 *pOut)
{
	uint8 *pCur, *pEnd;
	uint32 color, i;
	uint8 runLen;
	int nLeft;


	if(!pCompressed)
		return LTFALSE;

	nLeft = LIGHTMAP_MAX_TOTAL_PIXELS;
	pCur = (uint8*)pCompressed;
	pEnd = pCur + dataLen;
	while(pCur < pEnd)
	{
		color = *((uint32*)pCur);
		pCur += 4;

		if(color & 0x80000000)
		{
			runLen = *pCur;
			color &= 0x7FFFFFFF;
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


LTBOOL CompressShadowMap(uint32 *pData, uint32 width, uint32 height,
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


	if(!pCompressed)
		return LTFALSE;

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
