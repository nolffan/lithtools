#include "bdefs.h"
#include "preworld.h"
#include "preleaf.h"
#include "pregeometry.h"
#include "processing.h"
#include "editobjects.h"

// talon: written from the image (0x4144f0..0x414e40); Build 69 makes no
// leak files. The file's name is ours. The paths are PVertexArrays: the
// shipped code's are CMoArray<PVertex>'s (MakeLeakFile stores its vtable,
// 0x4749f8, whose GenAppend is 0x43b6f0, and FindLeakPath calls its
// CopyArray2, 0x414d70, and Insert2, 0x43ddb0).


// talon: NOLF's FindLeakPath is 0x420a40 (339 bytes, its EH funclets
// 0x420b93), before LeakFile's range in the map. It constructs vCenter
// with testPath at the top (0x420a56), not in the loop. Built /Ob0 it is
// byte-identical; as the file is built (inlining), the out-of-line calls
// the shipped code makes (CMoArray's CopyArray, Append, the PVertex
// constructor) are inlined, and /Ob0 for the file loses LeakFile's
// templates (0x421550, 0x421a20). MakeLeakFile (0x420be0) built /Ob0 has
// the shipped instructions but in an unaligned frame: the shipped one is
// a $$1 (0x420bee) with a 0xac-byte frame, 16 bytes of it unused
// (-0x18..-0x24), where ours is 0x80; what makes icl align it is open.

// Looks for a way through the portals from pLeaf to pTarget. path holds
// the portal centers on the way to pLeaf, and gets the ones on to pTarget.
static BOOL FindLeakPath(CPreLeaf *pLeaf, CPreLeaf *pTarget, PVertexArray &path)
{
	PVertexArray testPath;
	PVertex vCenter;
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
			(PVector&)vCenter = pPortal->m_BasePoly.GetCenter();
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
	PVertexArray path;
	CBaseEditObj *pInsideObj, *pOutsideObj;
	CPreLeaf *pInside, *pOutside;
	PVertex vPos;
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
