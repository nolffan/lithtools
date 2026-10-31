// talon: the 1998 RezMgr of lithrez 1.00 (0x40ad50..0x40dc04), written from
// the image with Build 69's rezmgr.cpp as a guide; this is the full version
// (keys, descriptions, items by ID and the compaction on close).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "rezmgr.h"




//***************************************************************************************************
// file format data structures

#pragma pack(1)
struct FileMainHeaderStruct {
  char CR1;
  char LF1;
  char FileType[RezMgrUserTitleSize];
  char CR2;
  char LF2;
  char UserTitle[RezMgrUserTitleSize];
  char CR3;
  char LF3;
  char EOF1;
  DWORD FileFormatVersion; 	// the file format version number only 1 is possible here right now
  DWORD RootDirPos; 		// Position of the root directory structure in the file
  DWORD RootDirSize;        // Size of root directory
  DWORD RootDirTime;        // Time Root dir was last updated
  DWORD NextWritePos;       // Position of first directory in the file
  DWORD Time; 				// Time resource file was last updated
  DWORD LargestKeyAry;		// Size of the largest key array in the resource file
  DWORD LargestDirNameSize; // Size of the largest directory name in the resource file (including 0 terminator)
  DWORD LargestRezNameSize;	// Size of the largest resource name in the resource file (including 0 terminator)
  DWORD LargestCommentSize;	// Size of the largest comment in the resource file (including 0 terminator)
  BYTE  IsSorted;           // If 0 then data is not sorted if 1 then it is sorted
};

enum FileDirEntryType {
  ResourceEntry = 0,
  DirectoryEntry = 1
};

struct FileDirEntryDirHeader {
  DWORD Pos; 					// File positon of dir entry
  DWORD Size;					// Size of directory data
  DWORD Time;					// Last time anything in directory was modified
//  char Name[];				// Name of this directory
};

struct FileDirEntryRezHeader {
  DWORD Pos; 					// File positon of dir entry
  DWORD Size;					// Size of directory data
  DWORD Time;					// Last time this resource was modified
  DWORD ID;                     // Resource ID number
  DWORD Type;					// Type of resource this is
  DWORD NumKeys;				// The number of keys to read in for this resource
//  char Name[];				// The name of this resource
//  char Comment[];             // The comment data for this resource
//  DWORD Keys[];				// The key values for this resource
};

struct FileDirEntryHeader {
  DWORD Type;
  union {
    FileDirEntryRezHeader Rez;
	FileDirEntryDirHeader Dir;
  };
};
#pragma pack()

//***************************************************************************************************
// CRezItm implementation

//---------------------------------------------------------------------------------------------------
CRezItm::CRezItm()
{
  m_pRezFile = NULL;
  m_pParentDir = NULL;
  m_sName = NULL;
  m_heID.SetRezItm(this);
  m_heName.SetRezItm(this);
};

//---------------------------------------------------------------------------------------------------
void CRezItm::InitRezItm(CRezDir* pParentDir, REZNAME sName, REZID nID, CRezTyp* pType, REZDESC sDesc,
                    REZSIZE nSize, DWORD nFilePos, REZTIME nTime, DWORD nNumKeys, REZKEYVAL* pKeyAry,
				    CBaseRezFile* pRezFile) {
  ASSERT(pParentDir != NULL);
  m_pRezFile = pRezFile;
  m_pParentDir = pParentDir;
  if (sName == NULL) m_sName = NULL;
  else {
    m_sName = new char [strlen(sName)+1];
    ASSERT(m_sName != NULL);
    if (m_sName != NULL) strcpy(m_sName,sName);
  }

  m_nID = nID;
  m_pType = pType;

  if (sDesc == NULL) m_sDescription = NULL;
  else {
    m_sDescription = new char [strlen(sDesc)+1];
    ASSERT(m_sDescription != NULL);
    if (m_sDescription != NULL) strcpy(m_sDescription,sDesc);
  }

  m_nSize = nSize;
  m_nFilePos = nFilePos;
  m_nTime = nTime;

  m_nNumKeys = nNumKeys;
  if (pKeyAry == NULL) m_pKeyAry = NULL;
  else {
    m_pKeyAry = new REZKEYVAL[nNumKeys];
    ASSERT(m_pKeyAry != NULL);
    if (m_pKeyAry != NULL) {
      for (unsigned int i = 0; i < nNumKeys; i++) m_pKeyAry[i] = pKeyAry[i];
    }
  }

  m_pData = NULL;
  m_nCurPos = 0;

  m_heID.SetRezItm(this);
  m_heName.SetRezItm(this);
};

//---------------------------------------------------------------------------------------------------
void CRezItm::TermRezItm() {
  // free up all members
  if (m_sName != NULL) delete [] m_sName;
  if (m_sDescription != NULL) delete [] m_sDescription;

  if (m_pParentDir != NULL) {
    if (m_pParentDir->m_pMemBlock == NULL) {
      if (m_pData != NULL) delete [] m_pData;
    }
  }
  else if (m_pData != NULL) delete [] m_pData;

  if (m_pKeyAry != NULL) delete [] m_pKeyAry;

  // set members back to default settings
  m_sName = NULL;
  m_nID = 0;
  m_pType = NULL;
  m_sDescription = NULL;
  m_nTime = 0;
  m_nSize = 0;
  m_pData = NULL;
  m_nNumKeys = 0;
  m_pKeyAry = NULL;
  m_pParentDir = NULL;
  m_nFilePos = 0;
  m_nCurPos = 0;

  m_heID.SetRezItm(NULL);
  m_heName.SetRezItm(NULL);
};

//---------------------------------------------------------------------------------------------------
REZTYPE	CRezItm::GetType() { 
  ASSERT(m_pType != NULL); 
  return m_pType->GetType(); 
};

