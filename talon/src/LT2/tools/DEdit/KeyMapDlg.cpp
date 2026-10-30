// KeyMapDlg.cpp - the 2001 Key Mappings dialog, written from the image
// (0x42ce61..0x42d25c); its table is .data 0x4f8418.  The names are ours
// (talon).

#include "bdefs.h"
#include "resource.h"
#include "keymapdlg.h"

// The list control (DEdit.rc's dialog 199).
#define IDC_KEYMAP_LIST		1088

#define NUM_KEYMAP_KEYS		2
#define NUM_KEYMAPS			40

// One key mapping: up to two key strings, a key character and up to two
// more key strings, then the mode, the view and the description.
struct KeyMapping
{
	UINT	m_KeyPrefix[NUM_KEYMAP_KEYS];
	char	m_Key;
	UINT	m_KeySuffix[NUM_KEYMAP_KEYS];
	UINT	m_Mode;
	UINT	m_View;
	UINT	m_Description;
};

static KeyMapping g_KeyMappings[NUM_KEYMAPS] =
{
	{ {IDS_SPACEBAR, 0}, 0, {0, 0}, IDS_BRUSHGEOMETRY, IDS_ALL, IDS_ADDPOINTDESC },
	{ {IDS_CTRLPLUS, IDS_SPACEBAR}, 0, {0, 0}, IDS_GEOMETRY, IDS_ALL, IDS_SNAPADDPOINTDESC },
	{ {0, 0}, 'I', {IDS_PLUSMOUSE, 0}, IDS_ALL, IDS_PERSPECTIVE, IDS_XZMOVEDESC },
	{ {0, 0}, 'O', {IDS_PLUSMOUSE, 0}, IDS_ALL, IDS_PERSPECTIVE, IDS_ORIENTATIONDESC },
	{ {0, 0}, 'I', {IDS_PLUSMOUSE, 0}, IDS_ALL, IDS_PARALLEL, IDS_PLANEGRIDDESC },
	{ {0, 0}, 'O', {IDS_PLUSMOUSE, 0}, IDS_ALL, IDS_PARALLEL, IDS_ZOOMDESC },
	{ {0, 0}, 'P', {0, 0}, IDS_GEOMETRY, IDS_PERSPECTIVE, IDS_FOCUSGREENDESC },
	{ {0, 0}, 'T', {IDS_PLUSLMB, 0}, IDS_BRUSH, IDS_ALL, IDS_TAGBRUSHDESC },
	{ {0, 0}, 'A', {IDS_PLUSLMB, 0}, IDS_BRUSH, IDS_ALL, IDS_APPLYTEXTURETOBRUSH },
	{ {0, 0}, 'U', {0, 0}, IDS_ALL, IDS_ALL, IDS_UNTAGDESC },
	{ {0, 0}, 'G', {0, 0}, IDS_GEOMETRY, IDS_ALL, IDS_PLACEGRIDONPOLYDESC },
	{ {IDS_890, 0}, 0, {0, 0}, IDS_ALL, IDS_PERSPECTIVE, IDS_PLACEGRIDONAXISDESC },
	{ {0, 0}, 'B', {IDS_PLUSLMB, 0}, IDS_ALL, IDS_ALL, IDS_TAGRECTDESC },
	{ {0, 0}, 'E', {IDS_PLUSRMB, 0}, IDS_GEOMETRY, IDS_ALL, IDS_EXTRUDEPOLYDESC },
	{ {0, 0}, 'Q', {IDS_PLUSRMB, 0}, IDS_GEOMETRY, IDS_ALL, IDS_SCALEPOLYDESC },
	{ {0, 0}, 'R', {IDS_PLUSRMB, 0}, IDS_GEOMETRY, IDS_ALL, IDS_EXTRUDEREMOVEPOLYDESC },
	{ {0, 0}, 'W', {IDS_PLUSRMB, 0}, IDS_GEOMETRY, IDS_ALL, IDS_SCALEREMOVEPOLYDESC },
	{ {IDS_INSERT, 0}, 0, {0, 0}, IDS_GEOMETRY, IDS_PERSPECTIVE, IDS_INSERTVERTDESC },
	{ {IDS_DELETE, 0}, 0, {0, 0}, IDS_GEOMETRY, IDS_ALL, IDS_DELETEGREENVERTDESC },
	{ {IDS_END, 0}, 0, {0, 0}, IDS_GEOMETRY, IDS_ALL, IDS_DELETEREDPOLYDESC },
	{ {IDS_PLUSMINUS, 0}, 0, {0, 0}, IDS_ALL, IDS_ALL, IDS_GRIDSPACINGDESC },
	{ {0, 0}, 'Z', {IDS_PLUSMOUSE, 0}, IDS_ALL, IDS_ALL, IDS_RESIZEGRIDDESC },
	{ {0, 0}, 'F', {0, 0}, IDS_GEOMETRY, IDS_ALL, IDS_FLIPNORMALDESC },
	{ {0, 0}, 'C', {IDS_PLUSMOUSE, 0}, IDS_ALL, IDS_PERSPECTIVE, IDS_MOVEFARCLIPDESC },
	{ {0, 0}, 'H', {0, 0}, IDS_ALL, IDS_ALL, IDS_CARVEDESC },
	{ {IDS_CTRLPLUS, 0}, 'B', {0, 0}, IDS_ALL, IDS_ALL, IDS_BRUSHMODEDESC },
	{ {IDS_CTRLPLUS, 0}, 'G', {0, 0}, IDS_ALL, IDS_ALL, IDS_GEOMETRYMODEDESC },
	{ {IDS_CTRLPLUS, 0}, 'H', {0, 0}, IDS_ALL, IDS_ALL, IDS_OBJECTMODEDESC },
	{ {IDS_CTRLPLUSALTPLUS, 0}, 'M', {0, 0}, IDS_ALL, IDS_ALL, IDS_MODECYCLEDESC },
	{ {IDS_CTRLPLUS, 0}, 'A', {0, 0}, IDS_ALL, IDS_ALL, IDS_APPLYCOLORDESC },
	{ {IDS_CTRLPLUS, 0}, 'T', {0, 0}, IDS_ALL, IDS_ALL, IDS_APPLYTEXTUREBRUSHDESC },
	{ {IDS_CTRLPLUS, 0}, 'Z', {0, 0}, IDS_ALL, IDS_ALL, IDS_UNDODESC },
	{ {IDS_CTRLPLUS, 0}, 'Y', {0, 0}, IDS_ALL, IDS_ALL, IDS_REDODESC },
	{ {IDS_F6SHIFTF6, 0}, 0, {0, 0}, IDS_ALL, IDS_ALL, IDS_NEXTPREVPANEDESC },
	{ {0, 0}, 'X', {IDS_PLUSMOUSE, 0}, IDS_ALL, IDS_ALL, IDS_MOVEMARKERDESC },
	{ {0, 0}, 'M', {IDS_PLUSLMB, IDS_PLUSMOUSE}, IDS_BRUSHOBJECT, IDS_ALL, IDS_MOVEBRUSHOBJECTDESC },
	{ {0, 0}, 'M', {IDS_PLUSLMB, IDS_PLUSMOUSE}, IDS_GEOMETRY, IDS_ALL, IDS_MOVEGREENVERTDESC },
	{ {0, 0}, 'S', {IDS_PLUSLMB, IDS_PLUSMOUSE}, IDS_GEOMETRY, IDS_ALL, IDS_MOVETAGVERTSDESC },
	{ {0, 0}, 'K', {0, 0}, IDS_ALL, IDS_ALL, IDS_INITTEXTURESDESC },
	{ {0, 0}, 'Y', {0, 0}, IDS_GEOMETRY, IDS_ALL, IDS_TAGVERTREDPOLYDESC },
};

