//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : Navigator.cpp
//
//	PURPOSE	  : Implements the CNavigator class.
//
//	CREATED	  : October 3 1996
//
//
//------------------------------------------------------------------

// Includes....
#include "bdefs.h"
#include "navigator.h"
#include "oldtypes.h"

// talon: the 2001 one (0x424b40..0x425490): no LTA files; the look-at
// point is saved too, and a description is its length and its characters.

/***************************************************************/
// CNavigator Class
/***************************************************************/

void CNavigator::UpdateViewerDistance()
{
	m_LookAtDist = (Pos() - m_LookAt).Mag();
}

void CNavigator::LookAt(LTVector vPt)
{
	m_LookAt = vPt;
	UpdateViewerDistance();
	UpdateLooking();
}

/*********************************************************/
void CNavigator::UpdateLooking()
{
#ifdef DIRECTEDITOR_BUILD
	// talon: DEdit's 2001 (0x43b52f): no collinearity test, and the
	// forward vector normalized last.  The processors' objects keep Build
	// 69's.
	Forward() = m_LookAt - Pos();

	Up( ).Init( 0.0f, 1.0f, 0.0f );

	MakeRight();
	MakeUp();

	Right().Norm();
	Up().Norm();
	Forward().Norm();
#else
	Forward() = m_LookAt - Pos();
	Forward().Norm();

	Up( ).Init( 0.0f, 1.0f, 0.0f );

	//make sure that up is not colinnear with the forward
	if(Forward().Dot( Up() ) > 0.9f)
	{
		Up().Init(1.0f, 0.0f, 0.0f);
	}

	MakeRight();
	MakeUp();

	Right().Norm();
	Up().Norm();
#endif

	// Keep them the correct distance from the lookat point.
	Pos() = m_LookAt - Forward() * m_LookAtDist;
}


BOOL CNavigator::LoadTBW(CAbstractIO& InFile)
{
	InFile >> m_Up.x >> m_Up.y >> m_Up.z;
	InFile >> m_Forward.x >> m_Forward.y >> m_Forward.z;
	InFile >> m_Right.x >> m_Right.y >> m_Right.z;
	InFile >> m_Position.x >> m_Position.y >> m_Position.z;
	InFile >> m_LookAt.x >> m_LookAt.y >> m_LookAt.z;
	InFile >> m_LookAtDist;
	
	return TRUE;
}

BOOL CNavigator::SaveTBW(CAbstractIO& OutFile)
{
	OutFile << m_Up.x;
	OutFile << m_Up.y;
	OutFile << m_Up.z;
	OutFile << m_Forward.x;
	OutFile << m_Forward.y;
	OutFile << m_Forward.z;
	OutFile << m_Right.x;
	OutFile << m_Right.y;
	OutFile << m_Right.z;
	OutFile << m_Position.x;
	OutFile << m_Position.y;
	OutFile << m_Position.z;
	OutFile << m_LookAt.x;
	OutFile << m_LookAt.y;
	OutFile << m_LookAt.z;
	OutFile << m_LookAtDist;
	
	return TRUE;
}


/*
( up 0.000000 1.000000 0.000000 )
( forward 0.000000 0.000000 1.000000 )
( right 1.000000 0.000000 0.000000 )
( position 0.000000 0.000000 -6144.000000 )
( lookat 0.000000 0.000000 0.000000 )
( lookatdist -0.001327 )
*/

			

/***************************************************************/
// CNavigatorPosItem Class
/***************************************************************/

// Constructor
CNavigatorPosItem::CNavigatorPosItem()
{
	m_lpszDescription=NULL;

	SetDescription("\0");
}

// Destructor
CNavigatorPosItem::~CNavigatorPosItem()
{
	Term();
}

/*********************************************************/
// Termination
void CNavigatorPosItem::Term()
{
	// Delete the navigator items
	unsigned int i;
	for (i=0; i < m_NavigatorArray.GetSize(); i++)
	{
		delete m_NavigatorArray[i];
	}
	m_NavigatorArray.SetSize(0);

	// Delete the description
	if (m_lpszDescription)
	{
		delete []m_lpszDescription;
		m_lpszDescription=NULL;
	}
}

// Set the description
void CNavigatorPosItem::SetDescription(char *lpszDescription)
{
	// Delete the description
	if (m_lpszDescription)
	{
		delete []m_lpszDescription;
		m_lpszDescription=NULL;
	}

	// Copy the description
	if (lpszDescription)
	{
		int nBufferSize=strlen(lpszDescription)+1;
		m_lpszDescription=new char[nBufferSize];
		strcpy(m_lpszDescription, lpszDescription);
	}
}



