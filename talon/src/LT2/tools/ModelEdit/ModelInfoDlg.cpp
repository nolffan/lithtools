// ModelInfoDlg.cpp : implementation file
//

#include "precompile.h"
#include "modeledit.h"
#include "modelinfodlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModelInfoDlg dialog


CModelInfoDlg::CModelInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CModelInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CModelInfoDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CModelInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CModelInfoDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CModelInfoDlg, CDialog)
	//{{AFX_MSG_MAP(CModelInfoDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModelInfoDlg message handlers


void CModelInfoDlg::DrawToTextThing(char *pStr, ...)
{
	CWnd *pWnd = GetDlgItem(IDC_MODELINFOTEXT);
	va_list marker;
	char msg[256];
	int nLen;

	if(pWnd)
	{
		va_start(marker, pStr);
		vsprintf(msg, pStr, marker);
		va_end(marker);

		strcat(msg, "\r\n");
		nLen = pWnd->SendMessage(EM_GETLIMITTEXT, 0, 0);
		pWnd->SendMessage(EM_SETSEL, nLen, nLen);
		pWnd->SendMessage(EM_REPLACESEL, FALSE, (LPARAM)msg);
	}
}


// 0x42185c: the geometry memory of one LOD of every piece.
DWORD CModelInfoDlg::ShowPieceMem(DWORD iLOD)
{
	uint32 i, memPieces, memVerts, memTris;
	PieceLOD *pLOD;


	memPieces = m_pModel->NumPieces() * sizeof(ModelPiece);
	
	memTris = 0;
	memVerts = 0;
	for(i=0; i < m_pModel->NumPieces(); i++)
	{
		pLOD = m_pModel->GetPiece(i)->GetLOD((uint32)iLOD);
		
		memTris += pLOD->m_Tris.GetSize() * sizeof(ModelTri);
		memVerts += pLOD->m_Verts.GetSize() * sizeof(ModelVert);
	}
	
	DrawToTextThing("Pieces: %d", memPieces);
	DrawToTextThing("Verts: %d", memVerts);
	DrawToTextThing("Tris: %d", memTris);
	
	return memPieces+memVerts+memTris;
}

// 0x4218ec: the most weights any vertex of the base LODs has.
DWORD CModelInfoDlg::CalcMaxWeights()
{
	DWORD i, j, nMax;
	ModelPiece *pPiece;

	nMax = 0;
	for(i=0; i < m_pModel->NumPieces(); i++)
	{
		pPiece = m_pModel->GetPiece(i);
		for(j=0; j < pPiece->m_Verts; j++)
		{
			if(pPiece->m_Verts[j].m_nWeights > nMax)
				nMax = pPiece->m_Verts[j].m_nWeights;
		}
	}

	return nMax;
}

// 0x421926: the number of vertex weights in the base LODs.
DWORD CModelInfoDlg::CalcNumWeights()
{
	DWORD i, j, total;
	ModelPiece *pPiece;

	total = 0;
	for(i=0; i < m_pModel->NumPieces(); i++)
	{
		pPiece = m_pModel->GetPiece(i);
		for(j=0; j < pPiece->m_Verts; j++)
		{
			total += pPiece->m_Verts[j].m_nWeights;
		}
	}

	return total;
}

// 2001's (0x42195c).
BOOL CModelInfoDlg::OnInitDialog() 
{
	DWORD i, total;
	ModelNode *pNode;
	ModelPiece *pPiece;
	DWORD totalMem;


	CDialog::OnInitDialog();

	DrawToTextThing("File Version : %d", m_pModel->GetFileVersion());
	DrawToTextThing(" ");
	DrawToTextThing("Number of animations: %d", m_pModel->m_Anims.GetSize());
	DrawToTextThing("------------------------------");
	total = 0;
	for(i=0; i < m_pModel->m_Anims; i++)
	{
		DrawToTextThing("   %s", m_pModel->GetAnim(i)->GetName());
		total += m_pModel->GetAnim(i)->m_KeyFrames.GetSize();
	}
	DrawToTextThing("------------------------------");
	DrawToTextThing("Average keyframes per animation: %d", total / m_pModel->m_Anims.GetSize());

	DrawToTextThing("Number of triangles: %d", m_pModel->CalcNumTris(0));
	DrawToTextThing("Number of vertices: %d", m_pModel->CalcNumVerts());
	DrawToTextThing("Number of nodes: %d", m_pModel->NumNodes());
	DrawToTextThing("Number of vertex weights: %d", CalcNumWeights());
	DrawToTextThing("Average weights per vertex: %.3f", (float)CalcNumWeights() / m_pModel->CalcNumVerts());
	DrawToTextThing("Maximum weights per vertex: %d", CalcMaxWeights());

	DrawToTextThing("");
	DrawToTextThing("Nodes ----------------");
	for(i=0; i < m_pModel->m_FlatNodeList; i++)
	{
		pNode = m_pModel->m_FlatNodeList[i];
		DrawToTextThing("%s (%s)", pNode->GetName(), (pNode->m_Flags & MNODE_REMOVABLE) ? "removable" : "normal");
	}

	DrawToTextThing("");
	DrawToTextThing("Pieces ----------------");
	for(i=0; i < m_pModel->NumPieces(); i++)
	{
		pPiece = m_pModel->GetPiece(i);
		DrawToTextThing("%s, texture %d", pPiece->GetName(), pPiece->m_iTexture);
	}

	DrawToTextThing("");
	DrawToTextThing("Geometry Memory ----------------");
	DrawToTextThing("LOD 0 -------");
	totalMem = ShowPieceMem(0);
	for(i=0; i < m_pModel->m_LODs; i++)
	{
		DrawToTextThing("LOD %d -------", i+1);
		// talon: retail bug, kept: shows LOD i under the heading of LOD i+1
		// (0x421bad), so LOD 0 is counted twice and the last LOD never.
		totalMem += ShowPieceMem(i);
	}

	DrawToTextThing("");
	DrawToTextThing("Weights: %d", m_pModel->m_VertexWeights.GetSize() * sizeof(NewVertexWeight));
	DrawToTextThing("TOTAL: %d", totalMem);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
