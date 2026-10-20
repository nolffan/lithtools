#include "bdefs.h"
#include "preworld.h"
#include "preleaf.h"
#include "processing.h"
#include "editobjects.h"

// talon: written from the image (0x4144f0..0x414e40); Build 69 makes no
// leak files. The file's name is ours.


// Looks for a way through the portals from pLeaf to pTarget. path holds
// the portal centers on the way to pLeaf, and gets the ones on to pTarget.
static BOOL FindLeakPath(CPreLeaf *pLeaf, CPreLeaf *pTarget, CMoArray<PVector> &path)
{
	CMoArray<PVector> testPath;
	CPrePortal *pPortal;
	uint32 i;

	if(pLeaf->m_Flags & LEAFF_VISITED)
		return FALSE;

	pLeaf->m_Flags |= LEAFF_VISITED;
	if(pLeaf == pTarget)
		return TRUE;

	for(i=0; i < pLeaf->m_Portals; i++)
	{
		pPortal = pLeaf->m_Portals[i];

		if(pPortal->m_pPartner && pPortal->m_pPartner->m_pLeaf)
		{
			PVector vCenter(pPortal->m_BasePoly.GetCenter());
			testPath.CopyArray(path);
			testPath.Append(vCenter);

			if(FindLeakPath(pPortal->m_pPartner->m_pLeaf, pTarget, testPath))
			{
				path.CopyArray(testPath);
				return TRUE;
			}
		}
	}

	return FALSE;
}


// Writes the path from the inside of the world to its OutsideDef, as line
// segments, to pFilename.
BOOL MakeLeakFile(CPreWorld *pWorld, const char *pFilename)
{
	CMoArray<PVector> path;
	CBaseEditObj *pInsideObj, *pOutsideObj;
	CPreLeaf *pInside, *pOutside;
	PVector vPos;
	FILE *fp;
	uint32 i;


	DrawStatusText("Creating leak file %s", pFilename);

	pInside = FindInsideLeaf(pWorld, &pInsideObj);
	if(!pInside)
	{
		DrawStatusText("** Couldn't find an inside leaf for leak file.");
		return FALSE;
	}

	pOutside = FindObjectLeaf(pWorld, "OutsideDef", &pOutsideObj);
	if(!pOutside)
	{
		DrawStatusText("** Couldn't find an outside leaf.");
		DrawStatusText("** (Or the world is sealed correctly).");
		return FALSE;
	}

	for(i=0; i < pWorld->m_Leaves; i++)
		pWorld->m_Leaves[i]->m_Flags &= ~LEAFF_VISITED;

	if(!FindLeakPath(pInside, pOutside, path))
		DrawStatusText("** Couldn't find a path from inside to outside.");

	fp = fopen(pFilename, "wt");
	if(fp)
	{
		VEC_COPY(vPos, pInsideObj->GetPos());
		path.Insert(0, vPos);

		VEC_COPY(vPos, pOutsideObj->GetPos());
		path.Append(vPos);

		for(i=0; i < path.GetSize()-1; i++)
		{
			fprintf(fp, "%f %f %f %f %f %f\n", VEC_EXPAND(path[i]), VEC_EXPAND(path[i+1]));
		}

		fclose(fp);
		return TRUE;
	}
	else
	{
		DrawStatusText("** Unable to open leak file %s.", pFilename);
		return FALSE;
	}
}