BOOL CNavigatorPosItem::LoadTBW(CAbstractIO& InFile)
{
	// Reset this class
	Term();

	uint32 nNumNav = 0;
	InFile >> nNumNav;

	for(uint32 nCurrNav = 0; nCurrNav < nNumNav; nCurrNav++)
	{
		CNavigator *pNavigator = new CNavigator;
		pNavigator->LoadTBW(InFile);
		m_NavigatorArray.Add(pNavigator);
	}

	// Load the description
	uint32 nLen = 0;
	InFile >> nLen;
	if(nLen > 0)
	{
		char *pszDescription = new char[nLen+1];
		memset(pszDescription, 0, nLen+1);
		InFile.Read(pszDescription, nLen);
		SetDescription(pszDescription);
		delete [] pszDescription;
	}
	else
	{
		SetDescription("");
	}
	
	return TRUE;
}

BOOL CNavigatorPosItem::SaveTBW(CAbstractIO& OutFile)
{
	//save each navigator
	OutFile << m_NavigatorArray.GetSize();

	for (uint32 i=0; i < m_NavigatorArray.GetSize(); i++)
	{
		m_NavigatorArray[i]->SaveTBW(OutFile);
	}

	// Save the description
	char *lpszDescription = GetDescription();
	if(lpszDescription)
	{
		uint32 nLen = strlen(lpszDescription);
		OutFile << nLen;
		OutFile.Write(lpszDescription, nLen);
	}
	else
	{
		OutFile << (uint32)0;
	}

	return TRUE;
}


/***************************************************************/
// CNavigatorPosArray Class
/***************************************************************/

BOOL CNavigatorPosArray::LoadTBW(CAbstractIO& InFile)
{
	// Remove all of the items
	uint32 i;
	for (i=0; i < GetSize(); i++)
	{
		delete GetAt(i);
	}
	SetSize(0);

	// Load the number of items
	uint32 nNumPos = 0;
	InFile >> nNumPos;

	// Load each item
	for (i = 0; i < nNumPos; i++)
	{
		CNavigatorPosItem *pItem = new CNavigatorPosItem;
		pItem->LoadTBW(InFile);
		Add(pItem);
	}

	return TRUE;
}

BOOL CNavigatorPosArray::SaveTBW(CAbstractIO& OutFile)
{
	//write out the size
	OutFile << GetSize();

	for(uint32 nCurrPos = 0; nCurrPos < GetSize(); nCurrPos++)
	{
		GetAt(nCurrPos)->SaveTBW(OutFile);
	}

	return TRUE;
}

#ifdef DIRECTEDITOR_BUILD
// talon: Build 69's LTA loading and saving, DEdit's alone (the 2001 one
// writes "navigator", "description", "up", "forward", ... at 0x0043b71c);
// the processors have none.
#include "ltamgr.h"
#include "ltasaveutils.h"

/*
( up 0.000000 1.000000 0.000000 )
( forward 0.000000 0.000000 1.000000 )
( right 1.000000 0.000000 0.000000 )
( position 0.000000 0.000000 -6144.000000 )
( lookat 0.000000 0.000000 0.000000 )
( lookatdist -0.001327 )
*/

BOOL CNavigator::LoadLTA(CLTANode* pParseNode)
{
	// Load the orientation vectors
	CLTANode* pTemp = CLTAUtil::ShallowFindList(pParseNode, "up");
	if( pTemp ) {
		//file >> m_Up.x;			file >> m_Up.y;			file >> m_Up.z;
		m_Up.x = GetFloat(pTemp->GetElement(1));
		m_Up.y = GetFloat(pTemp->GetElement(2));
		m_Up.z = GetFloat(pTemp->GetElement(3));
	}
	pTemp = CLTAUtil::ShallowFindList(pParseNode, "forward");
	if( pTemp ){
		// file >> m_Forward.x;	file >> m_Forward.y;	file >> m_Forward.z;
		m_Forward.x = GetFloat(pTemp->GetElement(1));
		m_Forward.y = GetFloat(pTemp->GetElement(2));
		m_Forward.z = GetFloat(pTemp->GetElement(3));
	}
	pTemp = CLTAUtil::ShallowFindList(pParseNode, "right");
	if( pTemp ){
		// file >> m_Right.x;		file >> m_Right.y;		file >> m_Right.z;	
		m_Right.x = GetFloat(pTemp->GetElement(1));
		m_Right.y = GetFloat(pTemp->GetElement(2));
		m_Right.z = GetFloat(pTemp->GetElement(3));
	}
	
	// Load the position
	pTemp = CLTAUtil::ShallowFindList(pParseNode, "position");
	if( pTemp ) {
		// file >> m_Position.x;	file >> m_Position.y;	file >> m_Position.z;
		m_Position.x = GetFloat(pTemp->GetElement(1));
		m_Position.y = GetFloat(pTemp->GetElement(2));
		m_Position.z = GetFloat(pTemp->GetElement(3));
	}

	// Load the look-at vector
	pTemp = CLTAUtil::ShallowFindList(pParseNode, "lookat");
	if( pTemp ) {
		// file >> m_LookAt.x;		file >> m_LookAt.y;		file >> m_LookAt.z;
		m_LookAt.x = GetFloat(pTemp->GetElement(1));
		m_LookAt.y = GetFloat(pTemp->GetElement(2));
		m_LookAt.z = GetFloat(pTemp->GetElement(3));
	}

	// Load the look-at distance
	pTemp = CLTAUtil::ShallowFindList(pParseNode, "lookatdist");
	if( pTemp ) {
		// file >> m_LookAtDist;
		m_LookAtDist = GetFloat(pTemp->GetElement(1));
	}
	
	return TRUE;
}

