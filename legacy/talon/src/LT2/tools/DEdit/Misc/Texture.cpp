//------------------------------------------------------------------
//
//	FILE	  : Texture.cpp
//
//	PURPOSE	  : Implements the CTexture class and its support stuff.
//
//	CREATED	  : October 15 1996
//
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------

// Includes....
#include "bdefs.h"
#include "texture.h"
#include "dedit.h"
#include "editprojectmgr.h"
#include "d_filemgr.h"
#include "dtxmgr.h"
#include "sysstreamsim.h"
#include "pixelformat.h"
#include "LTTexture.h"

extern SDWORD g_MipMapOffset;	// talon: as dedit_concommand.cpp defines it
extern void d3d_UnbindFromTexture(void *pBinding);


extern SDWORD g_CV_MaxTextureMemory;
DECLARE_DLINK(g_Textures);
DWORD g_TextureMemory = 0;

FormatMgr g_FormatMgr;



// Frees textures from the head of g_Textures until g_TextureMemory < wantedVal.
void dib_FreeTextures(DWORD wantedVal)
{
	CTexture *pTexture;

	while(g_TextureMemory > wantedVal)
	{
		if(g_Textures.m_pNext == &g_Textures)
		{
			AddDebugMessage("Internal error in texture list!");
			return;
		}

		pTexture = (CTexture*)g_Textures.m_pNext->m_pData;
		dib_DestroyDibTexture(pTexture);
	}
}


void dib_FreeTextureMemory()
{
	dib_FreeTextures(g_CV_MaxTextureMemory);
}


void dib_FreeAllTextures()
{
	dib_FreeTextures(0);
}

CTexture* dib_LoadMipmap(struct DFileIdent_t *pIdent, int iMipmap)
{
	CTexture *pTexture;
	CDib *pDib;
	CMoByteArray buf;
	CDibMgr *pDibMgr;
	CMoFileIO file;
	
 	DWORD i, size;
	WORD nMipMaps;
	int width, height;
 	DWORD curOffset;
	
	TextureData *pData;
	TextureMipData *pMip;

	CString fullName;
	DStream *pStream;
	DRESULT dResult;
	FMConvertRequest cRequest;



	pDibMgr = GetProject()->GetDibMgr();
	pTexture = NULL;
	pDib = NULL;

	fullName = dfm_GetFullFilename(GetFileMgr(), pIdent->m_Filename);
	pStream = streamsim_Open((LPCTSTR)fullName, "rb");
	if(!pStream)
	{
		AddDebugMessage("Unable to open texture file: %s.", fullName);
		return NULL;
	}

	dResult = dtx_Create(pStream, &pData, TRUE);  //! we now need to load the sections

	pStream->Release();

	if(dResult != DE_OK)
	{
		AddDebugMessage("dtx_Create() for texture %s failed.", fullName);
		return NULL;
	}

	// Make sure the mipmap is valid.
	if(iMipmap >= pData->m_Header.m_nMipmaps || iMipmap < 0)
		return NULL;


	pMip = &pData->m_Mips[iMipmap];
	width = (int)pMip->m_Width;
	height = (int)pMip->m_Height;
	pDib = pDibMgr->AddDib(width, -height, 16);
	if(pDib)
	{
		pTexture = new CTexture;
		pTexture->m_pDibMgr = pDibMgr;
		pTexture->m_pDib = pDib;
		memcpy(&pTexture->m_Header, &pData->m_Header, sizeof(DtxHeader));		

		// Setup the conversion request.
		cRequest.m_pDest = (BYTE*)pDib->GetBuf16();
		cRequest.m_pDestFormat->Init(BPP_16, 0, 0x7C00, 0x3E0, 0x1F);
		cRequest.m_DestPitch = pDib->GetPitch() * 2;
		
		cRequest.m_pSrc = pMip->m_Data;
		dtx_SetupDTXFormat(pData, cRequest.m_pSrcFormat);
		cRequest.m_SrcPitch = pMip->m_Pitch; 

		cRequest.m_Width = pMip->m_Width;
		cRequest.m_Height = pMip->m_Height;
		cRequest.m_Flags = 0;

		//! we need to set the pallete for BPP_32P
		if( pData->m_Header.GetBPPIdent() == BPP_32P )
		{
			// find the pallete section
			DtxSection* pSection = dtx_FindSection(pData, "PALLETE32");
			if( pSection )
			{
				cRequest.m_pSrcPalette = (RPaletteColor*)pSection->m_Data;  
			}
		}

		// Convert the data.
		dResult = g_FormatMgr.ConvertPixels(&cRequest);
							

		pTexture->m_Bindings = NULL;
 		pTexture->m_nBindings = 0;


		// Free up older textures	
		pTexture->m_MemoryUse = width * height * 2;
		g_TextureMemory += pTexture->m_MemoryUse;
		dib_FreeTextureMemory();

		pTexture->m_UIMipmapOffset = pData->m_Header.GetUIMipmapOffset();
		pTexture->m_UIMipmapScale = pData->m_Header.GetUIMipmapScale();

		// Put it at the end of the textures list.
		pTexture->m_Link.m_pData = pTexture;
		dl_Insert(g_Textures.m_pPrev, &pTexture->m_Link);

	}
	else
	{
		AddDebugMessage("CDibMgr::AddDib() for texture %s failed.", fullName);
	}

	dtx_Destroy(pData);

	return pTexture;
}


