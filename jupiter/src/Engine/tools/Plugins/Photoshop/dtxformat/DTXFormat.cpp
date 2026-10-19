// ADOBE SYSTEMS INCORPORATED
// Copyright  1993 - 2002 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this 
// file in accordance with the terms of the Adobe license agreement
// accompanying it.  If you have received this file from a source
// other than Adobe, then your use, modification, or distribution
// of it requires the prior written permission of Adobe.
//-------------------------------------------------------------------
//-------------------------------------------------------------------------------
//
//	File:
//		DTXFormat.h
//
//	Description:
//		This file contains the header prototypes and macros for the
//		File Format module DTXFormat, 
//		which writes a flat file with merged document pixels.
//
//	Use:
//		Format modules are called from the Save, Save as,
//		and Save a copy dialogs.
//
//-------------------------------------------------------------------------------

//-------------------------------------------------------------------------------
//	Includes
//-------------------------------------------------------------------------------
#include "PhotoshopSDK.h"
#include "DTXFormat.h"
#include "textureprop.h"

//-------------------------------------------------------------------------------
//	Prototypes
//-------------------------------------------------------------------------------
static void DoReadPrepare (void);
static void DoReadStart (void);
static void DoReadContinue (void);
static void DoReadFinish (void);
static void DoOptionsPrepare (void);
static void DoOptionsStart (void);
static void DoOptionsContinue (void);
static void DoOptionsFinish (void);
static void DoEstimatePrepare (void);
static void DoEstimateStart (void);
static void DoEstimateContinue (void);
static void DoEstimateFinish (void);
static void DoWritePrepare (void);
static void DoWriteStart (void);
static void DoWriteContinue (void);
static void DoWriteFinish (void);
static void DoFilterFile (void);
//static void AddComment (void);


// let's use the TIFF spec. to do cross platform files
const int16 BIGENDIAN = 0x4d4d;
const int16 LITTLEENDIAN = 0x4949;
const int16 TESTENDIAN = 0x002a;

const int32 DESIREDMATTING = 0;

static int CheckIdentifier (char identifier []);
static void SetIdentifier (char identifier []);
static int32 RowBytes (void);

static void ReadSome (int32 count, void * buffer);
static void WriteSome (int32 count, void * buffer);
static void ReadRow (Ptr pixelData, bool needsSwap);
static void WriteRow (Ptr pixelData);
static void DisposeImageResources (void);
static void SwapRow(int32 rowBytes, Ptr pixelData);

static void InitData(void);
static void CreateDataHandle(void);
static void LockHandles(void);
static void UnlockHandles(void);

static VPoint GetFormatImageSize(void);
static void SetFormatImageSize(VPoint inPoint);
static void SetFormatTheRect(VRect inRect);

#if 0
// For debugging purposes
static void DebugMessage(const char* pMsg, ...)
{
	char msg[512] = "";
	va_list marker;

	va_start(marker, pMsg);
	_vsnprintf(msg, 499, pMsg, marker);
	va_end(marker);

	FILE* fp = fopen("c:\\temp\\debug_msg.txt", "a");
	if(fp)
	{
		fprintf(fp, "%s\n", msg);
		fclose(fp);
	}
}
#endif

//stub
short ShowVersionAlert (HWND hDlg,
						short alertID, 
						short stringID,
						Str255 versText1,
						Str255 versText2)
{
	return 0;
}

//-------------------------------------------------------------------------------------------------
// Lithtech functions
//-------------------------------------------------------------------------------------------------
void* dalloc(unsigned int size)
{
	return (char*)malloc((size_t)size);
}

void dfree(void *ptr)
{
	free(ptr);
}

// Hook Stdlith's base allocators.
void* DefStdlithAlloc(unsigned int size)
{
	return malloc(size);
}

void DefStdlithFree(void *ptr)
{
	free(ptr);
}

void dsi_PrintToConsole(char *pMsg, ...)
{
}

void dsi_OnReturnError(int err)
{
}

// Bits-per-pixel identifiers.
enum BPPIdent
{
	BPP_8P=0,		// 8 bit palettized
	BPP_8,			// 8 bit RGB
	BPP_16,
	BPP_32,
	BPP_S3TC_DXT1,
	BPP_S3TC_DXT3,
	BPP_S3TC_DXT5,
	BPP_32P,		//! this was added for true color pallete support
	NUM_BIT_TYPES
};

