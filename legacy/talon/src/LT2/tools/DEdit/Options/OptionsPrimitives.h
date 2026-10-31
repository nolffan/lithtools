// OptionsPrimitives.h: interface for the COptionsPrimitives class.
//
// 2001's primitive options ("\\Primitives", 0x43fdce..0x440039), written
// from the image: Build 69 has no such class. The class name follows the
// registry key; the member and helper names are ours.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPTIONSPRIMITIVES_H__INCLUDED_)
#define AFX_OPTIONSPRIMITIVES_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "optionsbase.h"

// The settings of one primitive
class CPrimitiveSettings
{
public:
	int		m_nNumSides;
	int		m_nVerticalDivisions;
	float	m_fRadius;
};

class COptionsPrimitives : public COptionsBase  
{
public:
	COptionsPrimitives();
	virtual ~COptionsPrimitives();

	// Load/Save
	BOOL	Load();
	BOOL	Save();

	// Access to the settings
	CPrimitiveSettings	*GetSphereSettings()	{ return &m_Sphere; }
	CPrimitiveSettings	*GetDomeSettings()		{ return &m_Dome; }

protected:
	// Saves/loads one primitive's settings, under the name prefix
	void	SaveSettings(CString sName, CPrimitiveSettings *pSettings);
	void	LoadSettings(CString sName, CPrimitiveSettings *pSettings);

protected:
	CPrimitiveSettings	m_Sphere;
	CPrimitiveSettings	m_Dome;
};

#endif // !defined(AFX_OPTIONSPRIMITIVES_H__INCLUDED_)
