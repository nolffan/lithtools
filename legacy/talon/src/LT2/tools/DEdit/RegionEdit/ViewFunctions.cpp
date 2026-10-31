//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : ViewFunctions.cpp
//
//	PURPOSE	  : Implements all the non-message handler functions
//              for CRegionView.
//
//	CREATED	  : October 18 1996
//
//
//------------------------------------------------------------------

// Includes....
#include "bdefs.h"
#include "optionsbase.h"
#include "optionsprimitives.h"		//for the registry utils
#include "regiondoc.h"
#include "regionview.h"

#include "dedit.h"
#include "projectbar.h"
#include "edithelpers.h"
#include "mainfrm.h"
#include "stringdlg.h"
#include "cylinderprimdlg.h"
#include "sphereprimitivedlg.h"
#include "texture.h"
#include "node_ops.h"
#include "edit_actions.h"
#include "regmgr.h"
#include "sysstreamsim.h"
#include "de_world.h"
#include "pregeometry.h"
#include "geomroutines.h"
#include "converge.h"

#include <set>
#include <vector>


// These define which way brush selection handles want to scale.
CVector	g_SelectionHandleVectors[] =
{
	CVector(1.0f, 0.0f, 0.0f),		// X handle
	CVector(-1.0f, 0.0f, 0.0f),		// X handle

	CVector(0.0f, 1.0f, 0.0f),		// Y handle
	CVector(0.0f, -1.0f, 0.0f),		// Y handle

	CVector(0.0f, 0.0f, 1.0f),		// Z handle
	CVector(0.0f, 0.0f, -1.0f),		// Z handle

	// XY handles
	CVector(1.0f,   1.0f, 0.0f),
	CVector(-1.0f,  1.0f, 0.0f),
	CVector(1.0f,  -1.0f, 0.0f),
	CVector(-1.0f, -1.0f, 0.0f),

	// XZ handles
	CVector(1.0f,   0.0f, 1.0f),
	CVector(-1.0f,  0.0f, 1.0f),
	CVector(1.0f,   0.0f, -1.0f),
	CVector(-1.0f,  0.0f, -1.0f),

	// YZ handles
	CVector(0.0f,  1.0f,   1.0f),
	CVector(0.0f,  -1.0f,  1.0f),
	CVector(0.0f,  1.0f,  -1.0f),
	CVector(0.0f,  -1.0f, -1.0f)
};

void CRegionView::LoadProjFile( CAbstractIO &file )
{
	CRect		rect;
	WORD		tempWord;


	file >> tempWord;	
	file >> tempWord;	
	file >> tempWord;	
	file >> tempWord;	
	file >> tempWord;	
	file >> tempWord;	
	file >> tempWord;	
	file >> tempWord;	SetGridSpacing(tempWord);

	file >> tempWord;	SetViewMode( (int)tempWord );

	file >> FarZ();

	file >> Nav().Pos().x >> Nav().Pos().y >> Nav().Pos().z;
	file >> Nav().Up().x >> Nav().Up().y >> Nav().Up().z;
	file >> Nav().Right().x >> Nav().Right().y >> Nav().Right().z;
	file >> Nav().Forward().x >> Nav().Forward().y >> Nav().Forward().z;

	file >> Nav().m_LookAt.x >> Nav().m_LookAt.y >> Nav().m_LookAt.z;
	file >> Nav().m_LookAtDist;

	file >> ViewDef()->m_Magnify;
	file >> EditGrid().m_DrawSize;
}


void CRegionView::SaveProjFile( CAbstractIO &file )
{
	CRect		rect;
	WORD		tempWord;
	
	//output an empty header to be backwards compatible
	tempWord = 0;
	file << tempWord;		
	file << tempWord;
	file << tempWord;
	file << tempWord;
	file << tempWord;
	file << tempWord;	
	file << tempWord;

	tempWord = (WORD)m_dwGridSpacing;	file << tempWord;
	
	tempWord = (WORD)GetViewMode();	file << tempWord;

	file << FarZ();

	file << Nav().Pos().x << Nav().Pos().y << Nav().Pos().z;
	file << Nav().Up().x << Nav().Up().y << Nav().Up().z;
	file << Nav().Right().x << Nav().Right().y << Nav().Right().z;
	file << Nav().Forward().x << Nav().Forward().y << Nav().Forward().z;
	
	file << Nav().m_LookAt.x << Nav().m_LookAt.y << Nav().m_LookAt.z;
	file << Nav().m_LookAtDist;

	file << ViewDef()->m_Magnify;
	file << EditGrid().m_DrawSize;
}


CPoint CRegionView::GetCurMousePos()
{
	CPoint		ret;

	GetCursorPos( &ret );
	ScreenToClient( &ret );

	return ret;
}

// talon: 2001's view keeps these flags itself (0x46623c..0x466289).
int CRegionView::GetShadeMode()
{
	return m_nShadeMode;
}

void CRegionView::SetShadeMode( int mode )
{
	m_nShadeMode = mode;
}

BOOL CRegionView::IsWalkThroughMovement()
{
	return m_bWalkThroughMovement;
}

void CRegionView::SetWalkThroughMovement( BOOL bWalkThrough )
{
	m_bWalkThroughMovement = bWalkThrough;
}

BOOL CRegionView::IsSelectBackfaces()
{
	return m_bSelectBackfaces;
}

void CRegionView::SetSelectBackfaces( BOOL bBackface )
{
	m_bSelectBackfaces = bBackface;
}

BOOL CRegionView::IsShowNormals()
{
	return m_bShowNormals;
}

void CRegionView::SetShowNormals( BOOL bShow )
{
	m_bShowNormals = bShow;
}

int CRegionView::GetEditMode()
{
	return GetMainFrame()->GetWorldEditMode();
}


/////////////////////////////////////////////////////////////////////////////
// Tagging functions

BOOL CRegionView::GetSelectedPoly( CVertRefArray &list )
{
	uint32		i;

	list.SetSize(0);
	if( IPoly().IsValid() )
	{
		for( i=0; i < IPoly()()->NumVerts(); i++ )
			list.Append( CVertRef(IPoly()()->m_pBrush, IPoly()()->Index(i)) );
	}

	return list > 0;
}


BOOL CRegionView::GetSelectedVerts( CVertRefArray &list )
{
	list.GenCopyList(TaggedVerts());
	return list.GenGetSize() > 0;
}


BOOL CRegionView::GetImmediateVert( CVertRefArray &list )
{
	uint32			i;
	
	list.SetSize(0);
	
	if( IVert().IsValid() )
	{
		if( IsParallelViewType() )
		{
			// Find any other points parallel to this one.
			for( i=0; i < IVert().m_pBrush->m_Points; i++ )
			{
				if( i != IVert().m_iVert )
				{
					CEditVert	&testVert = IVert().m_pBrush->m_Points[i];
					
					if( (testVert.m_Projected.x == IVert()().m_Projected.x) && 
						(testVert.m_Projected.y == IVert()().m_Projected.y) )
						list.Append( CVertRef(IVert().m_pBrush, i) );
				}
			}
		}

		list.Append( IVert() );
	}

	return list > 0;
}


CBrushRef CRegionView::GetMouseOverBrush( CReal *pDist, CReal rMinDist )
{
	CPolyRef	ref;
	CBrushRef	brushRef;
	CPoint		point = GetCurMousePos();
	CEditRay	ray;

	ray = ViewDef()->MakeRayFromScreenPoint( point );
	ref = CastRayAtPolies( ray, pDist, rMinDist );
	brushRef.Init( ref.m_pBrush );

	return brushRef;
}





BOOL CRegionView::GetSelectedBrushes( CVertRefArray &list )
{
	uint32		i, k;
	CEditBrush	*pBrush;
	CWorldNode	*pNode;

	for( i=0; i < GetRegion()->m_Selections; i++ )
	{
		if((pNode = GetRegion()->m_Selections[i])->GetType() == Node_Brush)
		{
			pBrush = pNode->AsBrush();

			for( k=0; k < pBrush->m_Points; k++ )
				list.Append( CVertRef(pBrush, k) );
		}
	}
	
	return list > 0;
}


BOOL CRegionView::MakeTaggedEdgeList( CEdgeRefArray &list )
{
	list.SetSize(0);
	
	if( IEdge().IsValid() )
		list.Append( IEdge() );

	return list > 0;
}





typedef CLinkedList<CVertRef> CVertRefList;

// talon: 2001's (0x466501): the rectangle in pixels of the view's pixel
// scale, word counters, frozen nodes not skipped.
void CRegionView::TagPointsInRect( CRect &rect )
{
	uint16 i, k;
	CPoint point;
	CRect norm = rect;
	CEditBrush *pBrush;
	LPOS pos;

	
	norm.NormalizeRect();

	norm.left /= m_nPixelScale;
	norm.top /= m_nPixelScale;
	norm.right /= m_nPixelScale;
	norm.bottom /= m_nPixelScale;

	// Only tag in selected brushes if we're in geometry mode and a brush is selected
	if ((GetEditMode() == GEOMETRY_EDITMODE) && (GetRegion()->m_Selections.GetSize() != 0))
	{
		for( i=0; i < GetRegion()->m_Selections; i++ )
		{
			CWorldNode *pNode = GetRegion()->m_Selections[i];

			if( pNode->IsFlagSet(NODEFLAG_HIDDEN) || (pNode->GetType() != Node_Brush))
				continue;

			// Get the brush pointer...
			pBrush = pNode->AsBrush();

			if( !pBrush->IsVisible() )
				continue;

			for( k=0; k < pBrush->m_Points; k++ )
			{
				CEditVert	&vert = pBrush->m_Points[k];
				
				if( vert.IsInFrustum() )
				{
					point.x = (int)vert.m_Projected.x;
					point.y = (int)vert.m_Projected.y;

					if( norm.PtInRect(point) )
						ToggleVertexTag( CVertRef(pBrush, k) );
				}
			}
		}			
	}
	else
	{
		for(pos=m_pRegion->m_Brushes; pos; )
		{
			pBrush = m_pRegion->m_Brushes.GetNext(pos);

			if(	pBrush->IsFlagSet(NODEFLAG_HIDDEN) || !pBrush->IsVisible())
				continue;

			for( k=0; k < pBrush->m_Points; k++ )
			{
				CEditVert	&vert = pBrush->m_Points[k];
				
				if( vert.IsInFrustum() )
				{
					point.x = (int)vert.m_Projected.x;
					point.y = (int)vert.m_Projected.y;

					if( norm.PtInRect(point) )
						ToggleVertexTag( CVertRef(pBrush, k) );
				}
			}
		}			
	}
}


// talon: 2001's (0x4666ca): no recursion through the tree and no
// selection filter, just the region's objects; the rectangle is in pixels
// of the view's pixel scale.
void CRegionView::GetObjectsInRect( CRect &rect, CMoArray< CWorldNode * > &objects )
{
	CRect norm = rect;
	norm.NormalizeRect();

	norm.left /= m_nPixelScale;
	norm.top /= m_nPixelScale;
	norm.right /= m_nPixelScale;
	norm.bottom /= m_nPixelScale;

	for( uint16 i=0; i < m_pRegion->m_Objects; i++ )
	{
		CWorldNode *pObj = m_pRegion->m_Objects[i];

		CVector &vPos = pObj->GetPos();
		CPoint testPt;
		if( TransformAndProjectInFrustum(vPos, testPt) && norm.PtInRect(testPt) && !pObj->IsFlagSet(NODEFLAG_HIDDEN) )
			objects.Append(pObj);
	}
}


void CRegionView::ToggleVertexTag( CVertRef ref )
{
	GenListPos pos;

	if(TaggedVerts().GenFindElement(ref, pos))
		TaggedVerts().GenRemoveAt(pos);
	else
		TaggedVerts().GenAppend(ref);
}


BOOL CRegionView::GetClosestPoint( CPoint &point, CVector &vClosest, BOOL bSelectedOnly, CReal *pDist )
{
	CReal			dist=(CReal)MAX_CREAL, minDist = (CReal)MAX_CREAL;
	CVertRef		closest;
	BOOL			bResult = FALSE;

	closest = GetClosestVert( point, bSelectedOnly, &dist );
	if( closest.IsValid( ))
	{
		minDist = dist;
		vClosest = closest( );
		bResult = TRUE;
	}

	CObjectRef obj = GetClosestObject( point, bSelectedOnly, &dist );
	if( obj.IsValid() && dist < minDist )
	{
		minDist = dist;
		vClosest = obj()->GetPos();
		bResult = TRUE;
	}

	if( pDist )
		*pDist = minDist;

	return bResult;
}


CVertRef CRegionView::GetClosestVert( CPoint &point, BOOL bSelectedOnly, CReal *pDist )
{
	CWorldNode		*pNode;
	CEditBrush		*pBrush;
	CReal			dist=(CReal)MAX_CREAL, minDist = (CReal)MAX_CREAL;
	uint32			i;
	CVertRef		vert, minVert;
	LPOS pos;
	CPoint		testPt;


	// Do all brushes...
	if( !bSelectedOnly )
	{
		for(pos=m_pRegion->m_Brushes; pos; )
		{
			pBrush = m_pRegion->m_Brushes.GetNext(pos);

			if(	pBrush->IsFlagSet(NODEFLAG_HIDDEN) )
			{
				continue;
			}
			
			vert = GetClosestVert(pBrush, point, &dist);
			if( dist < minDist )
			{
				minVert = vert;
				minDist = dist;
			}
		}
	}
	// Do only selected brushes...
	else
	{
		for( i=0; i < GetRegion()->m_Selections; i++ )
		{
			pNode = GetRegion()->m_Selections[i];

			if(	pNode->IsFlagSet(NODEFLAG_HIDDEN) )
			{
				continue;
			}

			// Handle movement for Brush nodes...
			if(pNode->GetType() == Node_Brush)
			{
				// Get the brush pointer...
				pBrush = pNode->AsBrush();

				vert = GetClosestVert(pBrush, point, &dist);
				if( dist < minDist )
				{
					minVert = vert;
					minDist = dist;
				}
			}
		}
	}

	if( pDist )
		*pDist = minDist;

	return minVert;
}


