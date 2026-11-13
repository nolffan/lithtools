#include "bdefs.h"
#include "processing.h"
#include "gettextureflags.h"
#include "preworld.h"
#include "gettextureinfo.h"

// talon: the 2001 version (0x4131d0..0x413490): one texture per surface,
// and only its flags are taken.


// Applies texture flags to all the world's surfaces.  Returns the number
// of surfaces it applied flags to (in the last world model: the count
// starts over for each).
uint32 ApplyTextureFlagsToWorld(CGLinkedList<TInfo*> &theList, CPreMainWorld *pMainWorld)
{
	uint32		i, nFlagsGotten;
	GPOS		pos;
	CPreWorld	*pWorld;
	CPreSurface	*pSurface;
	TInfo		*pInfo;

	for(i=0; i < pMainWorld->m_WorldModels; i++)
	{
		pWorld = pMainWorld->m_WorldModels[i];

		nFlagsGotten = 0;
		for(pos=pWorld->m_Surfaces; pos; )
		{
			pSurface = pWorld->m_Surfaces.GetNext(pos);

			if(pSurface->m_pTextureName)
			{
				pInfo = FindTInfo(theList, pSurface->m_pTextureName);
				if(pInfo)
				{
					pSurface->m_TextureFlags = pInfo->m_Flags;
					++nFlagsGotten;
				}
			}
		}
	}

	return nFlagsGotten;
}


// talon: near (0x41f1d0, 164+54 bytes, its EH funclet 0x41f274): it has an
// EH frame (theList), and calls ApplyTextureFlagsToWorld (0x41f130, which
// matches) and CGLinkedList's constructor (0x41f360) and destructor
// (0x41f340) out of line, where ours inlines them. The shipped code tests
// GetTextureInfo's count with the found arm first (je to the messages,
// 0x41f213) and zeroes the count in that arm and adds Apply's result into
// it (0x41f218, 0x41f229), as written here. Built /Ob0 the instructions are
// then the shipped ones, but for the messages arm: the shipped code puts it
// after the EH funclet (0x41f27d), ours before it; without an else, the
// same.
uint32 GetTextureFlags(CPreMainWorld *pWorld)
{
	CGLinkedList<TInfo*>	theList;
	uint32					nFlagsGotten;

	if(GetTextureInfo(pWorld, theList))
	{
		nFlagsGotten = 0;
		nFlagsGotten += ApplyTextureFlagsToWorld(theList, pWorld);

		GDeleteAndRemoveElements(theList);
		DrawStatusText("Imported %d texture flags", nFlagsGotten);

		return nFlagsGotten;
	}
	else
	{
		DrawStatusText("** Couldn't find any textures.");
		DrawStatusText("** (Is project path set?)");
		return 0;
	}
}
