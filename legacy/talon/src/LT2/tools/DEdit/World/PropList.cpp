//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : PropList.cpp
//
//	PURPOSE	  : Implements CPropList and related classes.
//
//	CREATED	  : December 1 1996
//
//
//------------------------------------------------------------------

// Includes....
#include "bdefs.h"
#include "proplist.h"
#include "genericprop_setup.h"
#include "geomroutines.h"
// talon: what Build 69's adds, for its functions below
#include "ltamgr.h"


// ----------------------------------------------------------------------- //
// CBaseProp.
// ----------------------------------------------------------------------- //

CBaseProp::CBaseProp( const char *pName )
{
	m_PropFlags = 0;

	m_pPropDef = NULL;

	if( pName )
		strncpy( m_Name, pName, MAX_PROPNAME_LEN );
	else
		m_Name[0] = 0;
}

void CBaseProp::Copy( CBaseProp *pOther )
{ 
	m_pPropDef = pOther->m_pPropDef; 
}

// ----------------------------------------------------------------------- //
// CStringProp.
// ----------------------------------------------------------------------- //

CStringProp::CStringProp( const char *pName ) : CBaseProp(pName)
{
	m_String[0] = 0;
	m_Type = PT_STRING;
}

void CStringProp::LoadData( CAbstractIO &file )
{
	file.ReadString( m_String, MAX_STRINGPROP_LEN );
}

void CStringProp::LoadDataLTA( CLTANode* pNode )
{
	strncpy(m_String,
		PairCdr( CLTAUtil::ShallowFindList(pNode, "data" ) ),
			MAX_STRINGPROP_LEN);
}


void CStringProp::SaveData( CAbstractIO &file )
{
	if( strlen( m_String ) < MAX_STRINGPROP_LEN )
		file.WriteString( m_String );
	else
		file.WriteString( "" );
}

void CStringProp::Copy( CBaseProp *pOther )
{
	CBaseProp::Copy(pOther);
	memcpy( m_String, ((CStringProp*)pOther)->m_String, MAX_STRINGPROP_LEN );
}

void CStringProp::SetupString(char *pStr)
{
	SAFE_STRCPY(m_String, pStr);
}

void CStringProp::SetString(const char *lpszString)
{
	if (!lpszString)
	{
		return;
	}

	int nLength=sizeof(m_String);

	if (nLength > 0)
	{
		strncpy(m_String, lpszString, sizeof(m_String));	
		m_String[sizeof(m_String)-1]='\0';
	}
}

void CStringProp::SetupGenericProp(GenericProp *pProp)
{
	gp_InitString(pProp, m_String);
}



// ----------------------------------------------------------------------- //
// CVectorProp.
// ----------------------------------------------------------------------- //

CVectorProp::CVectorProp( const char *pName ) : CBaseProp(pName)
{
	m_Vector.Init( 0.0f, 0.0f, 0.0f );
	m_Type = PT_VECTOR;
}

void CVectorProp::LoadData( CAbstractIO &file )
{
	file >> m_Vector.x >> m_Vector.y >> m_Vector.z;
}

void CVectorProp::LoadDataLTA( CLTANode* pNode )
{
	CLTANode* pVectorNode = PairCdrNode(CLTAUtil::ShallowFindList( CLTAUtil::ShallowFindList(pNode, "data" ), "vector"));
	if( pVectorNode && pVectorNode->GetNumElements() == 3 )
	{
		m_Vector.x = GetFloat(pVectorNode->GetElement(0));
		m_Vector.y = GetFloat(pVectorNode->GetElement(1));
		m_Vector.z = GetFloat(pVectorNode->GetElement(2));
	}

}


void CVectorProp::SaveData( CAbstractIO &file )
{
	file << m_Vector.x << m_Vector.y << m_Vector.z;
}

void CVectorProp::Copy( CBaseProp *pOther )
{
	CBaseProp::Copy(pOther);
	m_Vector = ((CVectorProp*)pOther)->m_Vector;
}

// talon: 0x4448b0 zeroes the vector with VEC_INIT's chained assignment
void CVectorProp::SetupString(char *pStr)
{
	VEC_INIT(m_Vector);
	sscanf(pStr, "%f %f %f", &m_Vector.x, &m_Vector.y, &m_Vector.z);
}

void CVectorProp::SetupGenericProp(GenericProp *pProp)
{
	gp_InitVector(pProp, &m_Vector);
}



// ----------------------------------------------------------------------- //
// CRotationProp.
// ----------------------------------------------------------------------- //

CRotationProp::CRotationProp(const char *pName) : CBaseProp(pName)
{
	m_Type = PT_ROTATION;
}