// talon: 2001's (0x4669a3): the point in pixels of the pixel scale, word
// indices.
CVertRef CRegionView::GetClosestVert( CEditBrush *pBrush, CPoint point, CReal *pDist )
{
	uint16		i, k;
	uint16		minIndex = 0xFFFF;
	CEditPoly	*pPoly;
	CPoint		testPt;

	CReal		viewerDist, minViewerDist = (CReal)MAX_CREAL;
	int32		dist, minDist = MAX_SDWORD;

	point.x /= m_nPixelScale;
	point.y /= m_nPixelScale;

	for( i=0; i < pBrush->m_Polies; i++ )
	{
		pPoly = pBrush->m_Polies[i];

		for( k=0; k < pPoly->NumVerts(); k++ )
		{
			if( TransformAndProjectInFrustum(pPoly->Pt(k), testPt) )
			{
				dist = SQR(testPt.x - point.x) + SQR(testPt.y - point.y);
				if( dist == minDist )
				{
					viewerDist = (pPoly->Pt(k) - Nav().Pos()).Mag();
					if( viewerDist < minViewerDist )
					{
						minViewerDist = viewerDist;
						minIndex = pPoly->m_Indices[k];
						minDist = dist;
					}
				}
				else if( dist < minDist )
				{
					minViewerDist = (pPoly->Pt(k) - Nav().Pos()).Mag();
					minIndex = pPoly->m_Indices[k];
					minDist = dist;
				}
			}
		}
	}

	if( pDist )
		*pDist = (CReal)minDist;

	if( minIndex == 0xFFFF )
		return CVertRef( NULL, minIndex );
	else
		return CVertRef( pBrush, minIndex );
}


// talon: 2001's (0x466b6d): the point is in pixels of the view's pixel
// scale; word indices; frozen brushes count.
CVertRef CRegionView::GetClosestVert( CVertRefArray &vertArray, CPoint point, CReal *pDist )
{
	uint16		i;
	uint16		minIndex = 0xFFFF;
	CPoint		testPt;

	CReal		viewerDist, minViewerDist = (CReal)MAX_CREAL;
	int32		dist, minDist=MAX_SDWORD;

	point.x /= m_nPixelScale;
	point.y /= m_nPixelScale;

	for( i=0; i < vertArray.GetSize( ); i++ )
	{
		if(	vertArray[i].m_pBrush->IsFlagSet(NODEFLAG_HIDDEN) )
		{
			continue;
		}

		if( TransformAndProjectInFrustum(vertArray[i](), testPt) )
		{
			dist = SQR(testPt.x - point.x) + SQR(testPt.y - point.y);
			if( dist == minDist )
			{
				viewerDist = (vertArray[i]() - Nav().Pos()).Mag();
				if( viewerDist < minViewerDist )
				{
					minViewerDist = viewerDist;
					minIndex = i;
					minDist = dist;
				}
			}
			else if( dist < minDist )
			{
				minViewerDist = (vertArray[i]() - Nav().Pos()).Mag();
				minIndex = i;
				minDist = dist;
			}
		}
	}

	if( pDist )
		*pDist = (CReal)minDist;

	if( minIndex == 0xFFFF )
		return CVertRef( NULL, minIndex );
	else
		return vertArray[minIndex];
}


CEdgeRef CRegionView::GetClosestEdge( CPoint &point, BOOL bSelectedOnly )
{
	uint16			i;
	CEditBrush		*pBrush;
	CEdgeRef		testRef, bestRef;
	CReal			dist, minDist = (CReal)MAX_CREAL;
	LPOS pos;

	// Find the closest in all brushes
	if (!bSelectedOnly)
	{
		for(pos=m_pRegion->m_Brushes; pos; )
		{
			pBrush = m_pRegion->m_Brushes.GetNext(pos);
			
			if(pBrush->IsFlagSet(NODEFLAG_HIDDEN))
			{
				continue;
			}

			testRef = GetClosestEdge( pBrush, point, &dist );
			if( testRef.IsValid() && (dist < minDist) )
			{
				bestRef = testRef;
				minDist = dist;
			}
		}
	}
	// Find the closest in only the selected brushes
	else
	{
		for( i=0; i < GetRegion()->m_Selections; i++ )
		{
			CWorldNode *pNode = GetRegion()->m_Selections[i];

			if(pNode->IsFlagSet(NODEFLAG_HIDDEN))
			{
				continue;
			}

			// Handle movement for Brush nodes...
			if(pNode->GetType() == Node_Brush)
			{
				// Get the brush pointer...
				pBrush = pNode->AsBrush();
				testRef = GetClosestEdge( pBrush, point, &dist );
				if( testRef.IsValid() && (dist < minDist) )
				{
					bestRef = testRef;
					minDist = dist;
				}
			}
		}			
	}

	return bestRef;
}


CEdgeRef CRegionView::GetClosestEdge( CEditBrush *pBrush, CPoint point, CReal *pDist )
{
	uint16			i, k;
	CEditPoly		*pPoly;
	TLVertex line[2];
	CVector			points[2];
	CVector			lineVec, normal, pointVec;

	CReal			dot, maxLineDot, bestLineDot = (CReal)MAX_CREAL;
	CEdgeRef		bestEdge;

	point.x /= m_nPixelScale;
	point.y /= m_nPixelScale;

	pointVec.Init( (CReal)point.x, (CReal)point.y, 0.0f );

	for( i=0; i < pBrush->m_Polies; i++ )
	{
		pPoly = pBrush->m_Polies[i];

		for( k=0; k < pPoly->NumVerts(); k++ )
		{
			maxLineDot = 0.0f;
			
			TransformPt( pPoly->Pt(k), line[0].m_Vec );
			TransformPt( pPoly->NextPt(k), line[1].m_Vec );

			if( ClipLineToFrustum(line) )
			{
				ViewDef()->ProjectPt( line[0].m_Vec, points[0] );
				ViewDef()->ProjectPt( line[1].m_Vec, points[1] );

				lineVec = points[1] - points[0];
				lineVec.z = 0.0f;
				lineVec.Norm();
				normal.Init( -lineVec.y, lineVec.x, 0.0f );

				// Test the distance from the line 'caps'.
				dot = lineVec.Dot( pointVec - points[1] );
				if( dot > 0 )
				{
					maxLineDot = dot;
				}
				else
				{
					dot = (-lineVec).Dot( pointVec - points[0] );
					if( dot > 0 )
						maxLineDot = dot;
				}
				
				// Test the distance from the line itself.
				dot = normal.Dot( pointVec - points[0] );
				maxLineDot = MAX( maxLineDot, ABS(dot) );
			
				if( maxLineDot < bestLineDot )
				{
					bestLineDot = maxLineDot;
					bestEdge.Init( pBrush, pPoly->m_Indices[k], pPoly->m_Indices.Next(k) );
				}
			}
		}
	}	

	if( pDist )
		*pDist = bestLineDot;

	return bestEdge;
}


CPolyRef CRegionView::CastRayAtPolies( CEditRay &ray, CReal *pDist, CReal rMinDist, BOOL bSelectedOnly )
{
	uint32			i;
	CReal			dist, minDist = (CReal)MAX_CREAL;
	CPolyRef		ref, bestRef;
	CEditBrush		*pBrush;
	LPOS pos;

	// All brushes..
	if (!bSelectedOnly)
	{
		for(pos=m_pRegion->m_Brushes; pos; )
		{
			pBrush = m_pRegion->m_Brushes.GetNext(pos);

			if(pBrush->IsFlagSet(NODEFLAG_HIDDEN) || pBrush->IsFlagSet(NODEFLAG_FROZEN))
			{
				continue;
			}

			ref = CastRayAtPolies( pBrush, ray, &dist );
			if( ref.IsValid() && (dist < minDist) && ( dist > rMinDist ))
			{
				bestRef = ref;
				minDist = dist;
			}
		}
	}
	// Selected only
	else
	{
		for( i=0; i < GetRegion()->m_Selections; i++ )
		{
			CWorldNode *pNode = GetRegion()->m_Selections[i];

			if(pNode->IsFlagSet(NODEFLAG_HIDDEN) || pNode->IsFlagSet(NODEFLAG_FROZEN))
			{
				continue;
			}

			if(pNode->GetType() == Node_Brush)
			{
				// Get the brush pointer...
				pBrush = pNode->AsBrush();

				ref = CastRayAtPolies( pBrush, ray, &dist );
				if( ref.IsValid() && (dist < minDist) && ( dist > rMinDist ))
				{
					bestRef = ref;
					minDist = dist;
				}
			}
		}
	}

	if(pDist)
	{
		*pDist = minDist;
	}

	return bestRef;
}
	
	
CPolyRef CRegionView::CastRayAtPolies( CEditBrush *pBrush, CEditRay &ray, CReal *pDist )
{	
	CReal			t;
	CReal			minDist = (CReal)MAX_CREAL;
	uint32			i;

	CEditBrush		*pMinBrush = NULL;
	uint32			minIndex = BAD_INDEX;
	

	for( i=0; i < pBrush->m_Polies; i++ )
	{
		if( pBrush->m_Polies[i]->IntersectRay(ray, t, IsSelectBackfaces()) )
		{
			if( t < minDist )
			{
				pMinBrush = pBrush;
				minDist = t;
				minIndex = i;
			}
		}
	}

	if( pDist )
		*pDist = minDist;

	return CPolyRef( pMinBrush, minIndex );
}


// talon: 2001's (0x467352): the region's objects (or the selected ones),
// no tree recursion or filter, the point in pixels of the pixel scale; it
// returns an object ref.
CObjectRef CRegionView::GetClosestObject( CPoint point, BOOL bSelectedOnly, CReal *pDist )
{
	CObjectRef		ret;
	uint32			i;
	CPoint			testPt;
	CReal			viewerDist, minViewerDist = (CReal)MAX_CREAL;
	int32			dist, minDist = MAX_SDWORD;

	point.x /= m_nPixelScale;
	point.y /= m_nPixelScale;

	if( !bSelectedOnly )
	{
		for( i=0; i < m_pRegion->m_Objects; i++ )
		{
			CBaseEditObj *pObj = m_pRegion->m_Objects[i];

			if( pObj->IsFlagSet(NODEFLAG_HIDDEN) )
				continue;

			CVector &vPos = pObj->GetPos();
			if( TransformAndProjectInFrustum(vPos, testPt) )
			{
				dist = SQR(testPt.x - point.x) + SQR(testPt.y - point.y);
				if( dist == minDist )
				{
					viewerDist = (vPos - Nav().Pos()).Mag();
					if( viewerDist < minViewerDist )
					{
						minViewerDist = viewerDist;
						ret.m_pObject = m_pRegion->m_Objects[i];
						minDist = dist;
					}
				}
				else if( dist < minDist )
				{
					ret.m_pObject = m_pRegion->m_Objects[i];
					minViewerDist = (vPos - Nav().Pos()).Mag();
					minDist = dist;
				}
			}
		}
	}
	else
	{
		for( i=0; i < m_pRegion->m_Selections; i++ )
		{
			CWorldNode *pNode = m_pRegion->m_Selections[i];

			if( pNode->IsFlagSet(NODEFLAG_HIDDEN) || (pNode->GetType() != Node_Object) )
				continue;

			CVector &vPos = pNode->GetPos();
			if( TransformAndProjectInFrustum(vPos, testPt) )
			{
				dist = SQR(testPt.x - point.x) + SQR(testPt.y - point.y);
				if( dist == minDist )
				{
					viewerDist = (vPos - Nav().Pos()).Mag();
					if( viewerDist < minViewerDist )
					{
						minViewerDist = viewerDist;
						ret.m_pObject = (CBaseEditObj*)pNode;
						minDist = dist;
					}
				}
				else if( dist < minDist )
				{
					ret.m_pObject = (CBaseEditObj*)pNode;
					minViewerDist = (vPos - Nav().Pos()).Mag();
					minDist = dist;
				}
			}
		}
	}

	if( pDist )
		*pDist = (CReal)minDist;

	return ret;
}




					
/////////////////////////////////////////////////////////////////////////////
// Custom CRegionView functions


BOOL CRegionView::GetCurrentTextureName( char* &pTextureName, BOOL bUseSprites )
{
	char szTextureName[256];

	GetProjectBar()->GetCurrentTextureName( szTextureName, bUseSprites );
	if( strlen( szTextureName ))
	{
		pTextureName = GetRegionDoc()->m_RegionStringHolder.AddString( szTextureName );
		return TRUE;
	}
	else
		return FALSE;
}

void CRegionView::GiveMemoryWarning()
{
	CString str;

	str.LoadString( IDS_LOWONMEMORY );
	MessageBox( str, AfxGetAppName(), MB_OK );
}


void CRegionView::UpdatePlanesOfPolies( CVertRefArray &verts )
{
	uint16			i, k;
	CEditBrush		*pBrush;
	CEditPoly		*pPoly;

	
	for( i=0; i < verts; i++ )
	{
		pBrush = verts[i].m_pBrush;

		for( k=0; k < pBrush->m_Polies; k++ )
		{
			pPoly = pBrush->m_Polies[k];
			if( pPoly->m_Indices.FindElement(verts[i].m_iVert) )
				pPoly->UpdatePlane();
		}		
	}
}


