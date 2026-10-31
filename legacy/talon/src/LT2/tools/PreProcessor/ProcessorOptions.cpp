// talon: a module of its own in the image, 0x00442e30..0x004445fa, after
// Processing: the "Processing Options" dialog (dialog 130), which reads
// processor.opt, shows the options, builds the command line from them
// (-bsp, -light, -splitweight...) and writes processor.opt back (the
// names are ours). Its vtable is 0x475a68, its message map 0x4758b0.
//
// What precedes it from 0x441d30 (CMoArray< CMoArray<X*> >'s GenList
// methods, InternalNiceSetSize, BaseNew/BaseDelete, the CLinkedList
// pieces at 0x442740..0x442d10) is template code Processing.obj emits:
// the vtable those methods fill, 0x47578c, is installed at 0x43f839 in
// Processing's own code.

#include "bdefs.h"
#include "processing.h"
#include "ProcessorOptions_decl.h"


// The token after *ppIn's spaces, copied to *ppOut and terminated there
// (0x443910); NULL at the end of the line.
static char* ProcessorOptions_GetToken(char **ppIn, char **ppOut);


// 0x442e30
CProcessingOptionsDlg::CProcessingOptionsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CProcessingOptionsDlg::IDD, pParent)
{
}


// 0x443090
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
	DDX_Control(pDX, IDC_PO_AIVOLUMES, m_AIVolumes);
}


// 0x4758b0 (GetMessageMap 0x4431d0)
BEGIN_MESSAGE_MAP(CProcessingOptionsDlg, CDialog)
	ON_BN_CLICKED(IDC_PO_REPLACETEXTURES, OnReplaceTextures)
	ON_BN_CLICKED(IDC_PO_IMPORTGEOMETRY, OnImportGeometry)
	ON_BN_CLICKED(IDC_PO_TEXTUREFLAGS, OnTextureFlags)
	ON_BN_CLICKED(IDC_PO_IGNOREHIDDEN, OnIgnoreHidden)
	ON_BN_CLICKED(IDC_PO_FIXTJUNC, OnFixTJunc)
	ON_BN_CLICKED(IDC_PO_LIGHT, OnLight)
	ON_BN_CLICKED(IDC_PO_LIGHTANIMS, OnLightAnims)
	ON_BN_CLICKED(IDC_PO_SHADOWS, OnShadows)
	ON_BN_CLICKED(IDC_PO_VERTICESONLY, OnVerticesOnly)
	ON_BN_CLICKED(IDC_PO_LAMBERTIAN, OnLambertian)
	ON_BN_CLICKED(IDC_PO_OPTIMIZE, OnOptimize)
	ON_BN_CLICKED(IDC_PO_FAST, OnFast)
	ON_BN_CLICKED(IDC_PO_AIVOLUMES, OnAIVolumes)
	ON_BN_CLICKED(IDC_PO_SLIDERLEFT, OnSliderLeft)
	ON_BN_CLICKED(IDC_PO_SLIDERRIGHT, OnSliderRight)
	ON_WM_LBUTTONUP()
	ON_WM_PAINT()
END_MESSAGE_MAP()


// The slider's position, in the static after "Slider Reference:" (0x4431e0).
LRESULT CProcessingOptionsDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message == WM_HSCROLL)
	{
		CWnd *pWnd = GetDlgItem(IDC_PO_SLIDERREF);
		if(pWnd)
		{
			char str[16];
			sprintf(str, "%d", m_SplitWeight.GetPos());
			pWnd->SetWindowText(str);
		}
	}

	return CDialog::WindowProc(message, wParam, lParam);
}