char *g_szBPPIdent[NUM_BIT_TYPES] =
{
	"BPP_8P",
	"BPP_8",
	"BPP_16",
	"BPP_32",
	"BPP_DXT1",
	"BPP_DXT3",
	"BPP_DXT5",
	"BPP_32P"
};

//-------------------------------------------------------------------------------------------------
// End Lithtech functions
//-------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------
//	Globals -- Define global variables for plug-in scope.
//-------------------------------------------------------------------------------
SPBasicSuite * sSPBasic = NULL;
SPPluginRef gPluginRef = NULL;

FormatRecord * gFormatRecord = NULL;
int32 * gDataHandle = NULL;
Data * gData = NULL;
int16 * gResult = NULL;

#define gCountResources gFormatRecord->resourceProcs->countProc
#define gGetResources   gFormatRecord->resourceProcs->getProc
#define gAddResource	gFormatRecord->resourceProcs->addProc


#define gStuff  gFormatRecord
BufferID		gPixelBuffer = 0;
Ptr				gPixelData = NULL;

static void AllocatePixelBuffer(int nSize)
{
	*gResult = AllocateBuffer(nSize, &gPixelBuffer);
	gPixelData = LockBuffer(gPixelBuffer, FALSE);
}

static void FreePixelBuffer()
{
	if (gPixelBuffer != 0)
	{
		FreeBuffer (gPixelBuffer);

		gPixelBuffer = 0;
		gPixelData = 0;
	}
}

//-------------------------------------------------------------------------------
//
//	PluginMain / main
//
//	All calls to the plug-in module come through this routine.
//	It must be placed first in the resource.  To achieve this,
//	most development systems require this be the first routine
//	in the source.
//
//	The entrypoint will be "pascal void" for Macintosh,
//	"void" for Windows.
//
//	Inputs:
//		const int16 selector						Host provides selector indicating
//													what command to do.
//
//		FormatRecord * formatParamBlock				Host provides pointer to parameter
//													block containing pertinent data
//													and callbacks from the host.
//													See PIFormat.h.
//
//	Outputs:
//		FormatRecord * formatParamBlock				Host provides pointer to parameter
//													block containing pertinent data
//													and callbacks from the host.
//													See PIFormat.h.
//
//		int32 * data								Use this to store a handle to our
//													global parameters structure, which
//													is maintained by the host between
//													calls to the plug-in.
//
//		int16 * result								Return error result or noErr.  Some
//													errors are handled by the host, some
//													are silent, and some you must handle.
//													See PIGeneral.h.
//
//-------------------------------------------------------------------------------