// talon: near (0x467782): 2001's CEditGrid::IntersectRay (0x4230d8) has no
// bSnapToGrid (EditGrid.h's default still pushes one).
BOOL CRegionView::GetVertexFromPoint( CPoint point, CVector &vertex )
{
	return EditGrid().IntersectRay( GetGridSpacing(), ViewDef()->MakeRayFromScreenPoint(point), vertex, NULL );
}


// talon: 2001's (0x4677ce): Build 69's CRVTrackerDrawPoly::UpdateDrawPolyVertex
// in the view; with Ctrl down the point snaps to the immediate vertex.
void CRegionView::UpdateDrawPolyVertex( CPoint &point )
{
	if( IsKeyDown(VK_CONTROL) && IVert().IsValid() )
	{
		DrawingBrush().m_Points.Last() = IVert()();
		return;
	}

	CVector vIntersection;
	if( GetVertexFromPoint(point, vIntersection) && (DrawingBrush().m_Points > 0) )
	{
		if( !IsPerspectiveViewType() )
		{
			LTVector vOffset;
			vOffset = EditGrid().Forward();
			vOffset = EditGrid().Forward() * vOffset.Dot( GetRegion()->m_vMarker );
			vIntersection += vOffset;
		}

		DrawingBrush().m_Points.Last() = vIntersection;
	}
}


// talon: 2001's (0x46790a): the same for the split poly's vertex, snapping
// only to a vertex of the brush being split.
void CRegionView::UpdateSplitPolyVertex( CPoint &point )
{
	if( IsKeyDown(VK_CONTROL) && IVert().IsValid() && (IVert().m_pBrush == m_PolyToSplit.m_pBrush) )
	{
		DrawingBrush().m_Points.Last() = IVert()();
		return;
	}

	CVector vIntersection;
	if( GetVertexFromPoint(point, vIntersection) )
	{
		if( !IsPerspectiveViewType() )
		{
			LTVector vOffset;
			vOffset = EditGrid().Forward();
			vOffset = EditGrid().Forward() * vOffset.Dot( GetRegion()->m_vMarker );
			vIntersection += vOffset;
		}

		DrawingBrush().m_Points.Last() = vIntersection;
	}
}


BOOL CRegionView::UpdateImmediateSelection()
{
	CPoint			point = GetCurMousePos();
	CVertRef		vert;
	CEdgeRef		edge;
	CPolyRef		poly;
	CBrushRef		brush;
	CObjectRef		obj;
	CEditRay		ray;
	BOOL			bRet = FALSE;
	BOOL			bSelectionsOnly = GetRegion()->m_Selections.GetSize() > 0; // do we want to restrict the cast to selection?


	// We don't want to restrict the cast to the selection if it includes 
	// only objects (no geometry), so check the selection list

	if (bSelectionsOnly) 
	{
		bSelectionsOnly = false; // we will assume that the selected list is all objects until proven otherwise

		for (int i=0; (!bSelectionsOnly) && (i < GetRegion()->m_Selections.GetSize()) ; i++) {

			// if this is true, the selection list includes at least one brush
			if (GetRegion()->m_Selections[i]->GetType() != Node_Object)  bSelectionsOnly = TRUE;
		}
	}


	if( GetEditMode() == GEOMETRY_EDITMODE )
	{
		ray = ViewDef()->MakeRayFromScreenPoint( point );

		vert = GetClosestVert( point, bSelectionsOnly );
		edge = GetClosestEdge( point, bSelectionsOnly );
		poly = CastRayAtPolies( ray, NULL, 0.0f, bSelectionsOnly );
			
		brush.Init(poly.m_pBrush);
		
		if( (vert != IVert()) || (edge != IEdge()) || (poly != IPoly()) || (brush != IBrush()) )
			bRet = TRUE;
	}

	IVert() = vert;
	IEdge() = edge;
	IPoly() = poly;
	IBrush() = brush;
	
	return bRet;
}


void CRegionView::UpdateCursor()
{
	HCURSOR		theCursor;

	if( IsParallelViewType() && (GetEditMode() == BRUSH_EDITMODE || GetEditMode() == OBJECT_EDITMODE) && GetRegion()->GetNumSelections() > 0)
	{
		int32	handle = GetCurrentMouseOverHandle();
		LTVector		scaleNormal, scaleOrigin;

		if( handle != m_CurMouseOverHandle )
		{
			GetHandleInfo( handle, &theCursor, scaleNormal, scaleOrigin );
			m_CurMouseOverHandle = handle;

			::SetClassLong( m_hWnd, GCL_HCURSOR, (LONG)theCursor );
			//	SetCursor( theCursor );
		}
	}
	else
	{
		// Check to see if the cursor wasn't set back
		if (m_CurMouseOverHandle != -1)
		{
			::SetClassLong( m_hWnd, GCL_HCURSOR, (LONG)m_CursorArrow);
		}
		m_CurMouseOverHandle = -1;
	}
}


BOOL CRegionView::GetHandleInfo( int theHandle, HCURSOR *pCursor, CVector &scaleNormal, CVector &scaleOrigin )
{
	CReal		dot;


	if( theHandle == -1 )
	{
		if( pCursor )
			*pCursor = m_CursorArrow;
	}
	else
	{
		ASSERT( theHandle < 18 );

		// See if you shouldn't be resizing it, but moving the selections.
		dot = g_SelectionHandleVectors[theHandle].Dot( Nav().Forward() );

		if( fabs(dot) > 0.9f )
		{
			m_bSizeHandleSelected = FALSE;
			
			if( pCursor )
				*pCursor = m_CursorSizeAll;
		}
		else
		{
			m_bSizeHandleSelected = TRUE;
			
			if (pCursor)
			{
				if (theHandle < 6)
				{
					// Use the movement handle if it's competing for focus
					dot = g_SelectionHandleVectors[theHandle].Dot( Nav().Up() );
					
					if( pCursor )
						*pCursor = (fabs(dot) > 0.9f) ? m_CursorNS : m_CursorWE;
				}
				else
				{
					switch (theHandle)
					{				
						// XY handles
						case 6:		*pCursor=m_CursorNESW; break;
						case 7:		*pCursor=m_CursorNWSE; break;
						case 8:		*pCursor=m_CursorNWSE; break;
						case 9:		*pCursor=m_CursorNESW; break;

						// XZ handles
						case 10:	*pCursor=m_CursorNESW; break;
						case 11:	*pCursor=m_CursorNWSE; break;
						case 12:	*pCursor=m_CursorNWSE; break;
						case 13:	*pCursor=m_CursorNESW; break;

						// YZ handles
						case 14:	*pCursor=m_CursorNWSE; break;
						case 15:	*pCursor=m_CursorNESW; break;
						case 16:	*pCursor=m_CursorNESW; break;
						case 17:	*pCursor=m_CursorNWSE; break;
					}
				}
			}

			scaleNormal = g_SelectionHandleVectors[theHandle];			
			scaleOrigin.x = m_BoxMiddle.x - (scaleNormal.x * m_BoxHalf.x);
			scaleOrigin.y = m_BoxMiddle.y - (scaleNormal.y * m_BoxHalf.y);
			scaleOrigin.z = m_BoxMiddle.z - (scaleNormal.z * m_BoxHalf.z);
		}
	}

	return m_bSizeHandleSelected;
}


int32 CRegionView::GetCurrentMouseOverHandle()
{
	CVector test;
	CPoint point = GetCurMousePos();
	CVector vecPoint( (CReal)point.x, (CReal)point.y, 0.0f );
	int i, minIndex;
	CReal testDist, minDist = (CReal)MAX_CREAL;

	// Search the first 6 handles (this makes it easy to grab the move handle)
	minIndex = -1;	
	for( i=0; i < 6; i++ )
	{
		if( m_HandlesInFrustum[i] == 1 )
		{
			test.Init( (CReal)m_HandlePos[i].x, (CReal)m_HandlePos[i].y, 0.0f );
			
			if( (vecPoint - test).Mag() < 8.0f )
			{
				if (fabs(g_SelectionHandleVectors[i].Dot( Nav().Forward())) > 0.9f)
				{
					return i;
				}

				testDist = (m_BoxHandles[i] - Nav().Pos()).Mag();
				if( testDist < minDist )
				{
					minDist = testDist;
					minIndex = i;									
				}
			}
		}
	}
	if (minIndex != -1)
	{
		return minIndex;
	}

	// Search the rest of the handles
	for( i=6; i < NUM_BOX_HANDLES; i++ )
	{
		if( m_HandlesInFrustum[i] )
		{
			test.Init( (CReal)m_HandlePos[i].x, (CReal)m_HandlePos[i].y, 0.0f );
			
			if( (vecPoint - test).Mag() < 8.0f )
			{
				testDist = (m_BoxHandles[i] - Nav().Pos()).Mag();
				if( testDist < minDist )
				{
					minDist = testDist;
					minIndex = i;

					// SHP 4/13/1999
					return i;
				}
			}
		}
	}
	return minIndex;
}

BOOL CRegionView::IsKeyDown( int key )
{
	return ( GetAsyncKeyState(key) & 0x8000 );
}


void CRegionView::SplitEdges()
{
	CEdgeRefArray		list;
	CEditVert			newPt;
	CEdgeRef			*pEdge;
	CEditPoly			*pPoly;
	CEditBrush			*pBrush;
	uint32				i, k, e;
	uint32				index;
	PreActionList actionList;

	
	if( MakeTaggedEdgeList(list) )
	{
		// Setup an undo.
		for(e=0; e < list; e++)
		{
			AddToActionListIfNew(&actionList, new CPreAction(ACTION_MODIFYNODE, list[e].m_pBrush), TRUE);
		}

		GetRegionDoc()->Modify(&actionList, TRUE);


		for( e=0; e < list; e++ )
		{
			pEdge = &list[e];
			pBrush = pEdge->m_pBrush;

			CEditVert	&vert1 = pEdge->Vert1();
			CEditVert	&vert2 = pEdge->Vert2();

			newPt = vert1 + ((vert2 - vert1) * 0.5f);
			pBrush->m_Points.Append( newPt );
			index = pBrush->m_Points.LastI();
		
			// Find every polygon with this edge.
			for( i=0; i < pBrush->m_Polies; i++ )
			{
				pPoly = pBrush->m_Polies[i];
				for( k=0; k < pPoly->NumVerts(); k++ )
				{
					if( ((pPoly->m_Indices[k] == pEdge->m_Vert1) && (pPoly->m_Indices.Next(k) == pEdge->m_Vert2)) ||
						 ((pPoly->m_Indices[k] == pEdge->m_Vert2) && (pPoly->m_Indices.Next(k) == pEdge->m_Vert1)) )
					{
						pPoly->m_Indices.Insert( k+1, index );
						break;
					}
				}
			}
		}
	}
}


//------------------------------------------------------------------------------------
//
// CRegionView::DeleteSelectedNodes()
//
// Purpose:  Delete all selected nodes, which includes brushes and objects.
//
//------------------------------------------------------------------------------------
void CRegionView::DeleteSelectedNodes()
{
	CWorldNode		*pNode;
	uint32			i, index;
	LPOS lPos;
	
	// Tag document as changed...
	GetRegionDoc()->SetupUndoForSelections();

	// Loop over all selections and delete the brushes or objects...
	for( i=0; i < GetRegion()->m_Selections; i++ )
	{
		// Get the node...
		pNode = GetRegion()->m_Selections[i];
		ASSERT( pNode );

		// Remove this node from node view tree...
		GetNodeView( )->RemoveFromTree( pNode );
		GetNodeView( )->DeleteNode( pNode );

		if( pNode->GetParent( ))
			pNode->RemoveFromTree( );

		// Delete the brush...
		if(pNode->GetType() == Node_Brush)
		{
			GetRegion()->RemoveBrush(pNode->AsBrush());
		}
		// Delete the object...
		else if( pNode->m_Type == Node_Object )
		{
			GetPropertiesDlg()->TermIf( &pNode->m_PropList );
			GetRegion()->RemoveObject(pNode->AsObject());
		}

		// Remove the object from the list of path nodes
		GetRegion()->RemoveNodeFromPath(pNode);
	}

	for( i=0; i < GetRegion()->m_Selections; i++ )
	{
		// Get the node...
		pNode = GetRegion()->m_Selections[i];
		pNode->ClearFlag(NODEFLAG_SELECTED);

		delete pNode;
	}
		
	// Clear the selections...
	GetRegion()->m_Selections.Term();
	GetRegionDoc()->NotifySelectionChange();

	// Force all views to redraw...
	GetRegionDoc()->UpdateAllViews( this );
}


void CRegionView::DeleteTaggedVertices()
{
	CVertRefArray		verts;
	uint32				i;
	int32				k, j;
	CEditPoly			*pPoly;
	CEditBrush			*pBrush;
	PreActionList actionList;


	if( TaggedVerts().GenGetSize() > 0 )
	{
		verts.GenCopyList(TaggedVerts());

		for(i=0; i < verts; i++)
		{
			AddToActionListIfNew(&actionList, new CPreAction(ACTION_MODIFYNODE, verts[i].m_pBrush), TRUE);
		}
	}
	else if( IVert().IsValid() )
	{
		verts.Append( IVert() );

		actionList.Append(new CPreAction(ACTION_MODIFYNODE, IVert().m_pBrush));
	}
	else
		return;

	GetRegionDoc()->Modify(&actionList, TRUE);


	for( i=0; i < verts; i++ )
	{
		pBrush = verts[i].m_pBrush;

		for( k=0; k < (int32)pBrush->m_Polies; k++ )
		{
			pPoly = pBrush->m_Polies[k];

			for( j=0; j < (int32)pPoly->NumVerts(); j++ )
			{
				if( pPoly->Index(j) == verts[i].m_iVert )
				{
					if( pPoly->NumVerts() <= 3 )
					{
						delete pPoly;
						pBrush->m_Polies.Remove( k );
						--k;
						break;
					}
					else
					{
						pPoly->m_Indices.Remove(j);
						--j;
					}
				}
			}
		}

		pBrush->RemoveVertex( verts[i].m_iVert );
		for( k=(int32)i+1; k < (int32)verts; k++ )
			if( (verts[k].m_pBrush == verts[i].m_pBrush) && (verts[k].m_iVert >= verts[i].m_iVert) )
				verts[k].m_iVert--;
	}

	UpdatePlanesOfPolies( verts );

	for(LPOS iterateMPos=m_pRegion->m_Brushes; iterateMPos; )
	{
		CEditBrush *pIterateBrush = m_pRegion->m_Brushes.GetNext(iterateMPos);
		pIterateBrush->RemoveUnusedVerts();
	}

	m_pRegion->RemoveUnusedBrushes();

	UpdateImmediateSelection();
	ClearSelections();
	GetDocument()->UpdateAllViews( this );
}


