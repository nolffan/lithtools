
#include "bdefs.h"

// talon: Build 69's, matching 2001 (0x425490..0x425520), with SetParentNodes.

#include "node.h"
#include "prepoly.h"



void CNode::ClearPolyPostRemove()
{
	if(!IsValidNode(this))
		return;

	if(m_pPoly)
		m_pPoly->SetPostRemove(FALSE);

	m_Sides[0]->ClearPolyPostRemove();
	m_Sides[1]->ClearPolyPostRemove();
}


void CNode::ClearNodePostRemove()
{
	if(!IsValidNode(this))
		return;
	
	m_bPostRemove = FALSE;

	m_Sides[0]->ClearNodePostRemove();
	m_Sides[1]->ClearNodePostRemove();
}


// talon: 2001 (0x4254f0).
void CNode::SetParentNodes(CNode *pParent)
{
	if(!IsValidNode(this))
		return;

	m_pParentNode = pParent;

	m_Sides[0]->SetParentNodes(this);
	m_Sides[1]->SetParentNodes(this);
}
