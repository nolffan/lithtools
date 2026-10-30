
#ifndef __PS2
#include <windows.h>
#endif
#include "bdefs.h"
#include "model.h"

// talon: the module opens with std::ctype<unsigned short>::id's 21-byte
// initializer (0x431e78), which only <xlocale> (via <string>) gives it.
#include <string>
#include "model_ops.h"
#include "conparse.h"
#include "geomroutines.h"

#include <math.h>
#include <float.h>


static int g_iCreateTransformKeys[2];
static float g_CreateTransformPercent;
static bool g_bRecurseFlipZ;
static Model *g_pRecurseModel;
static Model *g_pRecurseChildModel;
static LTMatrix *g_pRecurseTransforms;
static ModelAnim *g_pRecurseAnim;
static CMoArray<LTMatrix> *g_pNodeRelation;
static NodeControlFn g_pNC;
static void *g_pNCUserData;

extern void* dalloc(unsigned long size);
extern void* dalloc_z(unsigned long size);
extern void dfree(void *ptr);


#include <vector> // stl.
#include <set>

// talon: 2001's model_ops has two functions, model_RotateAnim (0x431f2c) and
// model_SetAnimFramerate (0x431ff4); Build 69's model_SetAnimTrans,
// model_OffsetAnimTrans, model_GetDimsFromBounding and the OBB generator
// (ExtentUtil, GenerateOBBFromGeom, model_CreateDefaultOBB) are later.

void model_RotateAnim(Model *pModel, uint32 iAnim, LTVector *pRot)
{
	uint32 i;
	LTMatrix mat[3];
	NodeKeyFrame *pKey;
	ModelAnim *pAnim;

	pAnim = pModel->GetAnimCheck(iAnim);
	if(!pAnim)
		return;

	gr_SetupMatrixEuler(mat[0].m, VEC_EXPAND(*pRot));

	// Translate it.
	for(i=0; i < pAnim->m_KeyFrames; i++)
	{
		pKey = &pAnim->GetRootNode()->m_KeyFrames[i];
		pKey->m_Quaternion.ConvertToMatrix(mat[1]);
		mat[2] = mat[0] * mat[1];
		pKey->m_Quaternion.ConvertFromMatrix(mat[2]);
	}
}


// talon: 2001's takes no tagged-key list (cdecl, three arguments: the
// framerate is 0x10(%ebp), 0x431ff8) and keeps a keyframe only for its
// string.
void model_SetAnimFramerate(Model *pModel, unsigned long iAnim, float framerate)
{
	unsigned long i, curMarkerTime, deltaTime;
	ModelAnim *pAnim;
	AnimKeyFrame *pKeyFrame;


	deltaTime = (unsigned long)(framerate * 1000.0f);
	if(deltaTime == 0)
		return;

	pAnim = pModel->GetAnimCheck(iAnim);
	if(!pAnim)
		return;

	if(pAnim->m_KeyFrames > 0)
	{
		curMarkerTime = pAnim->m_KeyFrames[0].m_Time;

		// Remove all keyframes until we encounter one with delta time > deltaTime.
		for(i=1; i < (pAnim->m_KeyFrames-1); i++)
		{
			pKeyFrame = &pAnim->m_KeyFrames[i];

			// Get rid of this keyframe if it's too close and doesn't have a keystring.
			if(pKeyFrame->m_Time < (curMarkerTime + deltaTime) &&
				(!pKeyFrame->m_pString || pKeyFrame->m_pString[0] == 0))
			{
				pAnim->RemoveKeyFrame(i);
				--i;
			}
			else
			{
				curMarkerTime = pKeyFrame->m_Time;
			}
		}
	}
}


#ifdef DIRECTEDITOR_BUILD
// talon: DEdit's model_ops has Build 69's model_GetDimsFromBounding too
// (0x43a144, after model_load's code): the old model dims reader
// (DEdit/LoadModelDims.cpp, 0x42da61) calls it.  ModelEdit's has not.

// {BP 1/20/98}
// Moved this code to this function because it was used in a couple places...
void model_GetDimsFromBounding( LTVector *pvBoundMax, LTVector *pvBoundMin, LTVector *pvDims )
{
	LTVector vDimsMax;

	// Set the model dims from the first frame of the first animation bounding box...
	// Not sure if BoundMin is always negative and BoundMax is always positive,
	// so got to do some absolute values...
	pvDims->x = ( pvBoundMin->x > 0.0f ) ? pvBoundMin->x : -pvBoundMin->x;
	pvDims->y = ( pvBoundMin->y > 0.0f ) ? pvBoundMin->y : -pvBoundMin->y;
	pvDims->z = ( pvBoundMin->z > 0.0f ) ? pvBoundMin->z : -pvBoundMin->z;
	vDimsMax.x = ( pvBoundMax->x > 0.0f ) ? pvBoundMax->x : -pvBoundMax->x;
	vDimsMax.y = ( pvBoundMax->y > 0.0f ) ? pvBoundMax->y : -pvBoundMax->y;
	vDimsMax.z = ( pvBoundMax->z > 0.0f ) ? pvBoundMax->z : -pvBoundMax->z;
	if( pvDims->x < vDimsMax.x )
		pvDims->x = vDimsMax.x;
	if( pvDims->y < vDimsMax.y )
		pvDims->y = vDimsMax.y;
	if( pvDims->z < vDimsMax.z )
		pvDims->z = vDimsMax.z;
}
#endif
