//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
// ------------------------------------------------------------------ //
//
//	FILE	  : EditRegion.cpp
//
//	PURPOSE	  : Implements the CEditRegion class.
//
//	CREATED	  : October 5 1996
//
//
// ------------------------------------------------------------------ //


// talon: the 2001 one, 0x410110..0x411670, written from the image with
// Build 69's as a guide: the ED file and the region's node helpers. No
// PrefabMgr or PrefabRef: the 2001 CEditRegion has no prefab manager (0x128
// bytes, its constructor 0x410110). Build 69's other functions (the LTA
// files, DEdit's) are not in the image.
//
// The 2001 ED file (LoadFile 0x410570, SaveFile 0x410af0):
//
//   uint32  version (1249)
//   uint8   compressed (the rest goes through a CCompressedIO)
//   uint32  info string length, then its bytes
//   uint32  x 8, written 0, read and ignored
//   uint32  brushes, then each brush (CEditBrush's 0x40da20 / 0x40dc00)
//   the node tree from the root (RecurseAndLoadNode / RecurseAndSaveNode)
//   the navigator positions (CNavigatorPosArray::LoadTBW / SaveTBW)
//


// Includes....
#include "bdefs.h"
#include "editregion.h"
#include "editpoly.h"
#include "node_ops.h"
#include "compressedio.h"
#include "memoryio.h"
#include "processing.h"


// talon: the version LoadFile wants (0x47e528) and the one SaveFile writes
// (0x47e52c): two globals (the names ours), both 1249.
DWORD g_EDLoadVersion = 1249;
DWORD g_EDSaveVersion = 1249;


// ------------------------------------------------------------------ //
// CEditRegion.
// ------------------------------------------------------------------ //

#pragma warning(disable:4355)

// 0x410110.
CEditRegion::CEditRegion()
{
	m_pStringHolder = &m_DefaultStringHolder;
	m_pInfoString = "";

	m_Selections.SetCacheSize( 100 );
	m_Objects.SetCacheSize( 100 );

	m_vMarker.Init( 0.0f, 0.0f, 0.0f );
	m_PathNodes.SetCacheSize( 50 );
	m_nLastNavigatorPos=0;

	m_RootNode.m_Flags |= NODEFLAG_WORLDROOT;
}

#pragma warning(default:4355)


// 0x4102e0.
CEditRegion::~CEditRegion()
{
	Term();
}


// 0x410490: Build 69's without ClearPathNodes.
void CEditRegion::Term()
{
	ClearSelections();
	m_RootNode.Term();

	// Must SetSize() on these because just deleting them won't remove them from the list.
	m_Brushes.RemoveAll();
	m_Objects.SetSize(0);

	// Remove the navigator position items
	unsigned int i;
	for (i=0; i < m_NavigatorPosArray.GetSize(); i++)
	{
		delete m_NavigatorPosArray[i];
	}
	m_NavigatorPosArray.SetSize(0);
}


// 0x410570, with its two catch blocks: 0x41096b (LithCatch's
// CMemoryException one) and 0x410991 (CLithException&, the EH table at
// 0x479268). The status is REGIONLOAD_INVALIDFILEVERSION for a wrong
// version (which goes to *pVersion: a pointer, tested for NULL),
// REGIONLOAD_INVALIDFILE for a bad file (m_LoadErrorCode says where: 1 a
// brush, 2 an exception, 3..6 the nodes), REGIONLOAD_OK otherwise. The
// brush indexer is freed after the try block (the EH state goes back to 0
// first, 0x4108f9).
//
// Retail bug, kept: the failure returns leak. A brush that fails to load
// (and every brush before it) is in m_Brushes and the indexer but not yet
// in the node tree, and the region frees brushes only through the tree
// (Term's m_Brushes.RemoveAll() frees just the list links), so they are
// never deleted; nor is the indexer on those paths (until the next
// LoadFile's SetSize). Processing stops on a failed load, so it never shows.
RegionLoadStatus CEditRegion::LoadFile( CAbstractIO &InFile, uint32 *pVersion, CEditProjectMgr *pProject )
{
	CCompressedIO	compressedIO;
	CAbstractIO		*pFile;
	uint32			version, nBrushes, len, dummy, i;
	BYTE			bCompressed;
	char			*pInfoString;
	CEditBrush		*pBrush;

	Term();

	InFile.EnableExceptions(TRUE);
	compressedIO.EnableExceptions(TRUE);
	m_LoadErrorCode = -1;

	LithTry
	{
		pFile = &InFile;

		InFile >> version;
		if( version != g_EDLoadVersion )
		{
			if( pVersion )
				*pVersion = version;

			m_LoadErrorCode = 0;
			return REGIONLOAD_INVALIDFILEVERSION;
		}

		InFile >> bCompressed;

		InFile >> len;
		pInfoString = new char[len+1];
		InFile.Read(pInfoString, len);
		pInfoString[len] = 0;
		m_pInfoString = m_pStringHolder->AddString(pInfoString, TRUE);
		delete [] pInfoString;

		InFile >> dummy;
		InFile >> dummy;
		InFile >> dummy;
		InFile >> dummy;
		InFile >> dummy;
		InFile >> dummy;
		InFile >> dummy;
		InFile >> dummy;

		if( bCompressed )
		{
			compressedIO.InitRead(&InFile);
			pFile = &compressedIO;
		}

		// The brushes.
		*pFile >> nBrushes;
		m_QuickBrushIndexer.SetSize(nBrushes);

		for( i=0; i < nBrushes; i++ )
		{
			pBrush = new CEditBrush;
			m_QuickBrushIndexer[i] = pBrush;
			m_Brushes.AddTail(pBrush);

			if( !pBrush->LoadTBW(*pFile, m_pStringHolder, version) )
			{
				m_LoadErrorCode = 1;
				return REGIONLOAD_INVALIDFILE;
			}
		}

		// The nodes.
		if( !RecurseAndLoadNode(*pFile, &m_RootNode, pProject) )
		{
			Term();
			return REGIONLOAD_INVALIDFILE;
		}

		m_NavigatorPosArray.LoadTBW(*pFile);

		m_RootNode.m_Flags |= NODEFLAG_WORLDROOT;
		UpdatePolyTriangles();
	}
	LithCatch( CLithException &x )
	{
		Term();
		m_LoadErrorCode = 2;
		return REGIONLOAD_INVALIDFILE;
	}

	m_QuickBrushIndexer.Term();
	return REGIONLOAD_OK;
}