void CRotationProp::LoadData( CAbstractIO &file )
{
	float fDummy;

	file >> m_EulerAngles.x;
	file >> m_EulerAngles.y;
	file >> m_EulerAngles.z;
	file >> fDummy; // Used to be stored as DRotations so we have to leave room.
}

void CRotationProp::LoadDataLTA( CLTANode* pNode )
{
	CLTANode* pRotationNode = PairCdrNode(CLTAUtil::ShallowFindList( CLTAUtil::ShallowFindList(pNode, "data" ), "eulerangles"));
	if( pRotationNode && pRotationNode->GetNumElements() == 3 )
	{
		m_EulerAngles.x = GetFloat(pRotationNode->GetElement(0));
		m_EulerAngles.y = GetFloat(pRotationNode->GetElement(1));
		m_EulerAngles.z = GetFloat(pRotationNode->GetElement(2));
	}
}


void CRotationProp::SaveData( CAbstractIO &file )
{
	file << m_EulerAngles.x;
	file << m_EulerAngles.y;
	file << m_EulerAngles.z;
	file << (float)0.0f; // Used to be stored as DRotations so we have to leave room.
}

void CRotationProp::Copy(CBaseProp *pOther)
{
	CBaseProp::Copy(pOther);
	m_EulerAngles = ((CRotationProp*)pOther)->GetEulerAngles();
}

void CRotationProp::SetupString(char *pStr)
{
	sscanf(pStr, "%f %f %f", &m_EulerAngles.x, &m_EulerAngles.y, &m_EulerAngles.z);
}

void CRotationProp::SetupGenericProp(GenericProp *pProp)
{
	gp_InitRotation(pProp, &m_EulerAngles);
}



// ----------------------------------------------------------------------- //
// CColorProp.
// ----------------------------------------------------------------------- //

CColorProp::CColorProp( const char *pName ) : CVectorProp(pName)
{
	m_Type = PT_COLOR;
}



// ----------------------------------------------------------------------- //
// CRealProp.
// ----------------------------------------------------------------------- //

CRealProp::CRealProp( const char *pName ) : CBaseProp(pName)
{
	m_Value = 0.0f;
	m_Type = PT_REAL;
}

void CRealProp::LoadData( CAbstractIO &file )
{
	file >> m_Value;
}

void CRealProp::LoadDataLTA( CLTANode* pNode )
{
	m_Value = GetFloat( PairCdrNode(CLTAUtil::ShallowFindList(pNode, "data" )) );
}


void CRealProp::SaveData( CAbstractIO &file )
{
	file << m_Value;
}

void CRealProp::Copy( CBaseProp *pOther )
{
	CBaseProp::Copy(pOther);
	m_Value = ((CRealProp*)pOther)->m_Value;
}

void CRealProp::SetupString(char *pStr)
{
	m_Value = (float)atof(pStr);
}

void CRealProp::SetupGenericProp(GenericProp *pProp)
{
	gp_InitFloat(pProp, m_Value);
}



// ----------------------------------------------------------------------- //
// CBoolProp.
// ----------------------------------------------------------------------- //

CBoolProp::CBoolProp( const char *pName ) : CBaseProp(pName)
{
	m_Value = 0;
	m_Type = PT_BOOL;
}

void CBoolProp::LoadData( CAbstractIO &file )
{
	file >> m_Value;
}

void CBoolProp::LoadDataLTA( CLTANode* pNode )
{
	m_Value = GetBool( PairCdrNode(CLTAUtil::ShallowFindList(pNode, "data" )) );
}


void CBoolProp::SaveData( CAbstractIO &file )
{
	file << m_Value;
}

void CBoolProp::Copy( CBaseProp *pOther )
{
	CBaseProp::Copy(pOther);
	m_Value = ((CBoolProp*)pOther)->m_Value;
}

void CBoolProp::SetupString(char *pStr)
{
	m_Value = (char)atof(pStr);
}

void CBoolProp::SetupGenericProp(GenericProp *pProp)
{
	gp_InitFloat(pProp, (float)m_Value);
}




// ----------------------------------------------------------------------- //
//      ROUTINE:        CPropList functions.
// ----------------------------------------------------------------------- //

CPropList::CPropList()
{
}


CPropList::~CPropList()
{
	Term();
}


void CPropList::Term()
{
	DeleteAndClearArray(m_Props);
}


CBaseProp* CPropList::GetProp(const char *pName, BOOL bAssert, uint32 *pIndex)
{
	for(DWORD i=0; i < m_Props; i++)
	{
		if( strcmp(m_Props[i]->m_Name, pName) == 0 )
		{
			if(pIndex)
				*pIndex = i;
			
			return m_Props[i];
		}
	}

	if( bAssert )
	{
		ASSERT( FALSE );
	}

	return NULL;
}