DLLExport MACPASCAL void PluginMain (const int16 selector,
						             void * formatParamBlock,
						             int32 * data,
						             int16 * result)
{
	//---------------------------------------------------------------------------
	//	(1) Update our global parameters from the passed in values.
	// 
	//	We removed that nasty passing around of globals. It's global right! So
	//	why pass it around. This also removes the use of some those nasty #defines.
	//---------------------------------------------------------------------------
	gFormatRecord = reinterpret_cast<FormatRecordPtr>(formatParamBlock);
	gPluginRef = reinterpret_cast<SPPluginRef>(gFormatRecord->plugInRef);
	gResult = result;
	gDataHandle = data;

	//---------------------------------------------------------------------------
	//	(2) Check for about box request.
	//
	// 	The about box is a special request; the parameter block is not filled
	// 	out, none of the callbacks or standard data is available.  Instead,
	// 	the parameter block points to an AboutRecord, which is used
	// 	on Windows.
	//---------------------------------------------------------------------------
	/*
	if (selector == formatSelectorAbout)
	{
		AboutRecordPtr aboutRecord = reinterpret_cast<AboutRecordPtr>(formatParamBlock);
		sSPBasic = aboutRecord->sSPBasic;
		gPluginRef = reinterpret_cast<SPPluginRef>(aboutRecord->plugInRef);
		DoAbout(gPluginRef, AboutID);
	}
	else
		*/
	{ // do the rest of the process as normal:

		sSPBasic = ((FormatRecordPtr)formatParamBlock)->sSPBasic;

		if (gCountResources == NULL ||
            gGetResources == NULL ||
            gAddResource == NULL ||
			gFormatRecord->advanceState == NULL)
		{
			*gResult = errPlugInHostInsufficient;
			return;
		}

		// new for Photoshop 8, big documents, rows and columns are now > 30000 pixels
		if (gFormatRecord->HostSupports32BitCoordinates != NULL)
		{
			gFormatRecord->PluginUsing32BitCoordinates = true;
		}

		//-----------------------------------------------------------------------
		//	(3) Allocate and initalize globals.
		//
		//-----------------------------------------------------------------------

 		if (*gDataHandle == NULL)
		{
			CreateDataHandle();
			if (*gResult != noErr)
			{ 
				return;
			}
			LockHandles();
			if (*gResult != noErr) 
			{
				return;
			}
			InitData();
		}

		if (*gResult == noErr)
		{
			LockHandles();
			if (*gResult != noErr)
			{
				return;
			}
		}

		//-----------------------------------------------------------------------
		//	(4) Dispatch selector.
		//-----------------------------------------------------------------------
		*gResult = noErr;

		switch (selector)
		{
			case formatSelectorReadPrepare:
				DoReadPrepare();
				break;
			case formatSelectorReadStart:
				DoReadStart();
				break;
			case formatSelectorReadContinue:
				DoReadContinue();
				break;
			case formatSelectorReadFinish:
				DoReadFinish();
				break;

			case formatSelectorOptionsPrepare:
				DoOptionsPrepare();
				break;
			case formatSelectorOptionsStart:
				DoOptionsStart();
				break;
			case formatSelectorOptionsContinue:
				DoOptionsContinue();
				break;
			case formatSelectorOptionsFinish:
				DoOptionsFinish();
				break;

			case formatSelectorEstimatePrepare:
				DoEstimatePrepare();
				break;
			case formatSelectorEstimateStart:
				DoEstimateStart();
				break;
			case formatSelectorEstimateContinue:
				DoEstimateContinue();
				break;
			case formatSelectorEstimateFinish:
				DoEstimateFinish();
				break;

			case formatSelectorWritePrepare:
				DoWritePrepare();
				break;
			case formatSelectorWriteStart:
				DoWriteStart();
				break;
			case formatSelectorWriteContinue:
				DoWriteContinue();
				break;
			case formatSelectorWriteFinish:
				DoWriteFinish();
				break;

			case formatSelectorFilterFile:
				DoFilterFile();
				break;
		}
			
		//-----------------------------------------------------------------------
		//	(5) Unlock data, and exit resource.
		//
		//	Result is automatically returned in *result, which is
		//	pointed to by gResult.
		//-----------------------------------------------------------------------	
		
		UnlockHandles();
	
	} // about selector special		

	// release any suites that we may have acquired
	if (selector == formatSelectorAbout ||
		selector == formatSelectorWriteFinish ||
		selector == formatSelectorReadFinish ||
		selector == formatSelectorOptionsFinish ||
		selector == formatSelectorEstimateFinish ||
		selector == formatSelectorFilterFile ||
		*gResult != noErr)
	{
		PIUSuitesRelease();
	}

} // end PluginMain


//-------------------------------------------------------------------------------
//
//	InitData
//	
//	Initalize any global values here.  Called only once when global
//	space is reserved for the first time.
//
//	Outputs:
//		Initializes any global values with their defaults.
//
//-------------------------------------------------------------------------------

static void InitData (void)
{	
	gData->needsSwap = false;
} // end InitData


/*****************************************************************************/

static int32 RowBytes (void)
{
	VPoint imageSize = GetFormatImageSize();
	return (imageSize.h * gFormatRecord->depth + 7) >> 3;
	
}

/*****************************************************************************/

static void DoReadPrepare (void)
{
	gFormatRecord->maxData = 0;
}

/*****************************************************************************/

static void DisposeImageResources (void)
{
	
	if (gFormatRecord->imageRsrcData)
	{
		
		sPSHandle->Dispose(gFormatRecord->imageRsrcData);
		
		gFormatRecord->imageRsrcData = NULL;
		
		gFormatRecord->imageRsrcSize = 0;
		
	}
	
}