// 0x4109e0 (the name ours): the ED file header.
void CEditRegion::SaveHeader( CAbstractIO &OutFile, uint32 version, BOOL bCompressed )
{
	OutFile << version;
	OutFile << (BYTE)bCompressed;

	OutFile << (DWORD)strlen(m_pInfoString);
	OutFile.Write(m_pInfoString, strlen(m_pInfoString));

	OutFile << (DWORD)0;
	OutFile << (DWORD)0;
	OutFile << (DWORD)0;
	OutFile << (DWORD)0;
	OutFile << (DWORD)0;
	OutFile << (DWORD)0;
	OutFile << (DWORD)0;
	OutFile << (DWORD)0;
}


// 0x410af0. Compressed, the body is written to memory first and compressed
// after the header.
void CEditRegion::SaveFile( CAbstractIO &OutFile, BOOL bCompress )
{
	uint32			version = g_EDSaveVersion;
	CMemoryIO		memFile;
	CCompressedIO	compressedIO;
	CAbstractIO		*pFile;
	LPOS			pos;
	uint32			i;

	memFile.EnableExceptions(TRUE);

	if( bCompress )
	{
		pFile = &memFile;
		memFile.SetCacheSize(1024*1024);
	}
	else
	{
		pFile = &OutFile;
		SaveHeader(OutFile, version, FALSE);
	}

	// Give the brushes indices.
	i=0;
	for( pos=m_Brushes; pos; )
		m_Brushes.GetNext(pos)->m_RegionBrushIndex = i++;

	*pFile << m_Brushes.GetSize();
	for( pos=m_Brushes; pos; )
		m_Brushes.GetNext(pos)->SaveTBW(*pFile, FALSE);

	RecurseAndSaveNode(*pFile, &m_RootNode);
	m_NavigatorPosArray.SaveTBW(*pFile);

	if( bCompress )
	{
		SaveHeader(OutFile, version, bCompress);
		memFile.SeekTo(0);
		compressedIO.WriteStream(&memFile, &OutFile, 50000, memFile.GetLen());
	}
}


