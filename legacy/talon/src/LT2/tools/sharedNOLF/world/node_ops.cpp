//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//

#include "bdefs.h"
#include "editregion.h"
#include "node_ops.h"

// talon: NOLF's (0x431c00..0x4325e0): Build 69's SetupBrushProperties
// (0x431c00), no_DetachNode, no_AttachNode, no_InitializeNewNode
// (0x431ec0), no_CreateNewBrush (0x432100) and no_DestroyNode, which takes
// the node off the region's lists itself. The 2001 CEditRegion has no
// AddBrush/AddObject: no_InitializeNewNode adds to its lists directly
// (inline at 0x432006 and 0x431ee1). AvP2's processor has only the
// detach, attach and destroy.


BOOL g_bCheckNodes = TRUE;

#ifdef DIRECTEDITOR_BUILD
	#include "regiondoc.h"
	#include "edithelpers.h"
	#include "nodeview.h"
#endif



// LandExport takes FOREVER while debugging unless we remove the asserts.
#ifdef NODEOPS_NO_ASSERT
	#define NODEOPS_CHECK(x)
#else
	#define NODEOPS_CHECK(x) ASSERT(x)
#endif


void SetupBrushProperties(CEditBrush *pBrush, CEditRegion *pRegion)
{
	CBaseProp	*pProp;

	SetupWorldNode("Brush", pBrush, pRegion);

	pProp = pBrush->m_PropList.GetProp("BrushType");
	if(pProp && (pProp->m_Type == PT_LONGINT))
		((CRealProp*)pProp)->m_Value = 129.0f;

	pProp = pBrush->m_PropList.GetProp("FrictionCoefficient");
	if(pProp && (pProp->m_Type == PT_REAL))
		((CRealProp*)pProp)->m_Value = 1.0f;
}


void no_DetachNode(CEditRegion *pRegion, CWorldNode *pNode, BOOL bAttachToRoot)
{
	if(pNode->GetParent())
	{
		pNode->GetParent()->m_Children.RemoveAt(pNode);
	}

	if(bAttachToRoot)
	{
		pNode->SetParent(&pRegion->m_RootNode);
		
		#ifdef DIRECTEDITOR_BUILD
			GetNodeView()->SetNode(pNode);
		#endif
		
		pRegion->m_RootNode.m_Children.Append(pNode);
	}
	else
	{
		pNode->SetParent(NULL);
	}
}


// Attaches the node to the parent.
void no_AttachNode(CEditRegion *pRegion, CWorldNode *pChild, CWorldNode *pParent)
{
	if(!pParent)
		pParent = &pRegion->m_RootNode;

	no_DetachNode(pRegion, pChild, FALSE);
	pChild->SetParent(pParent);

	NODEOPS_CHECK(pParent->m_Children.FindElement(pChild) == BAD_INDEX);
	
	// Really slows down LandExport in debug so LandExport turns this option off.
	if(g_bCheckNodes)
	{
		NODEOPS_CHECK(pRegion->CheckNodes(pParent) == NULL);
	}

	pParent->m_Children.Append(pChild);

	if(g_bCheckNodes)
	{
		NODEOPS_CHECK(pRegion->CheckNodes(pParent) == NULL);
	}

#ifdef DIRECTEDITOR_BUILD
	// Set the node in the node view if we are working on the active region
	if (pRegion == GetActiveRegionDoc()->GetRegion())
	{
		GetNodeView( )->SetNode(pChild);
	}
#endif
}


void no_InitializeNewNode(CEditRegion *pRegion, CWorldNode *pNode, CWorldNode *pParent)
{
	if(pNode->GetType() == Node_Brush)
	{
		pRegion->m_Brushes.AddTail(pNode->AsBrush());
	}
	else if(pNode->GetType() == Node_Object)
	{
		pRegion->m_Objects.Append(pNode->AsObject());
	}

	// Set it up in the node view.
	no_AttachNode(pRegion, pNode, pParent);

	// Select it if it was selected.
	if(pNode->IsFlagSet(NODEFLAG_SELECTED))
		pRegion->SelectNode(pNode);
}


CEditBrush* no_CreateNewBrush(CEditRegion *pRegion, CWorldNode *pParent)
{
	CEditBrush *pBrush;
	
	pBrush = new CEditBrush;
	SetupBrushProperties(pBrush, pRegion);
	no_InitializeNewNode(pRegion, pBrush, pParent);
	return pBrush;
}


void no_DestroyNode(CEditRegion *pRegion, CWorldNode *pNode, BOOL bDeleteChildren)
{
	GPOS pos;
	LPOS lPos;
	uint32 index;

	if(pNode->IsFlagSet(NODEFLAG_SELECTED))
		pRegion->UnselectNode(pNode);

	if(pNode->IsFlagSet(NODEFLAG_PATH))
	{
		index = pRegion->m_PathNodes.FindElement(pNode);
		if(index != BAD_INDEX)
			pRegion->m_PathNodes.Remove(index);
	}

	// Either delete or reattach the children.
	if(bDeleteChildren)
	{
		for(pos=pNode->m_Children; pos; )
		{
			no_DestroyNode(pRegion, pNode->m_Children.GetNext(pos), TRUE);
		}
	}
	else
	{
		// Just attach the children to the parent node.
		for(pos=pNode->m_Children; pos; )
		{
			no_AttachNode(pRegion, pNode->m_Children.GetNext(pos), pNode->GetParent());
		}
	}
	
	// Detach from its parent.
	no_DetachNode(pRegion, pNode, FALSE);

	// Remove it from a CEditRegion list if it's in one.	
	if(pNode->GetType() == Node_Brush)
	{
		lPos = pRegion->m_Brushes.Find(pNode->AsBrush());
		if(lPos)
			pRegion->m_Brushes.RemoveAt(lPos);
	}
	else if(pNode->GetType() == Node_Object)
	{
		pRegion->m_Objects.Remove(pRegion->m_Objects.FindElement(pNode->AsObject()));
	}

	delete pNode;
}