// Which options can be changed, from what's checked (0x443260): with no
// DAT to replace textures in, the geometry is imported.
void CProcessingOptionsDlg::UpdateControls()
{
	if(m_bDATExists)
	{
		m_ReplaceTextures.EnableWindow(TRUE);
		m_ReplaceTextures.SetCheck(m_pGlobs->m_bReplaceTextures);

		if(m_ReplaceTextures.GetCheck())
		{
			m_ImportGeometry.EnableWindow(FALSE);
			m_ImportGeometry.SetCheck(0);
			m_TextureFlags.EnableWindow(TRUE);
			m_TextureFlags.SetCheck(m_pGlobs->m_bTextureFlags);
			m_IgnoreHidden.EnableWindow(FALSE);
			m_IgnoreHidden.SetCheck(m_pGlobs->m_bIgnoreHidden);
			m_AIVolumes.EnableWindow(FALSE);
			m_AIVolumes.SetCheck(0);
			m_FixTJunc.EnableWindow(FALSE);
			m_FixTJunc.SetCheck(m_pGlobs->m_bFixTJunc);
		}
		else
		{
			m_ImportGeometry.EnableWindow(TRUE);
			m_ImportGeometry.SetCheck(m_pGlobs->m_bBsp);

			if(m_ImportGeometry.GetCheck())
			{
				m_TextureFlags.EnableWindow(TRUE);
				m_TextureFlags.SetCheck(m_pGlobs->m_bTextureFlags);
				m_FixTJunc.EnableWindow(TRUE);
				m_FixTJunc.SetCheck(m_pGlobs->m_bFixTJunc);
				m_AIVolumes.EnableWindow(FALSE);
				m_AIVolumes.SetCheck(1);
			}
			else
			{
				m_TextureFlags.EnableWindow(FALSE);
				m_TextureFlags.SetCheck(m_pGlobs->m_bTextureFlags);
				m_FixTJunc.EnableWindow(FALSE);
				m_FixTJunc.SetCheck(m_pGlobs->m_bFixTJunc);
				m_AIVolumes.EnableWindow(TRUE);
				m_AIVolumes.SetCheck(m_pGlobs->m_bAIVolumes);
			}

			m_IgnoreHidden.EnableWindow(TRUE);
			m_IgnoreHidden.SetCheck(m_pGlobs->m_bIgnoreHidden);
		}
	}
	else
	{
		m_ReplaceTextures.EnableWindow(FALSE);
		m_ReplaceTextures.SetCheck(0);
		m_ImportGeometry.EnableWindow(FALSE);
		m_ImportGeometry.SetCheck(1);
		m_TextureFlags.EnableWindow(TRUE);
		m_TextureFlags.SetCheck(m_pGlobs->m_bTextureFlags);
		m_IgnoreHidden.EnableWindow(TRUE);
		m_IgnoreHidden.SetCheck(m_pGlobs->m_bIgnoreHidden);
		m_AIVolumes.EnableWindow(FALSE);
		m_AIVolumes.SetCheck(1);
		m_FixTJunc.EnableWindow(TRUE);
		m_FixTJunc.SetCheck(m_pGlobs->m_bFixTJunc);
	}

	if(!m_ReplaceTextures.GetCheck())
	{
		m_Light.EnableWindow(TRUE);
		m_Light.SetCheck(m_pGlobs->m_bLight);

		if(m_Light.GetCheck())
		{
			m_LightAnims.EnableWindow(TRUE);
			m_LightAnims.SetCheck(m_pGlobs->m_bLightAnimations);
			m_Shadows.EnableWindow(TRUE);
			m_Shadows.SetCheck(m_pGlobs->m_bClipLight);
			m_VerticesOnly.EnableWindow(TRUE);
			m_VerticesOnly.SetCheck(m_pGlobs->m_bVerticesOnly);
			m_Lambertian.EnableWindow(TRUE);
		}
		else
		{
			m_LightAnims.EnableWindow(FALSE);
			m_LightAnims.SetCheck(m_pGlobs->m_bLightAnimations);
			m_Shadows.EnableWindow(FALSE);
			m_Shadows.SetCheck(m_pGlobs->m_bClipLight);
			m_VerticesOnly.EnableWindow(FALSE);
			m_VerticesOnly.SetCheck(m_pGlobs->m_bVerticesOnly);
			m_Lambertian.EnableWindow(FALSE);
		}

		m_Lambertian.SetCheck(m_pGlobs->m_bLambertian);
	}
	else
	{
		m_Light.EnableWindow(FALSE);
		m_Light.SetCheck(0);
		m_LightAnims.EnableWindow(FALSE);
		m_LightAnims.SetCheck(m_pGlobs->m_bLightAnimations);
		m_Shadows.EnableWindow(FALSE);
		m_Shadows.SetCheck(m_pGlobs->m_bClipLight);
		m_VerticesOnly.EnableWindow(FALSE);
		m_VerticesOnly.SetCheck(m_pGlobs->m_bVerticesOnly);
		m_Lambertian.EnableWindow(FALSE);
		m_Lambertian.SetCheck(m_pGlobs->m_bLambertian);
	}

	if(!m_ReplaceTextures.GetCheck())
	{
		m_Optimize.EnableWindow(TRUE);
		m_Optimize.SetCheck(m_pGlobs->m_bOptimize);

		if(m_Optimize.GetCheck())
			m_Fast.EnableWindow(TRUE);
		else
			m_Fast.EnableWindow(FALSE);

		m_Fast.SetCheck(m_pGlobs->m_bFast);
	}
	else
	{
		m_Optimize.EnableWindow(FALSE);
		m_Optimize.SetCheck(0);
		m_Fast.EnableWindow(FALSE);
		m_Fast.SetCheck(m_pGlobs->m_bFast);
	}
}


