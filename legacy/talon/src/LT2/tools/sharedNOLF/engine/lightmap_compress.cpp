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


// talon: near, 111/111 bytes: the shipped run branch masks the colour
// into the register it keeps (mov %edx,%ebx; and $0x7fff,%ebx, 0x422131),
// ours masks and then copies. The four orders of the run branch's three
// statements, `*pCur++` and `color = color & 0x7FFF` give ours, as do
// /Os, /Ob0 and /G5 for the file (/O2 matches nothing).
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


// talon: near, 95/94 bytes: the shipped bounds test adds with mov+add
// (0x422236); ours is a lea. runLength as int, uint8 or unsigned long, the
// sum either way round, `&pOut[runLength]`, the test negated or turned
// around, and pointer or integer compares all give the lea.
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
