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