/*********************************************************/
// Save
BOOL CNavigator::SaveLTA(CLTAFile* pFile, uint32 level)
{
	PrependTabs(pFile, level);
	pFile->WriteStr("( navigator");
		PrependTabs(pFile, level+1);
		pFile->WriteStrF("( up %f %f %f )", m_Up.x, m_Up.y, m_Up.z );
		PrependTabs(pFile, level+1);
		pFile->WriteStrF("( forward %f %f %f )", m_Forward.x, m_Forward.y, m_Forward.z );
		PrependTabs(pFile, level+1);
		pFile->WriteStrF("( right %f %f %f )", m_Right.x, m_Right.y, m_Right.z );
		PrependTabs(pFile, level+1);
		pFile->WriteStrF("( position %f %f %f )", m_Position.x, m_Position.y, m_Position.z );
		PrependTabs(pFile, level+1);
		pFile->WriteStrF("( lookat %f %f %f )", m_LookAt.x, m_LookAt.y, m_LookAt.z );
		PrependTabs(pFile, level+1);
		pFile->WriteStrF("( lookatdist %f )", m_LookAtDist );

	PrependTabs(pFile, level);
	pFile->WriteStr(")");

	return TRUE;
}

/*********************************************************/
// Load LTA
BOOL CNavigatorPosItem::LoadLTA(CLTANode* pParseNode)
{
	// Reset this class
	Term();

	// Load each navigator
	CLTANodeIterator NodeIter(pParseNode);
	CLTANode* pNavNode;

	while((pNavNode = NodeIter.FindNextList("navigator")) != NULL)
	{
		CNavigator *pNavigator = new CNavigator;
		pNavigator->LoadLTA(pNavNode);
		m_NavigatorArray.Add(pNavigator);
	}

	// Load the length of the description string
	DWORD dwBufferSize=0;
	CLTANode*  pDescription = CLTAUtil::ShallowFindList(pParseNode,"description");
	const char* descString = GetString(PairCdrNode(pDescription));
	dwBufferSize = strlen(descString);
	// file >> dwBufferSize;

	if (dwBufferSize > 0)
	{
		// Load the description buffer
		char *lpszBuffer=new char[dwBufferSize+1];
		memset(lpszBuffer, NULL, dwBufferSize+1);

		// file.Read(lpszBuffer, dwBufferSize);
		strcpy(lpszBuffer,descString);

		SetDescription(lpszBuffer);
		delete []lpszBuffer;
	}
	else
	{
		SetDescription("\0");
	}
	
	return TRUE;
}

/*********************************************************/
// SaveLTA
BOOL CNavigatorPosItem::SaveLTA(CLTAFile* pFile, uint32 level)
{
	PrependTabs(pFile, level);
	pFile->WriteStr("( navigatorpositem");
		// Save each navigator
		unsigned int i;
		for (i=0; i < m_NavigatorArray.GetSize(); i++)
		{
			m_NavigatorArray[i]->SaveLTA(pFile, level+1);
		}
		// Save the length of the description string
		char *lpszDescription=GetDescription();
		if (lpszDescription)
		{
			PrependTabs(pFile, level+1);
			pFile->WriteStrF("( description \"%s\" )", lpszDescription);
		}
		else
		{
			PrependTabs(pFile, level+1);
			pFile->WriteStr("( description \"\" )");
		}
	PrependTabs(pFile, level);
	pFile->WriteStr(")");
	return TRUE;
}

/***************************************************************/
// CNavigatorPosArray Class
/***************************************************************/

// Load
BOOL CNavigatorPosArray::LoadLTA(CLTANode* pParseNode)
{
	// Remove all of the items
	unsigned int i;
	for (i=0; i < GetSize(); i++)
	{
		delete GetAt(i);
	}
	SetSize(0);


	// Load the number of items
	uint32 dwNumItems=0;
	dwNumItems = pParseNode->GetNumElements();

	// Load each item
	for (i=0; i < dwNumItems; i++)
	{
		CNavigatorPosItem *pItem=new CNavigatorPosItem;
		pItem->LoadLTA(pParseNode->GetElement(i));
		Add(pItem);
	}

	return TRUE;
}

/*********************************************************/
// SaveLTA
BOOL CNavigatorPosArray::SaveLTA(CLTAFile* pFile, uint32 level)
{
	PrependTabs(pFile, level);
	pFile->WriteStr("( navigatorposlist (");
		// Save each item
		unsigned int i;
		for (i=0; i < GetSize(); i++)
		{
			GetAt(i)->SaveLTA(pFile, level+1);		
		}
	PrependTabs(pFile, level);
	pFile->WriteStr(") )");

	return TRUE;
}
#endif // DIRECTEDITOR_BUILD
