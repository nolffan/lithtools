// talon: a module of its own in the image, 0x00452780..0x00453fa0, after
// Processing: the "Processing Options" dialog (dialog 130), which reads
// processor.opt, shows the options, builds the command line from them
// (-bsp, -light, -splitweight...) and writes processor.opt back (the
// names are ours). Its vtable is 0x48a350, its message map 0x49b310.
// NOLF's (January 2001) is AvP2's before its slider reference, "<" and
// ">" buttons and AI volumes: four of the check boxes have click handlers,
// the rest are read when OK is pressed. icl emits CButton's and CEdit's
// constructors (0x453e80, 0x453e10), CWnd::GetWindowRect (0x453ef0) and
// the controls' ??_G (0x453f10..0x453f70) out of line here.

#include "bdefs.h"
#include "processing.h"
#include "ProcessorOptions_decl.h"


// The token after *ppIn's spaces, copied to *ppOut and terminated there
// (0x452780); NULL at the end of the line.
static char* ProcessorOptions_GetToken(char **ppIn, char **ppOut);


// 0x452820
CProcessingOptionsDlg::CProcessingOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProcessingOptionsDlg::IDD, pParent)
{
}


// 0x453520
void CProcessingOptionsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PO_LAMBERTIAN, m_Lambertian);
	DDX_Control(pDX, IDC_PO_FIXTJUNC, m_FixTJunc);
	DDX_Control(pDX, IDC_PO_PROJECTDIR, m_ProjectDir);
	DDX_Control(pDX, IDC_PO_TEXTUREFLAGS, m_TextureFlags);
	DDX_Control(pDX, IDC_PO_REPLACETEXTURES, m_ReplaceTextures);
	DDX_Control(pDX, IDC_PO_EXTRAPARAMS, m_ExtraParams);
	DDX_Control(pDX, IDC_PO_OPTIMIZE, m_Optimize);
	DDX_Control(pDX, IDC_PO_LIGHT, m_Light);
	DDX_Control(pDX, IDC_PO_FAST, m_Fast);
	DDX_Control(pDX, IDC_PO_VERTICESONLY, m_VerticesOnly);
	DDX_Control(pDX, IDC_PO_LIGHTANIMS, m_LightAnims);
	DDX_Control(pDX, IDC_PO_SHADOWS, m_Shadows);
	DDX_Control(pDX, IDC_PO_LOGFILE, m_LogFile);
	DDX_Control(pDX, IDC_PO_IMPORTGEOMETRY, m_ImportGeometry);
	DDX_Control(pDX, IDC_PO_SPLITWEIGHT, m_SplitWeight);
	DDX_Control(pDX, IDC_PO_IGNOREHIDDEN, m_IgnoreHidden);
}


// 0x49b310 (GetMessageMap 0x453ca0). talon: NOLF's dialog has no slider
// reference or "<" ">" buttons, and only these four check boxes keep the
// globals up to date as they're clicked.
BEGIN_MESSAGE_MAP(CProcessingOptionsDlg, CDialog)
	ON_BN_CLICKED(IDC_PO_IMPORTGEOMETRY, OnImportGeometry)
	ON_BN_CLICKED(IDC_PO_LIGHT, OnLight)
	ON_BN_CLICKED(IDC_PO_OPTIMIZE, OnOptimize)
	ON_WM_LBUTTONUP()
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_PO_REPLACETEXTURES, OnReplaceTextures)
END_MESSAGE_MAP()


// Which options can be changed, from what's checked (0x452b10).
void CProcessingOptionsDlg::UpdateControls()
{
	BOOL bNoReplace = !m_pGlobs->m_bReplaceTextures;

	m_ImportGeometry.EnableWindow(m_bDATExists);
	m_ImportGeometry.SetCheck(m_pGlobs->m_bBsp);
	m_TextureFlags.EnableWindow(m_pGlobs->m_bReplaceTextures || m_pGlobs->m_bBsp);
	m_TextureFlags.SetCheck(m_pGlobs->m_bTextureFlags);
	m_IgnoreHidden.SetCheck(m_pGlobs->m_bIgnoreHidden);
	m_FixTJunc.EnableWindow(m_pGlobs->m_bBsp);
	m_FixTJunc.SetCheck(m_pGlobs->m_bFixTJunc);

	m_Light.EnableWindow(bNoReplace);
	m_Light.SetCheck(m_pGlobs->m_bLight);
	m_LightAnims.EnableWindow(m_pGlobs->m_bLight & bNoReplace);
	m_Shadows.EnableWindow(m_pGlobs->m_bLight & bNoReplace);
	m_VerticesOnly.EnableWindow(m_pGlobs->m_bLight & bNoReplace);
	m_Lambertian.EnableWindow(m_pGlobs->m_bLight & bNoReplace);

	m_Fast.EnableWindow(m_pGlobs->m_bOptimize & bNoReplace);
	m_Optimize.EnableWindow(bNoReplace);
	m_Optimize.SetCheck(m_pGlobs->m_bOptimize);
}