// 0x63c464: set by the constructor, read nowhere in the image's code.
static CListCtrl *g_pKeyList;


CKeyMapDlg::CKeyMapDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CKeyMapDlg::IDD, pParent)
{
	g_pKeyList = &m_KeyList;
}

void CKeyMapDlg::DoDataExchange(CDataExchange* pDX)
{
	DDX_Control(pDX, IDC_KEYMAP_LIST, m_KeyList);
}

BEGIN_MESSAGE_MAP(CKeyMapDlg, CDialog)
END_MESSAGE_MAP()


// 0x42cee9
static void GetKeyMapStrings(DWORD iMapping, CString &key, CString &mode, CString &view, CString &description)
{
	CString str;
	int i;

	key = "";

	for(i=0; i < NUM_KEYMAP_KEYS && g_KeyMappings[iMapping].m_KeyPrefix[i]; i++)
	{
		str.LoadString(g_KeyMappings[iMapping].m_KeyPrefix[i]);
		key += str;
	}

	if(g_KeyMappings[iMapping].m_Key)
		key += g_KeyMappings[iMapping].m_Key;

	for(i=0; i < NUM_KEYMAP_KEYS && g_KeyMappings[iMapping].m_KeySuffix[i]; i++)
	{
		str.LoadString(g_KeyMappings[iMapping].m_KeySuffix[i]);
		key += str;
	}

	mode.LoadString(g_KeyMappings[iMapping].m_Mode);
	view.LoadString(g_KeyMappings[iMapping].m_View);
	description.LoadString(g_KeyMappings[iMapping].m_Description);
}

// 0x42cfc5: sorts by the column lParamSort.
static int CALLBACK CompareKeyMappings(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	CString strings1[4], strings2[4];

	GetKeyMapStrings(lParam1, strings1[0], strings1[1], strings1[2], strings1[3]);
	GetKeyMapStrings(lParam2, strings2[0], strings2[1], strings2[2], strings2[3]);
	return strings1[lParamSort].Compare(strings2[lParamSort]);
}

// 0x42d081
BOOL CKeyMapDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	CString str;
	CString key, mode, view, description;
	CRect rect;
	DWORD i;
	int iItem;

	m_KeyList.GetClientRect(&rect);

	str.LoadString(IDS_KEYS);
	m_KeyList.InsertColumn(0, str);
	str.LoadString(IDS_EDIT_MODE);
	m_KeyList.InsertColumn(1, str);
	str.LoadString(IDS_VIEW);
	m_KeyList.InsertColumn(2, str);
	str.LoadString(IDS_DESCRIPTION);
	m_KeyList.InsertColumn(3, str);

	for(i=0; i < NUM_KEYMAPS; i++)
	{
		GetKeyMapStrings(i, key, mode, view, description);

		iItem = m_KeyList.InsertItem(LVIF_TEXT, 0, key, 0, 0, 0, 0);
		if(iItem != -1)
		{
			m_KeyList.SetItem(iItem, 0, LVIF_PARAM, NULL, 0, 0, 0, i);
			m_KeyList.SetItemText(iItem, 1, mode);
			m_KeyList.SetItemText(iItem, 2, view);
			m_KeyList.SetItemText(iItem, 3, description);
		}
	}

	m_KeyList.SetColumnWidth(0, LVSCW_AUTOSIZE);
	m_KeyList.SetColumnWidth(1, LVSCW_AUTOSIZE);
	m_KeyList.SetColumnWidth(2, LVSCW_AUTOSIZE);
	m_KeyList.SetColumnWidth(3, LVSCW_AUTOSIZE);

	m_KeyList.SortItems(CompareKeyMappings, 1);

	return TRUE;
}