// Splits a command line into m_Argv, for InitProcessOptions (0x443880).
void CProcessingOptionsDlg::SetArgs(char *pCmdLine)
{
	char *pIn, *pOut;

	pIn = pCmdLine;
	m_Argc = 0;
	pOut = m_ArgBuffer;

	m_Argv[m_Argc] = ProcessorOptions_GetToken(&pIn, &pOut);
	while(m_Argv[m_Argc])
	{
		m_Argc++;
		m_Argv[m_Argc] = ProcessorOptions_GetToken(&pIn, &pOut);
	}
}


// 0x443910. The end test is a helper (inlined; the name is ours): written
// out twice, VC6 threads the loop's first test through the one before it
// and rotates the loop, where shipped tests at the top and jumps back.
inline BOOL ProcessorOptions_IsTokenEnd(char c)
{
	return c == 0 || c == '\n' || c == ' ';
}

static char* ProcessorOptions_GetToken(char **ppIn, char **ppOut)
{
	char *pRet;

	while(**ppIn == ' ')
		(*ppIn)++;

	if(ProcessorOptions_IsTokenEnd(**ppIn))
		return NULL;

	pRet = *ppOut;
	while(!ProcessorOptions_IsTokenEnd(**ppIn))
	{
		**ppOut = **ppIn;
		(*ppOut)++;
		(*ppIn)++;
	}

	**ppOut = 0;
	(*ppOut)++;

	return *pRet ? pRet : NULL;
}


// The checkboxes the click handlers don't keep, the project directory
// and the slider, into the globals (0x443980).
void CProcessingOptionsDlg::SaveControls()
{
	m_pGlobs->m_bAIVolumes = m_AIVolumes.GetCheck();
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

	CWnd *pWnd = GetDlgItem(IDC_PO_SLIDERREF);
	if(pWnd)
	{
		char str[16];
		sprintf(str, "%d", m_SplitWeight.GetPos());
		pWnd->SetWindowText(str);
	}
}


// The project directory and the slider, from the globals (0x443b60).
void CProcessingOptionsDlg::InitSlider()
{
	float fBalance;

	m_ProjectDir.SetWindowText(m_pGlobs->m_ProjectDir);

	fBalance = m_pGlobs->m_BalanceWeight / (m_pGlobs->m_SplitWeight + m_pGlobs->m_BalanceWeight);

	m_SplitWeight.SetRangeMin(0, FALSE);
	m_SplitWeight.SetRangeMax(1000, FALSE);
	m_SplitWeight.SetTicFreq(100);
	m_SplitWeight.SetPos((int)(fBalance * 1000.0f));
	m_SplitWeight.SetParent(this);

	CWnd *pWnd = GetDlgItem(IDC_PO_SLIDERREF);
	if(pWnd)
	{
		char str[16];
		sprintf(str, "%d", m_SplitWeight.GetPos());
		pWnd->SetWindowText(str);
	}
}


// The options as a command line, and the extra parameters (0x443c60).
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
		cmdLine += "-Lambertian ";

	if(m_pGlobs->m_bOptimize)
		cmdLine += "-optimize ";

	if(m_pGlobs->m_bFast)
		cmdLine += "-fast ";

	if(m_pGlobs->m_bAIVolumes)
		cmdLine += "-aivolumes ";

	if(m_pGlobs->m_bLogFile)
		cmdLine += "-logfile ";

	cmdLine += "-projectdir ";
	cmdLine += m_pGlobs->m_ProjectDir;
	cmdLine += " ";

	sprintf(str, "-splitweight %f ", m_pGlobs->m_SplitWeight);
	cmdLine += str;

	m_ExtraParams.GetWindowText(extraParams, sizeof(extraParams));
	extra = extraParams;
}