// Find prop with only part of it's name
CBaseProp* CPropList::GetMatchingProp(const char *pName, int type)
{
	CBaseProp	*pRet;

	if(!(pRet = GetProp(pName)))
		return NULL;

	if(pRet->m_Type == type)
		return pRet;

	return NULL;
}


CBaseProp* CPropList::GetPropByFlagsAndType(uint32 flags, int type, uint32 *index)
{
	DWORD i;
	CBaseProp *pProp;

	for(i=0; i < m_Props; i++)
	{
		pProp = m_Props[i];
	
		// 2001: any of the flags (0x44db5b)
		if(pProp->m_Type == type && (pProp->m_PropFlags & flags))
		{
			if(index)
				*index = i;

			return pProp;
		}
	}

	return NULL;
}


void CPropList::CopyValues( CPropList *pOtherList )
{
	CBaseProp	*pProp, *pOther;

	Term();
	m_Props.SetSize( pOtherList->m_Props );
	for( DWORD i=0; i < pOtherList->m_Props; i++ )
	{
		pOther = pOtherList->m_Props[i];
		pProp = pOther->CreateSameKind();
		m_Props[i] = pProp;

		ASSERT( pProp );
		
		pProp->m_Type = pOther->m_Type;
		strncpy( pProp->m_Name, pOther->m_Name, MAX_PROPNAME_LEN );
		pProp->m_PropFlags = pOther->m_PropFlags;
		pProp->Copy( pOther );
	}
}

// Copies the matching values (same name and type) from one list to another.
void CPropList::CopyMatchingValues(CPropList *pSourceList)
{
	// Loop through the property list copying common values
	int i;
	for (i=0; i < pSourceList->GetSize(); i++)
	{
		// The property for this index
		CBaseProp *pSrcProp=pSourceList->GetAt(i);

		// Find the matching property for this index
		CBaseProp *pMatchingProp=GetMatchingProp(pSrcProp->GetName(), pSrcProp->GetType());

		// Check to see if we found a property
		if (pMatchingProp)
		{
			// Copy the property from the old class to the new class
			pMatchingProp->Copy(pSrcProp);
		}
	}
}

bool CPropList::SameAs(CPropList* pPropList)
{
	uint32 mysize = GetSize();
	uint32 othersize = pPropList->GetSize();
	if( mysize == othersize )
	{
		for( uint32 i = 0; i < mysize; i++ )
		{
			if( !m_Props[i]->SameAs(pPropList->m_Props[i]) )
			{
				return false;
			}

			// 2001 (0x44dc8c): the values by type
			CBaseProp *pProp = m_Props[i];
			switch( pProp->m_Type )
			{
				case PT_STRING:
					if( !((CStringProp*)pProp)->SameAs((CStringProp*)pPropList->GetAt(i)) )
						return false;
					break;

				case PT_VECTOR:
				case PT_COLOR:
					if( !((CVectorProp*)pProp)->SameAs((CVectorProp*)pPropList->GetAt(i)) )
						return false;
					break;

				case PT_REAL:
				case PT_FLAGS:
				case PT_LONGINT:
					if( !((CRealProp*)pProp)->SameAs((CRealProp*)pPropList->GetAt(i)) )
						return false;
					break;

				case PT_BOOL:
					if( !((CBoolProp*)pProp)->SameAs((CBoolProp*)pPropList->GetAt(i)) )
						return false;
					break;

				case PT_ROTATION:
					if( !((CRotationProp*)pProp)->SameAs((CRotationProp*)pPropList->GetAt(i)) )
						return false;
					break;
			}
		}
	}
	else
	{
		return false;
	}

	return true;
}

CPropListContainer::CPropListContainer()
{
	Init();
}


void CPropListContainer::Init()
{
	m_propList.Term();
	m_propList.Init();
}


uint32 CPropListContainer::AddPropList(CPropList* propList, bool alwaysAdd)
{
	const uint32 size = m_propList.GetSize();

	// don't look for matching properties if alwaysAdd is true
	if( !alwaysAdd )
	{
		for( uint32 i = 0; i < size; i++ )
		{
			if( m_propList[i]->SameAs(propList) )
				return i;
		}
	}

	m_propList.Push(propList);
	return size;
}


void CPropListContainer::UnloadFromMemory()
{
	
	uint32 size = m_propList.GetSize();
	for( uint32 i = 0; i < size; i++ )
	{
		delete m_propList[i];
	}
	Init();
	
}


