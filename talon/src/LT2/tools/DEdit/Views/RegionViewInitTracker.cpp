#include "bdefs.h"
#include "objectimporter.h"
#include "dedit.h"
#include "regiondoc.h"
#include "regionview.h"
#include "stdrvtrackers.h"

// talon: the 2001 tracker set-up, written from the image (0x452bb7..
// 0x455025, one function: the function table's 0x453e95 row starts inside
// it). In the image it follows RegionView.cpp's constructor, destructor and
// message map (0x4528f2..0x452bb6) and comes before its other functions,
// so in 2001 it was part of RegionView.cpp. The 2001 trackers take their
// start and end events here (Build 69 reads them from the hotkey
// configuration): 53 trackers, names and keys as the image has them.

// The focus tracker's name is the one name the image reads from a variable
// (push 0x4f9aa8, a pointer to "Focus", RegionView.obj's first .data).
static LPCTSTR g_pszFocusTracker = "Focus";

void CRegionView::InitTracker()
{
	//the current tracker that is being added
	CRegionViewTracker *pRVTracker;

	//the tracker the drawing tracker lets override it
	CRegionViewTracker *pNavRotateTracker;

	pRVTracker = new CRVTrackerFocus(g_pszFocusTracker, this);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEMOVE));
	((CRVTrackerFocus *)pRVTracker)->SetLockEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Select.Depth", this, CRegionView::OnDepthSelect);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, 'Y'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'T', FALSE));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("ApplyTexture", this, CRegionView::OnApplyTextureClick);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, 'Y'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'A'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("SplitBrush", this, CRegionView::OnBrushSplitBrush);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'S'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'S'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("CarveBrush", this, CRegionView::OnBrushCarve);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'H'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'H'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Select.None", this, CRegionView::OnSelectNone);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'U'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'U'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("SetGridDir.Forward", this, CRegionView::OnSetGridOrientation);
	((CRVCallback *)pRVTracker)->SetData(GRID_FORWARD);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, '8'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, '8'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("SetGridDir.Right", this, CRegionView::OnSetGridOrientation);
	((CRVCallback *)pRVTracker)->SetData(GRID_RIGHT);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, '9'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, '9'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("SetGridDir.Up", this, CRegionView::OnSetGridOrientation);
	((CRVCallback *)pRVTracker)->SetData(GRID_UP);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, '0'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, '0'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("InitTextureSpace", this, CRegionView::OnInitTextureSpace);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'K'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'K'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Delete", this, CRegionView::OnDelete);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_DELETE));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_DELETE));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Nudge.Up", this, CRegionView::OnNudge);
	((CRVCallback *)pRVTracker)->SetData(VK_UP);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	((CRVCallback *)pRVTracker)->SetToggle(1, CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_UP));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_UP));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Nudge.Down", this, CRegionView::OnNudge);
	((CRVCallback *)pRVTracker)->SetData(VK_DOWN);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	((CRVCallback *)pRVTracker)->SetToggle(1, CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_DOWN));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_DOWN));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Nudge.Left", this, CRegionView::OnNudge);
	((CRVCallback *)pRVTracker)->SetData(VK_LEFT);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	((CRVCallback *)pRVTracker)->SetToggle(1, CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_LEFT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_LEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("Nudge.Right", this, CRegionView::OnNudge);
	((CRVCallback *)pRVTracker)->SetData(VK_RIGHT);
	((CRVCallback *)pRVTracker)->SetToggle(0, CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	((CRVCallback *)pRVTracker)->SetToggle(1, CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_RIGHT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_RIGHT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("DeleteEdges.Remove", this, CRegionView::OnDeleteEdges);
	((CRVCallback *)pRVTracker)->SetData(TRUE);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_HOME));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_HOME));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("DeleteEdges", this, CRegionView::OnDeleteEdges);
	((CRVCallback *)pRVTracker)->SetData(FALSE);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_END));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_END));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("SplitEdge", this, CRegionView::OnSplitEdges);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_INSERT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_INSERT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("DeleteTaggedPolies", this, CRegionView::OnDeleteTaggedPolygons);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_NEXT));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, VK_NEXT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("GridToPoly", this, CRegionView::OnGridToPoly);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'G'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'G'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("TagPolyVerts", this, CRegionView::OnTagPolyVerts);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'Y'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'Y'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("FlipNormal", this, CRegionView::OnFlipNormal);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'F'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'F'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("OrbitVertex", this, CRegionView::OnOrbitVertex);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'P'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'P'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("OrbitVertex", this, CRegionView::OnSelectLightNode);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("EditLightNode.Up", this, CRegionView::OnEditLightNode);
	((CRVCallback *)pRVTracker)->SetData(TRUE);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'Q'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'Q'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallback("EditLightNode.Down", this, CRegionView::OnEditLightNode);
	((CRVCallback *)pRVTracker)->SetData(FALSE);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'W'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'W'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNodeRotate("NodeRotate", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'N'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'N'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerTextureMove("TextureMove", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'R'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'R'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerTextureWrap("TextureWrap", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'L'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'L'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerBrushSize("BrushSize", this);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerCurveEdit("CurveEdit", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'P'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	pRVTracker->SetPhantomEnd(FALSE);
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerFarDist("FarDist", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'C'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'C'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerGridSize("GridSize", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'Z'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSERIGHT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerMarkerMove("MarkerMove", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'X'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'X'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertMove("VertMove.Poly", this, CRVTrackerVertMove::esm_Poly);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_SHIFT));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSERIGHT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertMove("VertMove.SelVert", this, CRVTrackerVertMove::esm_SelVert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'S'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'S'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertMove("VertMove.ImmVert", this, CRVTrackerVertMove::esm_ImmVert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'M'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'M'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertSnap("VertSnap.Edge.SelVert", this, CRVTrackerVertMove::esm_SelVert, CRVTrackerVertSnap::est_Edge);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'D'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'D'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertSnap("VertSnap.Edge", this, CRVTrackerVertMove::esm_ImmVert, CRVTrackerVertSnap::est_Edge);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'E'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'E'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertSnap("VertSnap.Vert.SelVert", this, CRVTrackerVertMove::esm_SelVert, CRVTrackerVertSnap::est_Vert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'B'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'B'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertSnap("VertSnap.Vert", this, CRVTrackerVertMove::esm_ImmVert, CRVTrackerVertSnap::est_Vert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'V'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'V'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertScale("VertScale.Poly", this, CRVTrackerVertScale::esm_Poly);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'W'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'W'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerVertScale("VertScale.SelVert", this, CRVTrackerVertScale::esm_SelVert);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'Q'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'Q'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNodeMove("NodeMove.Handle", this, CRVTrackerNodeMove::FLAG_HANDLE);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNodeMove("NodeMove.Snap", this, CRVTrackerNodeMove::FLAG_SNAP);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'M'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNodeMove("NodeMove.Perpendicular", this, CRVTrackerNodeMove::FLAG_PERP);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'M'));
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSERIGHT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSERIGHT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNavMove("NavMove", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'I'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'I'));
	pRVTracker->SetPhantomEnd(FALSE);
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerNavRotate("NavRotate", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'O'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'O'));
	pRVTracker->SetPhantomEnd(FALSE);
	m_cTrackerMgr.AddTracker(pRVTracker);

	pNavRotateTracker = pRVTracker;
	pRVTracker = new CRVTrackerDrawPoly("DrawPoly", this);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_SPACE));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_ESCAPE));
	pRVTracker->AddOverride(pNavRotateTracker);
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVTrackerTag("Tag.All", this, CRVTrackerTag::FLAG_ALL);
	pRVTracker->AddStartEvent(CUIMouseEvent(UIEVENT_MOUSEDOWN, UI_MOUSELEFT));
	pRVTracker->AddEndEvent(CUIMouseEvent(UIEVENT_MOUSEUP, UI_MOUSELEFT));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("UnJoinBrushes", this, CRegionView::OnBrushUnJoin);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'J'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'J'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("JoinBrushes", this, CRegionView::OnBrushJoin);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'J'));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'J'));
	m_cTrackerMgr.AddTracker(pRVTracker);

	pRVTracker = new CRVCallbackSimple("TriangulateBrushes", this, CRegionView::OnAutoTriangulate);
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, 'E'));
	pRVTracker->AddStartEvent(CUIKeyEvent(UIEVENT_KEYDOWN, VK_CONTROL));
	pRVTracker->AddEndEvent(CUIKeyEvent(UIEVENT_KEYUP, 'E'));
	m_cTrackerMgr.AddTracker(pRVTracker);
}
