// I know, this looks bad but there's a need to share some of these functions
// with dtxutil and it's a command line utility... getting that to compile was
// already a spoofing effort (ask terry what spoofing means).  Therefore I 
// didn't want TextureHelper.cpp to include any include files and the file that 
// includes TextureHelper.cpp should take care of that.  joseph@lith.com

#ifndef _TEXTURE_HELPER_CPP
#define _TEXTURE_HELPER_CPP

// talon: 2001's has the TGA and PCX savers, in this order (WriteTga
// 0x461bea, SavePcxAsTexture 0x461d74, SaveDtxAsTga 0x461e62); the 8-bit
// writer follows the texture code in Texture.cpp (0x4624a7), and Build 69's
// bump, normal and cube map savers aren't there.

// =====================================================================
BOOL WriteTga(CAbstractIO &file, TextureData *pData)
{
	CMoArray<DWORD> outputBuf;
	TextureMipData *pMip;
	DRESULT dResult;
	ConvertRequest cRequest;
	TGAHeader hdr;
	DWORD y, *pOutLine;
	BOOL retVal = FALSE;


	
	//! we don't support this yet
	if( pData->m_Header.GetBPPIdent() == BPP_32P )
	{
		retVal = FALSE;
	}
	else
	{
		pMip = &pData->m_Mips[0];

		if(!outputBuf.SetSize(pMip->m_Width * pMip->m_Height))
		{
			retVal = FALSE;
		}
		else
		{
			cRequest.m_pSrc = (BYTE*)pMip->m_Data;
			dtx_SetupDTXFormat(pData, cRequest.m_pSrcFormat);
			cRequest.m_SrcPitch = pMip->m_Pitch;


			cRequest.m_pDestFormat->Init(BPP_32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
			cRequest.m_pDest = (BYTE*)outputBuf.GetArray();
			cRequest.m_DestPitch = pMip->m_Width * sizeof(DWORD);
			cRequest.m_Width = pMip->m_Width;
			cRequest.m_Height = pMip->m_Height;
			cRequest.m_Flags = 0;
			dResult = g_FormatMgr.ConvertPixels(&cRequest);

			// Write it all out.
			memset(&hdr, 0, sizeof(hdr));
			hdr.m_Width = (WORD)pMip->m_Width;
			hdr.m_Height = (WORD)pMip->m_Height;
			hdr.m_PixelDepth = 32;
			hdr.m_ImageType = 2;
			file.Write(&hdr, sizeof(hdr));

			for(y=0; y < pMip->m_Height; y++)
			{
				pOutLine = &outputBuf[(pMip->m_Height-y-1) * pMip->m_Width];
				file.Write(pOutLine, sizeof(DWORD)*pMip->m_Width);
			}
			retVal = TRUE;
		}
	}
	return retVal;
}

// =====================================================================
BOOL SavePcxAsTexture(LoadedBitmap *pPcx, DStream *pStream, DWORD textureFlags)
{
	//DWORD iCurMipMap;
	TextureData *pData;
	//int bFullbrite;
	DRESULT dResult;
	FMConvertRequest cRequest;


	pData = dtx_Alloc(BPP_32, pPcx->m_Width, pPcx->m_Height, 4, NULL, NULL);
	if(!pData)
		return FALSE;

	pData->m_Header.m_UserFlags = textureFlags;


	// Convert the data.
	cRequest.m_pSrc = pPcx->m_Data.GetArray();
	cRequest.m_pSrcPalette = pPcx->m_Palette;
	*cRequest.m_pSrcFormat = pPcx->m_Format;
	cRequest.m_SrcPitch = pPcx->m_Pitch;

	cRequest.m_pDest = pData->m_Mips[0].m_Data;
	cRequest.m_pDestFormat->InitPValueFormat();
	cRequest.m_DestPitch = pData->m_Mips[0].m_Pitch;
	
	cRequest.m_Width = pPcx->m_Width;
	cRequest.m_Height = pPcx->m_Height;
	cRequest.m_Flags = 0;

	dResult = g_FormatMgr.ConvertPixels(&cRequest);
	if(dResult != LT_OK)
	{
		dtx_Destroy(pData);
		return FALSE;
	}

	// Generate mipmaps.
	dtx_BuildMipmaps(pData);

	// Save.
	dResult = dtx_Save(pData, pStream);
	dtx_Destroy(pData);

	return dResult == DE_OK;
}

// =====================================================================
BOOL SaveDtxAsTga(DStream *pDtxFile, CAbstractIO &outFile)
{
	TextureData *pData;
	BOOL bRet;
	

	if(dtx_Create(pDtxFile, &pData, FALSE) != DE_OK)
		return FALSE;

	bRet = WriteTga(outFile, pData);
	dtx_Destroy(pData);
	return bRet;
}

#endif
