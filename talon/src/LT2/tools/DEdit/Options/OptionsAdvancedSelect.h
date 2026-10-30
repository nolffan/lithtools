// OptionsAdvancedSelect.h: interface for the COptionsAdvancedSelect class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPTIONSADVANCEDSELECT_H__A0C7D813_F35B_11D2_BE16_0060971BDC6D__INCLUDED_)
#define AFX_OPTIONSADVANCEDSELECT_H__A0C7D813_F35B_11D2_BE16_0060971BDC6D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "optionsbase.h"

class COptionsAdvancedSelect : public COptionsBase  
{
public:
	COptionsAdvancedSelect();
	virtual ~COptionsAdvancedSelect();

	// Loads/Saves the advanced select options to the registry
	BOOL		Load();
	BOOL		Save();

	////////////////////////////////
	// Access to member variables

	// Options for the name field
	BOOL		GetUseNameField()				{ return m_bUseNameField; }
	BOOL		GetMatchWholeName()				{ return m_bMatchWholeName; }
	CString		GetNameField()					{ return m_sNameField; }

	void		SetUseNameField(BOOL bUse)		{ m_bUseNameField=bUse; }
	void		SetMatchWholeName(BOOL bMatch)	{ m_bMatchWholeName=bMatch; }
	void		SetNameField(CString sName)		{ m_sNameField=sName; }

	// Options for the class field
	BOOL		GetUseClassField()				{ return m_bUseClassField; }
	CString		GetClassField()					{ return m_sClassField; }

	void		SetUseClassField(BOOL bUse)		{ m_bUseClassField=bUse; }
	void		SetClassField(CString sClass)	{ m_sClassField=sClass; }

	// The selection operation
	int			GetSelectionOperation()					{ return m_nSelectionOperation; }
	void		SetSelectionOperation(int nOperation)	{ m_nSelectionOperation=nOperation; }

	BOOL		GetShowResults()				{ return m_bShowResults; }
	void		SetShowResults(BOOL bShowResults) { m_bShowResults = bShowResults; }

protected:
	BOOL		m_bUseNameField;		// True if the name field is to be used
	BOOL		m_bMatchWholeName;		// True if the whole name should be matched (false for partial string)
	CString		m_sNameField;			// The contents of the name field

	BOOL		m_bUseClassField;		// True if the class field should be used
	CString		m_sClassField;			// The class field

	int			m_nSelectionOperation;	// The selection operation (0-select 1-deselect)

	BOOL		m_bShowResults;
};

#endif // !defined(AFX_OPTIONSADVANCEDSELECT_H__A0C7D813_F35B_11D2_BE16_0060971BDC6D__INCLUDED_)