CTexture* dib_GetDibTexture(struct DFileIdent_t *pIdent)
{
	CTexture *pTexture;
	
	// Quick check to see if it's already loaded.
	// This goes in front of all the stack variables because the stack
	// variables have constructors which will slow this (time-critical) test down..
	if(!pIdent)
		return NULL;

	if(pIdent->m_pUser)
	{
		if(pIdent->m_UserType == 0)
			return (CTexture*)pIdent->m_pUser;
		else
			return NULL;
	}

	pTexture = dib_LoadMipmap(pIdent, g_MipMapOffset);
	if(pTexture)
	{
		// Bind the texture to the file.
		pIdent->m_pUser = pTexture;
		pIdent->m_UserType = 0;
		pTexture->m_pIdent = pIdent;
		return pTexture;
	}
	else
	{
		return NULL;
	}
}


void dib_DestroyDibTexture(CTexture *pTexture)
{
 	WORD i;

	// Remove it from the global list.
	ASSERT(g_TextureMemory >= pTexture->m_MemoryUse);
	g_TextureMemory -= pTexture->m_MemoryUse;
	dl_Remove(&pTexture->m_Link);

 	// Unbind all the bindings.
 	for(i=0; i < pTexture->m_nBindings; i++)
 	{
 		if(pTexture->m_Bindings[i])
 			d3d_UnbindFromTexture(pTexture->m_Bindings[i]);
 	}
 
 	if(pTexture->m_Bindings)
 	{		
 		dfree(pTexture->m_Bindings);
 	}


	if(pTexture->m_pIdent)
	{
		pTexture->m_pIdent->m_pUser = NULL;
	}

	GetProject()->GetDibMgr()->RemoveDib( pTexture->m_pDib );
	delete pTexture;
}



// I know, this looks bad but there's a need to share some of these functions
// with dtxutil and it's a command line utility... getting that to compile was
// already a spoofing effort (ask terry what spoofing means).  Therefore I 
// didn't want TextureHelper.cpp to include any include files and the file that 
// includes TextureHelper.cpp should take care of that.  joseph@lith.com

#include "TextureHelper.cpp"


// talon: 2001's (0x461e9e) prints the bump plane too, and Write8BitDTX
// calls it for every mipmap (0x462680).
void PrintTextureMipDataInfo(TextureMipData* data)
{
	char buffer[1024];
	OutputDebugString("\nTextureMipData {");
	sprintf(buffer,"\n\t m_Width %d m_Height %d", data->m_Width, data->m_Height );	OutputDebugString(buffer);
	sprintf(buffer,"\n\t m_Data %x", data->m_Data );								OutputDebugString(buffer);
	sprintf(buffer,"\n\t m_Pitch %d", data->m_Pitch );								OutputDebugString(buffer);
	sprintf(buffer,"\n\t m_BumpData %d", data->m_BumpData );						OutputDebugString(buffer);
	sprintf(buffer,"\n\t m_BumpPitch %d", data->m_BumpPitch );					OutputDebugString(buffer);

	sprintf(buffer,"\n\t color_data: \n" );	OutputDebugString(buffer);
	for( uint32 i = 0; i < data->m_Width * data->m_Height; i++ )
	{
		if( (i % data->m_Width) == 0 )
			OutputDebugString("\n");
		sprintf(buffer,"%d ", data->m_Data[i] );	OutputDebugString(buffer);
	}
	OutputDebugString("\n}");
}


