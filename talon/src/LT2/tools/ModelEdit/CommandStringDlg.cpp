// CommandStringDlg.cpp : implementation file
//

#include "precompile.h"
#include "modeledit.h"
#include "commandstringdlg.h"
#include "model_ops.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCommandStringDlg dialog


CCommandStringDlg::CCommandStringDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCommandStringDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCommandStringDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CCommandStringDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCommandStringDlg)
	DDX_Control(pDX, IDC_COMMANDSTRING, m_CommandString);
	//}}AFX_DATA_MAP
}


// talon: 2001's command helper controls, one per key of the model command
// string (the table at 0x4892e0, 27 entries of 0x208 bytes; dialog 157's
// controls). Several entries with one name take that key's values in turn.
struct CommandKey
{
	UINT	m_ID;
	char	m_Name[256];
	char	m_Format[256];
	bool	m_bCheck;
};

static CommandKey g_CommandKeys[] =
{
	{ IDC_CMDSTR_AMBIENTLIGHT,	"AmbientLight",	"AmbientLight %s; ",	false },
	{ IDC_CMDSTR_DIRLIGHT,	"DirLight",	"DirLight %s; ",	false },
	{ IDC_CMDSTR_DRAWINDEXEDDIST,	"DrawIndexedDist",	"DrawIndexedDist %s; ",	false },
	{ IDC_CMDSTR_FADERANGEMIN,	"FadeRangeMin",	"FadeRangeMin %s; ",	false },
	{ IDC_CMDSTR_FADERANGEMAX,	"FadeRangeMax",	"FadeRangeMax %s; ",	false },
	{ IDC_CMDSTR_FADESPRITETEXTURE,	"FadeSpriteTex",	"FadeSpriteTex %s; ",	false },
	{ IDC_CMDSTR_FADESPRITESIZEX,	"FadeSpriteSize",	"FadeSpriteSize %s ",	false },
	{ IDC_CMDSTR_FADESPRITESIZEY,	"FadeSpriteSize",	"%s; ",	false },
	{ IDC_CMDSTR_FOVXOFFSET,	"FOVXOffset",	"FOVXOffset %s; ",	false },
	{ IDC_CMDSTR_FOVYOFFSET,	"FOVYOffset",	"FOVYOffset %s; ",	false },
	{ IDC_CMDSTR_LODINCREMENT,	"LodIncrement",	"LodIncrement %s; ",	false },
	{ IDC_CMDSTR_LODMAXDIST,	"LodMaxDist",	"LodMaxDist %s; ",	false },
	{ IDC_CMDSTR_LODSTARTDIST,	"LodStartDist",	"LodStartDist %s; ",	false },
	{ IDC_CMDSTR_NOANIMATION,	"NoAnimation",	"NoAnimation; ",	true },
	{ IDC_CMDSTR_NORMALREF1,	"NormalRef",	"NormalRef %s ",	false },
	{ IDC_CMDSTR_NORMALREF2,	"NormalRef",	"%s; ",	false },
	{ IDC_CMDSTR_SHADOWENABLE,	"ShadowEnable",	"ShadowEnable; ",	true },
	{ IDC_CMDSTR_SHADOWPROJECTLENGTH,	"ShadowProjectLength",	"ShadowProjectLength %s; ",	false },
	{ IDC_CMDSTR_SHADOWLIGHTDIST,	"ShadowLightDist",	"ShadowLightDist %s; ",	false },
	{ IDC_CMDSTR_SHADOWSIZEX,	"ShadowSizeX",	"ShadowSizeX %s; ",	false },
	{ IDC_CMDSTR_SHADOWSIZEY,	"ShadowSizeY",	"ShadowSizeY %s; ",	false },
	{ IDC_CMDSTR_SHADOWCENTERX,	"ShadowCenterOffset",	"ShadowCenterOffset %s ",	false },
	{ IDC_CMDSTR_SHADOWCENTERY,	"ShadowCenterOffset",	"%s ",	false },
	{ IDC_CMDSTR_SHADOWCENTERZ,	"ShadowCenterOffset",	"%s; ",	false },
	{ IDC_CMDSTR_SPECULARENABLE,	"SpecularEnable",	"SpecularEnable; ",	true },
	{ IDC_CMDSTR_VISRADIUS,	"VisRadius",	"VisRadius %s; ",	false },
	{ IDC_CMDSTR_EXTRAPARAMS,	"Extra_Commands",	"%s",	false },
};

#define NUM_COMMANDKEYS	((int)(sizeof(g_CommandKeys) / sizeof(g_CommandKeys[0])))

// Set false around a change made from code, so the control's notification
// doesn't rebuild what was just set (0x48c9b8, 0x48c9b9).
static bool g_bUpdateCommandString = true;
static bool g_bUpdateControls = true;




BEGIN_MESSAGE_MAP(CCommandStringDlg, CDialog)
	//{{AFX_MSG_MAP(CCommandStringDlg)
	ON_EN_CHANGE(IDC_CMDSTR_AMBIENTLIGHT, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_DIRLIGHT, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_DRAWINDEXEDDIST, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_FADERANGEMAX, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_FADERANGEMIN, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_FADESPRITESIZEX, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_FADESPRITESIZEY, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_FADESPRITETEXTURE, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_FOVXOFFSET, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_FOVYOFFSET, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_LODINCREMENT, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_LODMAXDIST, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_LODSTARTDIST, OnChangeHelperControl)
	ON_BN_CLICKED(IDC_CMDSTR_NOANIMATION, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_NORMALREF2, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_NORMALREF1, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_SHADOWCENTERX, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_SHADOWCENTERY, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_SHADOWCENTERZ, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_SHADOWLIGHTDIST, OnChangeHelperControl)
	ON_BN_CLICKED(IDC_CMDSTR_SHADOWENABLE, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_SHADOWPROJECTLENGTH, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_SHADOWSIZEX, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_SHADOWSIZEY, OnChangeHelperControl)
	ON_BN_CLICKED(IDC_CMDSTR_SPECULARENABLE, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_VISRADIUS, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_CMDSTR_EXTRAPARAMS, OnChangeHelperControl)
	ON_EN_CHANGE(IDC_COMMANDSTRING, OnChangeCommandString)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCommandStringDlg message handlers

