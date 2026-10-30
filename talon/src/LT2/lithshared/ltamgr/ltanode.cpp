#include "ltanode.h"
#include "ltaconverter.h"

//the decimal accuracy for converting from double to str
#define LTA_DECIMAL_ACCURACY	6

//default constructor. Creates an atom with no value
CLTANode::CLTANode() :
	m_nFlags(0),
	m_pData(NULL)
{
}

//creates a list with room for the specified number of elements (2001,
//0x48bcb0)
CLTANode::CLTANode(uint32 nNumElements) :
	m_nFlags(0),
	m_pData(NULL)
{
	AllocateElements(nNumElements);
}

//creates an atom with the specified value (2001, 0x48bcd0)
CLTANode::CLTANode(const char* pszValue, bool bString) :
	m_nFlags(MASK_VALUE),
	m_pData(NULL)
{
	SetValue(pszValue, bString);
}

CLTANode::~CLTANode()
{
	Free();
}


//allocates room for the specified number of elements. Doing this will
//make the node a list and not an atom
bool CLTANode::AllocateElements(uint32 nNumElements)
{
	//clean up old stuff
	Free();

	//allocate the array
	m_pData = new CLTANode* [nNumElements];

	//sanity check
	if(m_pData == NULL)
	{
		return false;
	}

	//set up the size
	m_nFlags = nNumElements;

	//success
	return true;
}

//appends an element onto the end of the element list
bool CLTANode::AppendElement(CLTANode* pElement)
{
	ASSERT(IsList());
	
	//resize the array
	CLTANode** pNewBuffer = new CLTANode* [GetNumElements() + 1];

	//make sure it worked
	if(pNewBuffer == NULL)
	{
		return false;
	}

	//copy over the old data
	memcpy(pNewBuffer, m_pData, sizeof(CLTANode*) * GetNumElements());

	//add the new element onto the end
	pNewBuffer[GetNumElements()] = pElement;

	//delete the old buffer
	delete [] (CLTANode**)m_pData;

	//and set up the new buffer
	m_pData = pNewBuffer;
	m_nFlags++;

	return true;
}


//frees the memory associated with the node. If it is a list, it 
//will also free all the children nodes
void CLTANode::Free()
{
	if(IsAtom())
	{
		delete [] (char*)m_pData;
	}
	else
	{
		//first delete all the children
		for(uint32 nCurrElement = 0; nCurrElement < m_nFlags; nCurrElement++)
		{
			delete GetElement(nCurrElement);
		}

		//now clear the entire array
		delete [] (CLTANode**)m_pData;
	}

	m_nFlags	= 0;
	m_pData		= NULL;
}

//sets the value of the atom. Will not work if this already has children
bool CLTANode::SetValue(const char* pszValue, bool bString)
{
	return SetValue(pszValue, bString, strlen(pszValue));
}


//sets the value of the atom. Will not work if this already has children
bool CLTANode::SetValue(const char* pszValue, bool bString, uint32 nLen)
{
	//clean it up
	Free();

	//allocate the new buffer
	m_pData = new char [nLen + 1];

	//memory check
	if(m_pData == NULL)
	{
		return false;
	}

	//copy over the string
	strcpy((char*)m_pData, pszValue);

	//now set the flag appropriately
	if(bString)
	{
		m_nFlags = MASK_VALUE | MASK_STRING;
	}
	else
	{
		m_nFlags = MASK_VALUE;
	}

	return true;
}

bool CLTANode::SetValue(int32 nVal)
{
	//buffer used for setting the value
	static char pszBuffer[32];

	uint32 nLen = CLTAConverter::IntToStr(nVal, pszBuffer, 32);

	return SetValue(pszBuffer, false, nLen);
}

bool CLTANode::SetValue(double fVal)
{
	//buffer used for setting the value
	static char pszBuffer[32];

	uint32 nLen = CLTAConverter::RealToStr(fVal, pszBuffer, 32);

	return SetValue(pszBuffer, false, nLen);
}

bool CLTANode::SetValue(bool bVal)
{
	//buffer used for setting the value
	static char pszBuffer[32];

	uint32 nLen = CLTAConverter::BoolToStr(bVal, pszBuffer, 32);

	return SetValue(pszBuffer, false, nLen);
}
