// OptionsPrimitives.cpp: implementation of the COptionsPrimitives class.
//
// Written from the image (0x43fdce..0x440039).
//
//////////////////////////////////////////////////////////////////////

#include "bdefs.h"
#include "dedit.h"
#include "optionsprimitives.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

COptionsPrimitives::COptionsPrimitives()
{
}

COptionsPrimitives::~COptionsPrimitives()
{

}

/************************************************************************/
// Load
BOOL COptionsPrimitives::Load()
{
	LoadSettings("Sphere", &m_Sphere);
	LoadSettings("Dome", &m_Dome);

	return TRUE;
}

/************************************************************************/
// Save
BOOL COptionsPrimitives::Save()
{
	SaveSettings("Sphere", &m_Sphere);
	SaveSettings("Dome", &m_Dome);

	// The image's Save returns FALSE (0x43fe87)
	return FALSE;
}

/************************************************************************/
// Saves one primitive's settings
void COptionsPrimitives::SaveSettings(CString sName, CPrimitiveSettings *pSettings)
{
	SetDWordValue(sName+"NumSides", pSettings->m_nNumSides);
	SetDWordValue(sName+"VerticalDivisions", pSettings->m_nVerticalDivisions);
	SetDWordValue(sName+"Radius", (DWORD)(pSettings->m_fRadius*1000.0f));
}

/************************************************************************/
// Loads one primitive's settings
void COptionsPrimitives::LoadSettings(CString sName, CPrimitiveSettings *pSettings)
{
	pSettings->m_nNumSides=GetDWordValue(sName+"NumSides", 6);
	pSettings->m_nVerticalDivisions=GetDWordValue(sName+"VerticalDivisions", 4);
	pSettings->m_fRadius=(float)GetDWordValue(sName+"Radius", 50000)/1000.0f;
}
