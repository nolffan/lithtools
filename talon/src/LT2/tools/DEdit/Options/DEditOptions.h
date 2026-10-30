//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// DEditOptions.h: interface for the CDEditOptions class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEDITOPTIONS_H__D15CECC2_F290_11D2_BE14_0060971BDC6D__INCLUDED_)
#define AFX_DEDITOPTIONS_H__D15CECC2_F290_11D2_BE14_0060971BDC6D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "genregmgr.h"
#include "optionsbase.h"
#include "afxtempl.h"

// The options classes
class COptionsDisplay;
class COptionsAdvancedSelect;
class COptionsClassDialog;
class COptionsPrimitives;
class COptionsRun;
class COptionsWarnings;
class COptionsObjectBrowser;
class COptionsGenerateUniqueNames;
class COptionsClipboard;
class COptionsWindows;

// talon: 2001's, 0x4c bytes (its constructor 0x417fa2, destructor 0x418180):
// ten option sets (Build 69 has thirteen, and not Primitives or Warnings)
// and the auto-save time.
class CDEditOptions : public COptionsBase
{
public:
	CDEditOptions();
	virtual ~CDEditOptions();

	// Initialization
	BOOL					Init(CGenRegMgr *pRegMgr, CString sRegRoot);

	// Load/Save
	BOOL					Load();
	BOOL					Save();

	// Access to the options classes
	COptionsDisplay				*GetDisplayOptions()				{ return m_pDisplayOptions; }
	COptionsAdvancedSelect		*GetAdvancedSelectOptions()			{ return m_pAdvancedSelectOptions; }
	COptionsClassDialog			*GetClassDialogOptions()			{ return m_pClassDialogOptions; }
	COptionsPrimitives			*GetPrimitivesOptions()				{ return m_pPrimitivesOptions; }
	COptionsRun					*GetRunOptions()					{ return m_pRunOptions; }
	COptionsWarnings			*GetWarningsOptions()				{ return m_pWarningsOptions; }
	COptionsObjectBrowser		*GetObjectBrowserOptions()			{ return m_pObjectBrowserOptions; }
	COptionsGenerateUniqueNames	*GetGenerateUniqueNamesOptions()	{ return m_pGenerateUniqueNamesOptions; }
	COptionsClipboard			*GetClipboardOptions()				{ return m_pClipboardOptions; }
	COptionsWindows				*GetWindowsOptions()				{ return m_pWindowsOptions; }

	// Access to the autosave time
	DWORD					GetAutoSaveTime() const			{ return m_dwAutoSaveTime; }
	void					SetAutoSaveTime(DWORD dwTime)	{ m_dwAutoSaveTime=dwTime; }

protected:

	COptionsDisplay				*m_pDisplayOptions;				// 0x0c
	COptionsAdvancedSelect		*m_pAdvancedSelectOptions;		// 0x10
	COptionsClassDialog			*m_pClassDialogOptions;			// 0x14
	COptionsPrimitives			*m_pPrimitivesOptions;			// 0x18
	COptionsRun					*m_pRunOptions;					// 0x1c
	COptionsWarnings			*m_pWarningsOptions;			// 0x20
	COptionsObjectBrowser		*m_pObjectBrowserOptions;		// 0x24
	COptionsGenerateUniqueNames	*m_pGenerateUniqueNamesOptions;	// 0x28
	COptionsClipboard			*m_pClipboardOptions;			// 0x2c
	COptionsWindows				*m_pWindowsOptions;				// 0x30

	// Array of the options classes
	CArray<COptionsBase *, COptionsBase *>	m_optionsArray;		// 0x34

	// The autosave time
	DWORD					m_dwAutoSaveTime;					// 0x48
};

#endif // !defined(AFX_DEDITOPTIONS_H__D15CECC2_F290_11D2_BE14_0060971BDC6D__INCLUDED_)
