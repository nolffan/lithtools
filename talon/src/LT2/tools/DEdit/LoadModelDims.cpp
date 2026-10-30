// LoadModelDims.cpp - the 2001 model dims reader, written from the image
// (0x42da61..0x42dc69, with ModelLoadRequest's inline constructor at
// 0x42dc43).  The names are ours (talon).

#include "bdefs.h"
#include "model.h"
#include "loadmodeldims.h"

// model_load.cpp (0x4375e7)
LTBOOL mod_FindChunk(ILTStream *pStream, const char *pName);

// model_ops.cpp (0x43a144)
void model_GetDimsFromBounding(LTVector *pvBoundMax, LTVector *pvBoundMin, LTVector *pvDims);


BOOL GetModelDimsFromFile(DStream *pStream, CVector *pDims)
{
	ModelLoadRequest request;
	Model model(&g_DefAlloc, &g_DefAlloc);
	char str[512];
	uint32 dwVal;
	LTVector vBoundMax, vBoundMin;

	if(mod_FindChunk(pStream, "Header") && pStream->ReadString(str, sizeof(str)) == LT_OK)
	{
		if(stricmp(str, "MonolithExport Model File v6") == 0)
		{
			// The old format: its dims, or its first keyframe's box.
			if(mod_FindChunk(pStream, "Dimensions"))
			{
				pStream->Read(&pDims->x, sizeof(float));
				pStream->Read(&pDims->y, sizeof(float));
				pStream->Read(&pDims->z, sizeof(float));
				return TRUE;
			}
			else if(mod_FindChunk(pStream, "Animation"))
			{
				pStream->Read(&dwVal, sizeof(dwVal));
				if(dwVal)
				{
					pStream->ReadString(str, sizeof(str));
					pStream->Read(&dwVal, sizeof(dwVal));
					pStream->Read(&vBoundMax, sizeof(vBoundMax));
					pStream->Read(&vBoundMax, sizeof(vBoundMax));
					pStream->Read(&dwVal, sizeof(dwVal));
					if(dwVal)
					{
						pStream->Read(&dwVal, sizeof(dwVal));
						pStream->Read(&vBoundMax, sizeof(vBoundMax));
						pStream->Read(&vBoundMin, sizeof(vBoundMin));
						model_GetDimsFromBounding(&vBoundMax, &vBoundMin, pDims);
						return TRUE;
					}
				}
			}
		}
		else
		{
			pStream->SeekTo(0);
			request.m_pFile = pStream;
			if(model.Load(&request) != LT_OK)
				return FALSE;

			if(model.NumAnims() > 0)
			{
				*pDims = model.GetAnimInfo(0)->m_vDims;
				return TRUE;
			}
		}
	}

	return FALSE;
}
