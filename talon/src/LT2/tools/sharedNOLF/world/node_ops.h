//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//


// The NodeOps modules provides all the functions that are used to create,
// modify, and delete world nodes.

#ifndef __NODEOPS_H__
#define __NODEOPS_H__

	
	
	// Attach the node to a parent node.
	void no_AttachNode(CEditRegion *pRegion, CWorldNode *pNode, CWorldNode *pParent=NULL);

	// Detach the node.
	void no_DetachNode(CEditRegion *pRegion, CWorldNode *pNode, BOOL bAttachToRoot);



	// Removes all traces of the node's existance and burns its papers.
	// If bDeleteChildren is TRUE, then it calls no_DestroyNode for each child.
	// Otherwise, it attaches the children to the parent node.
	void no_DestroyNode(CEditRegion *pRegion, CWorldNode *pNode, BOOL bDeleteChildren);



#endif  // __NODEOPS_H__




