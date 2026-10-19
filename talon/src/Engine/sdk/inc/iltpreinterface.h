/*!  This file contains the interface that is passed to the game code
to handle debugging when a property value changes.  */

#ifndef __ILTPREINTERFACE_H__
#define __ILTPREINTERFACE_H__


	#include "ltbasedefs.h"

/*!  Preprocessor lighting interface.  Passed to the game code when
lightmaps are being generated.  */

	class ILTPreInterface
	{
	public:

/*!
\param pParse the ConParse to use
\return LT_OK while parsing is ongoing, and LT_FINISHED when done.

Use the console parser.

Used for:	Misc.
*/
		virtual LTRESULT Parse(ConParse *pParse)=0;


/*!
\param pMsg The message to send to the console
\return LT_OK.

Print a message to the debug "console".

Used for:	Misc.
*/
		virtual LTRESULT CPrint(const char *pMsg, ...)=0;


/*!
\param bShow LTTRUE to show the window, LTFALSE to hide the window
\return LT_OK.

Shows the debug "console: windoe.

Used for:	Misc.
*/
		virtual LTRESULT ShowDebugWindow(LTBOOL bShow)=0;


/*!
\param pName Name of object to look for
\return LT_OK if an object is found, or LT_NOTFOUND otherwise.

Find objects.

Used for:	Misc.
*/
		virtual LTRESULT FindObject(const char *pName)=0;


/*!
\param pObjName Name of the object (input)
\return The name of the class of the object, LTNULL if an error occured.

Get a type of object.

Used for:	Misc.
*/
		virtual const char* GetObjectClass( const char *pObjName )=0;


/*!
\return The path for the base project directory, LTNULL if an error occured.

Get the Rez path.

Used for:	Misc.
*/
		virtual const char* GetProjectDir( )=0;

/*
\return The name of the world.  'World1'

Get the name of the active world.

Used for:	Misc.
*/
		virtual const char* GetWorldName( )=0;

	};


#endif //! __ILTPREINTERFACE_H__