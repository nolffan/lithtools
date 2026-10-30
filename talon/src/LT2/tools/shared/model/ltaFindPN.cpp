// ------------------------------------------------------------------------
// ltaFindPN.cpp
//
// Searching the 2001 LTA reader's parse tree.
//
// talon: no source of this survives, nor its name: the module at 0x40b173
// (its initializer guards three template statics), sorted between the
// parse tree builder and ltalex. Written from ModelEdit.exe (Talon,
// 2001-12-19), 0x40b228..0x40b3cd. Build 69's CLTAUtil (ltamgr) has the
// same three, whose names they are given here.
// ------------------------------------------------------------------------
#pragma warning (disable:4786)

#include <string.h>
#include "ltaScene.h"


// 0x40b228
CParseNode *FindList( CParseNode *pn, const char *name )
{
	if( pn != NULL && pn->isList() )
	{
		int size = pn->getNumElements();
		if( size > 0 )
		{
			if( !pn->getElement(0)->isList() )
				if( strcmp( pn->getElement(0)->getValue(), name ) == 0 )
					return pn;

			for( int i = 0 ; i < size ; i++ )
			{
				CParseNode *found = FindList( pn->getElement(i), name );
				if( found )
					return found;
			}
		}
	}

	return NULL;
}


// 0x40b2aa
void FindAll( std::vector<CParseNode*> &found, CParseNode *pn, const char *name )
{
	if( pn != NULL && pn->isList() )
	{
		int size = pn->getNumElements();
		if( size > 0 )
		{
			if( !pn->getElement(0)->isList() )
				if( strcmp( pn->getElement(0)->getValue(), name ) == 0 )
					found.push_back( pn );

			for( int i = 0 ; i < size ; i++ )
				FindAll( found, pn->getElement(i), name );
		}
	}
}


// 0x40b333
CParseNode *ShallowFindList( CParseNode *pn, const char *name )
{
	if( pn != NULL && pn->isList() )
	{
		for( int i = 0 ; i < pn->getNumElements() ; i++ )
		{
			CParseNode *elem = pn->getElement(i);

			if( elem->isAtom() )
			{
				if( strcmp( elem->getValue(), name ) == 0 )
					return elem;
			}
			else if( elem->getNumElements() > 0 && elem->getElement(0)->isAtom() )
			{
				if( strcmp( elem->getElement(0)->getValue(), name ) == 0 )
					return elem;
			}
		}
	}

	return NULL;
}
