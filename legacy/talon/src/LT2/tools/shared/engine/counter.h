// This file defines some helpful profiling counter routines.
//
// talon: the 2001 counter, as Jupiter's runtime/kernel/src/sys/win/counter.h
// has it (cut to what ModelEdit's image has: 0x428b4c..0x428be4).

#ifndef __COUNTER_H__
#define __COUNTER_H__

	#include "ltbasetypes.h"

	// You can start a counter in its constructor with these.
	#define CSTART_NONE			0
	#define CSTART_MILLI		1
	#define CSTART_MICRO		2

	// Note : This is the class you need to use if you want to use it in the final build
	class CounterFinal
	{
	public:
						CounterFinal(unsigned long startMode=CSTART_NONE);

		// Measure in milliseconds (1/1,000).
		void			StartMS();
		unsigned long	EndMS();
		unsigned long	CountMS();

		// Measure in microseconds (1/1,000,000).
		void			StartMicro();
		unsigned long	EndMicro();
		unsigned long	CountMicro();

		unsigned long m_Data[2];
	};

	// How many ticks per second are there?
	unsigned long cnt_NumTicksPerSecond();

	// Start a counter  (final build)
	void cnt_StartCounterFinal(CounterFinal &cCounter);

	// Returns the number of ticks since you called cnt_StartCounterFinal
	unsigned long cnt_EndCounterFinal(CounterFinal &cCounter);


	typedef CounterFinal Counter;


	// Start a counter.
	inline void cnt_StartCounter(Counter &cCounter) { cnt_StartCounterFinal(reinterpret_cast<CounterFinal&>(cCounter)); }

	// Returns the number of ticks since you called StartCounter.
	inline unsigned long cnt_EndCounter(Counter &cCounter) { return cnt_EndCounterFinal(reinterpret_cast<CounterFinal&>(cCounter)); }


	// C++ helpers..
	// talon: gl_modelrender's DrawModel has one (0x4047b2, its destructor
	// 0x404ac9).
	class CountAdder
	{
		public:

			CountAdder(uint32 *pNum)
			{
				m_pNum = pNum;
				cnt_StartCounter(m_Counter);
			}

			~CountAdder()
			{
				*m_pNum += cnt_EndCounter(m_Counter);
			}

			Counter m_Counter;
			uint32 *m_pNum;
	};

#endif  // __COUNTER_H__
