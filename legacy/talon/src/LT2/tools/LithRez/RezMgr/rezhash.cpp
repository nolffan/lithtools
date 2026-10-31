
// talon: the 1998 RezMgr's hash tables (0x40dc10..0x40dde5), Build 69's
// with the table by ID.

#include <stdio.h>
#include <stdlib.h>
#include "rezmgr.h"
#include <string.h>

// -----------------------------------------------------------------------------------------
// CRezItmHashByName

unsigned int CRezItmHashByName::HashFunc() {
  ASSERT(m_pRezItm != NULL);
  return GetParentHash()->HashFunc(m_pRezItm->GetName());
};


// -----------------------------------------------------------------------------------------
// CRezItmHashTableByName

unsigned int CRezItmHashTableByName::HashFunc(REZCNAME pStr) {
  if (pStr == NULL) return 0;
  unsigned int Count;
  for (Count = 0; *pStr != '\0'; pStr++) { Count++; };
  ASSERT(GetNumBins() > 0);
  return (Count % GetNumBins());
};

CRezItm* CRezItmHashTableByName::Find(REZCNAME sName, BOOL bIgnoreCase) {
  ASSERT(sName != NULL);
  if (sName == NULL) return NULL;
  CRezItmHashByName* pItm = GetFirstInBin(HashFunc(sName));
  if (bIgnoreCase) {
    while (pItm != NULL) {
      ASSERT(pItm->GetRezItm() != NULL);
      ASSERT(pItm->GetRezItm()->GetName() != NULL);
      if (stricmp(pItm->GetRezItm()->GetName(),sName) == 0) return pItm->GetRezItm();
      pItm = pItm->NextInBin();
    }
  }
  else {
    while (pItm != NULL) {
      ASSERT(pItm->GetRezItm() != NULL);
      ASSERT(pItm->GetRezItm()->GetName() != NULL);
      if (strcmp(pItm->GetRezItm()->GetName(),sName) == 0) return pItm->GetRezItm();
      pItm = pItm->NextInBin();
    }
  }
  return NULL;
};


// -----------------------------------------------------------------------------------------
// CRezItmHashByID

unsigned int CRezItmHashByID::HashFunc() {
  ASSERT(m_pRezItm != NULL);
  return GetParentHash()->HashFunc(m_pRezItm->GetID());
};


// -----------------------------------------------------------------------------------------
// CRezItmHashTableByID

unsigned int CRezItmHashTableByID::HashFunc(REZID nID) {
  ASSERT(GetNumBins() > 0);
  return (nID % GetNumBins());
};

CRezItm* CRezItmHashTableByID::Find(REZID nID) {
  CRezItmHashByID* pItm = GetFirstInBin(HashFunc(nID));
  while (pItm != NULL) {
    ASSERT(pItm->GetRezItm() != NULL);
    if (pItm->GetRezItm()->GetID() == nID) return pItm->GetRezItm();
    pItm = pItm->NextInBin();
  }
  return NULL;
};


// -----------------------------------------------------------------------------------------
// CRezTypeHash

unsigned int CRezTypeHash::HashFunc() {
  ASSERT(m_pRezTyp != NULL);
  return GetParentHash()->HashFunc(m_pRezTyp->GetType());
};


// -----------------------------------------------------------------------------------------
// CRezTypeHashTable

unsigned int CRezTypeHashTable::HashFunc(REZTYPE nType) {
  ASSERT(GetNumBins() > 0);
  return (nType % GetNumBins());
};

CRezTyp* CRezTypeHashTable::Find(REZTYPE nType) {
  CRezTypeHash* pItm = GetFirstInBin(HashFunc(nType));
  while (pItm != NULL) {
    ASSERT(pItm->GetRezTyp() != NULL);
    if (pItm->GetRezTyp()->GetType() == nType) return pItm->GetRezTyp();
    pItm = pItm->NextInBin();
  }
  return NULL;
};


// -----------------------------------------------------------------------------------------
// CRezDirHash

unsigned int CRezDirHash::HashFunc() {
  ASSERT(m_pRezDir != NULL);
  return GetParentHash()->HashFunc(m_pRezDir->GetDirName());
};


// -----------------------------------------------------------------------------------------
// CRezDirHashTable

unsigned int CRezDirHashTable::HashFunc(REZCDIRNAME pStr) {
  ASSERT(pStr != NULL);
  if (pStr == NULL) return 0;
  unsigned int Count;
  for (Count = 0; *pStr != '\0'; pStr++) { Count++; };
  ASSERT(GetNumBins() > 0);
  return (Count % GetNumBins());
};

CRezDir* CRezDirHashTable::Find(REZCDIRNAME sName, BOOL bIgnoreCase) {
  ASSERT(sName != NULL);
  if (sName == NULL) return NULL;
  CRezDirHash* pItm = GetFirstInBin(HashFunc(sName));
  if (bIgnoreCase) {
    while (pItm != NULL) {
      ASSERT(pItm->GetRezDir() != NULL);
      ASSERT(pItm->GetRezDir()->GetDirName() != NULL);
      if (stricmp(pItm->GetRezDir()->GetDirName(),sName) == 0) return pItm->GetRezDir();
      pItm = pItm->NextInBin();
    }
  }
  else {
    while (pItm != NULL) {
      ASSERT(pItm->GetRezDir() != NULL);
      ASSERT(pItm->GetRezDir()->GetDirName() != NULL);
      if (strcmp(pItm->GetRezDir()->GetDirName(),sName) == 0) return pItm->GetRezDir();
      pItm = pItm->NextInBin();
    }
  }
  return NULL;
};