// talon: LTTexture's code (shared/world/LTTexture.cpp in Build 69) is
// here in 2001 (0x461fd5..0x4624a7), between the dump and the 8-bit writer.
// =======================================================
LTTextureHeader::LTTextureHeader(uint16 type, 
								 uint16 nummipmaps, 
								 uint16 width, 
								 uint16 height,
								 uint32 index)
{
	switch(type)
	{
	case TT_32BitTrueColor:
	case TT_8BitPalletized:
		m_type = type;
		break;
	default:
		SetToInvalid();
	}
	m_index = index;
	//! there should be a warning here
	m_nummipmaps = nummipmaps <= TT_MAX_NUMMIPMAPS ? nummipmaps : 4;  
	//! need to check for valid sizes 
	m_width = width;
	m_height = height;
}
// =======================================================
int LTTex32UsagePairCompare( const void *arg1, const void *arg2 )
{
	/* Compare all of both strings: */
	if( ((LTTex32UsagePair*)arg1)->m_num < ((LTTex32UsagePair*)arg2)->m_num )
		return 1;
	else if( ((LTTex32UsagePair*)arg1)->m_num > ((LTTex32UsagePair*)arg2)->m_num )
		return -1;
	else
		return 0;
}
// =======================================================
LTTexture::LTTexture()
{
	m_data = NULL;
}
// =======================================================
LTTexture::LTTexture(LTTextureHeader& header, uint8* pData, uint8* pPallete  )
{
	m_data = NULL;
	Init(header, pData, pPallete );
}
// =======================================================
LTTexture::~LTTexture()
{
	if(m_data)
		free(m_data);
	m_data = NULL;
}
// =======================================================
void LTTexture::Init(LTTextureHeader& header, uint8* pData, uint8* pPallete  )
{
	if(m_data)
	{
		free(m_data);
		m_data = NULL;
	}
	if( header.IsValid() && header.GetType() == LTTextureHeader::TT_8BitPalletized )
	{
		m_header = header;
		int texDataSize = m_header.GetWidth()*m_header.GetHeight();
		m_data = (uint8*) malloc(	sizeof(uint8)*256*4 +	// for the pallete
									sizeof(uint8)*texDataSize);
		if( m_data )
		{
			memcpy(m_data,pPallete,sizeof(uint8)*256*4);	// copy the pallete
			memcpy(m_data+sizeof(uint8)*256*4,pData,sizeof(uint8)*texDataSize);
		}
		else
		{
			m_header.SetToInvalid();
		}
	}
	else if( header.IsValid() && header.GetType() == LTTextureHeader::TT_32BitTrueColor )
	{	
		m_header = header;
		int texDataSize = m_header.GetWidth()*m_header.GetHeight()*4;
		m_data = (uint8*) malloc(sizeof(uint8)*texDataSize);
		if( m_data )
		{
			memcpy(m_data,pData,sizeof(uint8)*texDataSize);
		}
		else
		{
			m_header.SetToInvalid();
		}
	}
}
// =======================================================
uint8 LTTexture::NearestColor(LTTextureColor32* pColor, LTTex32UsagePair* pPallete )
{
	
	// find nearest matching color
	int index		= 0;
	int dist		= 255*3;
	int bestColor	= 0;
	int curDist     = 0;
	for( index = 0; index < 256; index++ )
	{
		
		curDist =	abs(pColor->r - pPallete[index].m_color.r) +
					abs(pColor->g - pPallete[index].m_color.g) +
					abs(pColor->b - pPallete[index].m_color.b);
		if( 0 == curDist )
		{
			bestColor = index;
			break;
		}
		if( curDist < dist )
			bestColor = index;
	}
	return bestColor;
	
	/*
	int  index		= 0;
	bool found		= false;
	for( index = 0; index < 256; index++ )
	{
		
		if( pColor->r == pPallete[index].m_color.r &&
			pColor->g == pPallete[index].m_color.g &&
			pColor->b == pPallete[index].m_color.b &&
			pColor->a == pPallete[index].m_color.a )
		{
			found = true;
			break;
		}
	}

	
	if( !found )
	{
		//printf("\nx (%d %d %d %d)", pColor->r, pColor->g, pColor->b, pColor->a );
	}
	
	return index;
	*/
}
// =======================================================
// creating the pallete (algo1)
void LTTexture::Convert32BitTo8BitPalletized(LTTexture& destTexture)
{
	// create an array of [color:numUsed] pairs
	// sort the array
	// pick the top 256 colors for the pallete...
	// then do nearest match

	uint32				texDataSize		= m_header.GetWidth() * m_header.GetHeight();
	LTTex32UsagePair*	pUsagePair		= (LTTex32UsagePair*) malloc(sizeof(LTTex32UsagePair)* texDataSize );
	int					curColorsFound	= 0;

	if( pUsagePair )
	{
		// clear the memory
		memset(pUsagePair,0, sizeof(LTTex32UsagePair)*texDataSize);

		// talon: 2001's (0x462220..0x46228c) matches and keeps colours by
		// r, g and b alone.
		// create an array of [color:numUsed] pairs
		for(unsigned int i = 0; i < (texDataSize); i++ )
		{
			int					index		= i * 4;  // for mapping 1-4, 2->8. etc since we're dealing with rgba
			LTTextureColor32*	curColor	= (LTTextureColor32*)&(m_data[index]);
			int					texColorIndex = 0;
			// let's see if this color was already found
			for( texColorIndex = 0; texColorIndex < curColorsFound; texColorIndex++ )
			{
				if( pUsagePair[texColorIndex].m_color.r == curColor->r &&
					pUsagePair[texColorIndex].m_color.g == curColor->g &&
					pUsagePair[texColorIndex].m_color.b == curColor->b )
				{
					pUsagePair[texColorIndex].m_num += 1;
					break;
				}
			}
			if( texColorIndex == curColorsFound ) // we did not find the color 
			{
				// let's add it
				pUsagePair[curColorsFound].m_color.r = curColor->r;
				pUsagePair[curColorsFound].m_color.g = curColor->g;
				pUsagePair[curColorsFound].m_color.b = curColor->b;
				curColorsFound++;
			}
		}

		// printf("\ncolorsFound: %d", curColorsFound );
		// sort the array, pick the top 256 colors for the pallete...
		qsort( (void *)pUsagePair, (size_t)curColorsFound, sizeof( LTTex32UsagePair ), LTTex32UsagePairCompare );

		//! research a better palletization algorithm
		//  LTTex32UsagePairQuantize( pUsagePair, curColorsFound );		

		// now let's create a width x height indexed texture
		uint8* pIndexedTexture = (uint8*) malloc(sizeof(uint8)*texDataSize);
		// let's fill that in with the nearest texture color match...
		if( pIndexedTexture )
		{
			for(unsigned int i = 0; i < (texDataSize); i++ )
			{
				int					index		= i * 4;  // for mapping 1-4, 2->8. etc since we're dealing with rgba
				LTTextureColor32*	curColor	= (LTTextureColor32*)&(m_data[index]);
				pIndexedTexture[i] = NearestColor(curColor, pUsagePair );
			}

			// put the data into destTexture 
			uint8* pPallete = (uint8*) malloc(sizeof(uint8) * 256 * 4);
			if(pPallete)
			{
				for(int palleteIndex = 0; palleteIndex < 256; palleteIndex++ )
				{
					/*
					pPallete[palleteIndex*4+0] = pUsagePair[palleteIndex].m_color.a;
					pPallete[palleteIndex*4+1] = pUsagePair[palleteIndex].m_color.b;
					pPallete[palleteIndex*4+2] = pUsagePair[palleteIndex].m_color.g;
					pPallete[palleteIndex*4+3] = pUsagePair[palleteIndex].m_color.r;
					*/
					pPallete[palleteIndex*4+0] = pUsagePair[palleteIndex].m_color.r;
					pPallete[palleteIndex*4+1] = pUsagePair[palleteIndex].m_color.g;
					pPallete[palleteIndex*4+2] = pUsagePair[palleteIndex].m_color.b;
					pPallete[palleteIndex*4+3] = 128;
				}

				LTTextureHeader texHeader(	LTTextureHeader::TT_8BitPalletized, 
											1,				// num mipmaps
											m_header.GetWidth(),	
											m_header.GetHeight(), 
											0 );		

				destTexture.Init(texHeader, pIndexedTexture, pPallete);

				free(pPallete);
				pPallete = NULL;
			}
			else
			{
				destTexture.m_header.SetToInvalid();
			}
			free(pIndexedTexture);
			pIndexedTexture = NULL;	
		}
		else
		{
			destTexture.m_header.SetToInvalid();
		}
		free(pUsagePair);
		pUsagePair = NULL;
	}
	else
	{
		destTexture.m_header.SetToInvalid();
	}
}
// =======================================================
void LTTexture::Create8BitPalletized(LTTexture& destTexture)
{
	if( m_header.IsValid() )
	{
		switch(m_header.GetType())
		{
		case LTTextureHeader::TT_32BitTrueColor:
			Convert32BitTo8BitPalletized(destTexture);
			break;
		default:
			break;
		}
	}
}
// =======================================================
void LTTexture::CreateMipMaps(uint32 numMipMaps)
{
	if( m_header.IsValid() )
	{
		switch(m_header.GetType())
		{
		case LTTextureHeader::TT_8BitPalletized:
			{
				numMipMaps = numMipMaps > 4 ? 4 : numMipMaps;
				numMipMaps = numMipMaps > 0 ? numMipMaps : 4;
				if( m_header.GetNumMipMaps() == 1 )
				{
					uint32 texDataSize = 0;
					uint32 w = m_header.GetWidth();
					uint32 h = m_header.GetHeight();
		
					unsigned int i = 0;
					for( i = 0; i < numMipMaps; i++)
					{
						texDataSize += w*h;
						w = w / 2;
						h = h / 2;
					}

					m_data = (uint8*) realloc(m_data,sizeof(uint8)*256*4 +	// for the pallete
													 sizeof(uint8)*texDataSize);
					
					w = m_header.GetWidth();
					h = m_header.GetHeight();

					uint32 sourceMipDataOffset	= (256*4);
					uint32 mipDataOffset		= sourceMipDataOffset+(w*h);

					for( i = 1; i < numMipMaps; i++)
					{	
						w = w / 2;
						h = h / 2;
						uint8* pSrcLine  = &(m_data[sourceMipDataOffset]);
						uint8* pDestLine = &(m_data[mipDataOffset]);
						for( unsigned int destH = 0; destH < h; destH++ )
						{
							for( unsigned int destW = 0; destW < w; destW++ )
							{
								pDestLine[destW] = pSrcLine[destW*2*i];
							}
							pSrcLine  += m_header.GetWidth()*2*i;
							pDestLine += w;
						}
						mipDataOffset = mipDataOffset+(w*h);
					}

					m_header.SetNumMipMaps((uint16)numMipMaps);
				}
			}
			break;
		default:
			break;
		}
	}	
}


