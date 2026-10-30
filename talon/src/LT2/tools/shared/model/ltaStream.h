// ------------------------------------------------------------------------
// ltaStream.h
//
// The 2001 LTA reader's input: the stream the lexer reads.
//
// talon: no source of this survives. Written from ModelEdit.exe (Talon,
// 2001-12-19): LTAStreamFrom_istream (0x40212e) news a four-byte object that
// holds the istream it is given, and the lexer (0x40b410) reads through that
// first member directly, read(), get() and peek() of the std::istream, with
// no virtual call: so a reference to the stream, nothing else.
// ------------------------------------------------------------------------

#ifndef __LTASTREAM_H__
#define __LTASTREAM_H__

#include <istream>


class ILTAStream
{
public :

	ILTAStream( std::istream &strm ) : m_Stream(strm) {}

	std::istream &m_Stream;
};


// 0x40212e
ILTAStream *LTAStreamFrom_istream( std::istream &strm );


#endif
