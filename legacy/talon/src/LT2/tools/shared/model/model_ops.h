
// This module defines a bunch of operations on Models.
// Note: The C style in here is on its way out.  All these functions will wind
// up in the model classes.

#ifndef __MODEL_OPS_H__
#define __MODEL_OPS_H__
#include <vector>


	// talon: 2001's two (0x431f2c, 0x431ff4); no SetAnimTrans/OffsetAnimTrans,
	// GetDimsFromBounding or default OBBs.
	void model_RotateAnim(Model *pModel, uint32 iAnim, LTVector *pRot);

	// Set the animation framerate.
	void model_SetAnimFramerate(Model *pModel, unsigned long iAnim, float framerate);

#endif  // __MODEL_OPS_H__


