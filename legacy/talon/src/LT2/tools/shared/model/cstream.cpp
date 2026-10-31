// ------------------------------------------------------------------------
// cstream.cpp
//
// The 2001 LTA reader's input stream from a c++ istream.
//
// talon: no source of this survives, nor its name: the module at 0x40210d,
// which sorts between ContinuousDlg and DimensionsDlg (the name is a guess,
// after ltaModel.h's "ltaModelLoad( c++_in_stream, allocator )"). Written
// from ModelEdit.exe (Talon, 2001-12-19): its one function, 0x40212e,
// called by ltaModelLoad (0x40bd3b) and ltaLoadChildModel (0x40f0c9).
// ------------------------------------------------------------------------
#pragma warning (disable:4786)

#include "ltaStream.h"


// 0x40212e
ILTAStream *LTAStreamFrom_istream( std::istream &strm )
{
	return new ILTAStream( strm );
}
