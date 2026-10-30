#include "bdefs.h"
#include <malloc.h>
#include <string.h>
#include "ltbasetypes.h"

// This module just implements dalloc and dfree for the tools.

// talon: 2001's has the stream operators for a rotation out of line ahead
// of dalloc (0x435b08, 0x435b46; the LT2 SDK's ltrotation.h has none, and
// they sit between pixelformat's last function and dalloc, a module without
// an initializer, so their module is unnamed: they're put here).  It has no
// dalloc_z (DEdit's has).
void LTStream_Read(ILTStream *pStream, LTRotation &quat)
{
	STREAM_READ(quat.m_Quat[0]);
	STREAM_READ(quat.m_Quat[1]);
	STREAM_READ(quat.m_Quat[2]);
	STREAM_READ(quat.m_Quat[3]);
}


void LTStream_Write(ILTStream *pStream, LTRotation &quat)
{
	STREAM_WRITE(quat.m_Quat[0]);
	STREAM_WRITE(quat.m_Quat[1]);
	STREAM_WRITE(quat.m_Quat[2]);
	STREAM_WRITE(quat.m_Quat[3]);
}


void* dalloc(uint32 size)
{
	if(size == 0)
		return 0;
	else
		return malloc(size);
}


#ifdef DIRECTEDITOR_BUILD
// talon: DEdit's (2001) has dalloc_z between dalloc and dfree (0x460433:
// zero size gives null, no exception, and a failed malloc isn't zeroed).
void* dalloc_z(uint32 size)
{
	void *pRet;

	if(size == 0)
		return 0;

	pRet = malloc(size);
	if(pRet)
		memset(pRet, 0, size);

	return pRet;
}
#endif


void dfree(void *ptr)
{
	free(ptr);
}