//---------------------------------------------------------------------------------------------------
BYTE* CRezItm::Load() {
  ASSERT(m_pParentDir != NULL);
  ASSERT(m_pRezFile != NULL);

  // check if the whole directory is in memory already
  if (m_pParentDir->m_pMemBlock != NULL) {
    return m_pParentDir->m_pMemBlock+m_nFilePos-m_pParentDir->m_nItemsPos;
  }

  // check if the data is already in memory
  if (m_pData != NULL) return m_pData;

  // allocate memory for the data
  if (m_nSize == 0) return NULL;
  m_pData = new BYTE[m_nSize];
  ASSERT(m_pData != NULL);
  if (m_pData == NULL) return NULL;

  // load in the data from disk
  ASSERT(m_pParentDir->m_pRezMgr != NULL);
  if (m_pRezFile->Read(m_nFilePos,0,m_nSize,m_pData) != m_nSize) {
    delete [] m_pData;
    m_pData = NULL;
  }

  return m_pData;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezItm::UnLoad() {
  if (m_pData != NULL) {
    delete [] m_pData;
    m_pData = NULL;
  } 
  return TRUE;
};

//---------------------------------------------------------------------------------------------------
BYTE* CRezItm::Create(DWORD Size) {
  ASSERT(m_pParentDir != NULL);
  ASSERT(m_pParentDir->m_pRezMgr != NULL);
  ASSERT(m_pParentDir->m_pRezMgr->m_bReadOnly != TRUE); // You can't do this if the resource was open read only!

  // store the size the resource used to be
  DWORD nOldSize = m_nSize;

  // make sure old memory is removed
  UnLoad();

  // make sure parent does not have resources in memory (if so remove them)
  ASSERT(m_pParentDir != NULL);
  if (m_pParentDir->m_pMemBlock != NULL) m_pParentDir->UnLoad();

  // allocate the new memory and set the new size member
  m_nSize = Size;
  m_pData = new BYTE[m_nSize];
  ASSERT(m_pData != NULL);

  // mark this resource as not existing in the resource file yet
  m_nFilePos = 0;

  // update the directory items size in the parent directory
  m_pParentDir->m_nItemsSize += m_nSize;
  m_pParentDir->m_nItemsSize -= nOldSize;

  // mark data as not sorted
  m_pParentDir->m_pRezMgr->m_bIsSorted = FALSE;

  // update the timestamp for this item
  MarkCurTime();
  
  return m_pData;
};

//---------------------------------------------------------------------------------------------------
void CRezItm::SetKey(REZKEYINDEX KeyIndex, REZKEYVAL KeyVal) {
  // expand the key array if necessary
  if (KeyIndex >= m_nNumKeys) {
    DWORD nOldNumKeys = m_nNumKeys;
    m_nNumKeys = KeyIndex+1;
    REZKEYVAL* pNewKeyAry = new REZKEYVAL[m_nNumKeys];
    ASSERT(pNewKeyAry != NULL);
    if (m_pKeyAry != NULL) {
      for (DWORD i = 0; i < nOldNumKeys; i++) pNewKeyAry[i] = m_pKeyAry[i];
      delete [] m_pKeyAry;
    }
    m_pKeyAry = pNewKeyAry;

    // update largest variables
    if (m_pParentDir->m_pRezMgr->m_nLargestKeyAry <= KeyIndex) m_pParentDir->m_pRezMgr->m_nLargestKeyAry = KeyIndex+1;
  }

  // set the key value
  m_pKeyAry[KeyIndex] = KeyVal;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezItm::SetDesc(char* NewDesc) {
  // remove the old description
  if (m_sDescription != NULL) delete [] m_sDescription;

  // allocate and copy the new one
  DWORD nSize = 0;
  if (NewDesc != NULL) nSize = strlen(NewDesc)+1;
  if (NewDesc != NULL) m_sDescription = new char[nSize];
  if (NewDesc != NULL) strcpy(m_sDescription,NewDesc);
  else m_sDescription = NULL;

  // update largest variables (talon: sic, the key array's)
  if (m_pParentDir->m_pRezMgr->m_nLargestCommentSize < nSize) m_pParentDir->m_pRezMgr->m_nLargestKeyAry = nSize;

  return TRUE;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezItm::Save() {
  ASSERT(m_pData != NULL);
  ASSERT(m_pRezFile != NULL);
  ASSERT(m_pParentDir != NULL);
  ASSERT(m_pParentDir->m_pRezMgr != NULL);
  ASSERT(m_pParentDir->m_pRezMgr->m_bReadOnly != TRUE); // You can't do this if the resource was open read only!

  // we don't have to do anything if the size is 0
  if (m_nSize <= 0) return TRUE;

  // if this resource has never been saved
  if (m_nFilePos == 0) {

    // mark resource file as not sorted
    m_pParentDir->m_pRezMgr->m_bIsSorted = FALSE;

    // write out the data
    if (m_pRezFile->Write(m_pParentDir->m_pRezMgr->m_nNextWritePos,0,m_nSize,m_pData) != m_nSize) {
      ASSERT(FALSE); // failed to write out data
      return FALSE;
    }

    // set the position we wrote the data to
    m_nFilePos = m_pParentDir->m_pRezMgr->m_nNextWritePos;

    // update start of dir data position
    m_pParentDir->m_pRezMgr->m_nNextWritePos += m_nSize;
  }

  // if this resource has been saved before (just write it where it belongs)
  else {

    // write out the data
    if (m_pRezFile->Write(m_nFilePos,0,m_nSize,m_pData) != m_nSize) {
      ASSERT(FALSE); // failed to write out data
      return FALSE;
    }
  }

  // update the timestamp for this item
  MarkCurTime();

  return TRUE;
};

//---------------------------------------------------------------------------------------------------
void CRezItm::MarkCurTime() {
  ASSERT(m_pParentDir != NULL);
  ASSERT(m_pParentDir->m_pRezMgr != NULL);

  // get the current time
  REZTIME nTime = m_pParentDir->m_pRezMgr->GetCurTime();

  // mark this item
  m_nTime = nTime;

  // mark this items directory
  m_pParentDir->m_nLastTimeModified = nTime;

  // mark the main rez file
  m_pParentDir->m_pRezMgr->m_nLastTimeModified = nTime;
};


//***************************************************************************************************
// CRezType implementation

//---------------------------------------------------------------------------------------------------
CRezTyp::CRezTyp(REZTYPE nType, CRezDir* pParentDir, unsigned int nByIDNumHashBins, unsigned int nByNameNumHashBins) : m_haID(nByIDNumHashBins) , m_haName(nByNameNumHashBins) {
  m_nType = nType;
  m_heType.SetRezTyp(this);
  m_pParentDir = pParentDir;
};                                    

//---------------------------------------------------------------------------------------------------
CRezTyp::CRezTyp(REZTYPE nType, CRezDir* pParentDir, unsigned int nByNameNumHashBins) : m_haID() , m_haName(nByNameNumHashBins) {
  m_nType = nType;
  m_heType.SetRezTyp(this);
  m_pParentDir = pParentDir;
};                                    

//---------------------------------------------------------------------------------------------------
CRezTyp::~CRezTyp() {
  // remove all of the items in the ByID hash table 
  // Don't delete objectes here, they are deleted in the ByName table below!
  if (m_pParentDir->m_pRezMgr->m_bItemByIDUsed)
  {
    CRezItmHashByID* pItm = m_haID.GetFirst();
    CRezItmHashByID* pDel;
    while (pItm != NULL) {
      pDel = pItm;
      pItm = pItm->Next();
      m_haID.Delete(pDel);
    }
  }

  // remove all of the items in the ByName hash table and delete the objects
  {
    CRezItmHashByName* pItm = m_haName.GetFirst();
    CRezItmHashByName* pDel;
    while (pItm != NULL) {
      pDel = pItm;
      pItm = pItm->Next();
      m_haName.Delete(pDel);
      ASSERT(pDel->GetRezItm() != NULL);
      pDel->GetRezItm()->TermRezItm();
      m_pParentDir->m_pRezMgr->DeAllocateRezItm(pDel->GetRezItm());
    }
  }

  m_nType = 0;
  m_heType.SetRezTyp(NULL);
};


//***************************************************************************************************
// CRezDir implementation

//---------------------------------------------------------------------------------------------------
CRezDir::CRezDir(CRezMgr* pRezMgr, CRezDir* pParentDir, REZDIRNAME szDirName, DWORD nDirPos, DWORD nDirSize, REZTIME nTime, unsigned int nDirNumHashBins, unsigned int nTypNumHashBins)
        : m_haDir(nDirNumHashBins), m_haTypes(nTypNumHashBins) {
  ASSERT(pRezMgr != NULL);
  ASSERT(szDirName != NULL);

  // allocate and copy name
  m_sDirName = new char[strlen(szDirName)+1];
  ASSERT(m_sDirName != NULL);
  if (m_sDirName != NULL) strcpy(m_sDirName,szDirName);

  // set valuse for other member functions
  m_nLastTimeModified = nTime;
  m_nDirSize = nDirSize;
  m_nDirPos = nDirPos;
  m_nItemsSize = 0;
  m_nItemsPos = 0;
  m_pMemBlock = NULL;
  m_pRezMgr = pRezMgr;
  m_pParentDir = pParentDir;
  m_heDir.SetRezDir(this);
};

//---------------------------------------------------------------------------------------------------
CRezDir::~CRezDir() {
  // remove all typ hash table contents and delete the objects
  {
    CRezTypeHash* pItm = m_haTypes.GetFirst();
    CRezTypeHash* pDel;
    while (pItm != NULL) {
      pDel = pItm;
      pItm = pItm->Next();
      m_haTypes.Delete(pDel);
      ASSERT(pDel->GetRezTyp() != NULL);
      delete pDel->GetRezTyp();
    }
  }

  // remove all dir hash table contents and delete the objects
  {
    CRezDirHash* pItm = m_haDir.GetFirst();
    CRezDirHash* pDel;
    while (pItm != NULL) {
      pDel = pItm;
      pItm = pItm->Next();
      m_haDir.Delete(pDel);
      ASSERT(pDel->GetRezDir() != NULL);
      delete pDel->GetRezDir();
    }
  }

  // remove simple member data
  if (m_sDirName != NULL) delete [] m_sDirName;
  if (m_pMemBlock != NULL) delete [] m_pMemBlock;

  // reset variables to default settings
  m_sDirName = NULL;
  m_nLastTimeModified = 0;
  m_nDirSize = 0;
  m_nDirPos = 0;
  m_nItemsSize = 0;
  m_nItemsPos = 0;
  m_pMemBlock = NULL;
  m_pRezMgr = NULL;
  m_pParentDir = NULL;
  m_heDir.SetRezDir(NULL);
};

//---------------------------------------------------------------------------------------------------
CRezItm* CRezDir::GetRez(REZNAME sRes, REZTYPE nType) {
  ASSERT(sRes != NULL);

  // get the Type for the resource
  CRezTyp* pTyp = m_haTypes.Find(nType);
  if (pTyp == NULL) return NULL;

  // get the item
  return pTyp->m_haName.Find(sRes,!GetParentMgr()->GetLowerCaseUsed());
};

//---------------------------------------------------------------------------------------------------
CRezItm* CRezDir::GetRez(REZID nID, REZTYPE nType) {
  // we can't do this if items are not stored by ID
  if (!m_pRezMgr->m_bItemByIDUsed) return NULL;

  // get the Type for the resource
  CRezTyp* pTyp = m_haTypes.Find(nType);
  if (pTyp == NULL) return NULL;

  // get the item
  return pTyp->m_haID.Find(nID);
};

//---------------------------------------------------------------------------------------------------
CRezItm* CRezDir::GetRezFromDosName(char* sDosName) {
  ASSERT(sDosName != NULL);
  char sExt[5];
  REZTYPE nType;

  // split the file name up into its parts
  char drive[_MAX_DRIVE+1];
  char dir[_MAX_DIR+1];
  char fname[_MAX_FNAME+1];
  char ext[_MAX_EXT+1];
  _splitpath(sDosName, drive, dir, fname, ext );

  // figure out the Type for this file
  if (strlen(ext) > 0) {
    strcpy(sExt,&ext[1]);
    strupr(sExt);
    nType = m_pRezMgr->StrToType(sExt);
  }
  else nType = 0;

  // call the normal GetRez function
  return GetRez(fname,nType);
};

//---------------------------------------------------------------------------------------------------
BOOL CRezDir::UnLoad(BOOL UnLoadAllSubDirs) {
  // delete this directory whole data if it exists
  if (m_pMemBlock != NULL) {
    delete [] m_pMemBlock;
    m_pMemBlock = NULL;
  }

  // if the memory is not stored globally then unload all items individually
  else {

    // go through all types in this directory 
    CRezTyp* pTyp = GetFirstType();
    while (pTyp != NULL) {

      // go through all items in this type and transfer them to the new dir
      CRezItm* pItm = GetFirstItem(pTyp);
      while (pItm != NULL) {
        pItm->UnLoad();
        pItm = GetNextItem(pItm);
      }

      pTyp = GetNextType(pTyp);
    }
  }

  // delete all sub directories data if we are supposed to
  if (UnLoadAllSubDirs) {
    CRezDirHash* pItm = m_haDir.GetFirst();
    while (pItm != NULL) {
      ASSERT(pItm->GetRezDir() != NULL);
      pItm->GetRezDir()->UnLoad(TRUE);
      pItm = pItm->Next();
    }
  }

  return TRUE;
};

//---------------------------------------------------------------------------------------------------
CRezDir* CRezDir::GetDir(REZCDIRNAME sDir) {
  ASSERT(sDir != NULL);
  if (sDir == NULL) return NULL;
  return m_haDir.Find(sDir,!GetParentMgr()->GetLowerCaseUsed());
};

//---------------------------------------------------------------------------------------------------
CRezDir* CRezDir::GetFirstSubDir() {
  CRezDirHash* pHash = m_haDir.GetFirst();
  if (pHash == NULL) return NULL;
  ASSERT(pHash->GetRezDir() != NULL);
  return pHash->GetRezDir();
};

//---------------------------------------------------------------------------------------------------
CRezDir* CRezDir::GetNextSubDir(CRezDir* pRezDir) {
  ASSERT(pRezDir != NULL);
  CRezDirHash* pHash = pRezDir->m_heDir.Next();
  if (pHash == NULL) return NULL;
  ASSERT(pHash->GetRezDir() != NULL);
  return pHash->GetRezDir();
};

//---------------------------------------------------------------------------------------------------
CRezTyp* CRezDir::GetFirstType() {
  CRezTypeHash* pHash = m_haTypes.GetFirst();
  if (pHash == NULL) return NULL;
  ASSERT(pHash->GetRezTyp() != NULL);
  return pHash->GetRezTyp();
};

//---------------------------------------------------------------------------------------------------
CRezTyp* CRezDir::GetNextType(CRezTyp* pRezType) {
  ASSERT(pRezType != NULL);
  CRezTypeHash* pHash = pRezType->m_heType.Next();
  if (pHash == NULL) return NULL;
  ASSERT(pHash->GetRezTyp() != NULL);
  return pHash->GetRezTyp();
};

//---------------------------------------------------------------------------------------------------
CRezItm* CRezDir::GetFirstItem(CRezTyp* pRezType) {
  ASSERT(pRezType != NULL);
  CRezItmHashByName* pHash = pRezType->m_haName.GetFirst();
  if (pHash == NULL) return NULL;
  return pHash->GetRezItm();
};

//---------------------------------------------------------------------------------------------------
CRezItm* CRezDir::GetNextItem(CRezItm* pRezItm) {
  ASSERT(pRezItm != NULL);
  CRezItmHashByName* pHash = pRezItm->m_heName.Next();
  if (pHash == NULL) return NULL;
  return pHash->GetRezItm();
};

//---------------------------------------------------------------------------------------------------
CRezDir* CRezDir::CreateDir(REZDIRNAME sDir) {
  ASSERT(sDir != NULL);
  ASSERT(m_pRezMgr != NULL);

  // make sure directory does not already exist
  CRezDir* pDir = m_haDir.Find(sDir,!GetParentMgr()->GetLowerCaseUsed());
  ASSERT(!(pDir != NULL));
  if (pDir != NULL) return NULL;

  // create new directory structure
  pDir = new CRezDir(m_pRezMgr, this, sDir, 0, 0, m_pRezMgr->GetCurTime(), m_pRezMgr->m_nDirNumHashBins, m_pRezMgr->m_nTypNumHashBins);
  ASSERT(pDir != NULL);
  if (pDir == NULL) return NULL;

  // add to this directories hash table
  m_haDir.Insert(&pDir->m_heDir);

  // update largest variables
  DWORD sNameLen = strlen(sDir);
  if (m_pRezMgr->m_nLargestDirNameSize <= sNameLen) m_pRezMgr->m_nLargestDirNameSize = sNameLen+1;

  return pDir;
};

//---------------------------------------------------------------------------------------------------
CRezItm* CRezDir::CreateRez(REZID nID, REZNAME sName, REZTYPE nType) {
  ASSERT(sName != NULL);
  ASSERT(m_pRezMgr != NULL);
  ASSERT(m_pRezMgr->m_bReadOnly == FALSE);  // You can't do this if the resource was open read only!

  // find or create the type for this resource
  CRezTyp* pTyp = GetOrMakeTyp(nType);
  ASSERT(pTyp != NULL);

  // make sure resource does not already exist in ID hash table
  CRezItm* pItm = NULL;
  if (m_pRezMgr->m_bItemByIDUsed) {
    pItm = pTyp->m_haID.Find(nID);
    if (pItm != NULL) return NULL;
  }

  // make sure resource does not already exist in Name hash table
  pItm = pTyp->m_haName.Find(sName,!GetParentMgr()->GetLowerCaseUsed());
  if (pItm != NULL) return NULL;

  // create new resource structure
  pItm = m_pRezMgr->AllocateRezItm();
  pItm->InitRezItm(this,sName,nID,pTyp,NULL,0,0,m_pRezMgr->GetCurTime(),0,NULL,m_pRezMgr->m_pPrimaryRezFile);
  ASSERT(pItm != NULL);
  if (pItm == NULL) return NULL;

  // add to the types hash tables
  if (m_pRezMgr->m_bItemByIDUsed) pTyp->m_haID.Insert(&pItm->m_heID);
  pTyp->m_haName.Insert(&pItm->m_heName);

  // update largest variables
  DWORD sNameLen = strlen(sName);
  if (m_pRezMgr->m_nLargestRezNameSize <= sNameLen) m_pRezMgr->m_nLargestRezNameSize = sNameLen+1;

  return pItm;
};

//---------------------------------------------------------------------------------------------------
CRezItm* CRezDir::CreateRezInternal(REZID nID, REZNAME sName, CRezTyp* pTyp, CBaseRezFile* pRezFile) {
  ASSERT(sName != NULL);
  ASSERT(m_pRezMgr != NULL);

  // create new resource structure
  CRezItm* pItm = m_pRezMgr->AllocateRezItm();
  ASSERT(pItm != NULL);
  if (pItm == NULL) return NULL; 
  pItm->InitRezItm(this,sName,nID,pTyp,NULL,0,0,m_pRezMgr->GetCurTime(),0,NULL,pRezFile);

  // add to the types hash tables
  if (m_pRezMgr->m_bItemByIDUsed) pTyp->m_haID.Insert(&pItm->m_heID);
  pTyp->m_haName.Insert(&pItm->m_heName);

  // update largest variables
  DWORD sNameLen = strlen(sName);
  if (m_pRezMgr->m_nLargestRezNameSize <= sNameLen) m_pRezMgr->m_nLargestRezNameSize = sNameLen+1;

  return pItm;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezDir::RemoveRezInternal(CRezTyp* pTyp, CRezItm* pItm) {
  ASSERT(pItm != NULL);

  // update the directory items size
  m_nItemsSize -= pItm->m_nSize;

  // remove from hash tables
  if (m_pRezMgr->m_bItemByIDUsed) pTyp->m_haID.Delete(&pItm->m_heID);
  pTyp->m_haName.Delete(&pItm->m_heName);

  // delete item
  pItm->TermRezItm();
  m_pRezMgr->DeAllocateRezItm(pItm);

  // mark data as not sorted
  m_pRezMgr->m_bIsSorted = FALSE;

  return TRUE;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezDir::ReadAllDirs(CBaseRezFile* pRezFile, DWORD Pos, DWORD Size, BOOL bOverwriteItems) {
  BOOL bRetFlag = TRUE;
  ASSERT(Pos > 0);

  // return if this is an empty directory
  if (Size <= 0) return TRUE;

  // clear out the DirPos variables in any currently existing directories so we don't try to read a dir we shouldn't
  {
    CRezDirHash* pDir = m_haDir.GetFirst();
    while (pDir != NULL) {
      CRezDir* pRezDir = pDir->GetRezDir();
      ASSERT(pRezDir != NULL);
      pRezDir->m_nDirPos = 0;
      pDir = pDir->Next();
    }
  }

  // read in this directory
  if (ReadDirBlock(pRezFile, Pos, Size, bOverwriteItems)) {

    // loop through all directorys in this directory and recursivly call this function
    CRezDirHash* pDir = m_haDir.GetFirst();
    while (pDir != NULL) {
      CRezDir* pRezDir = pDir->GetRezDir();
      ASSERT(pRezDir != NULL);
	  if (pRezDir->m_nDirPos != 0) {
        if (!pRezDir->ReadAllDirs(pRezFile, pRezDir->m_nDirPos, pRezDir->m_nDirSize, bOverwriteItems)) bRetFlag = FALSE;
      }
      pDir = pDir->Next();
    }
  }

  else bRetFlag = FALSE;

  return bRetFlag;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezDir::WriteAllDirs(CBaseRezFile* pRezFile, DWORD* Pos, DWORD* Size) {
  BOOL bRetFlag = TRUE;

  // first write out all directories contained in this directory
  CRezDirHash* pDir = m_haDir.GetFirst();
  while (pDir != NULL) {
    CRezDir* pRezDir = pDir->GetRezDir();
    ASSERT(pRezDir != NULL);
    if (!pRezDir->WriteAllDirs(pRezFile, &pRezDir->m_nDirPos,&pRezDir->m_nDirSize)) {
      bRetFlag = FALSE;
      break;
    }
    pDir = pDir->Next();
  }

  // write out this directory
  *Pos = m_pRezMgr->m_nNextWritePos;
  if (!WriteDirBlock(pRezFile, m_pRezMgr->m_nNextWritePos, Size)) bRetFlag = FALSE;
  else m_pRezMgr->m_nNextWritePos += *Size;

  return bRetFlag;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezDir::WriteDirBlock(CBaseRezFile* pRezFile, DWORD Pos, DWORD* Size) {
  DWORD nCurPos = Pos;
  char sZero = '\0';
  FileDirEntryHeader Hdr;

  // write out all of the directory entries
  Hdr.Type = DirectoryEntry;
  CRezDirHash* pDirHash = m_haDir.GetFirst();
  while (pDirHash != NULL) {
    CRezDir* pDir = pDirHash->GetRezDir();
    ASSERT(pDir != NULL);

    // fill out the header
    Hdr.Dir.Pos = pDir->m_nDirPos;
    Hdr.Dir.Size = pDir->m_nDirSize;
    Hdr.Dir.Time = pDir->m_nLastTimeModified;

    // write out the header
    nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Type);
    nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Dir.Pos);
    nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Dir.Size);
    nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Dir.Time);

    // write out the name
    if (pDir->m_sDirName == NULL) nCurPos += pRezFile->Write(nCurPos,0,1,&sZero);
    else nCurPos += pRezFile->Write(nCurPos,0,strlen(pDir->m_sDirName)+1,pDir->m_sDirName);

    pDirHash = pDirHash->Next();
  }

  // write out all of the resource entries
  Hdr.Type = ResourceEntry;
  CRezTypeHash* pTypHash = m_haTypes.GetFirst();
  while (pTypHash != NULL) {
    CRezItmHashByName* pItmHash = pTypHash->GetRezTyp()->m_haName.GetFirst();
    while (pItmHash != NULL) {
      CRezItm* pItm = pItmHash->GetRezItm();
      ASSERT(pItm != NULL);

      // fill out the header
      Hdr.Rez.Pos = pItm->m_nFilePos;
      Hdr.Rez.Size = pItm->m_nSize;
      Hdr.Rez.Time = pItm->m_nTime;
      Hdr.Rez.ID = pItm->m_nID;
      Hdr.Rez.Type = pTypHash->GetRezTyp()->m_nType;
      Hdr.Rez.NumKeys = pItm->m_nNumKeys;

      // write out the header
      nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Type);
      nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Rez.Pos);
      nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Rez.Size);
      nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Rez.Time);
      nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Rez.ID);
      nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Rez.Type);
      nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&Hdr.Rez.NumKeys);

      // write out the name
      if (pItm->m_sName == NULL) nCurPos += pRezFile->Write(nCurPos,0,1,&sZero);
      else nCurPos += pRezFile->Write(nCurPos,0,strlen(pItm->m_sName)+1,pItm->m_sName);

      // write out the description
      if (pItm->m_sDescription == NULL) nCurPos += pRezFile->Write(nCurPos,0,1,&sZero);
      else nCurPos += pRezFile->Write(nCurPos,0,strlen(pItm->m_sDescription)+1,pItm->m_sDescription);

      // write out the keys
      for (DWORD i = 0; i < pItm->m_nNumKeys; i++) {
        DWORD nKey = pItm->m_pKeyAry[i];
        nCurPos += pRezFile->Write(nCurPos,0,sizeof(DWORD),&nKey);
      }

      pItmHash = pItmHash->Next();
    }
    pTypHash = pTypHash->Next();
  }

  // the next write position is after this directory
  m_pRezMgr->m_nNextWritePos = nCurPos;
  *Size = nCurPos - Pos;

  return TRUE;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezDir::ReadDirBlock(CBaseRezFile* pRezFile, DWORD Pos, DWORD Size, BOOL bOverwriteItems) {
  ASSERT(Pos > 0);
  m_nItemsSize = 0;
  m_nItemsPos = 0xffffffff;
  DWORD nLastItemPos = 0;
  DWORD nLastItemSize = 0;

  // allocate memory for directory block
  BYTE* pBlk;
  pBlk = new BYTE[Size];
  ASSERT(pBlk != NULL);
  if (pBlk == NULL) return FALSE;

  // read in directory block (exit if error)
  if (pRezFile->Read(Pos,0,Size,pBlk) != Size) {
    delete [] pBlk;
    return FALSE;
  };

  // process all data in directory block 
  BYTE* pCur = pBlk;
  BYTE* pEnd = pBlk+Size;
  while (pCur < pEnd) {

    // if this is a directory entry
    if ((*(DWORD*)pCur) == DirectoryEntry) {
      pCur += sizeof(DWORD);

      // variables to store data read in
      DWORD Pos;
      DWORD Size;
      DWORD Time;
      char* sDirName;

      // convert simple header variables
      Pos = (*(DWORD*)pCur);
      pCur += sizeof(DWORD);
      Size = (*(DWORD*)pCur);
      pCur += sizeof(DWORD);
      Time = (*(DWORD*)pCur);
      pCur += sizeof(DWORD);

      // convert dir name
      sDirName = (char*)pCur;
      pCur += strlen(sDirName)+1;

	  // make sure this dir doesn't already exist if it does we don't need to add it again
	  CRezDir* pDir = m_haDir.Find(sDirName,!GetParentMgr()->GetLowerCaseUsed());
	  if (pDir == NULL) {	

        // construct new directory item
        pDir = new CRezDir(m_pRezMgr, this, sDirName, Pos, Size, Time, m_pRezMgr->m_nDirNumHashBins, m_pRezMgr->m_nTypNumHashBins);
        ASSERT(pDir != NULL);

        // insert new directory item in hash table
        m_haDir.Insert(&pDir->m_heDir);
      }
	  else {
	    pDir->m_nDirPos = Pos;
		pDir->m_nDirSize = Size;
		pDir->m_nLastTimeModified = Time;
	  }
    }

    // if this is a resource item entry
    else {
      ASSERT((*(DWORD*)pCur) == ResourceEntry); // Type is not a resource or a directory!!!!
      pCur += sizeof(DWORD);

      // variables to store data read in
      DWORD Pos;
      DWORD Size;
      DWORD Time;
      DWORD ID;
      DWORD Type;
      DWORD NumKeys;
      char* sName;
      char* sDescription;
      DWORD* pKeyAry;

      // convert simple header variables
      Pos = (*(DWORD*)pCur);
      pCur += sizeof(DWORD);
      Size = (*(DWORD*)pCur);
      pCur += sizeof(DWORD);
      Time = (*(DWORD*)pCur);
      pCur += sizeof(DWORD);
      ID = (*(DWORD*)pCur);
      pCur += sizeof(DWORD);
      Type = (*(DWORD*)pCur);
      pCur += sizeof(DWORD);
      NumKeys = (*(DWORD*)pCur);
      pCur += sizeof(DWORD);

      // convert item name
      sName = (char*)pCur;
      pCur += strlen(sName)+1;

      // find or create the type for this resource
      CRezTyp* pTyp = GetOrMakeTyp(Type);
      ASSERT(pTyp != NULL);

	  // if this gets set to true then this item will not be added
	  BOOL bSkipThisItem = FALSE;

	  // check if this ID already exists and deal with it
	  if (m_pRezMgr->m_bItemByIDUsed) {
        CRezItm* pDupIDItem = pTyp->m_haID.Find(ID);
		if (pDupIDItem != NULL) {
		  if (m_pRezMgr->m_bRenumberIDCollisions) {
		    ID = m_pRezMgr->m_nNextIDNumToUse;
			m_pRezMgr->m_nNextIDNumToUse++;
		  }
		  else if (bOverwriteItems) {
		    RemoveRezInternal(pTyp,pDupIDItem);
		  }
		  else {
  		    bSkipThisItem = TRUE;
		  }
		}
      }

	  // check if this name already exists and deal with it
	  {
        CRezItm* pDupNameItem = pTyp->m_haName.Find(sName);
		if (pDupNameItem != NULL) {
		  if (bOverwriteItems) {
		    RemoveRezInternal(pTyp,pDupNameItem);
		  }
		  else {
  		    bSkipThisItem = TRUE;
		  }
		}
      }

      // convert description name
      sDescription = (char*)pCur;
      pCur += strlen(sDescription)+1;
      if (sDescription[0] == '\0') sDescription = NULL;

      // convert KeyAry
      if (NumKeys > 0) {
        pKeyAry = new REZKEYVAL[NumKeys];
        ASSERT(pKeyAry != NULL);
        for (unsigned int i = 0; i < NumKeys; i++) {
          pKeyAry[i] = *(DWORD*)pCur;
          pCur += sizeof(DWORD);
        }
      }
      else pKeyAry = NULL;

	  // check if we are really going to add this item
 	  if (!bSkipThisItem) {

        // construct new resource item
		CRezItm* pItm = m_pRezMgr->AllocateRezItm();
        ASSERT(pItm != NULL);
		pItm->InitRezItm(this,sName,ID,pTyp,sDescription,Size,Pos,Time,NumKeys,pKeyAry,pRezFile);

        // insert new resource item in hash tables
        if (m_pRezMgr->m_bItemByIDUsed) pTyp->m_haID.Insert(&pItm->m_heID);
        pTyp->m_haName.Insert(&pItm->m_heName);

        // update the directory items size and position
        m_nItemsSize += pItm->m_nSize;
        if (pItm->m_nFilePos < m_nItemsPos) m_nItemsPos = pItm->m_nFilePos;
        if (pItm->m_nFilePos > nLastItemPos) {
          nLastItemPos = pItm->m_nFilePos;
          nLastItemSize = pItm->m_nSize;
        }
	  }

      // delete any needed allocations
      if (pKeyAry != NULL) delete [] pKeyAry;
    }
  };

  // free memory for block
  delete [] pBlk;

  return TRUE;
};