void CRegionView::DeleteTaggedEdges( BOOL bRemoveOriginalPolyPoints )
{
	CEditPoly				*pPoly1, *pPoly2, *pNew;
	uint32					nContaining, i;

	CMoArray<CEditPoly*>	excludeList;
	CMoDWordArray			pointList;
	CMoArray<CEditPoly*>	changedList;


	if( IEdge().IsValid() )
	{
		GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_MODIFYNODE, IEdge().m_pBrush), TRUE);

		if( IEdge().m_pBrush->FindPoliesWithEdge(IEdge().m_Vert1, IEdge().m_Vert2, pPoly1, pPoly2, nContaining) )
		{
			pNew = IEdge().m_pBrush->JoinPolygons( pPoly1, pPoly2, IEdge().m_Vert1, IEdge().m_Vert2 );
			pNew->UpdatePlane();

			// Remove the points from any connected polies.
			pointList.Append( IEdge().m_Vert1 );
			pointList.Append( IEdge().m_Vert2 );
			
			if( bRemoveOriginalPolyPoints )
				IEdge().m_pBrush->RemovePolyVerts( pointList, excludeList, changedList );

			IEdge().m_pBrush->RemoveUnusedVerts();
			GetRegion()->UpdateBrushGeometry(IEdge().m_pBrush);

			UpdateImmediateSelection();
			ClearSelections();
			GetDocument()->UpdateAllViews( this );
		}
	}
}


void CRegionView::DeleteTaggedPolygons()
{
	CPolyRefArray &aTaggedPolys = GetRegionDoc()->m_TaggedPolies;
	// Don't delete the "immediate" poly unless it's the only one in the array
	uint32 nStartPoly = (aTaggedPolys.GetSize() > 1) ? 1 : 0;
	uint32 nCurPoly, nPolyCount = 0;

	// Get the set of brushes we're going to be operating on
	typedef std::set<CEditBrush*> TBrushSet;
	TBrushSet aBrushSet;
	for (nCurPoly = nStartPoly; nCurPoly < aTaggedPolys.GetSize(); ++nCurPoly)
	{
		CPolyRef &cCurPoly = aTaggedPolys[nCurPoly];
		if( cCurPoly.IsValid() )
		{
			aBrushSet.insert(cCurPoly.m_pBrush);
			++nPolyCount;
		}
	}

	// You don't seem to have anything valid selected....
	if (aBrushSet.empty())
		return;

	// Set up an undo
	PreActionList actionList;
	TBrushSet::iterator iCurBrush = aBrushSet.begin();
	for (; iCurBrush != aBrushSet.end(); ++iCurBrush)
	{
		actionList.Append(new CPreAction(ACTION_MODIFYNODE, *iCurBrush));
	}
	GetRegionDoc()->Modify(&actionList, TRUE);

	// Remove the polys from the brushes
	// Note : This can't just delete them because the renderer might want to
	// draw during this function.  (For some unknown reason....)
	typedef std::vector<CEditPoly *> TPolyList;
	TPolyList aGarbage;
	aGarbage.reserve(nPolyCount);
	for (nCurPoly = nStartPoly; nCurPoly < aTaggedPolys.GetSize(); ++nCurPoly)
	{
		CPolyRef &cCurPoly = aTaggedPolys[nCurPoly];
		if( cCurPoly.IsValid() )
		{
			cCurPoly.m_pBrush->m_Polies[cCurPoly.m_iPoly] = 0;
			aGarbage.push_back(cCurPoly());
			cCurPoly.m_pBrush = 0;
		}
	}

	// Clean up the brushes
	for (iCurBrush = aBrushSet.begin(); iCurBrush != aBrushSet.end(); ++iCurBrush)
	{
		CEditBrush *pCurBrush = *iCurBrush;
		for (uint32 nCleanupPolyArray = 0; nCleanupPolyArray < pCurBrush->m_Polies.GetSize(); )
		{
			if (pCurBrush->m_Polies[nCleanupPolyArray])
				++nCleanupPolyArray;
			else
				pCurBrush->m_Polies.Remove(nCleanupPolyArray);
		}
		pCurBrush->RemoveUnusedVerts();
	}

	// Clean up the region & views
	m_pRegion->RemoveUnusedBrushes();
	UpdateImmediateSelection();
	ClearSelections();
	GetRegionDoc()->UpdateAllViews(this);
	DrawRect();

	// Actually delete the polys
	while (!aGarbage.empty())
	{
		delete aGarbage.back();
		aGarbage.pop_back();
	}
}

void CRegionView::DoFlipOperation()
{
	PreActionList actionList;
	CEditRegion *pRegion;

	pRegion = GetRegion();


	if( (pRegion->m_Selections == 0) || (pRegion->m_Selections[0]->GetType() != Node_Brush) )
	{
		AppMessageBox( "Can't flip without a selected brush", MB_OK );
		return;
	}

	uint uBrushLoop;

	// Add the brushes to the undo list
	for (uBrushLoop = 0; uBrushLoop < pRegion->m_Selections; uBrushLoop++)
	{
		// Add it to the undo list
		actionList.AddTail(new CPreAction(ACTION_MODIFYNODE, pRegion->m_Selections[uBrushLoop]));
	}

	GetRegionDoc()->Modify(&actionList, TRUE);

	// Go through the selected brushes
	for (uBrushLoop = 0; uBrushLoop < pRegion->m_Selections; ++uBrushLoop)
	{
		if (pRegion->m_Selections[uBrushLoop]->GetType() == Node_Brush) // verify selection is a brush
		{
			CEditBrush *pBrush = pRegion->m_Selections[uBrushLoop]->AsBrush();
	
			// Flip the polygons 
			for (uint32 uPolyLoop = 0; uPolyLoop < pBrush->m_Polies; ++uPolyLoop)
			{
				pBrush->m_Polies[uPolyLoop]->Flip();
			}

			// Remember to update the brush
			pBrush->UpdatePlanes();
		}
	}

	// Mmmm..  View update...
	GetRegionDoc()->RedrawAllViews();
}

/************************************************************************/
// Offsets (moves them in the world) the selected nodes by a vector
void CRegionView::OffsetSelectedNodes(CVector vOffset)
{
	// Setup the undo
	GetRegionDoc()->SetupUndoForSelections();

	// Get the region
	CEditRegion *pRegion=GetRegion();
	pRegion->OffsetSelectedNodes(vOffset);

	// Resize the box encompassing all the selections...
	GetRegionDoc()->UpdateSelectionBox();

	// Redraw the views
	GetRegionDoc()->UpdateAllViews( this );
}

/************************************************************************/
// Offsets the texture map for the selected polygon in the specified
// direction (use 1 for positive, -1 for negative, or zero for no
// movement).  The grid size is used to offset the texture.  If SHIFT
// is pressed, then the texture is moved one unit at a time.
void CRegionView::OffsetTextureForPoly(int nPDirection, int nQDirection, BOOL bGrid)
{
	if (!IPoly().IsValid())
		return;

	// Get the polygon
	CEditPoly *pPoly = IPoly()();		
	if(!pPoly)
	{
		return;
	}

	//make the undo for this node
	GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_MODIFYNODE, pPoly->m_pBrush), TRUE);

	// Figure out the amount to offset the texture by
	float fOffsetSize=0.0f;
	if (bGrid)
	{
		fOffsetSize=1.0f;
	}
	else
	{
		fOffsetSize=(float)m_dwGridSpacing;
	}
	
	// Determine the new O vector
	CTexturedPlane& Texture = pPoly->GetTexture(GetCurrTexture());

	CVector O=Texture.GetO()+(Texture.GetP()*fOffsetSize*(float)nPDirection)+(Texture.GetQ()*fOffsetSize*(float)nQDirection);	

	// Set the texture space on the polygon
	pPoly->SetTextureSpace(GetCurrTexture(), O, Texture.GetP(), Texture.GetQ());	

	// Redraw the views
	GetRegionDoc()->RedrawAllViews();				
}

/************************************************************************/
// Scale the texture map for the selected polygon in the specified
// direction (use 1 to expand, -1 to shrink, or zero for neither)
// The grid size is used to scale the texture.
void CRegionView::ScaleTextureForPoly(int nPDirection, int nQDirection)
{
	if (!IPoly().IsValid())
		return;

	// Get the polygon
	CEditPoly *pPoly = IPoly()();		
	if(!pPoly)
	{
		return;
	}

	// Check to see if the polygon has a texture
	CTexturedPlane& Texture = pPoly->GetTexture(GetCurrTexture());
	if(!Texture.m_pTextureFile)
	{
		return;
	}

	// Get the texture
	CTexture *pTexture=dib_GetDibTexture(Texture.m_pTextureFile);
	if(!pTexture || !pTexture->m_pDib)
	{
		return;
	}

	//make the undo for this node
	GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_MODIFYNODE, pPoly->m_pBrush), TRUE);

	// Get the width and the height of the texture		
	float fWidth = (float)pTexture->m_pDib->GetWidth();
	float fHeight = (float)pTexture->m_pDib->GetHeight();

	// Figure out the scaling values
	float fWidthScale=1.0f;
	float fHeightScale=1.0f;
	
	if (nPDirection > 0)
	{
		// Expand		
		fWidthScale=fWidth/(fWidth-(float)m_dwGridSpacing);
	}
	else if (nPDirection < 0)
	{
		// Shrink
		fWidthScale=(fWidth-(float)m_dwGridSpacing)/fWidth;
	}

	if (nQDirection > 0)
	{
		// Expand		
		fHeightScale=fHeight/(fHeight-(float)m_dwGridSpacing);
	}
	else if (nQDirection < 0)
	{
		// Shrink
		fHeightScale=(fHeight-(float)m_dwGridSpacing)/fHeight;
	}	

	// Set the texture space on the polygon
	pPoly->SetTextureSpace(GetCurrTexture(), Texture.GetO(), Texture.GetP()*fWidthScale, Texture.GetQ()*fHeightScale);	

	// Redraw the views
	GetRegionDoc()->RedrawAllViews();				
}

