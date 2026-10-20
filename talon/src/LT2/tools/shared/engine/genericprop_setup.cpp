
#include "bdefs.h"
#include "genericprop_setup.h"
#include "geomroutines.h"

// talon: the 2001 version (0x411f60..0x4121c0): sprintf and strncpy, and
// the rotation is set straight from the angles.


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