//---------------------------------------------------------------------------------------------------
CRezTyp* CRezDir::GetOrMakeTyp(REZTYPE nType) {
  // find the type 
  CRezTyp* pTyp = m_haTypes.Find(nType);

  // create a new type if this one is not found
  if (pTyp == NULL) {
    
    // create type structure
	if (m_pRezMgr->m_bItemByIDUsed)
	{
		pTyp = new CRezTyp(nType, this, m_pRezMgr->m_nByIDNumHashBins, m_pRezMgr->m_nByNameNumHashBins);
	}
	else
	{
		pTyp = new CRezTyp(nType, this, m_pRezMgr->m_nByNameNumHashBins);
	}
    ASSERT(pTyp != NULL);
    if (pTyp == NULL) return NULL;

    // add new type to typ hash table
    m_haTypes.Insert(&pTyp->m_heType);
  }

  return pTyp;
};


//***************************************************************************************************
// CRezMgr implementation

//---------------------------------------------------------------------------------------------------
CRezMgr::CRezMgr() {
  m_bFileOpened = FALSE; 
  m_bRenumberIDCollisions = TRUE;
  m_nNextIDNumToUse = 2000000000;	
  m_pPrimaryRezFile = NULL;
  m_nNumRezFiles = 0;
  m_nRootDirPos = 0;
  m_nNextWritePos = 0;
  m_bReadOnly = TRUE;
  m_pRootDir = NULL;
  m_nLastTimeModified = 0;
  m_bMustReWriteDirs = FALSE;
  m_nFileFormatVersion = 0;
  m_nLargestKeyAry = 0;
  m_nLargestDirNameSize = 0;
  m_nLargestRezNameSize = 0;
  m_nLargestCommentSize = 0;
  m_bIsSorted = TRUE;
  m_sFileName = NULL;
  m_nMaxOpenFilesInEmulatedDir = 3;
  m_sDirSeparators = NULL;
  m_bLowerCaseUsed = FALSE;
  m_bItemByIDUsed = FALSE;
  m_nByNameNumHashBins = kDefaultByNameNumHashBins;
  m_nByIDNumHashBins = kDefaultByIDNumHashBins;	
  m_nDirNumHashBins = kDefaultDirNumHashBins;	
  m_nTypNumHashBins = kDefaultTypNumHashBins;	
  m_nRezItmChunkSize = 100;	
  m_sUserTitle[0] = '\0';	
 };

