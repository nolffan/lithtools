//------------------------------------------------------------------
//
//	FILE	  : StdLith.h
//
//	PURPOSE	  : Includes all the useful files for your
//              programming pleasure.
//
//	CREATED	  : August 20 1996
//
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------

#ifndef __STDLITH_H__
	#define __STDLITH_H__


	// Includes....
	// talon: ../stdlithNOLF/, which Monolith's named ../stdlith/
	#include "../stdlithNOLF/stdlithdefs.h"
	#include "../stdlithNOLF/lithexception.h"
	#include "../stdlithNOLF/memory.h"

	#include "../stdlithNOLF/dynarray.h"
	
	#include "../stdlithNOLF/linklist.h"
	#include "../stdlithNOLF/goodlinklist.h"
	#include "../stdlithNOLF/multilinklist.h"
	#include "../stdlithNOLF/fastlinklist.h"
	#include "../stdlithNOLF/genhash.h"
	#include "../stdlithNOLF/glink.h"

	#include "../stdlithNOLF/stringholder.h"
	#include "../stdlithNOLF/morect.h"

	#include "../stdlithNOLF/abstractio.h"
	#include "../stdlithNOLF/fileio.h"
	#include "../stdlithNOLF/memoryio.h"
	#include "../stdlithNOLF/compressedio.h"

	#include "../stdlithNOLF/object_bank.h"
	#include "../stdlithNOLF/struct_bank.h"

	#include "../stdlithNOLF/helpers.h"
	#include "../stdlithNOLF/copy_lists.h"


	#ifndef NO_PRAGMA_LIBS
		#ifdef STDLITH_ALLOC_OVERRIDE
			// Use the 'allocator override' versions.
			#ifdef STDLITH_MFC
				#if defined( _DEBUG ) && defined( _AFXDLL )
					#pragma comment (lib, "\\proj\\libs\\debug\\StdLithMFCDLL_alloc.lib")
				#elif defined( _DEBUG ) && !defined( _AFXDLL )
					#pragma comment (lib, "\\proj\\libs\\debug\\StdLithMFC_alloc.lib")
				#elif !defined( _DEBUG ) && defined( _AFXDLL )
					#pragma comment (lib, "\\proj\\libs\\release\\StdLithMFCDLL_alloc.lib")
				#elif !defined( _DEBUG ) && !defined( _AFXDLL )
					#pragma comment (lib, "\\proj\\libs\\release\\StdLithMFC_alloc.lib")
				#endif
			#else
				#ifdef _DEBUG
					#pragma comment (lib, "\\proj\\libs\\debug\\StdLith_alloc.lib")
				#else
					#pragma comment (lib, "\\proj\\libs\\release\\StdLith_alloc.lib")
				#endif
			#endif
		#else
			#ifdef STDLITH_MFC
				#if defined( _DEBUG ) && defined( _AFXDLL )
					#pragma comment (lib, "\\proj\\libs\\debug\\StdLithMFCDLL.lib")
				#elif defined( _DEBUG ) && !defined( _AFXDLL )
					#pragma comment (lib, "\\proj\\libs\\debug\\StdLithMFC.lib")
				#elif !defined( _DEBUG ) && defined( _AFXDLL )
					#pragma comment (lib, "\\proj\\libs\\release\\StdLithMFCDLL.lib")
				#elif !defined( _DEBUG ) && !defined( _AFXDLL )
					#pragma comment (lib, "\\proj\\libs\\release\\StdLithMFC.lib")
				#endif
			#else
				#ifdef _DEBUG
					#pragma comment (lib, "\\proj\\libs\\debug\\StdLith.lib")
				#else
					#pragma comment (lib, "\\proj\\libs\\release\\StdLith.lib")
				#endif
			#endif
		#endif
	#endif


#endif
