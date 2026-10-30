// OptionsWarnings.h: interface for the COptionsWarnings class.
//
// 2001's warning options ("\\Warnings", 0x4401de..0x440217), written from
// the image: Build 69 has no such class. It has no options of its own.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPTIONSWARNINGS_H__INCLUDED_)
#define AFX_OPTIONSWARNINGS_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "optionsbase.h"

class COptionsWarnings : public COptionsBase  
{
public:
	COptionsWarnings();
	virtual ~COptionsWarnings();
};

#endif // !defined(AFX_OPTIONSWARNINGS_H__INCLUDED_)
