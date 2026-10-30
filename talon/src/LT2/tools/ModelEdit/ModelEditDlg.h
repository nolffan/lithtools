// ModelEditDlg.h : header file
//

#if !defined(AFX_MODELEDITDLG_H__A24253B6_085F_11D1_B05B_0020AFF7CDC1__INCLUDED_)
#define AFX_MODELEDITDLG_H__A24253B6_085F_11D1_B05B_0020AFF7CDC1__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#pragma warning(disable : 4018)

/////////////////////////////////////////////////////////////////////////////
// CModelEditDlg dialog

#include "renderwnd.h"
#include "keyframewnd.h"
#include "bdefs.h"
#include "model.h"
#include "gl_modelrender.h"
#include "myedit.h"
#include "resource.h"
#include "weighteditdlg.h"
#include "win_idle.h"
#include "lveditlist.h"
#include "draglist.h"
#include "animtracker.h"
#include "solidrectwnd.h"
#include "dimensionsdlg.h"
#include <fstream>
#include <string>

// For GetPlaybackActiveFlags.
#define PA_ACTIVE	(1<<0)
#define PA_VALID	(1<<1)


typedef enum
{
	RT_ROTATION,
	RT_POSITION
} RTType;

class RectTracker
{
public:
	UINT			m_CtlID;
	CRect			m_Rect;
	COLORREF		m_Color;
	SolidRectWnd	m_Wnd;
	
	// Position/rotation and which axis.
	RTType			m_Type;
	DWORD			m_Axis;	
};

// ------------------------------------------------------------------------
//this is the structure that will be passed into the thread main function that handles
//the loading of the LTA files.
 
// ------------------------------------------------------------------------
class CLoadLTAThreadParams
{
public:
	CLoadLTAThreadParams() :
		m_pInFile(NULL),
		m_pAllocCount(NULL),
		m_pModel(NULL),
		m_bChildLoad(false)
		{}

	// talon: 2001's (0x416f5c): the open stream comes first.
	std::ifstream*	m_pInFile;
	LAllocCount*	m_pAllocCount;
	Model*			m_pModel;
	bool			m_bChildLoad ;
	std::string			m_Filename ;
};

// ------------------------------------------------------------------------
// 
// ------------------------------------------------------------------------
class MEAnimInfo
{
public:

					MEAnimInfo();

	// Reset everything.
	void			Reset(Model *pModel=NULL);

	// talon: 2001's: back to the start of the current animation, stopped.
	void			ResetTime();

	void			StartPlayback();
	void			StopPlayback();

	Model*			GetModel()		{return m_Tracker.GetModel();}

	// The animation is valid if:
	// 1. GetModel() returns a non-null value.
	// 2. The animation indices are valid.
	// 3. The keyframe indices are valid.
	// (This just calls thru to AnimTimeRef::IsValid).
	BOOL			IsValid();

	// Update the time in the keyframe window.
	void			SetKeyframeWindowTime();
	
	uint32			CurAnimIndex()	{return m_Tracker.m_TimeRef.m_Cur.m_iAnim;}
	uint32			PrevAnimIndex()	{return m_Tracker.m_TimeRef.m_Prev.m_iAnim;}

	uint32			CurAnimTime()	{return m_Tracker.m_TimeRef.m_Cur.m_Time;}
	uint32			PrevAnimTime()	{return m_Tracker.m_TimeRef.m_Prev.m_Time;}

	uint32			CurKeyframe()	{return m_Tracker.m_TimeRef.m_Cur.m_iFrame;}
	uint32			PrevKeyframe()	{return m_Tracker.m_TimeRef.m_Prev.m_iFrame;}

	ModelAnim*		CurAnim();

	BOOL			IsAnimPlaying()			{ return m_bAnimPlayingForward; }
	BOOL			IsAnimPlayingForward()	{ return m_bAnimPlayingForward; }

	
public:
	
	BOOL			m_bAnimPlayingForward;
	DWORD			m_dwNumTagged;

