// talon: the 2001 counter (ModelEdit 0x428b4c..0x428be4): Jupiter's
// runtime/kernel/src/sys/win/counter.cpp, its final-build half. StartMS and
// StartMicro are one body in the image (0x428b97: the linker folded them).


//#include "bdefs.h"
#include <windows.h>
#include "counter.h"


//#define LOCK_AT_MICROSECONDS


static unsigned __int64 g_CountDiv = 1;


// Win2K has a VERY high counter frequency but it's more useful if it's in 
// microseconds.
class CountDivSetter
{
public:
	CountDivSetter()
	{
		LARGE_INTEGER perSec;
		
		QueryPerformanceFrequency(&perSec);
#ifdef LOCK_AT_MICROSECONDS
		g_CountDiv = perSec.QuadPart / 1000000;
#else
		g_CountDiv = 1;
#endif
	}
};
static CountDivSetter __g_CountDivSetter;



unsigned long cnt_NumTicksPerSecond()
{
	LARGE_INTEGER perSec;

	QueryPerformanceFrequency(&perSec);
	return (unsigned long)(perSec.QuadPart / g_CountDiv);	
}



CounterFinal::CounterFinal(unsigned long startMode)
{
	if(startMode == CSTART_MICRO)
		StartMicro();
	else if(startMode == CSTART_MILLI)
		StartMS();
}


void CounterFinal::StartMS()
{
	LARGE_INTEGER *pInt;

	pInt = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(pInt);
}

unsigned long CounterFinal::EndMS()
{
	LARGE_INTEGER curCount, *pInCount, perSec;
	LONGLONG timeElapsed;

	pInCount = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(&curCount);

	QueryPerformanceFrequency(&perSec);
	timeElapsed = curCount.QuadPart - pInCount->QuadPart;
	return (unsigned long)((timeElapsed*(LONGLONG)1000) / perSec.QuadPart);
}

unsigned long CounterFinal::CountMS()
{
	return EndMS();
}



void CounterFinal::StartMicro()
{
	LARGE_INTEGER *pInt;

	pInt = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(pInt);
}

unsigned long CounterFinal::EndMicro()
{
	LARGE_INTEGER curCount, *pInCount;

	pInCount = (LARGE_INTEGER*)m_Data;
	QueryPerformanceCounter(&curCount);

	return (unsigned long)((curCount.QuadPart - pInCount->QuadPart) / g_CountDiv);
}

unsigned long CounterFinal::CountMicro()
{
	return EndMicro();
}



void cnt_StartCounterFinal(CounterFinal &cCounter)
{
	cCounter.StartMicro();
}


unsigned long cnt_EndCounterFinal(CounterFinal &cCounter)
{
	return cCounter.EndMicro();
}


