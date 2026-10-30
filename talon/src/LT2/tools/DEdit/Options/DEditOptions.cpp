//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// DEditOptions.cpp: implementation of the CDEditOptions class.
//
//////////////////////////////////////////////////////////////////////

#include "bdefs.h"
#include "deditoptions.h"
#include "optionsbase.h"
#include "optionsdisplay.h"
#include "optionsadvancedselect.h"
#include "optionsclassdialog.h"
#include "optionsprimitives.h"
#include "optionsrun.h"
#include "optionswarnings.h"
#include "optionsobjectbrowser.h"
#include "optionsgenerateuniquenames.h"
#include "optionsclipboard.h"
#include "optionswindows.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// 0x417fa2
CDEditOptions::CDEditOptions()
{
	m_pDisplayOptions				= new COptionsDisplay;
	m_pAdvancedSelectOptions		= new COptionsAdvancedSelect;
	m_pClassDialogOptions			= new COptionsClassDialog;
	m_pPrimitivesOptions			= new COptionsPrimitives;
	m_pRunOptions					= new COptionsRun;
	m_pWarningsOptions				= new COptionsWarnings;
	m_pObjectBrowserOptions			= new COptionsObjectBrowser;
	m_pGenerateUniqueNamesOptions	= new COptionsGenerateUniqueNames;
	m_pClipboardOptions				= new COptionsClipboard;
	m_pWindowsOptions				= new COptionsWindows;

	m_dwAutoSaveTime	= 5;
}

// 0x418180: each set deleted and cleared by name (Build 69 walks the array)
CDEditOptions::~CDEditOptions()
{
	if (m_pDisplayOptions)
	{
		delete m_pDisplayOptions;
		m_pDisplayOptions = NULL;
	}
	if (m_pAdvancedSelectOptions)
	{
		delete m_pAdvancedSelectOptions;
		m_pAdvancedSelectOptions = NULL;
	}
	if (m_pClassDialogOptions)
	{
		delete m_pClassDialogOptions;
		m_pClassDialogOptions = NULL;
	}
	if (m_pPrimitivesOptions)
	{
		delete m_pPrimitivesOptions;
		m_pPrimitivesOptions = NULL;
	}
	if (m_pRunOptions)
	{
		delete m_pRunOptions;
		m_pRunOptions = NULL;
	}
	if (m_pWarningsOptions)
	{
		delete m_pWarningsOptions;
		m_pWarningsOptions = NULL;
	}
	if (m_pObjectBrowserOptions)
	{
		delete m_pObjectBrowserOptions;
		m_pObjectBrowserOptions = NULL;
	}
	if (m_pGenerateUniqueNamesOptions)
	{
		delete m_pGenerateUniqueNamesOptions;
		m_pGenerateUniqueNamesOptions = NULL;
	}
	if (m_pClipboardOptions)
	{
		delete m_pClipboardOptions;
		m_pClipboardOptions = NULL;
	}
	if (m_pWindowsOptions)
	{
		delete m_pWindowsOptions;
		m_pWindowsOptions = NULL;
	}
}

/************************************************************************/
// Initialization

// 0x41825c
BOOL CDEditOptions::Init(CGenRegMgr *pRegMgr, CString sRegRoot)
{
	// Call the base class
	if (!COptionsBase::Init(pRegMgr, sRegRoot))
	{
		return FALSE;
	}

	if (m_pDisplayOptions)
	{
		m_pDisplayOptions->Init(m_pRegMgr, m_sRegRoot+"\\OptionsDisplay");
		m_optionsArray.Add(m_pDisplayOptions);
	}
	if (m_pAdvancedSelectOptions)
	{
		m_pAdvancedSelectOptions->Init(m_pRegMgr, m_sRegRoot+"\\AdvancedSelect");
		m_optionsArray.Add(m_pAdvancedSelectOptions);
	}
	if (m_pClassDialogOptions)
	{
		m_pClassDialogOptions->Init(m_pRegMgr, m_sRegRoot+"\\ClassDlg");
		m_optionsArray.Add(m_pClassDialogOptions);
	}
	if (m_pPrimitivesOptions)
	{
		m_pPrimitivesOptions->Init(m_pRegMgr, m_sRegRoot+"\\Primitives");
		m_optionsArray.Add(m_pPrimitivesOptions);
	}
	if (m_pRunOptions)
	{
		m_pRunOptions->Init(m_pRegMgr, m_sRegRoot+"\\Run");
		m_optionsArray.Add(m_pRunOptions);
	}
	if (m_pWarningsOptions)
	{
		m_pWarningsOptions->Init(m_pRegMgr, m_sRegRoot+"\\Warnings");
		m_optionsArray.Add(m_pWarningsOptions);
	}
	if (m_pObjectBrowserOptions)
	{
		m_pObjectBrowserOptions->Init(m_pRegMgr, m_sRegRoot+"\\ObjectBrowser");
		m_optionsArray.Add(m_pObjectBrowserOptions);
	}
	if (m_pGenerateUniqueNamesOptions)
	{
		m_pGenerateUniqueNamesOptions->Init(m_pRegMgr, m_sRegRoot+"\\GenerateUniqueNames");
		m_optionsArray.Add(m_pGenerateUniqueNamesOptions);
	}
	if (m_pClipboardOptions)
	{
		m_pClipboardOptions->Init(m_pRegMgr, m_sRegRoot+"\\Clipboard");
		m_optionsArray.Add(m_pClipboardOptions);
	}
	if (m_pWindowsOptions)
	{
		m_pWindowsOptions->Init(m_pRegMgr, m_sRegRoot+"\\Windows");
		m_optionsArray.Add(m_pWindowsOptions);
	}

	return TRUE;
}

/************************************************************************/
// Loads all of the options classes (0x418492)
BOOL CDEditOptions::Load()
{
	// Load the options
	for (int nCurrOption=0; nCurrOption < m_optionsArray.GetSize(); nCurrOption++)
	{
		m_optionsArray[nCurrOption]->Load();
	}
	
	// Load the autosave time
	m_dwAutoSaveTime	= GetDWordValue("AutoSaveTime", 5);

	return TRUE;
}

/************************************************************************/
// Saves all of the options classes (0x4184c7)
BOOL CDEditOptions::Save()
{
	// Save the options
	for (int nCurrOption=0; nCurrOption < m_optionsArray.GetSize(); nCurrOption++)
	{
		m_optionsArray[nCurrOption]->Save();
	}

	// Save the autosave time
	SetDWordValue("AutoSaveTime", m_dwAutoSaveTime);

	return TRUE;
}
