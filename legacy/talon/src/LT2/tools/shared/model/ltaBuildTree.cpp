// ------------------------------------------------------------------------
// ltaBuildTree.cpp
//
// The 2001 LTA reader's whole-tree parser and printer: BuildParseTree and
// PrettyPrint, the two "lta externs" smmempnproc.cpp declares.
//
// talon: stand-in: the module at 0x409b6c..0x40b173 ("A"), first of the
// reader's modules by name, survives only as its static initializers and
// the library code it was the first in the link to instantiate:
// basic_string::substr (0x409be1), the ostream inserters for chars and C
// strings with flush (put 0x409ca8, operator<< 0x40a118), and num_put's and
// numpunct's members (0x40a286..0x40b14a), which only an inserter of a
// number instantiates. Its own functions were dropped by the linker
// (/OPT:REF: ModelEdit calls neither; smmempnproc builds its own tree), so
// their bodies cannot be checked against the image. They are written here
// to the reader's interface (the parse node's factories, slots 11 and 12,
// have no other user) and to instantiate the same library code; the file's
// name is ours.
// ------------------------------------------------------------------------
#pragma warning (disable:4786)

#include <iostream>
#include <string>
#include "ltaStream.h"
#include "ltalex.h"
#include "ltaParseNode.h"


// ------------------------------------------------------------------------
// read lists into root until its close paren (or the end of the stream)
// ------------------------------------------------------------------------
static int build_parse_tree( CLTALexAnalizer &lexan, ILTAStream *istr, CParseNode *root )
{
	for( ;; )
	{
		std::string cur_str ;

		switch( lexan.analize_stream( *istr, cur_str ) )
		{
		case CLTALexAnalizer::LEX_OTHER :
			root->append( root->newValueNode( cur_str.c_str(), 0 ) );
			break ;

		case CLTALexAnalizer::LEX_PUSH :
			{
				CParseNode *pn = root->newListNode();
				root->append( pn );
				build_parse_tree( lexan, istr, pn );
			}
			break ;

		case CLTALexAnalizer::LEX_POP :
			root->pop();
			return 0 ;

		case CLTALexAnalizer::LEX_STRING :
			// strip out the quotes
			if( cur_str.size() > 2 )
				cur_str = cur_str.substr( 1, cur_str.size() - 2 );
			root->append( root->newValueNode( cur_str.c_str(), 1 ) );
			break ;

		case CLTALexAnalizer::LEX_EOF :
			return 1 ;
		}
	}
}


// ------------------------------------------------------------------------
// BuildParseTree( stream, root )
// parse the whole stream into root
// ------------------------------------------------------------------------
int BuildParseTree( ILTAStream *str, CParseNode *root )
{
	CLTALexAnalizer lexan ;
	return build_parse_tree( lexan, str, root );
}


// ------------------------------------------------------------------------
// PrettyPrint( out-stream, root-parsenode, depth )
// ------------------------------------------------------------------------
void PrettyPrint( std::ostream & os, CParseNode * node, int depth )
{
	int i ;

	for( i = 0 ; i < depth ; i++ )
		os << ' ' ;

	if( node->isAtom() )
	{
		if( node->isString() )
			os << '"' << node->getValue() << '"' << std::endl ;
		else
			os << node->getValue() << std::endl ;
		return ;
	}

	os << "( ; " << node->getNumElements() << std::endl ;

	for( i = 0 ; i < node->getNumElements() ; i++ )
		PrettyPrint( os, node->getElement(i), depth + 1 );

	for( i = 0 ; i < depth ; i++ )
		os << ' ' ;
	os << ")" << std::endl ;
}