/************************************************************************/
// Mirrors the selected brushes along one of the axis specified in vAxis.
// vAxis must either be (1, 0, 0) (0,1,0) or (0,0,1)
void CRegionView::MirrorSelectedNodes(CVector vAxis)
{
	DMatrix mOReflection, mPQReflection;
	DVector O, P, Q;
	uint32 iPoly;
	CEditPoly *pPoly;

	// Make sure that the axis is correct
	if (vAxis != CVector(1,0,0) && vAxis != CVector(0,1,0) && vAxis != CVector(0,0,1))
	{
		ASSERT(FALSE);
		return;
	}

	// Calculate the bounding box for the selected brushes
	CBoundingBox boundingBox;	

	// Get the edit region
	CEditRegion *pRegion=GetRegion();
	if (!pRegion)
	{
		return;
	}

	// Setup the undo
	GetRegionDoc()->SetupUndoForSelections();

	// Indicates that the following iterator is seeing the first selected brush
	BOOL bFirstSelection=TRUE;

	// Create the bounding box
	int i;
	for (i=0; i < pRegion->GetNumSelections(); i++)
	{
		// Check to see if this is a brush
		CWorldNode *pNode=pRegion->GetSelection(i);
		if (pNode->GetType() == Node_Brush)
		{
			// Get the node as a brush
			CEditBrush *pBrush=pNode->AsBrush();

			// Set the initial bounding box points
			if (bFirstSelection)
			{
				boundingBox.Init(pBrush->m_Points[0]);
				bFirstSelection=FALSE;
			}

			// Update the bounding box with the brush points
			int j;
			for (j=0; j < pBrush->m_Points.GetSize(); j++)
			{
				boundingBox.Extend(pBrush->m_Points[j]);
			}

			m_pView->GetRegion()->UpdateBrushGeometry(pBrush);
		}
		else if (pNode->GetType() == Node_Object)
		{
			CBaseEditObj *pObject=pNode->AsObject();

			// Set the initial bounding box points
			if (bFirstSelection)
			{
				boundingBox.Init(pObject->GetPos());
				bFirstSelection=FALSE;
			}
			else
			{
				// Update the bounding box
				boundingBox.Extend(pObject->GetPos());
			}
		}
	}

	// Get the center point of the bounding box
	CVector vCenter(boundingBox.m_Min.x+((boundingBox.m_Max.x-boundingBox.m_Min.x)/2),
					boundingBox.m_Min.y+((boundingBox.m_Max.y-boundingBox.m_Min.y)/2),
					boundingBox.m_Min.z+((boundingBox.m_Max.z-boundingBox.m_Min.z)/2));


	//the axis that we need to mirror the texture coordinates over
	//Note: This is not the same as the axis passed in. For example, if they
	//pass in the Y axis, then we need to mirror using the X Axis since
	//it takes a plane normal to use to reflect -JohnO
	LTVector vMirrorTexCoords;

	// Mirror the points for the selected nodes
	for (i=0; i < pRegion->GetNumSelections(); i++)
	{
		// Check to see if this is a brush
		CWorldNode *pNode=pRegion->GetSelection(i);
		if (pNode->GetType() == Node_Brush)
		{
			// Get the node as a brush
			CEditBrush *pBrush=pNode->AsBrush();

			// Mirror each point
			int j;
			for (j=0; j < pBrush->m_Points.GetSize(); j++)
			{
				// Get this point
				CVector *pVec=&pBrush->m_Points[j];

				if (vAxis.x == 1.0)
				{
					//determine the axis we need to mirror OPQ's over
					vMirrorTexCoords.Init(0, 1, 0);
					pVec->y += (vCenter.y-pVec->y)*2;
				}
				else if (vAxis.y == 1.0)
				{
					//determine the axis we need to mirror OPQ's over
					vMirrorTexCoords.Init(1, 0, 0);
					pVec->x += (vCenter.x-pVec->x)*2;
				}
				else if (vAxis.z == 1.0)
				{
					//determine the axis we need to mirror OPQ's over
					vMirrorTexCoords.Init(0, 0, 1);
					pVec->z += (vCenter.z-pVec->z)*2;
				}
			}

			// For texture coordinates..
			mOReflection.SetupReflectionMatrix(vMirrorTexCoords, vCenter);
			mPQReflection.SetupReflectionMatrix(vMirrorTexCoords, DVector(0.0f, 0.0f, 0.0f));


			pBrush->FlipNormals();
			
			// Update the polies with a stuck texture.
			for(iPoly=0; iPoly < pBrush->m_Polies; iPoly++)
			{
				pPoly = pBrush->m_Polies[iPoly];

				for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
				{
					CTexturedPlane& Texture = pPoly->GetTexture(nCurrTex);

					Texture.GetTextureSpace(O, P, Q);
					
					O = mOReflection * O;
					P = mPQReflection * P;
					Q = mPQReflection * Q;

					pPoly->SetTextureSpace(nCurrTex, O, P, Q);
				}
			}
		}
		else if (pNode->GetType() == Node_Object)
		{
			CBaseEditObj *pObject=pNode->AsObject();

			// Get this point
			LTVector vec=pObject->GetPos();

			if (vAxis.x == 1.0)
			{
				vec.y += (vCenter.y-vec.y)*2;
			}
			else if (vAxis.y == 1.0)
			{
				vec.x += (vCenter.x-vec.x)*2;
			}
			else if (vAxis.z == 1.0)
			{
				vec.z += (vCenter.z-vec.z)*2;
			}
			pObject->SetPos(vec);
		}
	}

	// Recalculate plane equations for all planes...
	GetRegion()->UpdatePlanes();
	GetRegion()->CleanupGeometry( );

	// Update the selection box
	GetRegionDoc()->UpdateSelectionBox();	

	// Update properties...
	GetRegionDoc()->SetupPropertiesDlg( FALSE );
}

bool CRegionView::ExtrudePoly( CPolyRef poly, CVertRefArray &newVerts, bool bRemoveOriginal, bool bFlipAll, bool bNewBrush)
{
	uint32			i;
	CEditPoly		*pExtruder;
	CEditPoly		*pCap, *pBase, *pNewPoly;
	CEditBrush		*pBrush;

	if(bNewBrush) 
	{
		// Setup a new brush
		pBrush = no_CreateNewBrush(GetRegion(), GetRegion()->GetActiveParentNode());
		ASSERT( pBrush );
		if( !pBrush )
			return false;
	}
	else
	{
		pBrush = poly.m_pBrush;
	}

	pExtruder = poly();
	newVerts.SetSize(0);
	
	// Make the cap polygon .. a copy of iPoly, reversed.
	pCap = new CEditPoly( pBrush );
	ASSERT( pCap );
	if( !pCap )
	{
		if(bNewBrush) 
			delete pBrush;
		return false;
	}

	pCap->m_Indices.SetSize( pExtruder->NumVerts() );
	newVerts.SetSize( pExtruder->NumVerts() );

	if(bNewBrush) // Making a new brush
	{
		// Make the base polygon .. the opposite of the cap
		pBase = new CEditPoly( pBrush );
		ASSERT( pBase );
		if( !pBase )
		{
			delete pBrush;
			return false;
		}

		pBase->m_Indices.SetSize( pExtruder->NumVerts() );

		// Make base poly
		for( i=0; i < pExtruder->NumVerts(); i++ )
		{
			CEditVert	vert =pExtruder->m_pBrush->m_Points[pExtruder->Index(i)];
			pBrush->m_Points.Append( vert );

			newVerts[i].Init( pBrush, i);
			pBase->m_Indices[i] = i;
		}
	}
		
	// Make cap poly	
	for( i=0; i < pExtruder->NumVerts(); i++ )	
	{
		CEditVert	vert = pExtruder->m_pBrush->m_Points[pExtruder->Index(i)];
		pBrush->m_Points.Append( vert );

		newVerts[i].Init( pBrush, pBrush->m_Points.LastI() );
		pCap->m_Indices[i] = pBrush->m_Points.LastI() ;
	}
	
	
	// For each edge, make a 4 point polygon connecting the edges of the 2 polies.
	for( i=0; i < pExtruder->m_Indices; i++ )
	{
		pNewPoly = new CEditPoly( pBrush );
		pNewPoly->m_Indices.SetSize(4);

		if(bNewBrush)
		{
			pNewPoly->m_Indices[0] = pBase->Index(i);
			pNewPoly->m_Indices[1] = pBase->NextIndex(i);
		}
		else
		{
			pNewPoly->m_Indices[0] = pExtruder->Index(i);
			pNewPoly->m_Indices[1] = pExtruder->NextIndex(i);
		}
		
		pNewPoly->m_Indices[2] = pCap->NextIndex(i);
		pNewPoly->m_Indices[3] = pCap->Index(i);

		// Setup texture stuff for poly
		for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
		{
			CTexturedPlane& ExtrudeTex = pExtruder->GetTexture(nCurrTex);
			CTexturedPlane& Texture    = pNewPoly->GetTexture(nCurrTex);

			pNewPoly->SetTextureSpace(nCurrTex, ExtrudeTex.GetO(), ExtrudeTex.GetP(), ExtrudeTex.GetQ());
			Texture.m_pTextureName = ExtrudeTex.m_pTextureName;
			Texture.m_pTextureFile = ExtrudeTex.m_pTextureFile;
		}

		pNewPoly->UpdatePlane();

		if( bFlipAll )
			pNewPoly->Flip();

		pBrush->m_Polies.Append( pNewPoly );
	}

	if( bFlipAll )
	{
		pCap->Flip();
	}

	if(bNewBrush)
	{
		// Flip base
		if(!bFlipAll)  pBase->Flip();

		// Setup texture stuff for the base.
		for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
		{
			CTexturedPlane& ExtrudeTex = pExtruder->GetTexture(nCurrTex);
			CTexturedPlane& Texture    = pBase->GetTexture(nCurrTex);
			
			pBase->SetTextureSpace(nCurrTex, ExtrudeTex.GetO(), ExtrudeTex.GetP(), ExtrudeTex.GetQ());
			Texture.m_pTextureName = ExtrudeTex.m_pTextureName;
			Texture.m_pTextureFile = ExtrudeTex.m_pTextureFile;
		}

		pBase->UpdatePlane();
		pBrush->m_Polies.Append( pBase );
	}

	// Setup texture stuff for the cap.
	for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
	{
		CTexturedPlane& ExtrudeTex = pExtruder->GetTexture(nCurrTex);
		CTexturedPlane& Texture    = pCap->GetTexture(nCurrTex);
		
		pCap->SetTextureSpace(nCurrTex, ExtrudeTex.GetO(), ExtrudeTex.GetP(), ExtrudeTex.GetQ());
		Texture.m_pTextureName = ExtrudeTex.m_pTextureName;
		Texture.m_pTextureFile = ExtrudeTex.m_pTextureFile;
	}

	pCap->UpdatePlane();
	pBrush->m_Polies.Append( pCap );
	
	if( bRemoveOriginal && !bNewBrush)
	{
		ClearSelections( FALSE, FALSE, TRUE );
		delete pExtruder;
		pBrush->m_Polies.Remove( poly.m_iPoly );
	}

	//update bounding information
	pBrush->UpdateBoundingInfo();

	return true;
}

void CRegionView::ClearSelections( BOOL bClearVerts, BOOL bClearEdges, BOOL bClearPolies, BOOL bClearBrushes )
{
	if( bClearVerts )
	{
		TaggedVerts().GenRemoveAll();
		IVert().Term();
	}

	if( bClearEdges )
		IEdge().Term();

	if( bClearPolies )
	{
		TaggedPolies().Term();
		TaggedPolies().SetSize(1);
	}

	if( bClearBrushes )
		IBrush().Term();
}

void CRegionView::FinishDrawingPoly()
{
	CEditBrush		*pNewBrush;
	CStringDlg		dlg;
	CPolyRef		polyRef;
	CReal			thickness;
	uint32			i;
	CVertRefArray	newVerts;
	CString str;
	BOOL			bOk, bValidNumber;


	// Create the new brush and setup an undo.
	pNewBrush = no_CreateNewBrush(GetRegion(), GetRegion()->GetActiveParentNode());
	GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_ADDEDNODE, pNewBrush), TRUE);


	// Ask them for a brush thickness if they're in brush edit mode.
	if( GetEditMode() == BRUSH_EDITMODE )
	{
		//load the default in from the registry
		uint32 nDefaultBrushSize = ::GetApp()->GetOptions().GetDWordValue("DefaultBrushSize", 64);

		dlg.m_bAllowNumbers = TRUE;
		bOk = TRUE;
		bValidNumber = FALSE;
		dlg.m_EnteredText.Format( "%d", nDefaultBrushSize );
		while( bOk && !bValidNumber )
		{
			if( bOk = ( dlg.DoModal(IDS_NEWBRUSHCAPTION, IDS_ENTERBRUSHTHICKNESS) == IDOK ))
			{
				thickness = (CReal)atoi( dlg.m_EnteredText );
				if( thickness > 0.0f )
					bValidNumber = TRUE;
				else
					AppMessageBox( IDS_ERROR_BRUSH_THICKNESS, MB_OK );
			}
		}

		if( bOk )
		{
			//save this value for future use
			::GetApp()->GetOptions().SetDWordValue("DefaultBrushSize", (DWORD)thickness);

			SetupDrawingBrush( pNewBrush );
			polyRef.Init( pNewBrush, 0 );
			
			if( ExtrudePoly( polyRef, newVerts, FALSE, TRUE /*flip all*/) )
			{
				for( i=0; i < newVerts; i++ )
					newVerts[i]() -= polyRef()->Normal() * thickness;

				for( i=0; i < pNewBrush->m_Polies; i++ )
				{
					pNewBrush->m_Polies[i]->UpdatePlane();

					for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
					{
						pNewBrush->m_Polies[i]->SetupBaseTextureSpace(nCurrTex);
						pNewBrush->m_Polies[i]->GetTexture(nCurrTex).UpdateTextureID();
					}
				}
			}
			else
			{
				ASSERT( FALSE );
			}				
		}
		else
		{
			no_DestroyNode(GetRegion(), pNewBrush, FALSE);
			DrawingBrush().Term();
			m_EditState = EDIT_NOSTATE;

			// Redraw everything.
			GetRegionDoc()->RedrawAllViews();
			return;
		}
	}
	else
	{
		// Make the new brush.
		SetupDrawingBrush( pNewBrush );
	}

	//update the bounding information for this polygon
	pNewBrush->UpdateBoundingInfo();

	// Close everything.
	DrawingBrush().Term();
	m_EditState = EDIT_NOSTATE;
	UpdateImmediateSelection();


	// Make the new brush the selection.
	if( GetMainFrame()->GetNodeSelectionMode() != MULTI_SELECTION )
		GetRegion()->ClearSelections();
	
	GetRegion()->SelectNode( pNewBrush->AsNode() );
	GetRegionDoc()->NotifySelectionChange();

	// Redraw everything.
	GetRegionDoc()->RedrawAllViews();
}

#pragma optimize ("", on)

void CRegionView::SetupDrawingBrush( CEditBrush* pNewBrush )
{
	CEditBrush		*pDrawingBrush;
	CEditPoly		*pNew;
	uint32			i;
	char			*pTexName;
	char			*pPhysicsName;

	pDrawingBrush = &DrawingBrush();

	// Finish the poly.

	pDrawingBrush->m_Points.Pop();
	pDrawingBrush->m_Points.Pop();


	// Add on a new brush.
	pNewBrush->CopyEditBrush( pDrawingBrush );

	// Add on a new poly.
	pNew = new CEditPoly( pNewBrush );
	pNewBrush->m_Polies.Append( pNew );


	// Setup the new poly.
	pNew->m_Indices.SetSize( pNewBrush->m_Points.GetSize() );
	for( i=0; i < pNew->m_Indices; i++ )
		pNew->m_Indices[i] = i;
	
	// Make sure it points towards the viewer.

	pNew->UpdatePlane();
	if( (pNew->Normal().Dot(Nav().Pos()) - pNew->Dist()) < 0 )
		pNew->Flip();

	for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
		pNew->SetupBaseTextureSpace(nCurrTex);
	
	// if a texture is selected, assign it
	if( GetCurrentTextureName(pTexName) )
		pNew->GetTexture(GetCurrTexture()).m_pTextureName = pTexName;

	pNew->GetTexture(GetCurrTexture()).UpdateTextureID();
}

#pragma optimize ("", on)

