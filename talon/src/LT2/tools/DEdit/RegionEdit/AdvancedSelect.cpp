//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// AdvancedSelect.cpp: implementation of the CAdvancedSelect class.
//
//////////////////////////////////////////////////////////////////////

#include "bdefs.h"
#include "dedit.h"
#include "advancedselect.h"
#include "regiondoc.h"
#include "edithelpers.h"
#include "nodeview.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAdvancedSelect::CAdvancedSelect()
{

}

CAdvancedSelect::~CAdvancedSelect()
{

}

/************************************************************************/
// Initialization
BOOL CAdvancedSelect::Init(CRegionDoc *pRegionDoc)
{
	if (pRegionDoc)
	{
		m_pRegionDoc=pRegionDoc;
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/************************************************************************/
// Performs a selection based on the current criteria
void CAdvancedSelect::Select(BOOL bIncludeChildren, BOOL bShowResults)
{
	// Return if there isn't any selection criteria
	if (m_classCriteria.GetSize() == 0 && m_nameCriteria.GetSize() == 0)
	{
		return;
	}
	
	int nNumFound = RecurseAndSelect(m_pRegionDoc->GetRegion()->GetRootNode(), TRUE, bIncludeChildren);

	// Make sure that the root node didn't get selected
	m_pRegionDoc->GetRegion()->UnselectNode(m_pRegionDoc->GetRegion()->GetRootNode());	
	m_pRegionDoc->NotifySelectionChange();

	// Display the results
	if (bShowResults)
	{
		char sResult[255];
		sprintf(sResult, "Selected %d objects", nNumFound);
		MessageBox(NULL, sResult, "Done", MB_OK);
	}
}

/************************************************************************/
// Performs an unselection based on the current criteria
void CAdvancedSelect::Unselect(BOOL bIncludeChildren, BOOL bShowResults)
{
	// Return if there isn't any selection criteria
	if (m_classCriteria.GetSize() == 0 && m_nameCriteria.GetSize() == 0)
	{
		return;
	}

	int nNumFound = RecurseAndSelect(m_pRegionDoc->GetRegion()->GetRootNode(), FALSE, bIncludeChildren);
	m_pRegionDoc->NotifySelectionChange();

	// Display the results
	if (bShowResults)
	{
		char sResult[255];
		sprintf(sResult, "Un-Selected %d objects", nNumFound);
		MessageBox(NULL, sResult, "Done", MB_OK);
	}
}

/************************************************************************/
// Recurses and selects nodes based on the criteria.
int CAdvancedSelect::RecurseAndSelect(CWorldNode *pParentNode, BOOL bSelect, BOOL bIncludeChildren)
{
	int nResult = 0;

	// Select the node if it meets the criteria
	if (IsPassClassCriteria(pParentNode) && IsPassNameCriteria(pParentNode))
	{
		++nResult;

		// Check to see if the selection should be done recursively
		if (bIncludeChildren)
		{
			// Select the node and its children
			if (bSelect)
			{
				m_pRegionDoc->GetRegion()->RecurseAndSelect(pParentNode);
				
				// Highlight the node in the node view
				GetNodeView()->HighlightNode(pParentNode);
			}
			else
			{
				m_pRegionDoc->GetRegion()->RecurseAndUnselect(pParentNode);		
			}
			return nResult;
		}
		else
		{
			// Select the node
			if (bSelect)
			{
				m_pRegionDoc->GetRegion()->SelectNode(pParentNode);		

				// Highlight the node in the node view
				GetNodeView()->HighlightNode(pParentNode);
			}
			else
			{
				m_pRegionDoc->GetRegion()->UnselectNode(pParentNode);		
			}
		}
	}

	// Recurse through the children
	GPOS pos=pParentNode->m_Children;
	while (pos)
	{
		nResult += RecurseAndSelect(pParentNode->m_Children.GetNext(pos), bSelect, bIncludeChildren);
	}

	return nResult;
}

/************************************************************************/
// Returns true if the node passes the class criteria.  True is also
// returned if there isn't any class criteria.
BOOL CAdvancedSelect::IsPassClassCriteria(CWorldNode *pNode)
{
	// Determine if this node meets the class criteria
	if (m_classCriteria.GetSize() > 0)
	{		
		int i;
		for (i=0; i < m_classCriteria.GetSize(); i++)
		{
			// Compare the class names
			if (stricmp(m_classCriteria[i].GetClass(), pNode->GetClassName()) == 0)
			{
				return TRUE;				
			}
		}
	}
	else
	{
		return TRUE;
	}

	// The class name was not found
	return FALSE;
}

/************************************************************************/
// Returns true if the node passes the name criteria.  True is also
// returned if there isn't any naming criteria.
BOOL CAdvancedSelect::IsPassNameCriteria(CWorldNode *pNode)
{
	// Get the name
	CString sName;

	// Special case container nodes and get their name from the label
	if (pNode->m_Type == Node_Null)
	{
		sName=pNode->GetNodeLabel();
	}
	else
	{
		CBaseProp *pProp = pNode->m_PropList.GetProp(g_NameName, FALSE);
		if(pProp && pProp->m_Type == PT_STRING)
		{
			sName=((CStringProp*)pProp)->m_String;
		}				
	}

	// Determine if the node meets the name criteria
	if (m_nameCriteria.GetSize() > 0)
	{		
		int i;
		for (i=0; i < m_nameCriteria.GetSize(); i++)
		{
			// Determine if we should just search on a partial string
			if (m_nameCriteria[i].IsPartialString())
			{				
				CString sSearch=m_nameCriteria[i].GetName();	// The string to search for

				sName.MakeUpper();
				sSearch.MakeUpper();

				// Search for the string
				if (sName.Find(sSearch) != -1)
				{
					return TRUE;					
				}
			}
			else
			{
				// Check to see if we are comparing on an empty string
				if (m_nameCriteria[i].GetName().GetLength() == 0)
				{
					if(sName.GetLength() == 0)
					{
						return TRUE;
					}
				}
				// Compare the two strings
				else if (stricmp(m_nameCriteria[i].GetName(), sName) == 0)
				{
					return TRUE;
				}
			}
		}
	}
	else
	{
		return TRUE;
	}

	// The name was not found
	return FALSE;
}

/************************************************************************/
// Adds a class type selection criteria
void CAdvancedSelect::AddClassCriteria(CString sClassType)
{
	// Setup the criteria data
	CClassCriteria criteria;
	criteria.SetClass(sClassType);

	// Add the critera
	m_classCriteria.Add(criteria);
}

/************************************************************************/
// Adds a class name criteria
void CAdvancedSelect::AddNameCriteria(CString sName, BOOL bPartialString)
{
	// Setup the criteria data
	CNameCriteria criteria;
	criteria.SetName(sName);
	criteria.SetPartialString(bPartialString);

	// Add the criteria
	m_nameCriteria.Add(criteria);
}

/************************************************************************/
// Clears the current selection criteria
void CAdvancedSelect::ClearCriteria()
{
	m_classCriteria.RemoveAll();
	m_nameCriteria.RemoveAll();
}

