#include "bdefs.h"

#include "conparse.h"
#include "parse_world_info.h"

// talon: the 2001 one (0x425940..0x425b10), from Build 69's runtime source
// with LightTableRes and PBlockSize.


LTBOOL ParseAmbientLight(char *pStr, LTVector *pAmbient)
{
	ConParse parse;

	parse.Init(pStr);
	pAmbient->Init();

	if(parse.ParseFind("AmbientLight", LTFALSE, 4))
	{
		// <AmbientLight> <r> <g> <b>
		// colors are 0-255
		pAmbient->x = (float)atof(parse.m_Args[1]);
		pAmbient->y = (float)atof(parse.m_Args[2]);
		pAmbient->z = (float)atof(parse.m_Args[3]);
		VEC_CLAMP(*pAmbient, 0.0f, 255.0f);
		return LTTRUE;
	}
	
	return LTFALSE;
}


float ParseLightTableRes(char *pStr)
{
	ConParse parse;

	parse.Init(pStr);
	if(parse.ParseFind("LightTableRes", LTFALSE, 2))
		return (float)atof(parse.m_Args[1]);

	return DEFAULT_LIGHT_TABLE_RES;
}


float ParsePBlockSize(char *pStr)
{
	ConParse parse;

	parse.Init(pStr);
	if(parse.ParseFind("PBlockSize", LTFALSE, 2))
		return (float)atof(parse.m_Args[1]);

	return DEFAULT_PBLOCK_SIZE;
}
