#ifndef __PROCESSOROPTIONS_DECL_H__
#define __PROCESSOROPTIONS_DECL_H__

// talon: the "Processing Options" dialog (resource dialog 130), from the
// shipped Processor.exe (AvP2 v1.0). The class, member and control names
// are ours; the dialog's title and its controls' captions are the
// resource's. The app (0x42f900) makes one on its stack, points m_pGlobs
// at the processor's globals (0x42fba3) and runs DoModal.

// The controls (dialog 130's IDs; the names are ours, from the captions).
#define IDD_PROCESSINGOPTIONS			130
#define IDC_PO_SPLITWEIGHT				1013	// msctls_trackbar32
#define IDC_PO_IMPORTGEOMETRY			1015	// "Import Geometry"
#define IDC_PO_LIGHT					1016	// "Apply Lighting"
#define IDC_PO_IGNOREHIDDEN				1017	// "Ignore Hidden"
#define IDC_PO_SHADOWS					1018	// "Shadows"
#define IDC_PO_VERTICESONLY				1019	// "Vertex Lighting Only"
#define IDC_PO_OPTIMIZE					1020	// "Full Optimization"
#define IDC_PO_FAST						1021	// "Fast Approximation"
#define IDC_PO_LOGFILE					1022	// "Log File"
#define IDC_PO_EXTRAPARAMS				1023	// the edit after "Extra Parameters:"
#define IDC_PO_FIXTJUNC					1024	// "Fix T Junctions"
#define IDC_PO_PROJECTDIR				1025	// the edit after "Project Directory:"
#define IDC_PO_LIGHTANIMS				1026	// "Light Animations"
#define IDC_PO_LAMBERTIAN				1027	// "Lambertian Lightmaps"
#define IDC_PO_REPLACETEXTURES			1029	// "Replace Textures"
#define IDC_PO_TEXTUREFLAGS				1030	// "Import Texture Flags"
#define IDC_PO_SLIDERREF				1031	// the static after "Slider Reference:"
#define IDC_PO_SLIDERLEFT				1032	// "<"
#define IDC_PO_AIVOLUMES				1033	// "Import AI and Music Vol."
#define IDC_PO_SLIDERRIGHT				1034	// ">"

// The string table's 104: "Processing Options for %1!s!.ed".
#define IDS_PO_TITLE					104

class CProcessorGlobs;

// 0x7b8 bytes: CDialog's 0x5c, then ours. The constructor (0x442e30)
// builds the controls in this order; DoDataExchange (0x443090) ties them
// to their IDs.
class CProcessingOptionsDlg : public CDialog
{
public:
						CProcessingOptionsDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_PROCESSINGOPTIONS };

	// The helpers (the names are ours).
	void				UpdateControls();								// 0x443260
	void				SetArgs(char *pCmdLine);						// 0x443880
	void				SaveControls();									// 0x443980
	void				InitSlider();									// 0x443b60
	void				BuildCommandLine(CString &cmdLine, CString &extra);	// 0x443c60

	virtual void		DoDataExchange(CDataExchange* pDX);				// 0x443090
	virtual LRESULT		WindowProc(UINT message, WPARAM wParam, LPARAM lParam);	// 0x4431e0
	virtual BOOL		OnInitDialog();									// 0x443e60
	virtual void		OnOK();											// 0x444470

	afx_msg void		OnReplaceTextures();							// 0x4440c0
	afx_msg void		OnImportGeometry();								// 0x4440f0
	afx_msg void		OnTextureFlags();								// 0x444120
	afx_msg void		OnIgnoreHidden();								// 0x444150
	afx_msg void		OnFixTJunc();									// 0x444180
	afx_msg void		OnLight();										// 0x4441b0
	afx_msg void		OnLightAnims();									// 0x4441e0
	afx_msg void		OnShadows();									// 0x444210
	afx_msg void		OnVerticesOnly();								// 0x444240
	afx_msg void		OnLambertian();									// 0x444270
	afx_msg void		OnOptimize();									// 0x4442a0
	afx_msg void		OnFast();										// 0x4442d0
	afx_msg void		OnSliderLeft();									// 0x444330
	afx_msg void		OnSliderRight();								// 0x4443d0
	afx_msg void		OnLButtonUp(UINT nFlags, CPoint point);			// 0x4445d0
	afx_msg void		OnPaint();										// 0x4445e0
	DECLARE_MESSAGE_MAP()

public:

	CProcessorGlobs		*m_pGlobs;			// 0x5c (set by the app, 0x42fba3)
	CRect				m_Rect;				// 0x60 (OnInitDialog's GetWindowRect)
	uint32				m_Unknown_70[4];	// 0x70: nothing here uses it
	BOOL				m_bDATExists;		// 0x80: the output DAT opens (OnInitDialog)
	uint32				m_Argc;				// 0x84: SetArgs' tokens, for InitProcessOptions
	char				*m_Argv[40];		// 0x88
	char				m_ArgBuffer[600];	// 0x128: where SetArgs copies them

	CButton				m_Lambertian;		// 0x380
	CButton				m_FixTJunc;			// 0x3bc
	CEdit				m_ProjectDir;		// 0x3f8
	CButton				m_TextureFlags;		// 0x434
	CButton				m_ReplaceTextures;	// 0x470
	CEdit				m_ExtraParams;		// 0x4ac
	CButton				m_Optimize;			// 0x4e8
	CButton				m_Light;			// 0x524
	CButton				m_Fast;				// 0x560
	CEdit				m_Unknown_59c;		// 0x59c: constructed, never tied to a control
	CButton				m_VerticesOnly;		// 0x5d8
	CButton				m_LightAnims;		// 0x614
	CButton				m_Shadows;			// 0x650
	CButton				m_LogFile;			// 0x68c
	CButton				m_ImportGeometry;	// 0x6c8
	CSliderCtrl			m_SplitWeight;		// 0x704
	CButton				m_IgnoreHidden;		// 0x740
};

#endif  // __PROCESSOROPTIONS_DECL_H__
