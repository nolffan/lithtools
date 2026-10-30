//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// OptionsRun.cpp: implementation of the COptionsRun class.
//
//////////////////////////////////////////////////////////////////////

#include "bdefs.h"
#include "dedit.h"
#include "optionsrun.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

COptionsRun::COptionsRun()
{

}

COptionsRun::~COptionsRun()
{

}

/************************************************************************/
// Load the options
BOOL COptionsRun::Load()
{
	// Load the executable path
	m_sExecutable=GetStringValue("ExecutablePath", "lithtech.exe");

	// Load the working directory
	m_sWorkingDirectory=GetStringValue("WorkingDirectory", "..\\Game");

	// Load the program arguments
	m_sProgramArguments=GetStringValue("ProgramArguments", "-rez AVP2 +runworld %WorldName%");

	return TRUE;
}

/************************************************************************/
// Save the options
BOOL COptionsRun::Save()
{
	// Save the executable path
	SetStringValue("ExecutablePath", m_sExecutable);

	// Save the working directory
	SetStringValue("WorkingDirectory", m_sWorkingDirectory);

	// Save the program arguments
	SetStringValue("ProgramArguments", m_sProgramArguments);

	return TRUE;
}
