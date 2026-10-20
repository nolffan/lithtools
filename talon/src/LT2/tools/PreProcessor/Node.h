
#ifndef __NODE_H__
#define __NODE_H__


	#include "preplane.h"
	class CPrePoly;
	class CPreLeaf;

	// Special node indices.
	#define NODE_IN		((CNode*)-1)
	#define NODE_OUT	((CNode*)-2)

	// The 2001 class (0x3c bytes; AddNode at 0x4050b0 constructs it). The
	// m_Unknown_* members are not used by BspGen; their modules will name them.
	class CNode : public CGLLNode
	{
		public:
						CNode()
						{
							m_pPlane				= NULL;
							m_pPoly					= NULL;
							m_pLeaf					= NULL;
							m_Unknown_28			= 0;
							m_pParentNode			= NULL;
							m_Sides[0] = m_Sides[1] = NULL;
							m_Unknown_1a			= 0;
						}

			const CPrePlane*	GetPlane() const	{ return m_pPlane; }
			const PVector&		Normal() const		{ ASSERT(GetPlane()); return GetPlane()->m_Normal; }
			PReal				Dist() const		{ ASSERT(GetPlane()); return GetPlane()->m_Dist; }

			void			ClearPolyPostRemove();
			void			ClearNodePostRemove();

			// Sets m_pParentNode down the tree (0x4254f0, the name is ours).
			void			SetParentNodes(CNode *pParent);

			uint32			m_Index;
			uint32			m_Unknown_0c[3];
			uint16			m_Unknown_18;
			uint16			m_Unknown_1a;

			const CPrePlane	*m_pPlane;
			CPrePoly		*m_pPoly;
			// The leaf on its front side, where that side is a leaf.
			CPreLeaf		*m_pLeaf;
			uint32			m_Unknown_28;
			CNode			*m_pParentNode;
			CNode			*m_Sides[2];

			uint8			m_bPostRemove;
	};

	typedef CGLinkedList<CNode*>	CNodeList;
	typedef CNode*					NODEREF;

	inline BOOL IsValidNode(NODEREF node) {return !!node && node!=NODE_IN && node!=NODE_OUT;}


#endif  // __NODE_H__