static void SetupFormatOptions()
{
	*gResult = noErr;
	TextureProp texture;
	if(!texture.Init(NULL, gFormatRecord->dataFork))
	{
		*gResult = readErr;
		return;
	}
	gFormatRecord->imageMode = plugInModeRGBColor;

	uint16 nWidth	= 0;
	uint16 nHeight	= 0;
	texture.GetDims(nWidth, nHeight);

	VPoint imageSize;
	imageSize.h = nWidth;
	imageSize.v = nHeight;
	SetFormatImageSize(imageSize);

	gFormatRecord->depth = 8;
	gFormatRecord->planes = 4;

	gFormatRecord->loPlane = 0;
	gFormatRecord->hiPlane = 3;
	gFormatRecord->colBytes = gFormatRecord->planes * (gFormatRecord->depth / 8);
	gFormatRecord->rowBytes = 4 * nWidth;
	gFormatRecord->planeBytes = 1;

	gFormatRecord->maxData = gFormatRecord->rowBytes * nHeight;
	*gResult = noErr;
}

/*****************************************************************************/

static void DoReadStart (void)
{
	// If you add fmtCanCreateThumbnail to the FormatFlags PiPL property
	// you will get called to create a thumbnail. The only way to tell
	// that you are processing a thumbnail is to check openForPreview in the
	// FormatRecord. You do not need to parse the entire file. You need to
	// process enough for a thumbnail view and you need to do it quickly.

	SetupFormatOptions();

}

/*****************************************************************************/

static void DoReadContinue (void)
{
	int32 done = 0;
	int32 total = 0;	
	int32 row = 0;

	*gResult = noErr;

	TextureProp texture;
	if(!texture.Init(NULL, gFormatRecord->dataFork))
	{
		*gResult = readErr;
		goto cleanup;
	}

	uint32 bufferSize = gFormatRecord->rowBytes * GetFormatImageSize().v;

	AllocatePixelBuffer(bufferSize);
	gFormatRecord->data = gPixelData;

	total = GetFormatImageSize().v;

	uint8 *pData = new uint8[bufferSize];
	if(!pData)
	{
		*gResult = readErr;
		goto cleanup;
	}

	bool bData = texture.GetTextureRGBAData(pData, bufferSize);
	if(!bData)
	{		
		*gResult = readErr;
		goto cleanup;
	}

	int x = gFormatRecord->rowBytes;
	int y = GetFormatImageSize().v;

	VRect theRect;
	theRect.left  = 0;
	theRect.right = gFormatRecord->rowBytes;

	uint8 *ppPixelData = (uint8*)gPixelData;
	for (int j = 0; (j < y) && (*gResult == noErr); ++j)
	{
		row = j;
		theRect.top = row;
		theRect.bottom = row + 1;
		SetFormatTheRect(theRect);
	
		// One scan line
		for (int k = 0; (k < x); ++k)
		{
			ppPixelData[k] =  pData[k + (x*j)];
		}

		if(*gResult == noErr)
		{
			*gResult = gFormatRecord->advanceState();
		}
		else
		{
			//FAIL!
		}
	
		gFormatRecord->progressProc(++done, total);
	}

	if(*gResult != noErr)
	{
		//FAIL!
	}

cleanup:
	if(pData)
	{
		delete[] pData;
		pData = NULL;
	}

	// Cleanup resources
	gFormatRecord->data = NULL;
	FreePixelBuffer();
	
}

/*****************************************************************************/

static void DoReadFinish (void)
{
}

/*****************************************************************************/

static void DoOptionsPrepare (void)
{
	gFormatRecord->maxData = 0;
}

/*****************************************************************************/

static void DoOptionsStart (void)
{
	gFormatRecord->data = NULL;
}

/*****************************************************************************/

static void DoOptionsContinue (void)
{
}

/*****************************************************************************/

static void DoOptionsFinish (void)
{
}

/*****************************************************************************/

static void DoEstimatePrepare (void)
{
	gFormatRecord->maxData = 0;
}

/*****************************************************************************/

static void DoEstimateStart (void)
{
	

	int32 dataBytes;
	
	VPoint imageSize = GetFormatImageSize();

	dataBytes = sizeof (TextureProp) + gFormatRecord->imageRsrcSize + RowBytes () * gFormatRecord->planes * imageSize.v;
					  
	gFormatRecord->minDataBytes = dataBytes;
	gFormatRecord->maxDataBytes = dataBytes;
	
	gFormatRecord->data = NULL;

}

/*****************************************************************************/

static void DoEstimateContinue (void)
{
}

/*****************************************************************************/

static void DoEstimateFinish (void)
{
}

/*****************************************************************************/

static void DoWritePrepare (void)
{
	gFormatRecord->maxData = 0;	
	DoEstimateStart();
}

/*****************************************************************************/

