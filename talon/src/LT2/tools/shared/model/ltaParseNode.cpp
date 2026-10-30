// ------------------------------------------------------------------------
// ltaParseNode.cpp
//
// The 2001 LTA reader's parse tree.
//
// talon: no source of this survives. Written from ModelEdit.exe (Talon,
// 2001-12-19), the module at 0x41215b, between ltaModel and ltaScene by
// name: CParseNode 0x4122bf..0x4125d3, then the vector<CParseNode*> code it
// is the first in the link to instantiate (0x4125e6, 0x412636, 0x412642).
// ------------------------------------------------------------------------
#pragma warning (disable:4786)

#include <string.h>
#include "ltaParseNode.h"


// 0x4122bf
CParseNode::CParseNode()
	: m_pValue(NULL), m_bIsString(0), m_children(0)
{
}


// 0x412381
CParseNode::CParseNode( const char *pstr, int bIsString )
	: m_bIsString(bIsString), m_children(0)
{
	m_pValue = new std::string( pstr );
}


// 0x41240a
CParseNode::~CParseNode()
{
	int size = m_children.size();

	if( m_pValue != NULL )
	{
		delete m_pValue;
		m_pValue = NULL;
	}

	for( int i = 0 ; i < size ; i++ )
	{
		CParseNode *pn = m_children[i];
		if( pn != NULL )
			delete pn;
	}

	m_children.clear();
}


// 0x41250a
const char *CParseNode::getValue()
{
	if( m_pValue != NULL )
		return m_pValue->c_str();
	else
		return NULL;
}


// 0x412521
void CParseNode::setValue( const char *pstr )
{
	if( m_pValue != NULL )
		*m_pValue = pstr;
	else
		m_pValue = new std::string( pstr );
}


// 0x41259b
void CParseNode::append( CParseNode *new_node )
{
	m_children.push_back( new_node );
}


// 0x4125ab
CParseNode *CParseNode::getElement( int index )
{
	int size = m_children.size();

	if( size > 0 && index < size )
		return m_children[index];

	return NULL;
}


// 0x4125d3
int CParseNode::getNumElements()
{
	return m_children.size();
}


// 0x412501
int CParseNode::isList()
{
	return m_pValue == NULL;
}


// 0x4122ee
int CParseNode::isAtom()
{
	return !isList();
}


// 0x41249f
void CParseNode::deleteChildren()
{
	int size = m_children.size();

	for( int i = 0 ; i < size ; i++ )
	{
		m_children[i]->deleteChildren();
		delete m_children[i];
		m_children[i] = NULL;
	}

	m_children.clear();
}


// 0x41232b
CParseNode *CParseNode::newValueNode( const char *pstr, int bIsString )
{
	return new CParseNode( pstr, bIsString );
}


// 0x4122f9
CParseNode *CParseNode::newListNode()
{
	return new CParseNode;
}