void CRegionView::SplitPolyWithEdge( CPolyRef poly, CEditVertArray &verts )
{
	CEditBrush	*pBrush = poly.m_pBrush;
	CEditPoly	*pPoly = poly();

	// Find the points in the brush
	uint32 nBrushIndex[2];
	nBrushIndex[0] = pBrush->FindClosestVert(verts[0]);
	nBrushIndex[1] = pBrush->FindClosestVert(verts.Last());
	// Deal gracefully with vertices that resolve to the same or invalid indices
	if ((nBrushIndex[0] == nBrushIndex[1]) ||
		(nBrushIndex[0] > pBrush->m_Points.GetSize()) ||
		(nBrushIndex[1] > pBrush->m_Points.GetSize()))
		return;

	// Find the indices in the polygon
	uint32 nPolyIndex[2];
	nPolyIndex[0] = pPoly->m_Indices.FindElement(nBrushIndex[0]);
	nPolyIndex[1] = pPoly->m_Indices.FindElement(nBrushIndex[1]);
	// Deal gracefully with bad indices
	if ((nPolyIndex[0] == nPolyIndex[1]) ||
		(nPolyIndex[0] >= pPoly->m_Indices) || 
		(nPolyIndex[1] >= pPoly->m_Indices))
		return;

	// Remember how many vertices we originally had...
	uint32 nTotalVertices = pPoly->m_Indices.GetSize();
	
	// Make a new poly
	CEditPoly *pNewPoly = new CEditPoly(pBrush);
	pNewPoly->CopyEditPoly(pPoly);

	// Add the new poly to the brush
	pBrush->m_Polies.Append(pNewPoly);

	// Sort them...
	if (nPolyIndex[0] > nPolyIndex[1])
	{
		uint32 nTemp = nPolyIndex[1];
		nPolyIndex[1] = nPolyIndex[0];
		nPolyIndex[0] = nTemp;
		CEditPoly *pTemp = pNewPoly;
		pNewPoly = pPoly;
		pPoly = pTemp;
	}

	// Delete the extra vertices from the original poly
	uint32 nRemoveLoop;
	for (nRemoveLoop = 0; nRemoveLoop < nPolyIndex[0]; ++nRemoveLoop)
		pPoly->m_Indices.Remove(0);
	for (nRemoveLoop = nPolyIndex[1] + 1; nRemoveLoop < nTotalVertices; ++nRemoveLoop)
		pPoly->m_Indices.Remove(pPoly->m_Indices.GetSize() - 1);

	// Delete the extra vertices from the new poly
	for (nRemoveLoop = nPolyIndex[0] + 1; nRemoveLoop < nPolyIndex[1]; ++nRemoveLoop)
		pNewPoly->m_Indices.Remove(nPolyIndex[0] + 1);

	// Clean up both polys
	pNewPoly->UpdatePlane();
	pPoly->UpdatePlane();
}

void CRegionView::OnImportTerrainMap( )
{
	CString importExt, fileMask, pathName, sFileName;
	CWorldNode *pNode;

	if( (GetRegion( )->m_Selections != 1) || 
		((pNode = GetRegion()->m_Selections[0])->GetType() != Node_Brush))
	{
		AppMessageBox( IDS_ERR_SINGLE_BRUSH, MB_OK );
		return;
	}

	importExt.LoadString( IDS_PCX_EXTENSION );
	fileMask.LoadString( IDS_PCX_FILEMASK );

	sFileName = GetProject()->m_BaseProjectDir + "\\Textures\\*" + importExt;
	CHelperFileDlg	dlg( TRUE, importExt, (LPCTSTR)sFileName, OFN_FILEMUSTEXIST, fileMask, this );
	if( dlg.DoModal() != IDOK )
		return;

	pathName = dlg.GetPathName( );
	
	DoImportTerrainMap(pNode->AsBrush(), pathName);
}

void SnapVector(CVector *pVec)
{
	pVec->x = (float)floor(pVec->x + 0.5f);
	pVec->y = (float)floor(pVec->y + 0.5f);
	pVec->z = (float)floor(pVec->z + 0.5f);
}

//------------------------------------------------------------------------------
//
//  CRegionView::DoImportTerrainMap
//
//	Creates a brushes represented by pixels in a PCX file, scaled by a bounding
//  brush.
//
//  pBrush is used as the bounding brush.  The extents of the brush enclose all
//  the new brushes.  The new brushes are scaled to completely fill the enclosing
//  brush.
//
//  The PCX file must be at least 2 pixels wide and 2 pixels high.  New brushes
//	are created in groups of 4.  Each one has a flat bottom and flat sides, but
//	the top can be sloped.  The base and top of each brush is a triangle, and the
//	4 brushes in a group fit together to make a square.  The height of the outside
//	corners of this square will be the height of the bounding brush times the 
//	corresponding pixel value divided by 255.  This means the pixel value is the
//	height scale.  The height of the center of the four brushes is the average
//	of the 4 corners.  If all 4 brushes have zero height, then none of the 4 are
//	created.  Each group of 4 pixels defines the heights of the 4 corners.
//
//  The upper left group of 4 pixels of the PCX file correspond to the corner
//	of the bounding brush with the maximum z and minimum x.  The lower right
//	group of 4 pixels of the PCX file correspond to the corner of the bounding
//	brush with the minimum z and maximum x.
//
//------------------------------------------------------------------------------
void CRegionView::DoImportTerrainMap( CEditBrush *pBrush, CString &pathName )
{
	CReal x_min, x_max, y_min, y_max, z_min, z_max;
	CReal x, z, x_delta, z_delta, thickness, xHigh, zHigh, x_half, z_half;

	CEditBrush *pNewBrush;
	CEditPoly *pNewPoly;
	CEditVert vEditVerts[3];
	CPolyRef polyRef;
	int u, v, iBrush;
	CMoFileIO outFile;
	LoadedBitmap pcxTexture;
	CWorldNode *pParent;
	CVertRefArray newVerts;
	int bits, planes;
	DStream *pInFile;
	BOOL bRet;


	BeginWaitCursor( );

	pInFile = streamsim_Open(pathName, "rb");
	if(!pInFile)
	{
		AppMessageBox( IDS_ERR_OPENFILE, MB_OK );
		EndWaitCursor( );
		return;
	}

	bRet = pcx_Create2(pInFile, &pcxTexture );
	pInFile->Release();

	if(!bRet)
	{
		AppMessageBox( IDS_ERR_TERRAINMAP_INVALID, MB_OK );
		EndWaitCursor( );
		return;
	}

	if(pcxTexture.m_Width < 2 || pcxTexture.m_Height < 2 || 
		pcxTexture.m_Format.m_BPP != BPP_8P)
	{
		AppMessageBox( IDS_ERR_TERRAINMAP_INVALIDDIMS, MB_OK );
		EndWaitCursor( );
		return;
	}

	// Add the null node to be parent...
	pParent = GetRegion()->AddNullNode(GetRegion()->GetRootNode());
	ASSERT( pParent );

	// Get x, y, z extents of brush...
	CBoundingBox BBox = pBrush->CalcBoundingBox();

	x_min = BBox.m_Min.x;
	y_min = BBox.m_Min.y;
	z_min = BBox.m_Min.z;
	x_max = BBox.m_Max.x;
	y_max = BBox.m_Max.y;
	z_max = BBox.m_Max.z;

	// Calculate the dimensions a 4 pixel group will map to...
	x_delta = ( x_max - x_min ) / ( pcxTexture.m_Width - 1 );
	z_delta = ( z_max - z_min ) / ( pcxTexture.m_Height - 1 );
	thickness = ( y_max - y_min );

	// Turn off the drawing of the node view
	if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
	{
		GetNodeView()->SetRedraw(FALSE);
	}

	// Loop over the z & v variabls...
	for( z = z_min, v = pcxTexture.m_Height - 2; v >= 0; z += z_delta, v-- )
	{
		// Get the next z coordinate...
		zHigh = z + z_delta;
		
		// Get the middle of the 4 pixel area...
		z_half = z + z_delta * 0.5f;

		// Loop over the x & u variables...
		for( x = x_min, u = 0; u < (int)(pcxTexture.m_Width - 1); x += x_delta, u++ )
		{
			// Get the next x coordinate...
			xHigh = x + x_delta;

			// Get the middle of the 4 pixel area...
			x_half = x + x_delta * 0.5f;

			// Find the heights for the 4 corners and middle...
			CReal y[5];
			y[0] = ( pcxTexture.Pixel( u, v ) / 255.0f ) * thickness;
			y[1] = ( pcxTexture.Pixel( u+1, v ) / 255.0f ) * thickness;
			y[2] = ( pcxTexture.Pixel( u+1, v+1 ) / 255.0f ) * thickness;
			y[3] = ( pcxTexture.Pixel( u, v+1 ) / 255.0f ) * thickness;
			y[4] = ( y[0] + y[1] + y[2] + y[3] ) / 4.0f;

			// Skip this 4 pixel area if there is no height...
			if( y[4] < 0.0001f )
				continue;

			// Find the x/y/z coordinates for the 4 corners ( middle already known )...
			// This defines a table of values so that each brush can be created in a loop...
			CReal xVert[4][2], zVert[4][2], yVert[4][2];
			xVert[0][0] = x;		xVert[0][1] = x;
			yVert[0][0] = y[0];		yVert[0][1] = y[3];
			zVert[0][0] = zHigh;	zVert[0][1] = z;
			xVert[1][0] = xHigh;	xVert[1][1] = x;
			yVert[1][0] = y[1];		yVert[1][1] = y[0];
			zVert[1][0] = zHigh;	zVert[1][1] = zHigh;
			xVert[2][0] = xHigh;	xVert[2][1] = xHigh;
			yVert[2][0] = y[2];		yVert[2][1] = y[1];
			zVert[2][0] = z;		zVert[2][1] = zHigh;
			xVert[3][0] = x;		xVert[3][1] = xHigh;
			yVert[3][0] = y[3];		yVert[3][1] = y[2];
			zVert[3][0] = z;		zVert[3][1] = z;

			// Loop over the 4 brushes to be created out of the 4 pixel area...
			for( iBrush = 0; iBrush < 4; iBrush++ )
			{
				// Setup a new brush and a new starting poly...
				pNewBrush = new CEditBrush;
				pNewPoly = new CEditPoly( pNewBrush );
				pNewBrush->m_Polies.Append( pNewPoly );

				vEditVerts[2].Init( xVert[iBrush][0], y_min, zVert[iBrush][0] );
				vEditVerts[1].Init( x_half, y_min, z_half );
				vEditVerts[0].Init( xVert[iBrush][1], y_min, zVert[iBrush][1] );

				SnapVector(&vEditVerts[0]);
				SnapVector(&vEditVerts[1]);
				SnapVector(&vEditVerts[2]);

				// Make bottom of brush
				pNewPoly->m_Indices.Append( 0 );
				pNewBrush->m_Points.Append( vEditVerts[0] );
				pNewPoly->m_Indices.Append( 1 );
				pNewBrush->m_Points.Append( vEditVerts[1] );
				pNewPoly->m_Indices.Append( 2 );
				pNewBrush->m_Points.Append( vEditVerts[2] );

				// Set normal...
				pNewPoly->Normal().Init( 0.0f, -1.0f, 0.0f );
				pNewPoly->Dist() = pNewPoly->Normal().Dot( pNewPoly->Pt(0) );

				// Extrude...
				polyRef.Init( pNewBrush, 0 );
				if( ExtrudePoly( polyRef, newVerts, FALSE, TRUE /*flip all*/) )
				{
					CEditVert *pVert;
					pVert = &newVerts[0]( );
					newVerts[2]() -= polyRef()->Normal() * yVert[iBrush][0];
					newVerts[1]() -= polyRef()->Normal() * y[4];
					newVerts[0]() -= polyRef()->Normal() * yVert[iBrush][1];

					for( uint32 j=0; j < pNewBrush->m_Polies; j++ )
					{
						pNewBrush->m_Polies[j]->UpdatePlane();

						for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
							pNewBrush->m_Polies[j]->SetupBaseTextureSpace(nCurrTex);					}
				}

				// Setup the node stuff.
				SetupBrushProperties(pNewBrush, GetRegion());
				GetRegion()->AttachNode(pNewBrush->AsNode(), pParent);
				GetNodeView( )->AddNode(pNewBrush->AsNode());

				// Add on the new brush.
				GetRegion()->AddBrush(pNewBrush);
			}
		}
	}	
	
	// Start drawing the node view again
	if (GetProjectBar()->m_pVisibleDlg == GetNodeView())
	{
		// Turn redraw back on
		GetNodeView()->SetRedraw(TRUE);

		// Redraw the window
		GetNodeView()->Invalidate();	
	}

	GetRegion( )->UpdatePlanes();
	GetRegion( )->CleanupGeometry();
	GetRegionDoc()->RedrawAllViews();

	EndWaitCursor( );
}



// talon: the primitives: 2001 has them here (0x46ab4c..0x46c079), Build 69
// in CreatePrimitive.cpp.

// talon: the radius comes back through a float return (the /Op round trip
// through a temporary at 0x46b9a7, 0x46c00f): 2001's settings have an inline
// accessor that OptionsPrimitives.h lacks.
static inline float PrimitiveRadius(CPrimitiveSettings *pSettings)
{
	return pSettings->m_fRadius;
}


