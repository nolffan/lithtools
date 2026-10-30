//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//

// This file handles all the undo/redo operations for DEdit.

#ifndef __UNDO_MGR_H__
#define __UNDO_MGR_H__

	#include "bdefs.h"
	#include "worldnode.h"


	class CRegionDoc;
	class CNodeReplacement;
	class CEditAction;
	class CPreAction;


	#define EditActionList CGLinkedList<CEditAction*>
	#define PreActionList CGLinkedList<CPreAction*>

	
	class CUndoMgr
	{
		public:
	
						CUndoMgr();
						~CUndoMgr();

			BOOL		Init(CRegionDoc *pDoc, uint32 nUndos, uint32 nRedos);
			void		Term();

			// Call before you do an action.. this will add an undo for the action.
			void		PreAction(CPreAction *pAction);

			// You can consider one undo/redo as a list of actions.  They will be 
			// executed in the reverse order that they're in the list.
			void		PreActions(PreActionList *pActions, BOOL bClearList=FALSE);
			
			void		Undo();
			void		Redo();

			EditActionList* SetupNewUndoSlot();

			void		SetupReversePreActions(PreActionList *pMain, EditActionList *pReverse);
			void		SetupReverseEditActions(EditActionList *pMain, EditActionList *pReverse);
			void		ExecuteActions(EditActionList *pList);

			void		SetUndoLimit(uint32 nLimit) { Term(); Init(m_pDoc, nLimit, nLimit); }

		public:

			CRegionDoc					*m_pDoc;
			
			CMoArray<EditActionList*>	m_Undos;
			uint32						m_nUndos;

			CMoArray<EditActionList*>	m_Redos;
			uint32						m_nRedos;

			// talon: 2001's CUndoMgr is 0x38 bytes (CRegionDoc's m_UndoMgr at 0x2e4,
			// the next member at 0x31c): no node count or warning (Build 69's).
			BOOL						m_bCanAddActions;
	};


#endif  // __UNDO_MGR_H__