// The defaults, then processor.opt's options (0x443e60): its first line
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
	m_pGlobs->m_bTextureFlags = TRUE;
	m_pGlobs->m_bIgnoreHidden = FALSE;
	m_pGlobs->m_bFixTJunc = TRUE;
	m_pGlobs->m_bAIVolumes = FALSE;

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
	{
		m_pGlobs->m_bBsp = TRUE;
		m_pGlobs->m_bReplaceTextures = FALSE;
	}

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
		InitSlider();
		UpdateControls();
	}

	return TRUE;
}


// The checkboxes the dialog keeps up to date as they're clicked
// (0x4440c0..0x444300).
void CProcessingOptionsDlg::OnReplaceTextures()
{
	m_pGlobs->m_bReplaceTextures = m_ReplaceTextures.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnImportGeometry()
{
	m_pGlobs->m_bBsp = m_ImportGeometry.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnTextureFlags()
{
	m_pGlobs->m_bTextureFlags = m_TextureFlags.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnIgnoreHidden()
{
	m_pGlobs->m_bIgnoreHidden = m_IgnoreHidden.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnFixTJunc()
{
	m_pGlobs->m_bFixTJunc = m_FixTJunc.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnLight()
{
	m_pGlobs->m_bLight = m_Light.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnLightAnims()
{
	m_pGlobs->m_bLightAnimations = m_LightAnims.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnShadows()
{
	m_pGlobs->m_bClipLight = m_Shadows.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnVerticesOnly()
{
	m_pGlobs->m_bVerticesOnly = m_VerticesOnly.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnLambertian()
{
	m_pGlobs->m_bLambertian = m_Lambertian.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnOptimize()
{
	m_pGlobs->m_bOptimize = m_Optimize.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnFast()
{
	m_pGlobs->m_bFast = m_Fast.GetCheck();
	UpdateControls();
}

void CProcessingOptionsDlg::OnAIVolumes()
{
	m_pGlobs->m_bAIVolumes = m_AIVolumes.GetCheck();
	UpdateControls();
}


// "<" and ">" step the slider (0x444330, 0x4443d0).
//
// talon: the range end is read into a local and pos stepped in the
// compare. `if(pos - 1 >= m_SplitWeight.GetRangeMin())` gives a
// `lea -1(%eax),%ebx` for the shipped mov+dec (0x44435a, 0x444362), and
// `int pos = GetPos() - 1;` (pos--, --pos, pos -= 1, an early return, an
// inline helper) gives the mov+dec with ebx and edi swapped.
void CProcessingOptionsDlg::OnSliderLeft()
{
	int pos = m_SplitWeight.GetPos();
	int minPos = m_SplitWeight.GetRangeMin();
	if(--pos >= minPos)
	{
		m_SplitWeight.SetPos(pos);

		CWnd *pWnd = GetDlgItem(IDC_PO_SLIDERREF);
		if(pWnd)
		{
			char str[16];
			sprintf(str, "%d", m_SplitWeight.GetPos());
			pWnd->SetWindowText(str);
		}
	}
}

void CProcessingOptionsDlg::OnSliderRight()
{
	int pos = m_SplitWeight.GetPos();
	int maxPos = m_SplitWeight.GetRangeMax();
	if(++pos <= maxPos)
	{
		m_SplitWeight.SetPos(pos);

		CWnd *pWnd = GetDlgItem(IDC_PO_SLIDERREF);
		if(pWnd)
		{
			char str[16];
			sprintf(str, "%d", m_SplitWeight.GetPos());
			pWnd->SetWindowText(str);
		}
	}
}


// Takes the options, parses them into the globals and saves them in
// processor.opt (0x444470).
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


// 0x4445d0
void CProcessingOptionsDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	CDialog::OnLButtonUp(nFlags, point);
}


// 0x4445e0
void CProcessingOptionsDlg::OnPaint()
{
	CPaintDC dc(this); // device context for painting
}
