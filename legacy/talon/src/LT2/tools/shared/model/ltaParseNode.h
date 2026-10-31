// ------------------------------------------------------------------------
// ltaParseNode.h
//
// The 2001 LTA reader's parse tree: a node is either an atom (a symbol or a
// string, its value) or a list of nodes. Build 69 replaced it with ltamgr's
// CLTANode, whose API is this one capitalised (GetValue, GetElement,
// GetNumElements, IsList, IsAtom, IsString).
//
// talon: no source of this survives. Written from ModelEdit.exe (Talon,
// 2001-12-19): the class layout from its constructors (0x4122bf, 0x412381;
// sizeof 0x1c, `new` at 0x40bccc), the virtual functions in the order of
// its vtable (0x474ec0, 13 slots; CExecPN's, 0x4765ec, overrides slots 0,
// 3 and 4). The names smmempnproc.cpp still calls (append, pop, isAtom,
// isString, getValue, deleteChildren, the (const char*, int) constructor)
// are the 2001 ones; the others are named after Build 69's CLTANode.
// ------------------------------------------------------------------------

#ifndef __LTAPARSENODE_H__
#define __LTAPARSENODE_H__

#pragma warning (disable:4786)

#include <string>
#include <vector>


class CParseNode
{
public :

	// list constructor (0x4122bf)
	CParseNode();

	// value constructor (0x412381): bIsString says whether the value was a
	// quoted string or a symbol in the file.
	CParseNode( const char *pstr, int bIsString = 0 );

	// 0x41240a (slot 0, ??_G 0x412365)
	virtual ~CParseNode();

	// slot 1 (0x41250a): the value of an atom, NULL for a list
	virtual const char *getValue();

	// slot 2 (0x412521)
	virtual void setValue( const char *pstr );

	// slot 3 (0x41259b): append a new list or an element to this list
	virtual void append( CParseNode *new_node );

	// slot 4 (0x4117e2, an empty function): called by the parser when the
	// close paren of this list is read.
	virtual void pop() {}

	// slot 5 (0x4125ab): element i of a list, NULL if there is none
	virtual CParseNode *getElement( int index );

	// slot 6 (0x4093b7, folded with an identical getter)
	virtual int isString() { return m_bIsString; }

	// slot 7 (0x4125d3)
	virtual int getNumElements();

	// slot 8 (0x412501)
	virtual int isList();

	// slot 9 (0x4122ee)
	virtual int isAtom();

	// slot 10 (0x41249f): delete every descendant
	virtual void deleteChildren();

	// slots 11 and 12 (0x41232b, 0x4122f9): make a node of this kind
	virtual CParseNode *newValueNode( const char *pstr, int bIsString );
	virtual CParseNode *newListNode();

protected :

	std::string					*m_pValue;		// +0x04, NULL for a list
	int							 m_bIsString;	// +0x08
	std::vector<CParseNode*>	 m_children;	// +0x0c
};


// ------------------------------------------------------------------------
// Searching a parse tree (0x40b228..0x40b3cd; Build 69's CLTAUtil has the
// same three).
// ------------------------------------------------------------------------

// the first list, pn itself or below it, whose first element is the atom
// name (0x40b228)
CParseNode *FindList( CParseNode *pn, const char *name );

// every list, pn itself and below it, whose first element is the atom
// name (0x40b2aa)
void FindAll( std::vector<CParseNode*> &found, CParseNode *pn, const char *name );

// the first element of pn that is the atom name, or a list whose first
// element is (0x40b333)
CParseNode *ShallowFindList( CParseNode *pn, const char *name );


#endif