void CRegionView::OnCreatePrimitiveBox( )
{
	CStringDlg		dlg;
	BOOL			bOk;
	BOOL			bValidNumber;
	CReal			thickness;

	dlg.m_bAllowNumbers = TRUE;
	bOk = TRUE;
	bValidNumber = FALSE;
	
	dlg.m_EnteredText.Format( "%d", 128 );

	while( bOk && !bValidNumber )
	{
		if( bOk = ( dlg.DoModal(IDS_NEWBRUSHCAPTION, IDS_ENTERBRUSHTHICKNESS) == IDOK ))
		{
			thickness = (CReal)atoi( dlg.m_EnteredText );
			if( thickness > 0.0f )
				bValidNumber = TRUE;
			else
				AppMessageBox( IDS_ERROR_BRUSH_THICKNESS, MB_OK );
		}
	}
	
	if( bOk )
		DoCreatePrimitiveBox( GetRegion( )->m_vMarker, thickness );
}


//------------------------------------------------------------------------------
//
//  CRegionView::DoCreatePrimitiveBox
//
//	Creates a brush in the shape of a box.
//
//------------------------------------------------------------------------------
void CRegionView::DoCreatePrimitiveBox( CVector &vCenter, CReal fSide )
{
	CEditBrush *pNewBrush;
	CEditPoly *pNewPoly;
	CReal fHalfSide;
	CPolyRef polyRef;
	CVertRefArray newVerts;
	DWORD i;
	CEditVert vEditVerts[4];

	fHalfSide = fSide / 2.0f;

	// Setup a new brush and a new starting poly...
	pNewBrush = no_CreateNewBrush(GetRegion(), GetRegion()->GetActiveParentNode());
	ASSERT( pNewBrush );
	if( !pNewBrush )
		return;

	// Setup an undo.
	GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_ADDEDNODE, pNewBrush), TRUE);

	pNewPoly = new CEditPoly( pNewBrush );
	ASSERT( pNewPoly );
	if( !pNewPoly )
	{
		delete pNewBrush;
		return;
	}

	pNewBrush->m_Polies.Append( pNewPoly );

	vEditVerts[0].Init( vCenter.x - fHalfSide, vCenter.y + fHalfSide, vCenter.z - fHalfSide );
	vEditVerts[1].Init( vCenter.x - fHalfSide, vCenter.y + fHalfSide, vCenter.z + fHalfSide );
	vEditVerts[2].Init( vCenter.x + fHalfSide, vCenter.y + fHalfSide, vCenter.z + fHalfSide );
	vEditVerts[3].Init( vCenter.x + fHalfSide, vCenter.y + fHalfSide, vCenter.z - fHalfSide );

	// Make top of brush
	for( i = 0; i < 4; i++ )
	{
		pNewPoly->m_Indices.Append( i );
		pNewBrush->m_Points.Append( vEditVerts[i] );
	}

	// Set normal...
	pNewPoly->Normal().Init( 0.0f, 1.0f, 0.0f );
	pNewPoly->Dist() = pNewPoly->Normal().Dot( pNewPoly->Pt(0) );

	// Extrude...
	polyRef.Init( pNewBrush, 0 );
	if( ExtrudePoly( polyRef, newVerts, FALSE, TRUE /*flip all*/) )
	{
		CEditVert *pVert;
		pVert = &newVerts[0]( );
		for( i = 0; i < 4; i++ )
		{
			newVerts[i]() -= polyRef()->Normal() * fSide;
		}

		for( DWORD j=0; j < pNewBrush->m_Polies; j++ )
		{
			pNewBrush->m_Polies[j]->UpdatePlane();

			for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
				pNewBrush->m_Polies[j]->SetupBaseTextureSpace(nCurrTex);
		}
	}

	GetRegion()->UpdateBrushGeometry(pNewBrush);
	GetRegion()->CleanupGeometry();

	// Setup the node stuff.
//	SetupBrushProperties(pNewBrush, GetRegion());

	// Make the new brush the selection.
	if( GetMainFrame()->GetNodeSelectionMode() != MULTI_SELECTION )
		GetRegion()->ClearSelections();
	
	GetRegion()->SelectNode( pNewBrush->AsNode() );
	GetRegionDoc()->NotifySelectionChange();

	GetRegionDoc()->RedrawAllViews();

}


void CRegionView::OnCreatePrimitiveCylinder( )
{
	CCylinderPrimDlg	dlg;

	dlg.m_sCaption = "Cylinder Primitive";
	dlg.m_nHeight	= 100;
	dlg.m_nRadius	= 10;
	dlg.m_nNumSides = 6;

	if( dlg.DoModal( ) == IDOK )
		DoCreatePrimitiveCylinder( GetRegion( )->m_vMarker, dlg.m_nNumSides, ( CReal )dlg.m_nHeight, ( CReal )dlg.m_nRadius );
}


//------------------------------------------------------------------------------
//
//  CRegionView::DoCreatePrimitiveCylinder
//
//	Creates a brush in the shape of a cylinder.
//
//------------------------------------------------------------------------------
void CRegionView::DoCreatePrimitiveCylinder( CVector &vCenter, int nNumSides, CReal fHeight, CReal fRadius )
{
	CWorldNode *pParent;
	CEditBrush *pNewBrush;
	CEditPoly *pNewPoly;
	CReal fHalfHeight;
	CPolyRef polyRef;
	CVertRefArray newVerts;
	DWORD i;
	CEditVert vEditVerts[32];
	CMatrix matRot;
	CVector vRotationVector;

	
	vRotationVector.Init( 0.0f, 1.0f, 0.0f );

	ASSERT( nNumSides >= 3 );
	ASSERT( nNumSides <= 32 );
	if( nNumSides < 3 || 32 < nNumSides )
		return;

	fHalfHeight = fHeight / 2.0f;

	// Setup a new brush and a new starting poly...
	pNewBrush = no_CreateNewBrush(GetRegion(), GetRegion()->GetActiveParentNode());
	ASSERT( pNewBrush );
	if( !pNewBrush )
		return;

	// Setup an undo.
	GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_ADDEDNODE, pNewBrush), TRUE);

	pNewPoly = new CEditPoly( pNewBrush );
	ASSERT( pNewPoly );
	if( !pNewPoly )
	{
		delete pNewBrush;
		return;
	}

	pNewBrush->m_Polies.Append( pNewPoly );

	vEditVerts[0].Init( vCenter.x, vCenter.y + fHalfHeight, vCenter.z + fRadius );
	gr_SetupRotationAroundVector(&matRot, vRotationVector, MATH_CIRCLE / nNumSides);
	for( i = 1; i < nNumSides; i++ )
	{
		vEditVerts[i] = vEditVerts[i-1];
		vEditVerts[i] -= vCenter;
		matRot.Apply( vEditVerts[i] );
		vEditVerts[i] += vCenter;
	}


	// Make top of brush
	for( i = 0; i < nNumSides; i++ )
	{
		pNewPoly->m_Indices.Append( i );
		pNewBrush->m_Points.Append( vEditVerts[i] );
	}

	// Set normal...
	pNewPoly->Normal().Init( 0.0f, 1.0f, 0.0f );
	pNewPoly->Dist() = pNewPoly->Normal().Dot( pNewPoly->Pt(0) );

	// Extrude...
	polyRef.Init( pNewBrush, 0 );
	if( ExtrudePoly( polyRef, newVerts, FALSE, TRUE /*flip all*/) )
	{
		CEditVert *pVert;
		pVert = &newVerts[0]( );
		for( i = 0; i < nNumSides; i++ )
		{
			newVerts[i]() -= polyRef()->Normal() * fHeight;
		}

		for( DWORD j=0; j < pNewBrush->m_Polies; j++ )
		{
			pNewBrush->m_Polies[j]->UpdatePlane();
			for(uint32 nCurrTex = 0; nCurrTex < CEditPoly::NUM_TEXTURES; nCurrTex++)
				pNewBrush->m_Polies[j]->SetupBaseTextureSpace(nCurrTex);
		}
	}

	GetRegion()->UpdateBrushGeometry(pNewBrush);
	GetRegion()->CleanupGeometry();

	// Setup the node stuff.
//	SetupBrushProperties(pNewBrush, GetRegion());

	// Make the new brush the selection.
	if( GetMainFrame()->GetNodeSelectionMode() != MULTI_SELECTION )
		GetRegion()->ClearSelections();
	
	GetRegion()->SelectNode( pNewBrush->AsNode() );
	GetRegionDoc()->NotifySelectionChange();

	GetRegionDoc()->RedrawAllViews();

}


void CRegionView::OnCreatePrimitivePyramid( )
{
	CCylinderPrimDlg	dlg;

	dlg.m_sCaption = "Pyramid Primitive";
	dlg.m_nHeight	= 128;
	dlg.m_nRadius	= 64;
	dlg.m_nNumSides = 6;

	if( dlg.DoModal( ) == IDOK )
		DoCreatePrimitivePyramid( GetRegion( )->m_vMarker, dlg.m_nNumSides, ( CReal )dlg.m_nHeight, ( CReal )dlg.m_nRadius );
}


//------------------------------------------------------------------------------
//
//  CRegionView::DoCreatePrimitivePyramid
//
//	Creates a brush in the shape of a pyramid.
//
//------------------------------------------------------------------------------
void CRegionView::DoCreatePrimitivePyramid( CVector &vCenter, int nNumSides, CReal fHeight, CReal fRadius )
{
	CWorldNode *pParent;
	CEditBrush *pNewBrush;
	CMoArray<CEditPoly *> pNewPoly;
	CPolyRef polyRef;
	CVertRefArray newVerts;
	DWORD i;
	CMoArray<CEditVert> vEditVerts;
	CMatrix matRot;
	CVector vRotationVector;


	vRotationVector.Init( 0.0f, 1.0f, 0.0f );

	ASSERT( nNumSides >= 3 );
	//ASSERT( nNumSides <= 24 );
	if( nNumSides < 3 )//|| 24 < nNumSides )
		return;

	pNewPoly.SetSize(nNumSides + 1);
	vEditVerts.SetSize(nNumSides + 2);

	// Setup a new brush and a new starting poly...
	pNewBrush = no_CreateNewBrush(GetRegion(), GetRegion()->GetActiveParentNode());
	ASSERT( pNewBrush );
	if( !pNewBrush )
		return;

	// Setup an undo.
	GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_ADDEDNODE, pNewBrush), TRUE);

	for( i = 0; i <= nNumSides; i++ )
	{
		pNewPoly[i] = new CEditPoly( pNewBrush );
		ASSERT( pNewPoly[i] );
		if( !pNewPoly[i] )
		{
			delete pNewBrush;
			return;
		}
	}

	// Make base points...
	vEditVerts[0].Init( vCenter.x, vCenter.y - fHeight/2, vCenter.z + fRadius );
	pNewBrush->m_Points.Append( vEditVerts[0] );
	gr_SetupRotationAroundVector(&matRot, vRotationVector, -MATH_CIRCLE / nNumSides);
	for( i = 1; i < nNumSides; i++ )
	{
		vEditVerts[i] = vEditVerts[i-1];
		vEditVerts[i] -= vCenter;
		matRot.Apply( vEditVerts[i] );
		vEditVerts[i] += vCenter;
		pNewBrush->m_Points.Append( vEditVerts[i] );
	}

	// Make top point...
	vEditVerts[nNumSides].Init( vCenter.x, vCenter.y + fHeight/2, vCenter.z );
	pNewBrush->m_Points.Append( vEditVerts[nNumSides] );

	// Make base poly
	for( i = 0; i < nNumSides; i++ )
	{
		pNewPoly[0]->m_Indices.Append( i );
	}

	// Set normal...
	pNewPoly[0]->Normal().Init( 0.0f, -1.0f, 0.0f );
	pNewPoly[0]->Dist() = pNewPoly[0]->Normal().Dot( pNewPoly[0]->Pt(0) );
	pNewBrush->m_Polies.Append( pNewPoly[0] );

	// Make side poly's
	for( i = 0; i < nNumSides; i++ )
	{
		pNewPoly[i+1]->m_Indices.Append(i );
		pNewPoly[i+1]->m_Indices.Append(nNumSides );
		pNewPoly[i+1]->m_Indices.Append(( i + 1 ) % nNumSides );
		pNewBrush->m_Polies.Append( pNewPoly[i+1] );
	}

	GetRegion()->UpdateBrushGeometry(pNewBrush);
	GetRegion()->CleanupGeometry();

	// Setup the node stuff.
//	SetupBrushProperties(pNewBrush, GetRegion());

	// Make the new brush the selection.
	if( GetMainFrame()->GetNodeSelectionMode() != MULTI_SELECTION )
		GetRegion()->ClearSelections();
	
	GetRegion()->SelectNode( pNewBrush->AsNode() );
	GetRegionDoc()->NotifySelectionChange();

	GetRegionDoc()->RedrawAllViews();

}


/************************************************************************/
// The sphere primitive command was called
// talon: 2001's (0x46b96a) keeps its settings in the primitives options.
void CRegionView::OnCreatePrimitiveSphere() 
{
	// The sphere primitive dialog
	CSpherePrimitiveDlg dlg;

	COptionsPrimitives *pOptions = GetApp()->GetOptions().GetPrimitivesOptions();
	if (pOptions)
	{
		dlg.m_nSides				= pOptions->GetSphereSettings()->m_nNumSides;
		dlg.m_nVerticalSubdivisions	= pOptions->GetSphereSettings()->m_nVerticalDivisions;
		dlg.m_fRadius				= PrimitiveRadius(pOptions->GetSphereSettings());
	}

	// Create the sphere primitive dialog	
	if (dlg.DoModal() == IDOK)
	{
		DoCreatePrimitiveSphere(GetRegion()->m_vMarker, dlg.m_nSides+1, dlg.m_nVerticalSubdivisions, dlg.m_fRadius, FALSE);

		if (pOptions)
		{
			pOptions->GetSphereSettings()->m_nNumSides				= dlg.m_nSides;
			pOptions->GetSphereSettings()->m_nVerticalDivisions	= dlg.m_nVerticalSubdivisions;
			pOptions->GetSphereSettings()->m_fRadius				= dlg.m_fRadius;
		}
	}
}


