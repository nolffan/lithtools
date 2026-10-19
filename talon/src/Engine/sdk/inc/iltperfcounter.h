//-------------------------------------------------------------------
//
//   MODULE    : ILTPERFCOUNTER.H
//
//   PURPOSE   : Defines engine interface ILTPerfCounter
//
//   CREATED   : On 10/19/00 At 10:24:15 AM
//-------------------------------------------------------------------

#ifndef __ILTPERFCOUNTER_H__
#define __ILTPERFCOUNTER_H__

#ifndef __LTBASEDEFS_H__
#include "ltbasedefs.h"
#endif

#ifndef __LTBASETYPES_H__
#include "ltbasetypes.h"
#endif

#ifndef __LTMODULE_H__
#include "ltmodule.h"
#endif


// For non-windowes machines, MAKEFOURCC can be used to create the group IDs
//	e.g. MAKEFOURCC('B','A','S','C') == 'BASC' == 0x42415343
#ifndef MAKEFOURCC_PC
#define MAKEFOURCC_PC(ch3, ch2, ch1, ch0)								\
		((uint32)(uint8)(ch0) | ((uint32)(uint8)(ch1) << 8) |			\
		((uint32)(uint8)(ch2) << 16) | ((uint32)(uint8)(ch3) << 24 ))
#endif

/*!  
ILTPerfCounter interface. The ILTPerfCounter interface enables you to 
use LithTech's performance monitoring manager. The manager maintains groups of counters
added by all parts of the engine and game code to be graphically displayed. There 
are several console variables which can be used to configure how they are displayed. 

To monitor a section of your code, first create a counter, call StartCounter before your 
code segment, then call StopCounter at the end. Enable the performance counters to be 
displayed and the group with which you created your counter in. 

Define a holder to get this interface like this:
\code
define_holder(ILTPerfCounter, your_var);
\endcode
*/

class ILTPerfCounter : public IBase {
public :
    interface_version(ILTPerfCounter, 0);

/*!
\param uCounterGroup Counter group within which this counter will be added. Use FourCC for this (e.g. 'MINE' or MAKEFOURCC_PC('M','I','N','E'), the groups can be enabled in the console with PerfCounterGroupEnable MINE.
\param szCounterName String counter name (e.g. "MyTestCounter"). They will be displayed with this name in the graph.
\return The counter ID (0 is a failure).

Counters can be added through this function. Use the returned counter ID to Start and Stop your counter. If the counter has already been created, its ID will be returned to you. There aren't really any restrictions on counter group names.

\see StartCounter()
\see StopCounter()

Used for: Monitoring Performance.
*/
	virtual uint32		AddCounter(uint32 uCounterGroup, const char* szCounterName) = 0;

/*!
\param uCounterGroup Counter group within which this counter has be added.
\param szCounterName String counter name (e.g. "MyTestCounter").
\return The counter ID (0 is a failure).

Forget your counter's ID? Use this function to retreve it.

Used for: Monitoring Performance.
*/
	virtual uint32		GetCounterID(uint32 uCounterGroup, const char* szCounterName) = 0;

/*!
\param uCounterID The Counters ID (returned from AddCounter or GetCounterID).
\return Bool value - if the delete was successful.

Delete a counter from the list.

Used for: Monitoring Performance.
*/
	virtual bool		DeleteCounter(uint32 uCounterID) = 0;

/*!
\param uCounterID The Counters ID (returned from AddCounter or GetCounterID).

Start the counter. The current time is taken. you should call Stop counter once for each call to StartCounter. You can call these pairs multiple times per frame.

\see StopCounter()

Used for: Monitoring Performance.
*/
	virtual void		StartCounter(uint32 uCounterID) = 0;

/*!
\param uCounterID The Counters ID (returned from AddCounter or GetCounterID).

Stop the counter. The current time is taken and the difference from StartCounter is added to the current count.

\see StartCounter()

Used for: Monitoring Performance.
*/
	virtual void		StopCounter(uint32 uCounterID) = 0;
};

#endif 