//---------------------------------------------------------------------------------------------------
CRezMgr::~CRezMgr() {
  if (m_bFileOpened) Close();
  CBaseRezFile* pRezFile;
  while ((pRezFile = m_lstRezFiles.GetFirst()) != NULL) {
    m_lstRezFiles.Delete(pRezFile);
	m_nNumRezFiles--;
	delete pRezFile;
  }
  if (m_pRootDir != NULL) {
    delete m_pRootDir;
    m_pRootDir = NULL;
  }
  if (m_sFileName != NULL) {
    delete [] m_sFileName;
    m_sFileName = NULL;
  }
  if (m_sDirSeparators != NULL) 
  {
	  delete [] m_sDirSeparators;
	  m_sDirSeparators = NULL;
  }
  m_bFileOpened = FALSE; 
  m_pPrimaryRezFile = NULL;
  m_nRootDirPos = 0;
  m_nRootDirSize = 0;
  m_nRootDirTime = 0;
  m_nNextWritePos = 0;
  m_bReadOnly = TRUE;
  m_pRootDir = NULL;
  m_nLastTimeModified = 0;
  m_bMustReWriteDirs = FALSE;
  m_nFileFormatVersion = 1;
  m_nLargestKeyAry = 0;
  m_nLargestDirNameSize = 0;
  m_nLargestRezNameSize = 0;
  m_nLargestCommentSize = 0;
  m_bIsSorted = TRUE;
  m_sFileName = NULL;

  // remove all RezItm Chunks
  {
	CRezItmChunk* pChunk;
	while ((pChunk = m_lstRezItmChunks.GetFirst()) != NULL)
	{
		delete [] pChunk->m_pRezItmAry;
		m_lstRezItmChunks.Delete(pChunk);
		delete pChunk;
	}
  }
};

