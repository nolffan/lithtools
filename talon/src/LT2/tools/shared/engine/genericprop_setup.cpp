
#include "bdefs.h"
#include "genericprop_setup.h"
#include "geomroutines.h"

// talon: the 2001 version (0x411f60..0x4121c0): sprintf and strncpy, and
// the rotation is set straight from the angles.


// talon: the 2001 file constructs an unused global (0x484e04) at startup,
// whose constructor (0x411fa0) fills an eight-byte table (0x484e08) that
// nothing reads, and whose destructor is empty (0x42dd40, registered with
// atexit at 0x411f80). It came from a header not found yet; the class, its
// table and the names are ours, written from the image (the stores in
// shipped order).
class CGPUnknown_484e04
{
public:
	CGPUnknown_484e04();
	~CGPUnknown_484e04();

	static uint8 s_Table[8];
};

// 0x411fa0 (not inline: shipped calls it from the initializer).
CGPUnknown_484e04::CGPUnknown_484e04()
{
	s_Table[7] = 0;
	s_Table[6] = 1;
	s_Table[2] = 2;
	s_Table[3] = 3;
	s_Table[1] = 4;
	s_Table[0] = 5;
	s_Table[4] = 6;
	s_Table[5] = 7;
}

// 0x42dd40 (an empty function the linker folded with another).
CGPUnknown_484e04::~CGPUnknown_484e04()
{
}

uint8 CGPUnknown_484e04::s_Table[8];
static CGPUnknown_484e04 g_GPUnknown_484e04;


static void _SetNumProp(GenericProp *pProp, float val)
{
	sprintf(pProp->m_String, "%f", val);

	pProp->m_Float = val;
	pProp->m_Long = (uint32)val;
	pProp->m_Bool = (LTBOOL)val;
}


void gp_Init(GenericProp *pGeneric)
{
	pGeneric->m_Vec.Init();
	pGeneric->m_Rotation.Init();
	pGeneric->m_String[0] = 0;
	pGeneric->m_Long = 0;
	pGeneric->m_Float = 0.0f;
	pGeneric->m_Bool = false;
}


void gp_InitString(GenericProp *pGeneric, const char *pString)
{
	gp_Init(pGeneric);
	
	sscanf(pString, "%f %f %f", &pGeneric->m_Vec.x, &pGeneric->m_Vec.y, &pGeneric->m_Vec.z);

	if(stricmp(pString, "true") == 0)
	{
		_SetNumProp(pGeneric, 1.0f);
	}
	else if(stricmp(pString, "false") == 0)
	{
		_SetNumProp(pGeneric, 0.0f);
	}
	else
	{
		_SetNumProp(pGeneric, (float)atof(pString));
	}

	strncpy(pGeneric->m_String, pString, MAX_GP_STRING_LEN);
}


void gp_InitVector(GenericProp *pGeneric, LTVector *pVec)
{
	gp_Init(pGeneric);
	sprintf(pGeneric->m_String, "%f %f %f", pVec->x, pVec->y, pVec->z);
	pGeneric->m_Vec = *pVec;
	pGeneric->m_Color = *pVec;
}


void gp_InitFloat(GenericProp *pGeneric, float val)
{
	gp_Init(pGeneric);
	_SetNumProp(pGeneric, val);
}


void gp_InitRotation(GenericProp *pGeneric, LTVector *pAngles)
{
	gp_Init(pGeneric);

	pGeneric->m_Vec = *pAngles;
	gr_EulerToRotation(VEC_EXPAND(pGeneric->m_Vec), &pGeneric->m_Rotation);
}