static void DoWriteStart (void)
{
	int32 done = 0;
	int32 total = 0;	
	int32 row = 0;

	VPoint vPoint = GetFormatImageSize();
	uint32 nWidth	= vPoint.h;
	uint32 nHeight	= vPoint.v;

	gFormatRecord->depth = 8;

	// Don't set this. Photoshop fills this value for us.
	//gFormatRecord->planes = 4;

	switch(gFormatRecord->planes)
	{
	case 3: // RGB
		{
			gFormatRecord->loPlane = 0;
			gFormatRecord->hiPlane = 2;
		}
		break;
	case 4: // RGBA
		{
			gFormatRecord->loPlane = 0;
			gFormatRecord->hiPlane = 3;
		}
		break;
	default:
		{
			// Invalid number of planes/channels
			MessageBox(NULL, "Image format only supports 3 or 4 channels!", "Error", MB_OK);

			// Just return. No need to goto write_cleanup as nothing has been allocated.
			return; 
		}
		break;

	}


	gFormatRecord->colBytes = gFormatRecord->planes * (gFormatRecord->depth / 8);
	gFormatRecord->rowBytes = gFormatRecord->planes * nWidth;
	gFormatRecord->planeBytes = 1;
	gFormatRecord->maxData = gFormatRecord->rowBytes * nHeight;

	uint32 bufferSize = gFormatRecord->rowBytes;

	AllocatePixelBuffer(bufferSize);
	gFormatRecord->data = gPixelData;

	total = GetFormatImageSize().v;
	int x = gFormatRecord->rowBytes;
	int y = GetFormatImageSize().v;

	//TODO: Setup the texture prop
	TextureProp texture;
	bool bRet = texture.New(nWidth, nHeight, 1);

	if(!bRet)
	{
		MessageBox(NULL, "Invalid Image Dimensions", "Error", MB_OK);
		goto cleanup_write;
	}

	//Pop the UI
	if(!DoUI(&texture))
	{
		goto cleanup_write;
	}

	//Resize it!
	texture.Resize(nWidth, nHeight);

	//Write it!
	VRect theRect;
	theRect.left  = 0;
	theRect.right = gFormatRecord->rowBytes;

	uint8 *ppPixelData = (uint8*)gPixelData;	
	
	void* pVoidData = texture.GetDataBuffer();
	if(NULL == pVoidData)
	{
		//ERROR!
		*gResult = writErr;
		goto cleanup_write;
	}
	
	uint32 nDataSize = gFormatRecord->rowBytes * GetFormatImageSize().v;

	uint8 *pData = (uint8*)pVoidData;
	for (int j = 0; (j < y) && (*gResult == noErr); ++j)
	{
		row = j;
		theRect.top = row;
		theRect.bottom = row + 1;
		SetFormatTheRect(theRect);

		//Save one scan line
		if(*gResult == noErr)
		{
			*gResult = gFormatRecord->advanceState();
		}
		else
		{
			goto cleanup_write;
		}

		if(*gResult == noErr)
		{
			// Copy the image bytes based on RGB/RGBA
			switch(gFormatRecord->planes)
			{
			case 3: // RGB (no alpha)
				{
					// We need to insert 0xFF in the alpha channel.
					int nCounter = 0;
					int nTimesInc = 0;
					for (int k = 0; (k < x); ++k)
					{				
						pData[( ((4 * nWidth)*j)+ k + nTimesInc) ] = ppPixelData[k];						

						// After the writing of the 3rd byte, append our 0xFF for the aplha channel.
						if(nCounter == 2)
						{
							++nTimesInc;
							pData[( ((4 * nWidth)*j) + k + nTimesInc) ] = 0xFF;
							nCounter = 0;
						}
						else
						{
							++nCounter;												
						}
					}
				}
				break;
			case 4: // RGBA
				{
					for (int k = 0; (k < x); ++k)
					{
						pData[k + (x*j)] = ppPixelData[k];
					}
				}
				break;
			default:
				break;
			}
		}

		gFormatRecord->progressProc(++done, total);
	}

	// Convert the data
	//TURN RGBA into BGRA		
	for(int j = 0; j < y; ++j)
	{
		for(int k = 0; k < (4 * nWidth); k+=4)
		{	
			uint32 nData;
			uint8 *pnData = (uint8*)&nData;
			memcpy(pnData, &pData[(j*(4 * nWidth))+ (k)], sizeof(uint32));
				
			//TURN RGBA into BGRA
			uint8 nTemp = pnData[0];
			pData[(j*(4 * nWidth))+ (k)] = pnData[2];
			pData[(j*(4 * nWidth))+ (k + 2)] = nTemp;
		}
	}	

	texture.RebuildMipMaps();

	//Convert the data to the proper compression
	bool bRetConv = texture.ConvertToFormat(texture.m_TempBPPIdent);
	if(!bRetConv)
	{
		goto cleanup_write;
	}

	bool bRes = texture.Save(NULL, gFormatRecord->dataFork);
	if(!bRes)
	{

	}

cleanup_write:

	FreePixelBuffer();
	gFormatRecord->data = NULL;

	*gResult = noErr;
}