/************************************************************************/
// Create a sphere primitive
void CRegionView::DoCreatePrimitiveSphere(CVector &vCenter, int nSubdivisionsX, int nSubdivisionsY, CReal fRadius, BOOL bDome)
{
	// Do some error checking on the parameters
	if (nSubdivisionsX <= 0 || nSubdivisionsY <= 0 || fRadius <= 0.0f)
	{
		ASSERT(FALSE);
		return;
	}

	// Setup a new brush
	CEditBrush *pNewBrush = no_CreateNewBrush(GetRegion(), GetRegion()->GetActiveParentNode());
	ASSERT( pNewBrush );
	if( !pNewBrush )
	{
		return;
	}

	// Setup an undo.
	GetRegionDoc()->Modify(TRUE, new CPreAction(ACTION_ADDEDNODE, pNewBrush), TRUE);

	// The number of polygons for the center bands
	int nNumPolies=nSubdivisionsX*(nSubdivisionsY-1);

	// Add the number of polies for the bottom cap
	if (bDome)
	{
		nNumPolies++;
	}
	else
	{
		nNumPolies+=nSubdivisionsX;
	}

	// Add the number of polies for the top cap
	nNumPolies+=nSubdivisionsX;

	CEditPoly **ppNewPoly=new CEditPoly*[nNumPolies];

	int i;
	for (i=0; i < nNumPolies; i++)
	{
		ppNewPoly[i]=new CEditPoly(pNewBrush);
	}
	
	// The height for the middle bands is slightly less (one band worth total) than the radius.
	// This is so the triangle caps can be added.
	float fHeight=(fRadius-(fRadius/nSubdivisionsY/2.0f))*2.0f;

	// Initialize the vertices for the strips
	int nSubY;
	for (nSubY=0; nSubY < nSubdivisionsY; nSubY++)
	{
		// The percentage of the sphere that we are on in the Y direction
		// For example, the middle subdivision is 0.50
		float fYPercent;

		// Create the top half of the sphere if we are in dome mode
		if (bDome)
		{
			fYPercent=0.5f+((float)nSubY/(float)(nSubdivisionsY-1)/2.0f);
		}
		else
		{
			fYPercent=(float)nSubY/(float)(nSubdivisionsY-1);
		}
				
		// Calculate the Y position for this subdivision		
		float fYPos=(0.0f-(fHeight/2))+(fYPercent*fHeight);		

		// Calculate the radius (girth) for this subdivision
		float fCurrentRadius=cos(asin(fYPos/fRadius))*fRadius;

		int nSubX;
		for (nSubX=0; nSubX < nSubdivisionsX; nSubX++)
		{			
			// The percentage around the circle
			float fXPercent=(float)nSubX/(float)(nSubdivisionsX-1);

			// The current angle in radians
			float theta=fXPercent*3.14159f*2.0f;

			// Calculate the X and Y positions
			float fXPos=cos(theta)*fCurrentRadius;
			float fZPos=sin(theta)*fCurrentRadius;

			// Set the vertex
			CVector vPoint=CVector(fXPos, fYPos, fZPos)+vCenter;
			pNewBrush->m_Points.Append(vPoint);			
		}
	}

	// Add the top point
	CEditVert v;
	v.x = vCenter.x;
	v.y = vCenter.y + fRadius;
	v.z = vCenter.z;

	pNewBrush->m_Points.Append(v);
	int nTopPointIndex=pNewBrush->m_Points.GetSize()-1;

	// Add the bottom point if we aren't in dome mode
	int nBottomPointIndex;
	if (!bDome)
	{
		v.x = vCenter.x;
		v.y = vCenter.y - fRadius;
		v.z = vCenter.z;

		pNewBrush->m_Points.Append(v);
		nBottomPointIndex=pNewBrush->m_Points.GetSize()-1;
	}

	// Create the polygons for the center bands
	int nCurrentPoly=0;
	for (nSubY=0; nSubY < nSubdivisionsY-1; nSubY++)
	{
		int nSubX;
		for (nSubX=0; nSubX < nSubdivisionsX; nSubX++)
		{				
			// Calculate the polygon indices
			int nIndex1=(nSubY*nSubdivisionsX)+nSubX;							// Bottom left
			int nIndex2=((nSubY+1)*nSubdivisionsX)+nSubX;						// Top left
			int nIndex3=((nSubY+1)*nSubdivisionsX)+((nSubX+1)%nSubdivisionsX);	// Top right			
			int nIndex4=(nSubY*nSubdivisionsX)+((nSubX+1)%nSubdivisionsX);		// Bottom right
			
			// Add the polygon indices
			ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex1);
			ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex2);
			ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex3);
			ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex4);

			// Add the polygon			
			pNewBrush->m_Polies.Append(ppNewPoly[nCurrentPoly]);

			// Go to the next polygon
			nCurrentPoly++;
		}
	}

	// Add the bottom of the sphere
	if (bDome)
	{
		// Dome mode
		int nSubX;
		for (nSubX=0; nSubX < nSubdivisionsX; nSubX++)
		{		
			// Add the polygon indices
			ppNewPoly[nCurrentPoly]->m_Indices.Append(nSubX);			
		}

		// Add the polygon			
		pNewBrush->m_Polies.Append(ppNewPoly[nCurrentPoly]);

		// Go to the next polygon
		nCurrentPoly++;
	}
	else
	{
		// Sphere mode
		int nSubX;
		for (nSubX=0; nSubX < nSubdivisionsX; nSubX++)
		{
			// Calculate the polygon indices
			int nIndex1=nBottomPointIndex;
			int nIndex2=nSubX;
			int nIndex3=(nSubX+1)%nSubdivisionsX;

			// Add the polygon indices
			ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex1);
			ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex2);
			ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex3);

			// Add the polygon			
			pNewBrush->m_Polies.Append(ppNewPoly[nCurrentPoly]);

			// Go to the next polygon
			nCurrentPoly++;
		}
	}

	// Add the top of the sphere
	int nTopStartIndex=(nSubdivisionsY-1)*nSubdivisionsX;

	int nSubX;
	for (nSubX=0; nSubX < nSubdivisionsX; nSubX++)
	{
		// Calculate the polygon indices
		int nIndex1=nTopStartIndex+nSubX;
		int nIndex2=nTopPointIndex;
		int nIndex3=nTopStartIndex+((nSubX+1)%nSubdivisionsX);

		// Add the polygon indices
		ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex1);
		ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex2);
		ppNewPoly[nCurrentPoly]->m_Indices.Append(nIndex3);

		// Add the polygon			
		pNewBrush->m_Polies.Append(ppNewPoly[nCurrentPoly]);

		// Go to the next polygon
		nCurrentPoly++;
	}

	// Delete the temp array of poly pointers
	delete []ppNewPoly;
	ppNewPoly=NULL;

	GetRegion()->UpdateBrushGeometry(pNewBrush);
	GetRegion()->CleanupGeometry();

	// Make the new brush the selection.
	if( GetMainFrame()->GetNodeSelectionMode() != MULTI_SELECTION )
	{
		GetRegion()->ClearSelections();
	}
	
	GetRegion()->SelectNode( pNewBrush->AsNode() );
	GetRegionDoc()->NotifySelectionChange();

	// Redraw all of the views
	GetRegionDoc()->RedrawAllViews();
}


/************************************************************************/
// The dome primitive command was called
// talon: 2001's (0x46bfd2) keeps its settings in the primitives options.
void CRegionView::OnCreatePrimitiveDome() 
{
	// The sphere primitive dialog
	CSpherePrimitiveDlg dlg;

	COptionsPrimitives *pOptions = GetApp()->GetOptions().GetPrimitivesOptions();
	if (pOptions)
	{
		dlg.m_nSides				= pOptions->GetDomeSettings()->m_nNumSides;
		dlg.m_nVerticalSubdivisions	= pOptions->GetDomeSettings()->m_nVerticalDivisions;
		dlg.m_fRadius				= PrimitiveRadius(pOptions->GetDomeSettings());
	}

	// Create the sphere primitive dialog	
	if (dlg.DoModal() == IDOK)
	{
		DoCreatePrimitiveSphere(GetRegion()->m_vMarker, dlg.m_nSides+1, dlg.m_nVerticalSubdivisions, dlg.m_fRadius, TRUE);

		if (pOptions)
		{
			pOptions->GetDomeSettings()->m_nNumSides				= dlg.m_nSides;
			pOptions->GetDomeSettings()->m_nVerticalDivisions	= dlg.m_nVerticalSubdivisions;
			pOptions->GetDomeSettings()->m_fRadius				= dlg.m_fRadius;
		}
	}
}

// Rotates the polygon's texture coordinates around the specified origin and axis.
void RotatePolyTCoords(CEditPoly *pPoly, uint32 nTexture,
	const LTVector& vOrigin, const LTVector& vAxis, 
	float fRot, DVector &newO, DVector &newP, DVector &newQ)
{
	LTMatrix mRot;

	Mat_SetupRot(&mRot, (LTVector*)&vAxis, fRot);	// talon: the LT2 SDK's takes no const
	
	newO = (mRot * (pPoly->GetTexture(nTexture).GetO() - vOrigin)) + vOrigin;
	newP = mRot * pPoly->GetTexture(nTexture).GetP();
	newQ = mRot * pPoly->GetTexture(nTexture).GetQ();
}


// The Converge callback
class TCoordNormalConvertData
{
public:
	CEditPoly	*m_pPoly;
	DVector		m_vOrigin;
	DVector		m_vAxis;
};

float AlignTextureConvergeCB(float testVal, void *pUser)
{
	TCoordNormalConvertData *pData;
	DVector vNormal, newO, newP, newQ;

	pData = (TCoordNormalConvertData*)pUser;
	RotatePolyTCoords(pData->m_pPoly, GetCurrTexture(), pData->m_vOrigin, pData->m_vAxis,
		testVal, newO, newP, newQ);

	vNormal = newO.Cross(newP);
	vNormal.Norm();
	return vNormal.Dot(pData->m_pPoly->Normal());
}


BOOL FindJoinEdge(CEditPoly *pPoly1, CEditPoly *pPoly2, DVector edge[2])
{
	uint32 i, j;

	for(i=0; i < pPoly1->NumVerts(); i++)
	{
		for(j=0; j < pPoly2->NumVerts(); j++)
		{
			if((pPoly1->Pt(i).Equals(pPoly2->Pt(j)) && pPoly1->NextPt(i).Equals(pPoly2->NextPt(j))) ||
				(pPoly1->Pt(i).Equals(pPoly2->NextPt(j)) && pPoly1->NextPt(i).Equals(pPoly2->Pt(j))))
			{
				edge[0] = pPoly1->Pt(i);
				edge[1] = pPoly1->NextPt(i);
				return TRUE;
			}
		}
	}

	return FALSE;
}


void CRegionView::OnAlignPolyTextures()
{
	CEditPoly *polies[2], *pMaster, *pAlign;
	TCoordNormalConvertData theData;
	float fBestRot;
	DVector newO, newP, newQ;
	DVector vEdge[2];
	uint32 i;
	CMoArray<CEditPoly*> selectedPolies;
	PreActionList actionList;

	
	GetSelectedPolies(selectedPolies);
	
	if(selectedPolies.GetSize() < 2)
		return;

	// Setup the undo.
	for(i=0; i < selectedPolies; i++)
	{
		AddToActionListIfNew(&actionList, new CPreAction(ACTION_MODIFYNODE, selectedPolies[i]->m_pBrush), TRUE);
	}
	GetRegionDoc()->Modify(&actionList, TRUE);

	for(i=0; i < selectedPolies.GetSize() - 1; i++)
	{
		pMaster = selectedPolies[i];
		pAlign = selectedPolies[i+1];

		// Find the edge they join on.
		if(!FindJoinEdge(pMaster, pAlign, vEdge))
			break;

		// Copy the texture space.
		CTexturedPlane& AlignTex = pAlign->GetTexture(GetCurrTexture());
		CTexturedPlane& MasterTex = pMaster->GetTexture(GetCurrTexture());
		pAlign->SetTextureSpace(GetCurrTexture(), MasterTex.GetO(), MasterTex.GetP(), MasterTex.GetQ());

		// Rotate it around the edge vector until it is closest to the edge normal.
		theData.m_pPoly = pAlign;
		theData.m_vOrigin = vEdge[0];
		theData.m_vAxis = vEdge[1] - vEdge[0];
		theData.m_vAxis.Norm();

		fBestRot = Converge(0.0f, MATH_CIRCLE, AlignTextureConvergeCB, &theData,
			20, 20);

		// Apply the final one.
		RotatePolyTCoords(pAlign, GetCurrTexture(), theData.m_vOrigin, theData.m_vAxis, 
			fBestRot, newO, newP, newQ);
		
		pAlign->SetTextureSpace(GetCurrTexture(), newO, newP, newQ);
	}

	GetRegionDoc()->RedrawAllViews();
}




// talon: stand-in: 2001's light-texel selection (0x46c783, 761 bytes, in
// LIGHT_EDITMODE from OnSelectLightNode 0x45db67) isn't written yet: it
// needs RegionDoc's 2001 light members (doc+0x184, +0x320, +0x324, +0x328).
void CRegionView::SelectLightTexel()
{
}


// talon: stand-in: 2001's light poly pick (0x46c536, 589 bytes, in
// LIGHT_EDITMODE from 0x45cfd0) isn't written yet.
void CRegionView::PickLightPoly()
{
}


// talon: stand-in: 2001's texel tagging in a rectangle (0x46ca7c, 437 bytes,
// from the Tag tracker 0x4790c8) isn't written yet.
void CRegionView::TagLightTexelsInRect( CRect rect )
{
}