	UINT			m_AnimButtonID;

	CKeyframeWnd	m_Wnd;
	SolidRectWnd	m_ActiveFlagsWnd;
	AnimTracker		m_Tracker;
};

typedef CMoArray<MEAnimInfo*> MEAnimArray;


#define IDLE_DIALOG_DELAY	300

// The first animation is the only one you can edit.
#define ANIMINFO_MAIN	0


// ------------------------------------------------------------------------
// 
// ------------------------------------------------------------------------
class CModelEditDlg : public CDialog
{
friend class IdleChanger;

// Construction
public:
	CModelEditDlg(CWnd* pParent = NULL);	// standard constructor
	~CModelEditDlg();

// Dialog Data
	// talon: 2001's controls (the constructor at 0x416363, DoDataExchange
	// at 0x4169a4): list views for the pieces and nodes, not Build 69's
	// tree control.
	//{{AFX_DATA(CModelEditDlg)
	enum { IDD = IDD_MODELEDIT_DIALOG };
	CDragList		m_AnimList;
	CLVEditList		m_PieceList;
	CLVEditList		m_NodeList;
	CButton			m_GlobalSpace;
	CListBox		m_SocketList;
	CMyEdit			m_EditFrameTime;
	CMyEdit			m_EditFrameString;
	CListBox		m_ChildModelList;
	BOOL			m_bDrawSkeleton;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CModelEditDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:

	ModelSocket*	GetSelectedSocket();
	CButton*		GetTransformEdit(UINT id);
	BOOL			IsButtonSelected(UINT id);

	void			SetChangesMade()			{ m_bChangesMade = TRUE; }

	void			InitTaggedArrays();
	void			InitTaggedArray(uint32 iAnimInfo);

	void			DeleteModel();
	void			DeleteSocketAttachments();

	// dwActive is filled with a combination of the PA_ flags above.
	// If bAllNoKeysDown is TRUE, then if none of the keys are down it considers them all down.
	// NOTE: even if the keys are down, this only returns animations that are valid.
	void			GetPlaybackActiveFlags( DWORD dwActive[NUM_ANIM_INFOS], 
											BOOL bAllIfNoKeysDown=TRUE);

	void			SetCurrentPosition( DWORD iAnimInfo,
										DWORD nKeyframe1, 
										DWORD nKeyframe2, 
										float nPercentBetween);

	void			SetCurrentPosition(	CKeyframeWnd *pWnd,
										DWORD nKeyframe1, 
										DWORD nKeyframe2, 
										float nPercentBetween);

	Model*			GetModel() {return m_pModel;}
	void			UpdateEditFrameTime( );
	void			SetEditFrameTime();
	void			UpdateEditFrameString( );
	void			SetEditFrameString();
	BOOL			SetRenderTexture(const char *pFilename, DWORD nTexture = 0);

	// Very useful function.  If any of the animations are invalid or are from a base model (and shouldn't
	// be modified), it brings up an error message and returns false.
	BOOL			CheckModifyAnims(int *pAnims, int nAnims, BOOL bParentOnly=TRUE, BOOL bMessage=TRUE);

	// Gets a list of selected animations and calls CheckModifyAnims on it.
	BOOL			GetAnimList(int *pAnims, int listSizeBytes, int &nAnims, BOOL bParentOnly=TRUE, BOOL bMessage=TRUE);
	
	void			ProcessCommandString();

	void			FillAnimList();
	void			FillChildModelList();

	// Used when something is going to change an animation.. makes sure everything is valid
	// and fills in the current animation and keyframe indices.
	BOOL			GetEditAnimInfo(DWORD *pAnimIndex, DWORD *pKeyframeIndex);

	void			DoSave(const char *pFilename);

	int				DoMessageBox(UINT idString, int style);
	int				DoMessageBox(LPCTSTR pStr, int style);

	// Fills the list in with the selected animations that come from parent model
	// (which are the only animations that should ever be modified).
	int				GetSelectedParentAnims(int *pItems, int maxItems);