//---------------------------------------------------------------------------------------------------
BOOL CRezMgr::Open(const char* FileName, BOOL ReadOnly, BOOL CreateNew) {
  ASSERT(FileName != NULL); // NULL file name!
  ASSERT(!(ReadOnly && CreateNew)); // gee, we can't do both!
  m_bReadOnly = ReadOnly;

  // store file name in class
  if (m_sFileName != NULL) delete [] m_sFileName;
  m_sFileName = new char[strlen(FileName)+1];
  ASSERT(m_sFileName != NULL);
  strcpy(m_sFileName,FileName);

  // check if user has asked us to open a directory instead of a rez file
  if (IsDirectory(FileName)) {

    // make sure this is only open for read only
	ASSERT(m_bReadOnly != FALSE);  // we can only read from directories not write to them!
    if (m_bReadOnly == FALSE) return FALSE;

    // Create the CRezFileDirectoryEmulation object
    CRezFileDirectoryEmulation* pRezFile;
	pRezFile = new CRezFileDirectoryEmulation(this,m_nMaxOpenFilesInEmulatedDir);
    ASSERT(pRezFile != NULL);
    if (pRezFile == NULL) {
	  delete [] m_sFileName;
	  m_sFileName = NULL;
      return FALSE;
    }
    m_pPrimaryRezFile = pRezFile;
    m_lstRezFiles.Insert(pRezFile);
    m_nNumRezFiles++;

    // open the file
    if (!pRezFile->Open(FileName,ReadOnly,CreateNew)) return FALSE;
    m_bFileOpened = TRUE;

	// create empty root directory
    m_pRootDir = new CRezDir(this, NULL, "", 0, 0, GetCurTime(), m_nDirNumHashBins, m_nTypNumHashBins);
    ASSERT(m_pRootDir != NULL);

	// read in data from directory and all sub directories
	ReadEmulationDirectory(pRezFile,m_pRootDir,m_sFileName,FALSE);

    return TRUE;
  }

  // create a new CRezFile object for the RezFile
  CRezFile* pRezFile;
  pRezFile = new CRezFile(this);
  ASSERT(pRezFile != NULL);
  if (pRezFile == NULL) {
	delete [] m_sFileName;
	m_sFileName = NULL;
    return FALSE;
  }
  m_pPrimaryRezFile = pRezFile;
  m_lstRezFiles.Insert(pRezFile);
  m_nNumRezFiles++;

  // open the file
  if (!pRezFile->Open(FileName,ReadOnly,CreateNew)) return FALSE;
  m_bFileOpened = TRUE;

  // set up variables if this is a new file
  if (CreateNew) {
    m_nNextWritePos = sizeof(FileMainHeaderStruct);
    m_bMustReWriteDirs = TRUE;

    m_pRootDir = new CRezDir(this, NULL, "", 0, 0, GetCurTime(), m_nDirNumHashBins, m_nTypNumHashBins);
    ASSERT(m_pRootDir != NULL);
  }

  // if this is an old file read in header and directories
  else {

    // read in the header
    FileMainHeaderStruct Header;
    pRezFile->Read(0,0,sizeof(Header),&Header);

    // store header values in RezMgr class
    m_nNextWritePos =       Header.NextWritePos;
    m_nRootDirPos =         Header.RootDirPos;
    m_nRootDirSize =        Header.RootDirSize;
    m_nRootDirTime =        Header.RootDirTime;
    m_nLastTimeModified =   Header.Time;
    m_nFileFormatVersion =  Header.FileFormatVersion;
    m_nLargestKeyAry =      Header.LargestKeyAry;
    m_nLargestDirNameSize = Header.LargestDirNameSize;
    m_nLargestRezNameSize = Header.LargestRezNameSize;
    m_nLargestCommentSize = Header.LargestCommentSize;
    m_bIsSorted =           Header.IsSorted;

	// set the internal value for the user title in the header
	memcpy(m_sUserTitle, Header.UserTitle, RezMgrUserTitleSize);
	m_sUserTitle[RezMgrUserTitleSize] = '\0';
	{
		int i = RezMgrUserTitleSize-1;
		while (i--)
		{
			if (m_sUserTitle[i] != ' ') break;
			m_sUserTitle[i] = '\0';
		}
	}

    // verify that this is a valid file
    ASSERT(Header.CR1 == 0x0d);
    ASSERT(Header.LF2 == 0x0a);
    ASSERT(Header.EOF1 == 0x1a);
    ASSERT(m_nFileFormatVersion == 1);
	if (Header.CR1 != 0x0d) return FALSE;
	if (Header.LF2 != 0x0a) return FALSE;
	if (Header.EOF1 != 0x1a) return FALSE;
	if (m_nFileFormatVersion != 1) return FALSE;

    // create root directory
    m_pRootDir = new CRezDir(this, NULL, "", m_nRootDirPos, m_nRootDirSize, m_nRootDirTime, m_nDirNumHashBins, m_nTypNumHashBins);
    ASSERT(m_pRootDir != NULL);

    // read in directories
    m_pRootDir->ReadAllDirs(pRezFile, m_nRootDirPos, m_nRootDirSize, FALSE);
  }

  return TRUE;
}; 

