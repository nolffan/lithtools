// ------------------------------------------------------------------------
// ltalex.h
//
// The 2001 LTA reader's lexical analyser: splits an LTA stream into parens,
// strings and symbols.
//
// talon: no source of this survives. Written from ModelEdit.exe (Talon,
// 2001-12-19), 0x40b3ff..0x40b550; the names are the ones smmempnproc.cpp
// calls (analize_stream and the LEX_ tokens, numbered as analize_stream
// returns them and smmempnproc's switch, 0x426824, takes them).
// ------------------------------------------------------------------------

#ifndef __LTALEX_H__
#define __LTALEX_H__

#include <string>

class ILTAStream;


class CLTALexAnalizer
{
public :

	enum {
		LEX_NONE = 0,	// white space, a new line
		LEX_PUSH,		// (
		LEX_POP,		// )
		LEX_STRING,		// "..." quotes and all
		LEX_OTHER,		// a symbol
		LEX_EOF
	};

	// where the error messages go; printf when NULL
	typedef void (*ErrorFn)( const char *msg );

	CLTALexAnalizer();									// 0x40b3ff

	// reads the next token into str and says what it is (0x40b410)
	int  analize_stream( ILTAStream &strm, std::string &str );

	ErrorFn	m_pErrorFn;		// +0
	int		m_LineNum;		// +4
	int		m_ParenDepth;	// +8

private :

	int  IsSymbolChar( int c );							// 0x40b512
	void Error( const char *msg );						// 0x40b539
};


#endif