	// These return TRUE if the CModelEditDlg handles it.. FALSE if the caller should handle it.
	// iButton: (0=left, 1=middle, 2=right)
	BOOL			HandleButtonDown(int iButton, CPoint point);
	BOOL			HandleButtonUp(int iButton);
	void			StartTracker(uint32 iTracker, CPoint centerPt);


public:

	void			InterpForward(uint32 iAnimInfo, DWORD msDelta);

	BOOL			DoLoadModel(char *pFilename, BOOL bMessage=TRUE);
	void			UpdateMenuChecks();
	void			CheckChildModels(ModelLoadRequest *pRequest);
	
	
	void			FillNodeList();
	void			FillSocketList(BOOL bPreserveSel=FALSE);
	void			SetLODText(int nLOD);
	void			InitTheScrollBar(int nLODs);

	void			SortPiecesByTexture();
	void			FillPieceList();
	void			LoadRegistryTextures();

	BOOL			VerifyModels( Model *pModel1, Model *pModel2 );
	BOOL			RecurseAndVerifyModels( ModelNode* pNode1, ModelNode* pNode2 );
	
	void			MoveAnims(int dir);
	void			MovePieces(int dir);

	void			OffsetTrans(LTVector vec);
	void			OffsetDims(LTVector vec);

	void			DoAnimEdit(LTVector vec);

	void			SetCurrentLOD(DWORD iLOD);
	

	// note: these really belong somewhere else.
	static	float			CalcFOV();
	void			SetFOV(float FOV);

	MEAnimInfo*		GetAnimInfo(uint32 iAnimInfo);

	// Stop playback for some or all animations.
	void			StopAllPlayback();
	void			ResetAnimInfos();

	void			DrawActiveAnimRects(BOOL bForce);

	// Increments the keyframe position for the active animations (from GetPlaybackActiveAnims).
	void			IncKeyframe(BOOL bForward);

	// Bring up a rename dialog for the specified animation.
	void			DoRenameAnim(DWORD iAnim);

protected:
	// helper func, loads lta file, pops up while-load feedback dialog.
	Model*			load_LTA_Using_FeedBackDialog( const char *pFilename, bool isChildModel=false );



public:

	// Tracks which animations were active last frame so we don't redraw unnecessarily.
	DWORD					m_LastAnimActiveFlags[NUM_ANIM_INFOS];

	// The trackers.
	CMoArray<RectTracker*>	m_Trackers;

	// Where the mouse gets centered to.
	CPoint			m_TrackerCenterPt;
	
	// Are we tracking anything in a rect?
	BOOL			m_bTracking;

	// Which tracker are we tracking?
	DWORD			m_iCurTracker;

	CPoint			m_LastCursorPos;

	// Total accumulated mouse tracking.
	int				m_TotalDelta;

	CString			m_OldWindowText;

protected:

	WeightEditDlg	m_WeightEditDlg;

	// talon: 2001's (+0x3bc): the move up and move down buttons move the
	// selected pieces (MovePieces) instead of the animations.  2001 only
	// ever clears it (the constructor, OnSelChangeAnimList).  The name is
	// ours.
	BOOL			m_bMovePieces;
		
	HICON			m_hIcon;
	
	HACCEL			m_hAccel;
	CString			m_strFilename;

	CBitmapButton	m_btnNumber;
	CBitmapButton	m_btnPrevKey;
	CBitmapButton	m_btnStop;
	CBitmapButton	m_btnFPlay;
	CBitmapButton	m_btnNextKey;
	CBitmapButton	m_btnMoveUp;
	CBitmapButton	m_btnMoveDown;
	CBitmapButton	m_btnDelete;

	Model			*m_pModel;

	LAllocCount		m_AllocCount;

	CRenderWnd		m_RenderWnd; // gl render window.

	// Animation windows..
	MEAnimInfo		m_AnimInfos[NUM_ANIM_INFOS];

	// This is used to track the selected animations so we can know what order they
	// were selected in.
	CMoArray<DWORD>	m_SelectedAnims;