// Splits a command line into m_Argv, for InitProcessOptions (0x452d60).
void CProcessingOptionsDlg::SetArgs(char *pCmdLine)
{
	char *pIn, *pOut;

	pIn = pCmdLine;
	m_Argc = 0;
	pOut = m_ArgBuffer;

	while((m_Argv[m_Argc] = ProcessorOptions_GetToken(&pIn, &pOut)) != NULL)
		m_Argc++;
}


// 0x452780. talon: NOLF's takes a quoted token whole, without its quotes
// (the project directory is written quoted, BuildCommandLine).
static char* ProcessorOptions_GetToken(char **ppIn, char **ppOut)
{
	char *pRet;
	bool bInQuote;

	while(**ppIn == ' ')
		(*ppIn)++;

	if(**ppIn == 0 || **ppIn == '\n')
		return NULL;

	pRet = *ppOut;
	bInQuote = false;
	while(**ppIn != 0 && **ppIn != '\n')
	{
		if(**ppIn == ' ' && !bInQuote)
			break;

		if(**ppIn == '"')
			bInQuote = !bInQuote;
		else
			*(*ppOut)++ = **ppIn;

		(*ppIn)++;
	}

	*(*ppOut)++ = 0;

	return *pRet ? pRet : NULL;
}


// The checkboxes the click handlers don't keep, the project directory
// and the slider, into the globals (0x453010).
void CProcessingOptionsDlg::SaveControls()
{
	m_pGlobs->m_bReplaceTextures = m_ReplaceTextures.GetCheck();
	m_pGlobs->m_bLightAnimations = m_LightAnims.GetCheck();
	m_pGlobs->m_bClipLight = m_Shadows.GetCheck();
	m_pGlobs->m_bLogFile = m_LogFile.GetCheck();
	m_pGlobs->m_bVerticesOnly = m_VerticesOnly.GetCheck();
	m_pGlobs->m_bLambertian = m_Lambertian.GetCheck();
	m_pGlobs->m_bFast = m_Fast.GetCheck();
	m_pGlobs->m_bTextureFlags = m_TextureFlags.GetCheck();
	m_pGlobs->m_bIgnoreHidden = m_IgnoreHidden.GetCheck();
	m_pGlobs->m_bFixTJunc = m_FixTJunc.GetCheck();

	m_ProjectDir.GetWindowText(m_pGlobs->m_ProjectDir, sizeof(m_pGlobs->m_ProjectDir) - 1);

	m_pGlobs->m_BalanceWeight = (float)m_SplitWeight.GetPos() * 0.001f;
	m_pGlobs->m_SplitWeight = 1.0f - m_pGlobs->m_BalanceWeight;
}


// The check boxes the click handlers don't keep, the project directory
// and the slider, from the globals (0x453260; the name is ours).
void CProcessingOptionsDlg::InitControls()
{
	float fBalance;

	m_ReplaceTextures.SetCheck(m_pGlobs->m_bReplaceTextures);
	m_LightAnims.SetCheck(m_pGlobs->m_bLightAnimations);
	m_Shadows.SetCheck(m_pGlobs->m_bClipLight);
	m_LogFile.SetCheck(m_pGlobs->m_bLogFile);
	m_VerticesOnly.SetCheck(m_pGlobs->m_bVerticesOnly);
	m_Lambertian.SetCheck(m_pGlobs->m_bLambertian);
	m_Fast.SetCheck(m_pGlobs->m_bFast);
	m_TextureFlags.SetCheck(m_pGlobs->m_bTextureFlags);
	m_IgnoreHidden.SetCheck(m_pGlobs->m_bIgnoreHidden);
	m_FixTJunc.SetCheck(m_pGlobs->m_bFixTJunc);

	m_ProjectDir.SetWindowText(m_pGlobs->m_ProjectDir);

	fBalance = m_pGlobs->m_BalanceWeight / (m_pGlobs->m_SplitWeight + m_pGlobs->m_BalanceWeight);

	m_SplitWeight.SetRangeMin(0, FALSE);
	m_SplitWeight.SetRangeMax(1000, FALSE);
	m_SplitWeight.SetPos((int)(fBalance * 1000.0f));
}