/*****************************************************************************/

static void DoWriteContinue (void)
{
}

/*****************************************************************************/

static void DoWriteFinish (void)
{
}

/*****************************************************************************/

static void DoFilterFile (void)
{
	*gResult = noErr;
}

static VPoint GetFormatImageSize(void)
{
	VPoint returnPoint = { 0, 0};
	if (gFormatRecord->HostSupports32BitCoordinates && gFormatRecord->PluginUsing32BitCoordinates)
	{
		returnPoint.v = gFormatRecord->imageSize32.v;
		returnPoint.h = gFormatRecord->imageSize32.h;
	}
	else
	{
		returnPoint.v = gFormatRecord->imageSize.v;
		returnPoint.h = gFormatRecord->imageSize.h;
	}
	return returnPoint;
}

static void SetFormatImageSize(VPoint inPoint)
{
	if (gFormatRecord->HostSupports32BitCoordinates && 
		gFormatRecord->PluginUsing32BitCoordinates)
	{
		gFormatRecord->imageSize32.v = inPoint.v;
		gFormatRecord->imageSize32.h = inPoint.h;
		gFormatRecord->imageSize.v = static_cast<int16>(inPoint.v);
		gFormatRecord->imageSize.h = static_cast<int16>(inPoint.h);
	}
	else
	{
		gFormatRecord->imageSize.v = static_cast<int16>(inPoint.v);
		gFormatRecord->imageSize.h = static_cast<int16>(inPoint.h);
	}
}
static void SetFormatTheRect(VRect inRect)
{
	if (gFormatRecord->HostSupports32BitCoordinates && 
		gFormatRecord->PluginUsing32BitCoordinates)
	{
		gFormatRecord->theRect32.top = inRect.top;
		gFormatRecord->theRect32.left = inRect.left;
		gFormatRecord->theRect32.bottom = inRect.bottom;
		gFormatRecord->theRect32.right = inRect.right;
	}
	else
	{
		gFormatRecord->theRect.top = static_cast<int16>(inRect.top);
		gFormatRecord->theRect.left = static_cast<int16>(inRect.left);
		gFormatRecord->theRect.bottom = static_cast<int16>(inRect.bottom);
		gFormatRecord->theRect.right = static_cast<int16>(inRect.right);
	}
}

//-------------------------------------------------------------------------------
//
// CreateDataHandle
//
// Create a handle to our Data structure. Photoshop will take ownership of this
// handle and delete it when necessary.
//-------------------------------------------------------------------------------
static void CreateDataHandle(void)
{
	Handle h = sPSHandle->New(sizeof(Data));
	if (h != NULL)
		*gDataHandle = reinterpret_cast<int32>(h);
	else
		*gResult = memFullErr;
}

//-------------------------------------------------------------------------------
//
// LockHandles
//
// Lock the handles and get the pointers for gData and gParams
// Set the global error, *gResult, if there is trouble
//
//-------------------------------------------------------------------------------
static void LockHandles(void)
{
	if (*gDataHandle == NULL)
	{
		*gResult = formatBadParameters;
		return;
	}
	
	sPSHandle->SetLock(reinterpret_cast<Handle>(*gDataHandle), true, 
		               reinterpret_cast<Ptr *>(&gData), NULL);
	
	if (gData == NULL)
	{
		*gResult = memFullErr;
		return;
	}
}

//-------------------------------------------------------------------------------
//
// UnlockHandles
//
// Unlock the handles used by the data and params pointers
//
//-------------------------------------------------------------------------------
static void UnlockHandles(void)
{
	if (*gDataHandle != NULL)
	{
		sPSHandle->SetLock(reinterpret_cast<Handle>(*gDataHandle), false, reinterpret_cast<Ptr *>(&gData), FALSE);
	}
}