// 0x410d50 (the name ours): the children first, then the node's own data.
BOOL CEditRegion::RecurseAndLoadNode( CAbstractIO &InFile, CWorldNode *pNode, CEditProjectMgr *pProject )
{
	WORD		nChildren, i;
	uint32		type, index, dummy;
	CWorldNode	*pChild;
	char		label[64];

	InFile >> nChildren;
	for( i=0; i < nChildren; i++ )
	{
		InFile >> type;

		if( type == Node_Brush )
		{
			InFile >> index;
			if( index >= m_Brushes.GetSize() )
			{
				m_LoadErrorCode = 3;
				return FALSE;
			}

			pChild = m_QuickBrushIndexer[index];
		}
		else if( type == Node_Object )
		{
			pChild = new CBaseEditObj;
			m_Objects.Append((CBaseEditObj*)pChild);
		}
		else if( type == Node_Null )
		{
			pChild = new CWorldNode;
		}
		else
		{
			m_LoadErrorCode = 4;
			return FALSE;
		}

		pNode->m_Children.AddTail( pChild );
		pChild->SetParent( pNode );

		if( !RecurseAndLoadNode(InFile, pChild, pProject) )
			return FALSE;
	}

	if( !LoadNodeProperties(InFile, pNode, pProject) )
	{
		m_LoadErrorCode = 5;
		return FALSE;
	}

	// The unique ID (not kept).
	InFile >> dummy;
	InFile >> pNode->m_Flags;

	if( !InFile.ReadString(label, sizeof(label)) )
	{
		m_LoadErrorCode = 6;
		return FALSE;
	}

	if( pNode->m_Type == Node_Null )
		pNode->SetNodeLabel(label);

	if( pNode->m_Flags & NODEFLAG_SELECTED )
		SelectNode( pNode );

	if( pNode->m_Flags & NODEFLAG_PATH )
		m_PathNodes.Append( pNode );

	return TRUE;
}


// 0x4110a0 (the name ours).
void CEditRegion::RecurseAndSaveNode( CAbstractIO &OutFile, CWorldNode *pNode )
{
	GPOS		pos;
	CWorldNode	*pChild;

	OutFile << (WORD)pNode->m_Children.GetSize();
	for( pos=pNode->m_Children; pos; )
	{
		pChild = pNode->m_Children.GetNext(pos);

		OutFile << (DWORD)pChild->m_Type;
		if( pChild->m_Type == Node_Brush )
			OutFile << pChild->AsBrush()->m_RegionBrushIndex;

		RecurseAndSaveNode(OutFile, pChild);
	}

	SaveNodeProperties(OutFile, pNode);

	OutFile << pNode->GetUniqueID();
	OutFile << pNode->m_Flags;
	if( pNode->GetNodeLabel() )
		OutFile.WriteString((char*)pNode->GetNodeLabel());
	else
		OutFile.WriteString("");
}


// 0x411190.
uint32 CEditRegion::GetTotalNumPolies()
{
	uint32 ret=0;
	LPOS pos;

	for(pos=m_Brushes; pos; )
		ret += m_Brushes.GetNext(pos)->m_Polies.GetSize();

	return ret;
}


// 0x4111c0.
uint32 CEditRegion::GetTotalNumPoints()
{
	uint32 ret=0;
	LPOS pos;

	for(pos=m_Brushes; pos; )
		ret += m_Brushes.GetNext(pos)->m_Points.GetSize();

	return ret;
}


// 0x4111f0 (the name ours): remakes every poly's triangles (LoadFile's last
// step).
void CEditRegion::UpdatePolyTriangles()
{
	LPOS pos;
	CEditBrush *pBrush;
	uint32 i;

	for(pos=m_Brushes; pos; )
	{
		pBrush = m_Brushes.GetNext(pos);

		for(i=0; i < pBrush->m_Polies; i++)
			pBrush->m_Polies[i]->UpdateTriangles();
	}
}


// 0x411240: Build 69's without the frozen test.
void CEditRegion::SelectNode( CWorldNode *pNode, BOOL bUpdateSelectionArray )
{
	// Determine if the selection array should be used
	if (bUpdateSelectionArray)
	{
		// Find the selection
		uint32	index = m_Selections.FindElement( pNode );
		if( index == BAD_INDEX )
		{
			// Update the node flag
			pNode->m_Flags |= NODEFLAG_SELECTED;

			// Add the selection to the selection array
			m_Selections.Append( pNode );
		}
	}
	else
	{
		// Just update the node flag
		pNode->m_Flags |= NODEFLAG_SELECTED;
	}
}


// 0x411390: Build 69's without the early return for an unselected node.
void CEditRegion::UnselectNode( CWorldNode *pNode, BOOL bUpdateSelectionArray )
{
	// Determine if the selection array should be used
	if (bUpdateSelectionArray)
	{
		// Find the selection
		uint32	index = m_Selections.FindElement( pNode );
		if( index != BAD_INDEX )
		{
			// Update the node flag
			pNode->m_Flags &= ~NODEFLAG_SELECTED;

			// Add the selection to the selection array
			m_Selections.Remove( index );
		}
	}
	else
	{
		// Just update the node flag
		pNode->m_Flags &= ~NODEFLAG_SELECTED;
	}
}


// 0x411500.
void CEditRegion::DetachNode( CWorldNode *pNode, BOOL bAttachToRoot )
{
	no_DetachNode((CEditRegion*)this, pNode, bAttachToRoot);
}


// 0x411520.
void CEditRegion::ClearSelections()
{
	for( uint32 i=0; i < m_Selections; i++ )
		m_Selections[i]->ClearFlag(NODEFLAG_SELECTED);

	m_Selections.SetSize(0);
}