// The options as a command line, and the extra parameters (0x452de0).
void CProcessingOptionsDlg::BuildCommandLine(CString &cmdLine, CString &extra)
{
	char str[100];
	char extraParams[400];

	if(m_pGlobs->m_bReplaceTextures)
		cmdLine += "-replacetextures ";

	if(m_pGlobs->m_bTextureFlags)
		cmdLine += "-textureflags ";

	if(m_pGlobs->m_bBsp)
		cmdLine += "-bsp ";

	if(m_pGlobs->m_bIgnoreHidden)
		cmdLine += "-ignorehidden ";

	if(m_pGlobs->m_bFixTJunc)
		cmdLine += "-FixTJunc ";

	if(m_pGlobs->m_bLight)
		cmdLine += "-light ";

	if(!m_pGlobs->m_bClipLight)
		cmdLine += "-nocliplight ";

	if(m_pGlobs->m_bLightAnimations)
		cmdLine += "-lightanims ";

	if(m_pGlobs->m_bVerticesOnly)
		cmdLine += "-onlylightverts ";

	if(m_pGlobs->m_bLambertian)
		cmdLine += "-LambertianLightmaps ";

	if(m_pGlobs->m_bOptimize)
		cmdLine += "-optimize ";

	if(m_pGlobs->m_bFast)
		cmdLine += "-fast ";

	if(m_pGlobs->m_bLogFile)
		cmdLine += "-logfile ";

	// talon: NOLF's quotes the directory ("-projectdir \"", "\" ").
	cmdLine += "-projectdir \"";
	cmdLine += m_pGlobs->m_ProjectDir;
	cmdLine += "\" ";

	sprintf(str, "-splitweight %f ", m_pGlobs->m_SplitWeight);
	cmdLine += str;

	m_ExtraParams.GetWindowText(extraParams, sizeof(extraParams));
	extra = extraParams;
}


// The defaults, then processor.opt's options (0x453670): its first line
// is the command line, its second the extra parameters.
BOOL CProcessingOptionsDlg::OnInitDialog()
{
	CString str;
	char cmdLine[600], extra[600];
	FILE *fp;

	m_pGlobs->m_bReplaceTextures = FALSE;
	m_pGlobs->m_MaxDetail = 10000;
	m_pGlobs->m_SplitWeight = 1.0f;
	m_pGlobs->m_BalanceWeight = 1.0f;
	m_pGlobs->m_bLogFile = FALSE;
	m_pGlobs->m_bLight = FALSE;
	m_pGlobs->m_bClipLight = TRUE;
	m_pGlobs->m_bSkipDialog = FALSE;
	m_pGlobs->m_bTextureFlags = TRUE;
	m_pGlobs->m_bIgnoreHidden = FALSE;
	m_pGlobs->m_bFixTJunc = TRUE;

	fp = fopen(m_pGlobs->m_OutputFile, "rt");
	if(fp)
	{
		m_bDATExists = TRUE;
		fclose(fp);
	}
	else
	{
		m_bDATExists = FALSE;
	}

	if(!m_bDATExists)
		m_pGlobs->m_bBsp = TRUE;

	fp = fopen("processor.opt", "rt");
	if(fp)
	{
		fgets(cmdLine, sizeof(cmdLine), fp);
		fgets(extra, sizeof(extra), fp);

		if(strlen(cmdLine))
			cmdLine[strlen(cmdLine)-1] = 0;

		if(strlen(extra))
			extra[strlen(extra)-1] = 0;

		SetArgs(cmdLine);
		m_pGlobs->m_Argc = m_Argc;
		m_pGlobs->m_Argv = m_Argv;
		InitProcessOptions(m_pGlobs);

		fclose(fp);
	}

	if(m_pGlobs->m_bSkipDialog)
	{
		EndDialog(IDOK);
	}
	else
	{
		CDialog::OnInitDialog();

		str.FormatMessage(IDS_PO_TITLE, m_pGlobs->m_InputFile);
		SetWindowText(str);
		GetWindowRect(&m_Rect);

		m_ExtraParams.SetWindowText(extra);
		InitControls();
		UpdateControls();
	}

	return TRUE;
}


// The check boxes the dialog keeps up to date as they're clicked
// (0x453cb0, 0x453d20, 0x453d70, 0x453dc0).
void CProcessingOptionsDlg::OnReplaceTextures()
{
	m_pGlobs->m_bReplaceTextures = m_ReplaceTextures.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnOptimize()
{
	m_pGlobs->m_bOptimize = m_Optimize.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnLight()
{
	m_pGlobs->m_bLight = m_Light.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnImportGeometry()
{
	m_pGlobs->m_bBsp = m_ImportGeometry.GetCheck();
	UpdateControls();
}


// Takes the options, parses them into the globals and saves them in
// processor.opt (0x453a20).
void CProcessingOptionsDlg::OnOK()
{
	CString cmdLine, extra, all;
	FILE *fp;

	SaveControls();
	BuildCommandLine(cmdLine, extra);

	all = cmdLine + " " + extra;
	SetArgs((char*)(LPCTSTR)all);
	m_pGlobs->m_Argc = m_Argc;
	m_pGlobs->m_Argv = m_Argv;
	InitProcessOptions(m_pGlobs);

	fp = fopen("processor.opt", "wt");
	if(fp)
	{
		fprintf(fp, "%s\n", (LPCTSTR)cmdLine);
		fprintf(fp, "%s\n", (LPCTSTR)extra);
		fclose(fp);
	}

	CDialog::OnOK();
}


// 0x453d00
void CProcessingOptionsDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	CDialog::OnLButtonUp(nFlags, point);
}


// 0x453c30
void CProcessingOptionsDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
}