//---------------------------------------------------------------------------------------------------
BOOL CRezMgr::ReadEmulationDirectory(CRezFileDirectoryEmulation* pRezFileEmulation, CRezDir* pDir, char* sParamPath, BOOL bOverwriteItems) {
  ASSERT(pDir != NULL);
  ASSERT(this != NULL);
  ASSERT(sParamPath != NULL);
  ASSERT(m_bFileOpened);
  
  _finddata_t fileinfo;

  // figure out the path to this dir with added backslash
  char sPath[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
  strcpy(sPath,sParamPath);
  if (sPath[strlen(sPath)-1] != '\\') strcat(sPath,"\\");

  // figure out the find search string by adding *.* to search for everything
  char sFindPath[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
  strcpy(sFindPath,sPath);
  strcat(sFindPath,"*.*");

  // being search for everything in this directory using findfirst and findnext
  long nFindHandle = _findfirst( sFindPath, &fileinfo );
  if (nFindHandle >= 0) {

    // loop through all entries in this directory
    do {

      // skip the files . and ..
      if (strcmp(fileinfo.name,".") == 0) continue;
      if (strcmp(fileinfo.name,"..") == 0) continue;
      
      // if this is a subdirectory
      if ((fileinfo.attrib & _A_SUBDIR) == _A_SUBDIR) {

        // figure out the base name
        char sBaseName[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
        strcpy(sBaseName,fileinfo.name);
        if (!m_bLowerCaseUsed) strupr(sBaseName);

        // figure out the path name we are working on
        char sPathName[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
        strcpy(sPathName,sPath);
        strcat(sPathName,sBaseName);              
        strcat(sPathName,"\\");

        // create new directory entry in resource file (unless directory already exists)
        CRezDir* pNewDir;
		pNewDir = pDir->GetDir(sBaseName);
		if (pNewDir == NULL) pNewDir = pDir->CreateDir(sBaseName);

        // error if directory not found or created
        if (pNewDir == NULL) {
		  ASSERT(FALSE); 
          continue;
        }

        // call TransferDir on the new directory
        ReadEmulationDirectory(pRezFileEmulation,pNewDir,sPathName,bOverwriteItems);
      }

      // if this is a file add it to the resource file
      else {

        // figure out the file name we are working on
        char sFileName[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
        strcpy(sFileName,sPath);
        strcat(sFileName,fileinfo.name);              
        
        // split the file name up into its parts
        char drive[_MAX_DRIVE+1];
        char dir[_MAX_DIR+1];
        char fname[_MAX_FNAME+1];
        char ext[_MAX_EXT+1];
         _splitpath(sFileName, drive, dir, fname, ext );
        
        // figure out the Name for this file
        char sName[_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5];
        ASSERT(strlen(fname) < (_MAX_DRIVE+_MAX_DIR+_MAX_FNAME+_MAX_EXT+5));
        strcpy(sName,fname);
        strupr(sName);
        
        // figure out the ID for this file (if name is all digits use it as ID number, otherwise assign a number)
        REZID nID;
        {
          int nNameLen = strlen(sName);
          int i;
          for (i = 0; i < nNameLen; i++) {
            if ((sName[i] < '0') || (sName[i] > '9')) break;
          }
          if (i < nNameLen) {
            nID = m_nNextIDNumToUse;
            m_nNextIDNumToUse++;
          }
          else {
            nID = atol(sName);
          }
        }
        
        // figure out the Type for this file
        REZTYPE nType;
        char sExt[5];
        if (strlen(ext) > 0) {
          strcpy(sExt,&ext[1]);
          strupr(sExt);
          nType = StrToType(sExt);
        }
        else nType = 0;
        
        // convert type back to string 
        char sType[5];
        TypeToStr(nType,sType);
        
        // find or create the type for this resource
        CRezTyp* pTyp = pDir->GetOrMakeTyp(nType);
        ASSERT(pTyp != NULL);

        // create new resource if it doesn't exist
        CRezItm* pItm;
		pItm = pDir->GetRez(sName,nType);
		if (pItm == NULL) pItm = pDir->CreateRezInternal(nID,sName,pTyp,NULL);
		else {
		  if (bOverwriteItems) {
		    pDir->RemoveRezInternal(pTyp,pItm);
			pItm = pDir->CreateRezInternal(nID,sName,pTyp,NULL);
		  }
		  else {
		    pItm = NULL;
		  }
		}
        
        // make sure resource was created
        if (pItm != NULL) {

     	  // store the file time in the resource
		  pItm->SetTime((REZTIME)fileinfo.time_write);

		  // store the size of the file in the resource
		  pItm->m_nSize = fileinfo.size;

  		  // set the file pointer for the item
		  pItm->m_pRezFile = new CRezFileSingleFile(this, sFileName, pRezFileEmulation);
		  ASSERT(pItm->m_pRezFile != NULL);
        }
      }
        
    // get the next entry in this directory
    } while (_findnext(nFindHandle, &fileinfo) == 0);

    // close out the directory findfirst and findnext
    _findclose(nFindHandle);
  }

  return TRUE;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezMgr::Close(BOOL bCompact) {
  ASSERT(m_bFileOpened);
  BOOL bRetVal;

  // if this is not read only flush it
  if (!m_bReadOnly) Flush();

  // compact the file if we are supposed to (this closes the file)
  if (bCompact) bRetVal = Compact();
  else bRetVal = m_pPrimaryRezFile->Close();

  // remove the primary RezFile from the list of RezFiles
  m_lstRezFiles.Delete(m_pPrimaryRezFile);
  m_nNumRezFiles--;
  delete m_pPrimaryRezFile;
  m_pPrimaryRezFile = NULL;

  // delete and close out all the rest of the RezFiles
  CBaseRezFile* pRezFile;
  while ((pRezFile = m_lstRezFiles.GetFirst()) != NULL) {
	pRezFile->Close();
    m_lstRezFiles.Delete(pRezFile);
	m_nNumRezFiles--;
	delete pRezFile;
  }

  // remove memory in this object associated with the open file
  if (m_pRootDir != NULL) {
    delete m_pRootDir;
    m_pRootDir = NULL;
  }
  if (m_sFileName != NULL) {
    delete [] m_sFileName;
    m_sFileName = NULL;
  }

  // mark resource as closed
  m_bFileOpened = FALSE;

  return bRetVal;
}; 

//---------------------------------------------------------------------------------------------------
CRezDir* CRezMgr::GetRootDir() {
  ASSERT(m_bFileOpened);
  return m_pRootDir;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezMgr::Flush() {
  ASSERT(m_bReadOnly != TRUE);

  // write out directory and header if not read only
  if (!m_bReadOnly) {

    // save the next write pos for the header information
    unsigned long nSaveWritePos = m_nNextWritePos;

    // write out all of the directories
    m_pRootDir->WriteAllDirs(m_pPrimaryRezFile, &m_nRootDirPos,&m_nRootDirSize);
    
    // fill out the permant parts of the header
    FileMainHeaderStruct Header;
    Header.CR1 = 0x0d;
    Header.CR2 = 0x0d;
    Header.CR3 = 0x0d;
    Header.LF1 = 0x0a;
    Header.LF2 = 0x0a;
    Header.LF3 = 0x0a;
    Header.EOF1 = 0x1a;
    memset(Header.FileType,' ',RezMgrUserTitleSize);
    strcpy(Header.FileType,"RezMgr Version 1 Copyright (C) 1995 MONOLITH INC.");
    Header.FileType[strlen(Header.FileType)] = ' ';
    memset(Header.UserTitle,' ',RezMgrUserTitleSize);
	if (m_sUserTitle[0] != '\0') memcpy(Header.UserTitle,m_sUserTitle,strlen(m_sUserTitle));
    
    // fill out the variable parts of the header
    Header.FileFormatVersion      = 1;     
    Header.RootDirPos             = m_nRootDirPos; 			
    Header.RootDirSize            = m_nRootDirSize;              
    Header.RootDirTime            = m_nRootDirTime;           
    Header.NextWritePos           = nSaveWritePos;           
    Header.Time                   = m_nLastTimeModified; 					
    Header.LargestKeyAry          = m_nLargestKeyAry;			
    Header.LargestDirNameSize     = m_nLargestDirNameSize;    
    Header.LargestRezNameSize     = m_nLargestRezNameSize;	
    Header.LargestCommentSize     = m_nLargestCommentSize;	
    Header.IsSorted               = m_bIsSorted;                
    
    // write the header
    m_pPrimaryRezFile->Write(0,0,sizeof(Header),&Header);

    // actually flush the file 
    m_pPrimaryRezFile->Flush();

    return TRUE;
  }
  else return FALSE;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezMgr::Compact() {
  // if the file is already sorted we don't need to do anything
  if (m_bIsSorted) return TRUE;

  // save the original file name (talon: sic, a byte short)
  char* sOrigFileName = new char[strlen(m_sFileName+1)];
  strcpy(sOrigFileName,m_sFileName);

  // split the file name up into its parts
  char drive[_MAX_DRIVE+1];
  char dir[_MAX_DIR+1];
  char fname[_MAX_FNAME+1];
  char ext[_MAX_EXT+1];
  _splitpath(m_sFileName, drive, dir, fname, ext );

  // the temporary file goes in the same directory
  char* sTmpFileName = new char[strlen(m_sFileName)+16];
  strcpy(sTmpFileName,drive);
  strcat(sTmpFileName,dir);
  strcat(sTmpFileName,"REZMGRX0.TMP");

  // and so does the backup file
  char* sBakFileName = new char[strlen(m_sFileName)+16];
  strcpy(sBakFileName,drive);
  strcat(sBakFileName,dir);
  strcat(sBakFileName,fname);
  strcat(sBakFileName,".BAK");

  // copy everything to a new resource file
  CRezMgr NewRezMgr;
  NewRezMgr.Open(sTmpFileName,FALSE,TRUE);
  CompactTransferDir(NewRezMgr.m_pRootDir,m_pRootDir);
  NewRezMgr.m_bIsSorted = TRUE;
  NewRezMgr.Close();

  // replace the original file with the new one
  if (m_pPrimaryRezFile->Close()) {
    remove(sBakFileName);
    rename(sOrigFileName,sBakFileName);
    rename(sTmpFileName,sOrigFileName);
    delete [] sTmpFileName;
    delete [] sOrigFileName;
    delete [] sBakFileName;
  }

  return FALSE;
};

//---------------------------------------------------------------------------------------------------
BOOL CRezMgr::CompactTransferDir(CRezDir* pToDir, CRezDir* pFromDir) {
  // transfer all of the sub directories
  CRezDir* pDir = pFromDir->GetFirstSubDir();
  while (pDir != NULL) {
    CRezDir* pNewDir = pToDir->CreateDir(pDir->m_sDirName);
    CompactTransferDir(pNewDir,pDir);
    pNewDir->m_nLastTimeModified = pDir->m_nLastTimeModified;
    pDir = pFromDir->GetNextSubDir(pDir);
  }

  // transfer all of the items of every type
  CRezTyp* pTyp = pFromDir->GetFirstType();
  while (pTyp != NULL) {
    CRezItm* pItm = pFromDir->GetFirstItem(pTyp);
    while (pItm != NULL) {
      REZNAME sName = pItm->m_sName;
      REZID nID = pItm->m_nID;
      CRezItm* pNewItm = pToDir->CreateRez(nID,sName,pItm->GetType());

      // the data
      BYTE* pData = pNewItm->Create(pItm->m_nSize);
      if (pItm->m_nSize > 0) {
        BYTE* pOldData = pItm->Load();
        memcpy(pData,pOldData,pItm->m_nSize);
        pNewItm->Save();
      }

      // the keys
      if (pItm->m_nNumKeys > 0) {
        DWORD i = pItm->m_nNumKeys;
        pNewItm->SetKey(i,pItm->m_pKeyAry[i]);
        while (i != 0) {
          i--;
          pNewItm->SetKey(i,pItm->m_pKeyAry[i]);
        }
      }

      // the time and description
      pNewItm->SetTime(pItm->GetTime());
      pNewItm->SetDesc(pItm->m_sDescription);

      pItm = pFromDir->GetNextItem(pItm);
    }
    pTyp = pFromDir->GetNextType(pTyp);
  }

  return FALSE;
};

//---------------------------------------------------------------------------------------------------
REZTYPE CRezMgr::StrToType(char* pStr) {
  ASSERT(pStr != NULL);
  if (pStr == NULL) return 0;
  REZTYPE nType = 0;
  BYTE* pType = (BYTE*)&nType;

  // figure out length of string
  int nStrLen = strlen(pStr);

  // store string in type
  if (nStrLen > 0) pType[nStrLen-1] = pStr[0];
  if (nStrLen > 1) pType[nStrLen-2] = pStr[1];
  if (nStrLen > 2) pType[nStrLen-3] = pStr[2];
  if (nStrLen > 3) pType[nStrLen-4] = pStr[3];

  return nType;
};

//---------------------------------------------------------------------------------------------------
void CRezMgr::TypeToStr(REZTYPE nType, char* sType) {
  ASSERT(sType != NULL);
  if (sType == NULL) return;
  BYTE* pType = (BYTE*)&nType;
  int nStrLen = 0;

  // figure out the length of the string
  if (pType[3] != '\0') nStrLen = 4;
  else if (pType[2] != '\0') nStrLen = 3;
  else if (pType[1] != '\0') nStrLen = 2;
  else if (pType[0] != '\0') nStrLen = 1;

  // store type in string
  if (nStrLen > 0) sType[0] = pType[nStrLen-1];
  if (nStrLen > 1) sType[1] = pType[nStrLen-2];
  if (nStrLen > 2) sType[2] = pType[nStrLen-3];
  if (nStrLen > 3) sType[3] = pType[nStrLen-4];

  // put terminator on string
  sType[nStrLen] = '\0';

  return;
};

//---------------------------------------------------------------------------------------------------
void* CRezMgr::Alloc(DWORD NumBytes) {
  ASSERT(FALSE); // Not implemented!
  return NULL;
};

//---------------------------------------------------------------------------------------------------
void CRezMgr::Free(void* Ptr) {
  ASSERT(FALSE); // Not implemented!
};

//---------------------------------------------------------------------------------------------------
BOOL CRezMgr::DiskError() {
  return FALSE;
};

//---------------------------------------------------------------------------------------------------
REZTIME CRezMgr::GetCurTime() {
  time_t nTimeVal;
  nTimeVal = time(&nTimeVal);
  return (REZTIME)nTimeVal;
};

//---------------------------------------------------------------------------------------------------
inline BOOL CRezDir::IsGoodChar(char c)
{
	if (m_pRezMgr->m_sDirSeparators != NULL)
	{
		return (strchr(m_pRezMgr->m_sDirSeparators,c) == NULL);
	}
	else
	{
		if (c >= ' ' && c <= '.') return(TRUE);
		if (c >= '0' && c <= '9') return(TRUE);
		if (c >= 'A' && c <= 'Z') return(TRUE);
		if (c >= 'a' && c <= 'z') return(TRUE);
		return(FALSE);
	}
}

// [blg]
//---------------------------------------------------------------------------------------------------
CRezDir* CRezDir::GetDirFromPath(REZCDIRNAME sPath)
{
	// Strip off leading slash...

	int len = strlen(sPath);

	if (len > 1)
	{
		if (!IsGoodChar(sPath[0]))
		{
			sPath = &sPath[1];
			len--;
		}
	}


	// Read the first directory name...

	int i = 0;
	char sDir[1024];

	while (IsGoodChar(sPath[i]))
	{
		sDir[i] = sPath[i];
		i++;
		if (i >= 1023) break;
	}

	sDir[i] = '\0';

	CRezDir* pDir = GetDir(sDir);
	if (!pDir) return(NULL);


	// If this is the last name in the path string, we're done...

	if (sPath[i] == '\0')
	{
		return(pDir);
	}


	// Skip any white space...

	while (!IsGoodChar(sPath[i]))
	{
		i++;
		if (sPath[i] == '\0') return(pDir);
	}


	// Recursively continue...

	return(pDir->GetDirFromPath(&sPath[i]));
}

// [blg]
//---------------------------------------------------------------------------------------------------
CRezItm* CRezDir::GetRezFromDosPath(const char* sPath)
{
	char	sDir[1024];
	char	sRez[1024];


	// Strip off leading slash...

	int len = strlen(sPath);

	if (len >= 1023) return NULL;

	if (len > 1)
	{
		if (!IsGoodChar(sPath[0]))
		{
			sPath = &sPath[1];
			len--;
		}
	}


	// Strip off the rez name...

	int i   = len - 1;

	while (IsGoodChar(sPath[i]))
	{
		i--;
		if (i < 0) break;
	}

	if (i == len) return(NULL);

	strncpy(sRez, &sPath[i+1], 1023);
	sRez[1023] = '\0';


	// If we were only given a rez name, try to get it...

	if (i <= 1)
	{
		return(GetRezFromDosName(sRez));
	}


	// Copy everything except for the rez name...

	strncpy(sDir, sPath, i);
	sDir[i] = '\0';


	// Find the directory for this path...

	CRezDir* pDir = GetDirFromPath(sDir);

	if (!pDir) return(NULL);


	// Try to get the rez from the dir we just got...

	return(pDir->GetRezFromDosName(sRez));
}

//---------------------------------------------------------------------------------------------------
BOOL CRezMgr::IsDirectory(const char* sFileName) {
   struct stat buf;
   int result;

   // Get data associated with "stat.c"
   result = stat( sFileName, &buf );

   // Check if statistics are valid 
   if( result != 0 ) return FALSE;

   // is this a directory
   if ((buf.st_mode & _S_IFDIR) == _S_IFDIR) return TRUE;
   else return FALSE;
}

//---------------------------------------------------------------------------------------------------
CRezItm* CRezMgr::AllocateRezItm()
{
	CRezItm* pNewItem = NULL;
	CRezItmHashByName* pHash;

	pHash = m_hashRezItmFreeList.GetFirst();
	if (pHash != NULL) pNewItem = pHash->GetRezItm();

	// if we are out of free Rez Items then make a new chunk and allocate one from there
	if (pNewItem == NULL)
	{
		CRezMgr::CRezItmChunk* pNewChunk;
		pNewChunk = new CRezItmChunk;
		if (pNewChunk == NULL) return NULL;

		pNewChunk->m_pRezItmAry = new CRezItm[m_nRezItmChunkSize];
		if (pNewChunk->m_pRezItmAry == NULL)
		{
			delete pNewChunk;
			return NULL;
		}

		for (unsigned int i = 0; i < m_nRezItmChunkSize; i++)
		{
			pNewChunk->m_pRezItmAry[i].m_heName.SetRezItm(&pNewChunk->m_pRezItmAry[i]);
			m_hashRezItmFreeList.Insert(&pNewChunk->m_pRezItmAry[i].m_heName);
		}

		m_lstRezItmChunks.Insert(pNewChunk);		

		pNewItem = m_hashRezItmFreeList.GetFirst()->GetRezItm();
	}

	if (pNewItem != NULL)
	{
		m_hashRezItmFreeList.Delete(&pNewItem->m_heName);
	}

	return pNewItem;
}

//---------------------------------------------------------------------------------------------------
void CRezMgr::DeAllocateRezItm(CRezItm* pItem)
{

	if (pItem != NULL)
	{
		m_hashRezItmFreeList.Insert(&pItem->m_heName);
	}
}

//---------------------------------------------------------------------------------------------------
void CRezMgr::SetUserTitle(const char* sUserTitle)
{
	strncpy(m_sUserTitle, sUserTitle, RezMgrUserTitleSize);
	m_sUserTitle[RezMgrUserTitleSize] = '\0';
}
