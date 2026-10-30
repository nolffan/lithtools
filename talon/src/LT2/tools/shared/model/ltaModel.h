// --------------------------------------------------------------------------
// ltaModel.h
// lithtech.(c) 2000
// t.f 4//00
// 
/*
	lta model reading/writing/creating
	*/
#ifndef LTA_MODEL_DEFINED
#define LTA_MODEL_DEFINED


#include "ltaScene.h"
#include <istream>


// ------------------------------------------------------------------------
// ltaModelLoad( c++_in_stream, allocator )
// parses file into model file.
// bChildRefs is set to true, child models data will not be loaded, the model
// data base will have a reference to the child model localtion 
// ------------------------------------------------------------------------
// talon: 2001's reads a std::istream (0x40bcba; ModelEditDlg's load thread
// calls it at 0x41709c with the stream, the allocator and false). Build 69's
// CLTAReader and filename overloads are not in the image.
Model*  ltaModelLoad( std::istream& is, LAllocCount & allocCount, bool bChildRefs = false );

// ------------------------------------------------------------------------
// ltaLoadChildModel
// same as above except it does not process lods and does not get 
// child models bound to it.
// ------------------------------------------------------------------------
Model*  ltaLoadChildModel( const char * filename, LAlloc * allocCount  );



#endif