	BOOL			m_bEndDialog;

	DWORD			m_nCurrentLOD;
	BOOL			m_bChangesMade;
	BOOL			m_bFillingNodeList;

	DWORD			m_LastIdleTime;

	CWinIdle		m_cWinIdle;

	// Tracks if we're in a string change (so we don't get into it again).
	BOOL			m_bInStringChange;

public:

	void			OnIdle();

	// Turn off the default enter key behavior
	virtual void	OnOK();

	BOOL			UpdateSelectedAnimList();
	void			OnSelChangeAnimList();
	virtual void	OnDropAnimList(int iItem);

	// Generated message map functions
	//{{AFX_MSG(CModelEditDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnLoadModel();
	virtual void OnCancel();
	afx_msg LONG OnStartIdle(UINT, LONG);
	afx_msg void OnHelpAbout();
	afx_msg void OnPlayForward();
	afx_msg void OnStop();
	afx_msg void OnAnimMoveDown();
	afx_msg void OnAnimMoveUp();
	afx_msg void OnAnimDelete();
	afx_msg void OnNextKeyframe();
	afx_msg void OnPrevKeyframe();
	afx_msg void OnImport();
	afx_msg void OnSave();
	afx_msg void OnSaveAs();
	afx_msg void OnCreateSingleFrame();
	afx_msg void OnRenameAnim();
	afx_msg void OnWireframe();
	afx_msg void OnContinuous();
	afx_msg void OnCameraFollow();
	afx_msg void OnDims();
	afx_msg void OnAnimBox();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnBuildLODs();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnModelInfo();
	afx_msg void OnCommandString();
	afx_msg void OnDimensions();
	afx_msg void OnAnimationFramerate();
	afx_msg void OnAnimationCleanup();
	afx_msg void OnAnimationLength();
	afx_msg void OnAnimationInterpolation();
	afx_msg void OnTranslationButton();
	afx_msg void OnRotationButton();
	afx_msg void OnXsub();
	afx_msg void OnXadd();
	afx_msg void OnYsub();
	afx_msg void OnYadd();
	afx_msg void OnZsub();
	afx_msg void OnZadd();
	afx_msg void OnNumberAnim();
	afx_msg void OnDuplicateAnim();
	afx_msg void OnRenameNode();
	afx_msg void OnUnrotateTopNode();
	afx_msg void OnGenerateVertexNormals();
	afx_msg void OnReverseAnimation();
	afx_msg void OnSelectNullNodes();
	afx_msg void OnRemoveNode();
	afx_msg void OnSetTexture();
	afx_msg void OnDrawSkeleton();
	afx_msg void OnShowSockets();
	afx_msg void OnShowAttachments();
	afx_msg void OnSolidAttachments();
	afx_msg void OnAddChildModel();
	afx_msg void OnRemoveChildModel();
	afx_msg void OnAddSocket();
	afx_msg void OnRemoveSocket();
	afx_msg void OnRenameSocket();
	afx_msg void OnRebuildChildModelTree();
	afx_msg void OnShowNormals();
	afx_msg void OnShowNormalRef();
	afx_msg void OnProfile();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMood();
	afx_msg void OnInternalRadius();
	afx_msg void OnCalcInternalRadius();
	afx_msg void OnEditWeights();
	afx_msg void OnPieceInfo();
	afx_msg void OnPieceMerge();
	afx_msg void OnEndlabeleditNodeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChangedNodeList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	afx_msg void OnItemChangedPieceList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEditPieceList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginLabelEditAnimList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEditAnimList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemChangingAnimList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginDragAnimList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnSortAnimName();
	afx_msg void OnSortAnimRelation();
	afx_msg void OnDblClkSocketList();
	afx_msg void OnFrameStringChange();
	afx_msg void OnNewBgColor();
	afx_msg void OnOptionsShoworiginalmodel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MODELEDITDLG_H__A24253B6_085F_11D1_B05B_0020AFF7CDC1__INCLUDED_)
