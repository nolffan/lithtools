
#include "bdefs.h"
#include "processing.h"
#include "preworld.h"
#include "dtxmgr.h"
#include "streamsim.h"
#include "conparse.h"
#include "gettextureinfo.h"

// talon: the 2001 version (0x413490..0x413a10): one texture a surface,
// and the texture lights come from the DTX's LIGHTDEFS section.


TInfo* FindTInfo(CGLinkedList<TInfo*> &theList, const char *pTextureName)
{
	GPOS pos;
	TInfo *pFlags;

	for(pos=theList; pos; )
	{
		pFlags = theList.GetNext(pos);

		if(CHelpers::UpperStrcmp(pTextureName, pFlags->m_pTextureName))
		{
			return pFlags;
		}
	}

	return NULL;
}


// Reads the texture lights in a LIGHTDEFS section, one command each:
//   <type> <u> <v> [Z:<z>] <radius> [<inner r g b>] [<outer r g b>]
// where type is center, dirlight or anything else for an omni light
// (the name is ours).
static void ParseLightDefs(TInfo *pInfo, char *pDefs)
{
	char		*pCurPos;
	char		*args[PARSE_MAXARGS];
	char		argBuffer[PARSE_MAXARGS*PARSE_MAXARGLEN];
	int			nArgs, iArg;
	int			bMore;
	TLightDef	*pDef;

	pCurPos = pDefs;
	do
	{
		bMore = cp_Parse(pCurPos, &pCurPos, argBuffer, args, &nArgs);
		if(nArgs >= 4)
		{
			pDef = new TLightDef;

			pDef->m_bCentered = stricmp(args[0], "center") == 0;
			if(pDef->m_bCentered)
				pDef->m_bDirectional = FALSE;
			else
				pDef->m_bDirectional = stricmp(args[0], "dirlight") == 0;

			pDef->m_UOffset = (PReal)atof(args[1]);
			pDef->m_VOffset = (PReal)atof(args[2]);

			iArg = 3;
			if(toupper(args[3][0]) == 'Z' && args[3][1] == ':')
			{
				pDef->m_ZOffset = (PReal)atof(&args[3][2]);
				iArg = 4;
			}
			else
			{
				pDef->m_ZOffset = 1.0f;
			}

			pDef->m_Radius = (PReal)atof(args[iArg]);
			iArg++;

			if(iArg + 3 <= nArgs)
			{
				pDef->m_InnerColor.x = (PReal)atof(args[iArg]);
				iArg++;
				pDef->m_InnerColor.y = (PReal)atof(args[iArg]);
				iArg++;
				pDef->m_InnerColor.z = (PReal)atof(args[iArg]);
				iArg++;
			}

			if(iArg + 3 <= nArgs)
			{
				pDef->m_OuterColor.x = (PReal)atof(args[iArg]);
				iArg++;
				pDef->m_OuterColor.y = (PReal)atof(args[iArg]);
				iArg++;
				pDef->m_OuterColor.z = (PReal)atof(args[iArg]);
				iArg++;
			}

			pInfo->m_LightDefs.Append(pDef);
		}
	}
	while(bMore);
}


uint32 GetWorldTextureInfo(CPreWorld *pWorld, CGLinkedList<TInfo*> &theList)
{
	GPOS			pos;
	CPreSurface		*pSurface;
	uint32			nFlagsGotten;
	TInfo			*pFlags;
	char			fileName[256];
	ILTStream		*pStream;
	TextureData		*pData;
	DtxSection		*pSection;

	nFlagsGotten = 0;

	for(pos=pWorld->m_Surfaces; pos; )
	{
		pSurface = pWorld->m_Surfaces.GetNext(pos);

		if(!pSurface->m_pTextureName)
			continue;

		pFlags = FindTInfo(theList, pSurface->m_pTextureName);
		if(pFlags)
		{
			++nFlagsGotten;
		}
		else
		{
			sprintf(fileName, "%s\\%s", g_pGlobs->m_ProjectDir, pSurface->m_pTextureName);

			pStream = streamsim_Open(fileName, "rb");
			if(pStream)
			{
				if(dtx_Create(pStream, &pData, TRUE, TRUE) == LT_OK)
				{
					pFlags = new TInfo;
					pFlags->m_Width = pData->m_Header.m_BaseWidth;
					pFlags->m_Height = pData->m_Header.m_BaseHeight;
					pFlags->m_pTextureName = (char*)pSurface->m_pTextureName;
					pFlags->m_Flags = (uint16)pData->m_Header.m_UserFlags;
					theList.Append(pFlags);

					if(pSection = dtx_FindSection(pData, "LIGHTDEFS"))
						ParseLightDefs(pFlags, pSection->m_Data);

					++nFlagsGotten;
					dtx_Destroy(pData);
				}

				pStream->Release();
			}
		}
	}

	return nFlagsGotten;
}


uint32 GetTextureInfo(CPreMainWorld *pMainWorld, CGLinkedList<TInfo*> &theList)
{
	uint32 nFlagsGotten = 0;

	for(uint32 i = 0; i < pMainWorld->m_WorldModels.GetSize(); i++)
	{
		nFlagsGotten += GetWorldTextureInfo(pMainWorld->m_WorldModels[i], theList);
	}

	return nFlagsGotten;
}
