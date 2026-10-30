
// ------------------------------------------------------------------------
// modelcallbacks.cpp (the name is ours: nothing in the image names this
// module or its class)
// talon: 2001's, written from the image (0x433084..0x43311c, between
// modelallocations and pixelformat by name): CModelCallbackList, Model's
// member at 0x14.  Its ctor is the first call in Model's (0x42aa15), its
// Term the first in Model::Term's (0x42ae02).  Nothing in ModelEdit
// registers a callback; the registration and callback records are read
// here only as far as Notify walks them.
// ------------------------------------------------------------------------
#include "bdefs.h"
#include "model.h"

// talon: the module opens with std::ctype<unsigned short>::id's 21-byte
// initializer (0x433084), which only <xlocale> (via <string>) gives it.
#include <string>


// What Notify walks (0x4330f0..0x43310b): a registration has its callbacks
// at 0 and the next registration at 8; a callback is {fn, next}.
struct ModelCallback;

typedef void (*ModelCallbackFn)(CModelCallbackList *pList, void *pReg, uint32 a1, void *a2);

struct ModelCallback
{
	ModelCallbackFn	m_Fn;
	ModelCallback	*m_pNext;
};

struct ModelCallbackReg
{
	ModelCallback		*m_pCallbacks;
	void				*m_pUser;
	ModelCallbackReg	*m_pNext;
};


// 0x4330a5
CModelCallbackList::CModelCallbackList()
{
	Init(LTNULL);
}


// 0x4330b3 (a jump to Term)
CModelCallbackList::~CModelCallbackList()
{
	Term();
}


// 0x4330b8
LTBOOL CModelCallbackList::Init(void *pUser)
{
	m_pHead = LTNULL;
	m_pUser = pUser;
	return LTTRUE;
}


// 0x4330c8
LTRESULT CModelCallbackList::Term()
{
	uint32 val = 1;
	return Notify(0, &val);
}


// 0x4330e0
LTRESULT CModelCallbackList::Notify(uint32 a1, void *a2)
{
	ModelCallbackReg *pReg, *pNext;
	ModelCallback *pCallback;

	for(pReg=(ModelCallbackReg*)m_pHead; pReg; pReg=pNext)
	{
		pNext = pReg->m_pNext;

		for(pCallback=pReg->m_pCallbacks; pCallback; pCallback=pCallback->m_pNext)
		{
			pCallback->m_Fn(this, pReg, a1, a2);
		}
	}

	return LT_OK;
}
