//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// OptionsDisplay.cpp: implementation of the COptionsDisplay class.
//
//////////////////////////////////////////////////////////////////////

#include "bdefs.h"
#include "dedit.h"
#include "afxtempl.h"

#include "optionsbase.h"
#include "optionsdisplay.h"
#include "editprojectmgr.h"
#include "edithelpers.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

// Externs
extern TCHAR szRegKeyCompany[];
extern TCHAR szRegKeyApp[];
extern TCHAR szRegKeyVer[];

/************************************************************************/
// Load an item
BOOL CDisplayColorItem::Load(COptionsBase &options)
{
	// Get the color
	COLORREF cr=GetColor();

	// Create the default value
	CString sDefault;
	sDefault.Format("%d %d %d", GetRValue(cr), GetGValue(cr), GetBValue(cr));

	// Read the value from the registry
	CString sValue=options.GetStringValue(GetRegistryKey(), sDefault);

	// Parse the string
	int r=0, g=0, b=0;
	sscanf(sValue, "%d %d %d", &r, &g, &b);
	
	// Set the color
	m_crColor=RGB(r, g, b);

	return TRUE;
}

/************************************************************************/
// Save an item
BOOL CDisplayColorItem::Save(COptionsBase &options)
{
	// Get the color
	COLORREF cr=GetColor();

	// Save the value as an RGB triplette
	CString sRGB;
	sRGB.Format("%d %d %d", GetRValue(cr), GetGValue(cr), GetBValue(cr));

	options.SetStringValue(GetRegistryKey(), sRGB);	
	return TRUE;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

COptionsDisplay::COptionsDisplay()
{
	// Set the colors to their defaults
	ResetToDefaultColors();

	// Set the default display options		
	m_bShadePolygons=TRUE;
	m_bShowSurfaceColor=FALSE;
	m_nVertexDrawRule=kVertexDrawSelectedOnly;
	m_nVertexSize=2;
	m_nHandleSize=4;

	// Set the default D3D options
	m_bZBufferLines=FALSE;
	m_bUseD3DAsDefault=FALSE;
	m_nDefaultD3DDevice=0;
	m_nDefaultD3DMode=0;
}

COptionsDisplay::~COptionsDisplay()
{
}

/************************************************************************/
// Resets the colors to their defaults
void COptionsDisplay::ResetToDefaultColors()
{
	// Setup the color maps
	m_colorArray[kColorBackground]=CDisplayColorItem("ColorBackground", "Background", RGB(72,72,72));
	m_colorArray[kColorMajorGrid]=CDisplayColorItem("ColorMajorGrid", "MajorGrid", RGB(60,60,60));
	m_colorArray[kColorMinorGrid]=CDisplayColorItem("ColorMinorGrid", "MinorGrid", RGB(95,95,95));
	m_colorArray[kColorBrushDrawLines]=CDisplayColorItem("ColorBrush", "Brush", RGB(255,255,255));
	m_colorArray[kColorSelectedBrush]=CDisplayColorItem("ColorSelectedBrush", "Selected Brush", RGB(255,255,0));
	m_colorArray[kColorObject]=CDisplayColorItem("ColorObject", "Object", RGB(255,0,0));
	m_colorArray[kColorSelectedObject]=CDisplayColorItem("ColorSelectedObject", "Selected Object", RGB(255,255,255));
	m_colorArray[kColorObjectRadius]=CDisplayColorItem("ColorObjectRadius", "Object Radius", RGB(0,255,255));
	m_colorArray[kColorObjectLink]=CDisplayColorItem("ColorObjectLink", "Object Link", RGB(255,255,0));
	m_colorArray[kColorObjectFOV]=CDisplayColorItem("ColorObjectFOV", "Object FOV", RGB(0,255,255));
	m_colorArray[kColorObjectDims]=CDisplayColorItem("ColorObjectDims", "Object Dims", RGB(255,255,255));
	m_colorArray[kColorPath]=CDisplayColorItem("ColorPath", "Path", RGB(255,255,0));
	m_colorArray[kColorSelectedFace]=CDisplayColorItem("ColorSelectedFace", "Selected Face", RGB(255,0,0));
	m_colorArray[kColorSelectedEdge]=CDisplayColorItem("ColorSelectedEdge", "Selected Edge", RGB(0,255,0));
	m_colorArray[kColorBoundingBox]=CDisplayColorItem("ColorBoundingBox", "Bounding Box", RGB(192,192,192));
	m_colorArray[kColorOrigin]=CDisplayColorItem("ColorOrigin", "Origin", RGB(255,255,0));
	m_colorArray[kColorMarker]=CDisplayColorItem("ColorMarker", "Marker", RGB(0,255,0));
	m_colorArray[kColorHandleOutline]=CDisplayColorItem("ColorHandleOutline", "Handle Outline", RGB(255,255,255));
	m_colorArray[kColorHandleFill]=CDisplayColorItem("ColorHandleFill", "Handle Fill", RGB(0,0,0));
	m_colorArray[kColorVertexOutline]=CDisplayColorItem("ColorVertexOutline", "Vertex Outline", RGB(255,0,0));
	m_colorArray[kColorVertexFill]=CDisplayColorItem("ColorVertexFill", "Vertex Fill", RGB(255,0,0));
	m_colorArray[kColorTaggedVerticeOutline]=CDisplayColorItem("ColorTaggedVerticeOutline", "Tagged Vertice Outline", RGB(255,255,0));
	m_colorArray[kColorTaggedVerticeFill]=CDisplayColorItem("ColorTaggedVerticeFill", "Tagged Vertice Fill", RGB(255,255,0));
	m_colorArray[kColorLeakFinder]=CDisplayColorItem("ColorLeakFinder", "Leak Finder", RGB(255,255,0));
	m_colorArray[kColorImmediateVertice]=CDisplayColorItem("ColorImmediateVertice", "Immediate Vertex", RGB(50,255,50));
}

/************************************************************************/
// Loads the display options from the registry
BOOL COptionsDisplay::Load()
{
	// Load each color item
	int i;
	for (i=0; i < kNumColorOptions; i++)
	{
		GetColorItem(i)->Load(*this);
	}	

	m_bShadePolygons=GetBoolValue("ShadePolygons", TRUE);
	m_bShowSurfaceColor=GetBoolValue("ShowSurfaceColor", FALSE);
	m_nVertexSize=GetDWordValue("VertexSize", 2);
	m_nHandleSize=GetDWordValue("HandleSize", 4);
	m_nVertexDrawRule=GetDWordValue("VertexDrawRule", kVertexDrawSelectedOnly);

	// Load the D3D options
	m_bUseD3DAsDefault=GetBoolValue("UseD3DAsDefault", FALSE);
	m_nDefaultD3DDevice=GetDWordValue("DefaultD3DDevice", 0);
	m_nDefaultD3DMode=GetDWordValue("DefaultD3DMode", 0);

	// Always set the mipmap offset to zero
	m_dwMipMapOffset=0;

	m_bZBufferLines=GetBoolValue("ZBufferLines", FALSE);

	return TRUE;
}

/************************************************************************/
// Saves the display options to the registry
BOOL COptionsDisplay::Save()
{
	// Save each color item
	int i;
	for (i=0; i < kNumColorOptions; i++)
	{
		GetColorItem(i)->Save(*this);
	}	

	SetBoolValue("ShadePolygons",		m_bShadePolygons);
	SetBoolValue("ShowSurfaceColor",	m_bShowSurfaceColor);
	SetDWordValue("VertexSize",			m_nVertexSize);
	SetDWordValue("HandleSize",			m_nHandleSize);
	SetDWordValue("VertexDrawRule",		m_nVertexDrawRule);
	SetBoolValue("ZBufferLines",		m_bZBufferLines);

	// Save the D3D options
	SetBoolValue("UseD3DAsDefault",		m_bUseD3DAsDefault);
	SetDWordValue("DefaultD3DDevice",	m_nDefaultD3DDevice);
	SetDWordValue("DefaultD3DMode",		m_nDefaultD3DMode);	
	SetDWordValue("MipMapOffset",		m_dwMipMapOffset);

	return TRUE;
}

/************************************************************************/
// Returns a color
COLORREF COptionsDisplay::GetColor(int nColorType)
{
	CDisplayColorItem *pItem=GetColorItem(nColorType);
	if (pItem)
	{
		return pItem->GetColor();
	}
	else
	{
		return RGB(255,255,255);
	}
}

/************************************************************************/
// Returns a color item
CDisplayColorItem *COptionsDisplay::GetColorItem(int nColorType)
{
	if (nColorType >= 0 && nColorType < kNumColorOptions)
	{
		return &m_colorArray[nColorType];
	}
	return NULL;
}
