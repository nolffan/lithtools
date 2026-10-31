//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// OptionsDisplay.h: interface for the COptionsDisplay class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OPTIONSDISPLAY_H__5E3D9E56_D8AA_11D2_BE02_0060971BDC6D__INCLUDED_)
#define AFX_OPTIONSDISPLAY_H__5E3D9E56_D8AA_11D2_BE02_0060971BDC6D__INCLUDED_

#include "regmgr.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "optionsbase.h"

class CDisplayColorItem
{
public:
	CDisplayColorItem() {}
	CDisplayColorItem(CString sKey, CString sName, COLORREF crDefault)
	{
		SetRegistryKey(sKey);
		SetDisplayName(sName);
		SetColor(crDefault);
	}

	// Load/Save this item
	BOOL		Load(COptionsBase &options);
	BOOL		Save(COptionsBase &options);

	// Access to member variables
	void		SetRegistryKey(CString sKey)		{ m_sRegistryKey=sKey; }
	void		SetDisplayName(CString sName)		{ m_sDisplayName=sName; }
	void		SetColor(COLORREF cr)				{ m_crColor=cr; }

	CString		GetRegistryKey()					{ return m_sRegistryKey; }
	CString		GetDisplayName()					{ return m_sDisplayName; }
	COLORREF	GetColor()							{ return m_crColor; }

protected:
	CString		m_sRegistryKey;
	CString		m_sDisplayName;
	COLORREF	m_crColor;
};

class COptionsDisplay : public COptionsBase  
{
public:
	// The color options
	enum
	{
		kColorBackground=0,
		kColorMajorGrid,
		kColorMinorGrid,
		kColorBrushDrawLines,
		kColorSelectedBrush,
		kColorObject,
		kColorSelectedObject,
		kColorObjectRadius,
		kColorObjectLink,
		kColorObjectFOV,
		kColorObjectDims,
		kColorPath,
		kColorSelectedFace,
		kColorSelectedEdge,
		kColorBoundingBox,
		kColorOrigin,
		kColorMarker,
		kColorHandleOutline,
		kColorHandleFill,
		kColorVertexOutline,
		kColorVertexFill,
		kColorTaggedVerticeOutline,
		kColorTaggedVerticeFill,
		kColorLeakFinder,
		kColorImmediateVertice,
		kNumColorOptions
	};

	// The vertex draw rules
	enum
	{
		kVertexDrawAll=0,
		kVertexDrawSelectedOnly
	};

public:
	COptionsDisplay();
	virtual ~COptionsDisplay();

	// Load/Save
	BOOL		Load();
	BOOL		Save();

	// Resets the colors to their defaults
	void				ResetToDefaultColors();

	// Returns a color
	COLORREF			GetColor(int nColorType);

	// Returns a color item (NULL if the index is out of range)
	CDisplayColorItem	*GetColorItem(int nColorType);

	// Access to the options
	BOOL				IsShadePolygons()				{ return m_bShadePolygons; }
	void				SetShadePolygons(BOOL bShade)	{ m_bShadePolygons=bShade; }

	BOOL				IsShowSurfaceColor()			{ return m_bShowSurfaceColor; }
	void				SetShowSurfaceColor(BOOL bShow)	{ m_bShowSurfaceColor=bShow; }

	int					GetVertexDrawRule()				{ return m_nVertexDrawRule; }
	void				SetVertexDrawRule(int nRule)	{ m_nVertexDrawRule=nRule; }

	int					GetVertexSize()					{ return m_nVertexSize; }
	void				SetVertexSize(int nSize)		{ m_nVertexSize=nSize; }

	int					GetHandleSize()					{ return m_nHandleSize; }
	void				SetHandleSize(int nSize)		{ m_nHandleSize=nSize; }

	BOOL				IsZBufferLines()				{ return m_bZBufferLines; }
	void				SetZBufferLines(BOOL bZBuffer)	{ m_bZBufferLines=bZBuffer; }

	BOOL				IsUseD3DAsDefault() const		{ return m_bUseD3DAsDefault; }
	void				SetUseD3DAsDefault(BOOL bUse)	{ m_bUseD3DAsDefault=bUse; }

	int					GetDefaultD3DDevice() const			{ return m_nDefaultD3DDevice; }
	void				SetDefaultD3DDevice(int nDevice)	{ m_nDefaultD3DDevice=nDevice; }

	int					GetDefaultD3DMode() const		{ return m_nDefaultD3DMode; }
	void				SetDefaultD3DMode(int nMode)	{ m_nDefaultD3DMode=nMode; }

	DWORD				GetMipMapOffset()				{ return m_dwMipMapOffset; }
	void				SetMipMapOffset(DWORD dwOffset)	{ m_dwMipMapOffset=dwOffset; }

protected:
	CDisplayColorItem	m_colorArray[kNumColorOptions];		// The colors

	BOOL				m_bShadePolygons;					// True if the polygons should be shaded
	BOOL				m_bShowSurfaceColor;				// True if the surface color boxes should be displayed
	int					m_nVertexDrawRule;					// Sets the drawing rules for vertices (all, selected only, etc)
	int					m_nVertexSize;						// The size of a vertex
	int					m_nHandleSize;						// The size of a handle
	BOOL				m_bZBufferLines;					// True if the lines should be Z-Buffered in D3D mode
	BOOL				m_bUseD3DAsDefault;					// True if the views should start in D3D mode
	int					m_nDefaultD3DDevice;				// The default D3D device
	int					m_nDefaultD3DMode;					// The default D3D mode
	DWORD				m_dwMipMapOffset;					// The mipmap offset
};

#endif // !defined(AFX_OPTIONSDISPLAY_H__5E3D9E56_D8AA_11D2_BE02_0060971BDC6D__INCLUDED_)
