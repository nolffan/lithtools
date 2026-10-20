#include "bdefs.h"
#include "preworld.h"
#include "pregeometry.h"
#include "preleaf.h"
#include "prelightmap.h"
#include "lightmap_compress.h"
#include "node.h"
#include "editobjects.h"
#include "brushtoworld.h"
#include "splitpoly.h"
#include "editbrush.h"
#include "editpoly.h"
#include "findworldmodel.h"
#include "proputils.h"
#include "processing.h"

// talon: 0x42d060..0x42d7f0 in the image.

CPreLeaf::CPreLeaf() : m_Unknown_6c(0)
{
	m_Unknown_80 = 0xFFFF;
	m_Flags = 0;
	m_Index = 0;
	m_Unknown_9c = 0;
}


CPreLeaf::~CPreLeaf()
{
	uint32 i;

	for(i=0; i < m_Portals; i++)
		DeletePoly(m_Portals[i]);

	DeleteAndClearArray(m_Unknown_6c);
}