// 0x401cf1
void CCommandStringDlg::OnOK() 
{
	m_CommandString.GetWindowText(m_szCommandString, 2000);
	m_pModel->m_CommandString = m_pModel->AddString(m_szCommandString);
	m_pModel->ParseCommandString();
	
	CDialog::OnOK();
}

// 0x401d31
BOOL CCommandStringDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	if(m_pModel->m_CommandString)
		m_CommandString.SetWindowText(m_pModel->m_CommandString);

	UpdateControls();

	g_bUpdateCommandString = true;
	g_bUpdateControls = true;
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

// 0x401d68: the controls -> the command string.
void CCommandStringDlg::UpdateCommandString()
{
	char szValue[256];
	char szTemp[256];
	int i;

	if(!g_bUpdateCommandString)
	{
		g_bUpdateCommandString = true;
		return;
	}

	memset(m_szCommandString, 0, sizeof(m_szCommandString));

	for(i=0; i < NUM_COMMANDKEYS; i++)
	{
		if(g_CommandKeys[i].m_bCheck)
		{
			if(IsDlgButtonChecked(g_CommandKeys[i].m_ID))
				strcat(m_szCommandString, g_CommandKeys[i].m_Format);
		}
		else
		{
			if(GetDlgItemText(g_CommandKeys[i].m_ID, szValue, 256))
			{
				sprintf(szTemp, g_CommandKeys[i].m_Format, szValue);
				strcat(m_szCommandString, szTemp);
			}
		}
	}

	g_bUpdateControls = false;
	m_CommandString.SetWindowText(m_szCommandString);
}

// 0x401e31: the command string -> the controls.
// talon: near, 585 against 581 bytes. The shipped key search jumps
// straight to the "not a key" block when it runs out of keys, where ours sets
// -1 and tests it (or $-1; cmp), and the outer loop's latch sits before the
// key-found blocks. Tried with no luck: iKey = -1 and a separate counter, an
// inline or __forceinline FindKey returning -1, a goto out of the loop,
// "iKey == N -> -1", and the found branch written with continue.
void CCommandStringDlg::UpdateControls()
{
	bool bFound[NUM_COMMANDKEYS];
	char szKey[256];
	char szToken[256];
	char *pTokens[256];
	char szString[2000];
	int nTokens, i, iKey;
	int j, k;
	char *pTok, *pValue;

	if(!g_bUpdateControls)
	{
		g_bUpdateControls = true;
		return;
	}

	for(j=0; j < NUM_COMMANDKEYS; j++)
	{
		g_bUpdateCommandString = false;
		if(g_CommandKeys[j].m_bCheck)
			CheckDlgButton(g_CommandKeys[j].m_ID, 0);
		else
			SetDlgItemText(g_CommandKeys[j].m_ID, "");
	}

	memset(bFound, 0, sizeof(bFound));
	memset(pTokens, 0, sizeof(pTokens));
	nTokens = 0;

	m_CommandString.GetWindowText(szString, 2000);

	pTok = strtok(szString, ";");
	while(pTok)
	{
		pTokens[nTokens++] = pTok;
		pTok = strtok(NULL, ";");
	}

	for(i=0; i < nTokens; i++)
	{
		strcpy(szToken, pTokens[i]);
		pTok = strtok(szToken, " ");
		if(!pTok)
			continue;

		strcpy(szKey, pTok);

		for(iKey=0; ; iKey++)
		{
			if(iKey >= NUM_COMMANDKEYS)
			{
				iKey = -1;
				break;
			}

			if(!stricmp(szKey, g_CommandKeys[iKey].m_Name))
				break;
		}

		if(iKey == -1)
		{
			// Not a key: it goes to the extra commands.
			GetDlgItemText(IDC_CMDSTR_EXTRAPARAMS, szKey, 256);
			strcat(szKey, pTokens[i]);
			strcat(szKey, "; ");
			g_bUpdateCommandString = false;
			SetDlgItemText(IDC_CMDSTR_EXTRAPARAMS, szKey);
		}
		else if(g_CommandKeys[iKey].m_bCheck)
		{
			if(!bFound[iKey])
			{
				g_bUpdateCommandString = false;
				CheckDlgButton(g_CommandKeys[iKey].m_ID, 1);
				bFound[iKey] = true;
			}
		}
		else
		{
			for(;;)
			{
				pValue = strtok(NULL, " ");
				if(!pValue)
					break;

				for(k=0; k < NUM_COMMANDKEYS; k++)
				{
					if(!stricmp(szKey, g_CommandKeys[k].m_Name) && !bFound[k])
					{
						g_bUpdateCommandString = false;
						SetDlgItemText(g_CommandKeys[k].m_ID, pValue);
						bFound[k] = true;
					}
				}
			}
		}
	}
}

// 0x402076 (every helper control's notification, folded)
void CCommandStringDlg::OnChangeHelperControl()
{
	UpdateCommandString();
}

// 0x40207b
void CCommandStringDlg::OnChangeCommandString()
{
	UpdateControls();
}