// =======================================================
// talon: 2001's (0x4624a7) has no retainHeader: nothing of the existing
// file is kept, the pixel data is dumped (PrintTextureMipDataInfo) as it's
// copied, and the section and texture data are free()d (0x46272a, 0x462731).
BOOL Write8BitDTX(CAbstractIO &file, TextureData *pData, CString* pFilename)
{
	BOOL retVal = FALSE;
	
	//! we don't support this yet
	if( pData->m_Header.GetBPPIdent() == BPP_32P )
	{
		retVal = FALSE;
	}
	else
	{
		// this is taken from WriteAsTGA it handles the conversion of any texture type
		// into a 32bit texture, through this way we only have to write the code to 
		// convert BPP_32 -> BPP_32P to support palletized textures

		CMoArray<DWORD> outputBuf;
		TextureMipData *pMip;
		DRESULT dResult;
		ConvertRequest cRequest;

		pMip = &pData->m_Mips[0];
		if(!outputBuf.SetSize(pMip->m_Width * pMip->m_Height))
			return FALSE;
		
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

		if( LT_OK == dResult )
		{
			LTTextureHeader texHeader(	LTTextureHeader::TT_32BitTrueColor, 
										1,				// num mipmaps
										pMip->m_Width,	
										pMip->m_Height, 
										0 );			// everything right now is going to be 
														// indexed at zero, later on, each unique
														// texture will have it's own unique index
			LTTexture texture(texHeader, (uint8*)outputBuf.GetArray() );
			// convert into 8 bit texture		
			LTTexture palletizedTexture;
			texture.Create8BitPalletized(palletizedTexture);
			// Create mipmaps, there's something wrong with the 4th level of the generated
			//                 mipmap, I'll look into this as soon as possible.
			palletizedTexture.CreateMipMaps(3);
			
			uint32 allocSize, textureDataSize;
			TextureData* pTextureData = dtx_Alloc(	BPP_32P, 
													palletizedTexture.GetTextureHeader()->GetWidth(), 
													palletizedTexture.GetTextureHeader()->GetHeight(), 
													palletizedTexture.GetTextureHeader()->GetNumMipMaps(),
													&allocSize, &textureDataSize);
			if( pTextureData )
			{
				// now let's copy our pixel data into the mip data
				uint32 sourceMipDataOffset	= (256*4);
				uint32 w = palletizedTexture.GetTextureHeader()->GetWidth();
				uint32 h = palletizedTexture.GetTextureHeader()->GetHeight();

				for(int iMipmap=0; iMipmap < pTextureData->m_Header.m_nMipmaps; iMipmap++)
				{
					TextureMipData *pMip = &pTextureData->m_Mips[iMipmap];

					memcpy(	pMip->m_Data, 
							palletizedTexture.GetTextureData()+sourceMipDataOffset, // copy after the start of the pallete
							w*h);
					pMip->m_Pitch  = w;
					pMip->m_Height = h;
					pMip->m_Width  = w;

					PrintTextureMipDataInfo(pMip);

					sourceMipDataOffset = sourceMipDataOffset + (w * h);
					w = w / 2;
					h = h / 2;
				}
				pTextureData->m_Header.m_Extra[1] = (uint8)pTextureData->m_Header.m_nMipmaps;
				
				// now let's create the pallete section
				DtxSection* pPalleteSection = (DtxSection*) malloc(	sizeof(SectionHeader)+
																	sizeof(DtxSection*)+
																	((sizeof(uint8)*4)*256));
				if( pPalleteSection )
				{
					strcpy(pPalleteSection->m_Header.m_Type, "PALLETE32");  
					strcpy(pPalleteSection->m_Header.m_Name, "");
					memcpy(pPalleteSection->m_Data, palletizedTexture.GetTextureData(), ((sizeof(uint8)*4)*256) );
					pPalleteSection->m_Header.m_DataLen = ((sizeof(uint8)*4)*256);

					pPalleteSection->m_pNext = LTNULL;
					// now let's tell pTextureData about our Pallete Section
					pTextureData->m_pSections = pPalleteSection;
					pTextureData->m_Header.m_IFlags |= DTX_SECTIONSFIXED;

					ILTStream *pStream = streamsim_Open((LPCTSTR)(*pFilename), "wb");
					if(pStream)
					{
						// now save the dtx into a file
						if( LT_OK == dtx_Save(pTextureData, pStream) )
						{	
							pStream->Release();
							retVal = TRUE;
						}
					}			

					free(pPalleteSection);
				}
				
				free(pTextureData);
			}
		}
	}	
	return retVal;
}
// =====================================================================
// outFile appears to be unused
BOOL SaveDtxAs8Bit(DStream *pDtxFile, CAbstractIO &outFile, CString* pFilename)
{
	TextureData *pData;
	BOOL bRet = TRUE;
	
	if(dtx_Create(pDtxFile, &pData, FALSE) != DE_OK)
		return FALSE;

	bRet = Write8BitDTX(outFile, pData, pFilename);
	dtx_Destroy(pData);
	return bRet;
}
