// ------------------------------------------------------------------------
// ltalex.cpp
//
// The 2001 LTA reader's lexical analyser.
//
// talon: no source of this survives. Written from ModelEdit.exe (Talon,
// 2001-12-19), the module at 0x40b3de: its own functions 0x40b3ff..0x40b550,
// then the std::istream and ctype<char> code they are the first in the link
// to instantiate (read, peek, get, ipfx, use_facet, ctype<char>'s ...). It
// includes <istream>, not <iostream>: the module has no ios_base::Init.
// ------------------------------------------------------------------------
#pragma warning (disable:4786)

#include <stdio.h>
#include <string.h>
#include "ltaStream.h"
#include "ltalex.h"


// the token being read (0x493f20, 0x800 bytes)
static char s_Token[2048];


// 0x40b3ff
CLTALexAnalizer::CLTALexAnalizer()
{
	m_pErrorFn = NULL;
	m_ParenDepth = 0;
	m_LineNum = 1;
}


// 0x40b410
int CLTALexAnalizer::analize_stream( ILTAStream &strm, std::string &str )
{
	int ret;
	char c;
	int i = 1;

	strm.m_Stream.read( &c, 1 );
	if( strm.m_Stream.eof() )
		return LEX_EOF;

	if( c == ' ' || c == '\t' )
		return LEX_NONE;

	if( c == '(' )
	{
		m_ParenDepth++;
		ret = LEX_PUSH;
		goto out;
	}

	if( c == ')' )
	{
		m_ParenDepth--;
		return LEX_POP;
	}

	if( c == '\n' )
	{
		m_LineNum++;
		return LEX_NONE;
	}

	if( c == '"' )
	{
		s_Token[0] = c;
		do
		{
			strm.m_Stream.get( c );
			s_Token[i++] = c;
			if( c == '"' )
			{
				ret = LEX_STRING;
				goto done;
			}
		}
		while( i < 2047 );

		// talon: retail bug: a string of 2047 characters or more reports
		// both errors and comes back as a symbol, LEX_OTHER, with str
		// left as it was.
		Error( " unterminated or too-large string" );
		Error( "undefined character scanned" );
		return LEX_OTHER;
	}
	else
	{
		// talon: retail bug: a symbol is not bounds checked; one longer
		// than 2047 characters runs off the end of s_Token.
		i = 0;
		while( 1 )
		{
			s_Token[i++] = c;
			if( IsSymbolChar( strm.m_Stream.peek() ) )
				strm.m_Stream.read( &c, 1 );
			else
				break;
		}
		ret = LEX_OTHER;
	}

done:
	s_Token[i] = 0;
	str = s_Token;
out:
	return ret;
}


// 0x40b512
int CLTALexAnalizer::IsSymbolChar( int c )
{
	switch( c )
	{
	case ' ' :
	case '\t' :
	case '\n' :
	case '(' :
	case ')' :
		return 0;
	}

	return 1;
}


// 0x40b539
void CLTALexAnalizer::Error( const char *msg )
{
	if( m_pErrorFn == NULL )
		printf( msg );
	else
		m_pErrorFn( msg );
}
