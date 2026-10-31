#ifndef __PROCESSOROPTIONS_DECL_H__
#define __PROCESSOROPTIONS_DECL_H__

// talon: the "Processing Options" dialog (resource dialog 130), from the
// shipped Processor.exe (NOLF Tools 001). The class, member and control
// names are ours; the dialog's title and its controls' captions are the
// resource's. The app makes one on its stack, points m_pGlobs at the
// processor's globals and runs DoModal.

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

// The string table's 104: "Processing Options for %1!s!.ed".
#define IDS_PO_TITLE					104

class CProcessorGlobs;

// 0x77c bytes: CDialog's 0x5c, then ours. The constructor (0x452820)
// builds the controls in this order; DoDataExchange (0x453520) ties them
// to their IDs.
class CProcessingOptionsDlg : public CDialog
{
public:
						CProcessingOptionsDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_PROCESSINGOPTIONS };

	// The helpers (the names are ours).
	void				UpdateControls();								// 0x452b10
	void				SetArgs(char *pCmdLine);						// 0x452d60
	void				BuildCommandLine(CString &cmdLine, CString &extra);	// 0x452de0
	void				SaveControls();									// 0x453010
	void				InitControls();									// 0x453260

	virtual void		DoDataExchange(CDataExchange* pDX);				// 0x453520
	virtual BOOL		OnInitDialog();									// 0x453670
	virtual void		OnOK();											// 0x453a20

	afx_msg void		OnPaint();										// 0x453c30
	afx_msg void		OnReplaceTextures();							// 0x453cb0
	afx_msg void		OnLButtonUp(UINT nFlags, CPoint point);			// 0x453d00
	afx_msg void		OnOptimize();									// 0x453d20
	afx_msg void		OnLight();										// 0x453d70
	afx_msg void		OnImportGeometry();								// 0x453dc0
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
