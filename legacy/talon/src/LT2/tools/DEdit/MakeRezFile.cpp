// MakeRezFile.cpp - the 2001 rez file writer (BuildRezFile), written from
// the image (0x4310bc..0x431464): a directory tree packed file by file,
// then its directory.  The module and the names but BuildRezFile's are ours
// (talon); BuildRezFile and RezBuildInfo are RezBuildDlg.h's.
//
// The file: a dword, the directory's offset, written over the start of the
// first file's data (nothing reserves it: retail), the files' data, then
// the directory: for each entry the offset of the next (-1 ends a list),
// its type (1 a directory), its name's length (a word) and name, then a
// file's size, offset and time, or a directory's entries.

#include "bdefs.h"
#include <io.h>
#include "rezbuilddlg.h"

#define REZNODE_NAME_LEN	255

struct RezNode
{
	uint8		m_bDirectory;
	char		m_Name[REZNODE_NAME_LEN + 4];
	uint32		m_Time;			// 0x104
	uint32		m_Size;			// 0x108
	uint32		m_Offset;		// 0x10c
	RezNode		*m_pChildren;	// 0x110
	RezNode		*m_pNext;		// 0x114
};

static RezBuildInfo *g_pRezBuildInfo;	// 0x63c574


// 0x4310bc
static void FreeRezNode(RezNode *pNode)
{
	RezNode *pChild, *pNext;

	for(pChild=pNode->m_pChildren; pChild; pChild=pNext)
	{
		pNext = pChild->m_pNext;
		FreeRezNode(pChild);
	}

	free(pNode);
}

// 0x4310e9: packs a directory's files into pOutFile and returns its node.
static RezNode* AddRezDirectory(FILE *pOutFile, const char *pDir, const char *pName)
{
	RezNode *pDirNode, *pNode;
	char path[256];
	_finddata_t fileData;
	long hFind;
	FILE *fp;
	void *pData;

	pDirNode = (RezNode*)malloc(sizeof(RezNode));
	memset(pDirNode, 0, sizeof(RezNode));
	pDirNode->m_bDirectory = 1;
	strncpy(pDirNode->m_Name, pName, REZNODE_NAME_LEN);

	sprintf(path, "%s\\*.*", pDir);
	hFind = _findfirst(path, &fileData);
	if(hFind != -1)
	{
		do
		{
			if(fileData.name[0] == '.')
				continue;

			if(fileData.attrib & _A_SUBDIR)
			{
				sprintf(path, "%s\\%s", pDir, fileData.name);
				pNode = AddRezDirectory(pOutFile, path, fileData.name);
				pNode->m_pNext = pDirNode->m_pChildren;
				pDirNode->m_pChildren = pNode;
			}
			else
			{
				sprintf(path, "%s\\%s", pDir, fileData.name);
				if(g_pRezBuildInfo->m_FileCB)
					g_pRezBuildInfo->m_FileCB(path);

				fp = fopen(path, "rb");
				if(fp)
				{
					pNode = (RezNode*)malloc(sizeof(RezNode));
					memset(pNode, 0, sizeof(RezNode));
					pNode->m_bDirectory = 0;
					pNode->m_Offset = ftell(pOutFile);
					pNode->m_Size = fileData.size;
					pNode->m_Time = fileData.time_write;
					strncpy(pNode->m_Name, fileData.name, REZNODE_NAME_LEN);
					pNode->m_pNext = pDirNode->m_pChildren;
					pDirNode->m_pChildren = pNode;

					pData = malloc(fileData.size);
					fread(pData, fileData.size, 1, fp);
					fwrite(pData, fileData.size, 1, pOutFile);
					free(pData);
					fclose(fp);
				}
				else if(g_pRezBuildInfo->m_MessageCB)
				{
					sprintf(path, "Couldn't open %s.", fileData.name);
					g_pRezBuildInfo->m_MessageCB(path);
				}
			}
		}
		while(_findnext(hFind, &fileData) != -1);
	}

	_findclose(hFind);
	return pDirNode;
}

// 0x431371: writes a directory's entries.
static void WriteRezDirectory(FILE *pFile, RezNode *pDir)
{
	RezNode *pNode;
	uint32 startPos, nextPos, endPos;
	uint16 nameLen;

	// while(1), not for(;;): cl 9044 peels for(;;)'s first iteration
	pNode = pDir->m_pChildren;
	while(1)
	{
		startPos = ftell(pFile);
		nextPos = (uint32)-1;
		fwrite(&nextPos, sizeof(nextPos), 1, pFile);

		if(!pNode)
			return;

		fwrite(&pNode->m_bDirectory, 1, 1, pFile);
		nameLen = (uint16)strlen(pNode->m_Name);
		fwrite(&nameLen, sizeof(nameLen), 1, pFile);
		fwrite(pNode->m_Name, nameLen, 1, pFile);

		if(!pNode->m_bDirectory)
		{
			fwrite(&pNode->m_Size, sizeof(pNode->m_Size), 1, pFile);
			fwrite(&pNode->m_Offset, sizeof(pNode->m_Offset), 1, pFile);
			fwrite(&pNode->m_Time, sizeof(pNode->m_Time), 1, pFile);
		}
		else
		{
			WriteRezDirectory(pFile, pNode);
		}

		endPos = ftell(pFile);
		fseek(pFile, startPos, SEEK_SET);
		fwrite(&endPos, sizeof(endPos), 1, pFile);
		fseek(pFile, endPos, SEEK_SET);

		pNode = pNode->m_pNext;
	}
}

// 0x4312f2: returns 0, or 1 if the rez file can't be made.
int BuildRezFile(RezBuildInfo *pInfo)
{
	FILE *pFile;
	RezNode *pRoot;
	uint32 dirPos;

	g_pRezBuildInfo = pInfo;

	pFile = fopen(pInfo->m_pRezFile, "wb");
	if(!pFile)
		return 1;

	pRoot = AddRezDirectory(pFile, pInfo->m_pDir, "");

	dirPos = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	fwrite(&dirPos, sizeof(dirPos), 1, pFile);
	fseek(pFile, dirPos, SEEK_SET);

	WriteRezDirectory(pFile, pRoot);
	FreeRezNode(pRoot);
	fclose(pFile);
	return 0;
}
