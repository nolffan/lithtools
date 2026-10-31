//------------------------------------------------------------------
//
//	FILE	: BDEFS.CPP
//
//	PURPOSE	: Base definition code body
//
//	CREATED	: 3rd November 1995
//
//------------------------------------------------------------------

// Includes....

#include "bdefs.h"

// talon: Processor's dsi_PrintToConsole and dsi_OnReturnError are these
// empty ones (the shipped link keeps one copy of the empty functions it
// folds, 0x42dd40: dtx_Create's RETURN_ERRORs call it for both, 0x40d2fc,
// 0x40d5cc). Its g_ReturnErrString is the first thing in .data after the
// C runtime's tables (0x47e100), so this is the link's first object.
#define BDEFS_SIM


int32	g_DebugLevel=0;
char *g_ReturnErrString = "LT ERROR: %s returned %s (%s)";


#ifdef BDEFS_SIM
	void dsi_PrintToConsole(char *pStr, ...)
	{
	}

	void dsi_OnReturnError(int level)
	{
	}
#endif


#ifdef DALLOC_SIM
	void* dalloc(DWORD size)
	{
		return malloc(size);
	}

	void* dalloc_z(DWORD size)
	{
		void *ret;

		if(ret = malloc(size))
		{
			memset(ret, 0, size);
		}

		return ret;
	}

	void dfree(void *ptr)
	{
		free(ptr);
	}
#endif

	  