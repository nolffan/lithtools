#include "bdefs.h"

#include "iltmath.h"
#include "geomroutines.h"

// talon: the 2001 one (0x424210..0x424b40), from Build 69's runtime source
// with the image's differences: ILTMath's own virtuals are the
// implementation (CLTMath, which the lightmap maker embeds, adds nothing),
// and there are SetupEuler and GetEulerAngles taking a vector, and
// GetEulerAngles (written from the image). InterpolateRotation's code is
// folded with the lightmap maker's (0x41d6e0).


LTRESULT ILTMath::GetRotationVectors(LTRotation &rot, LTVector &right, LTVector &up, LTVector &forward)
{
	gr_GetRotationVectors(&rot, &right, &up, &forward);
	return LT_OK;
}

LTRESULT ILTMath::InterpolateRotation(LTRotation &rDest, LTRotation &rot1, LTRotation &rot2, float t)
{
	gr_InterpolateRotation(&rDest, &rot1, &rot2, t);
	return LT_OK;
}

LTRESULT ILTMath::SetupEuler(LTRotation &rRot, float pitch, float yaw, float roll)
{
	gr_EulerToRotation(pitch, yaw, roll, &rRot);
	return LT_OK;
}

LTRESULT ILTMath::SetupEuler(LTRotation &rRot, LTVector &vAngles)
{
	return SetupEuler(rRot, vAngles.x, vAngles.y, vAngles.z);
}

LTRESULT ILTMath::SetupTransformationMatrix(LTMatrix &mMat, LTVector &vTranslation, LTRotation &rRot)
{
	gr_SetupTransformation(&vTranslation, &rRot, LTNULL, &mMat);
	return LT_OK;
}

LTRESULT ILTMath::SetupTranslationMatrix(LTMatrix &mMat, LTVector &vTranslation)
{
	gr_SetupTransformation(&vTranslation, LTNULL, LTNULL, &mMat);
	return LT_OK;
}

LTRESULT ILTMath::SetupRotationMatrix(LTMatrix &mMat, LTRotation &rRot)
{
	gr_SetupTransformation(LTNULL, &rRot, LTNULL, &mMat);
	return LT_OK;
}

LTRESULT ILTMath::SetupTranslationFromMatrix(LTVector &vTranslation, LTMatrix &mMat)
{
	mMat.GetTranslation(vTranslation);
	return LT_OK;
}

LTRESULT ILTMath::SetupRotationFromMatrix(LTRotation &rRot, LTMatrix &mMat)
{
	MatrixToRotation(&mMat, &rRot);
	return LT_OK;
}

LTRESULT ILTMath::SetupRotationAroundPoint(LTMatrix &mMat, LTRotation &rRot, LTVector &vPoint)
{
	LTMatrix mForward, mRotate, mBackward;
	LTVector negativePoint;

	negativePoint = -vPoint;
	SetupTranslationMatrix(mForward, vPoint);
	SetupTranslationMatrix(mBackward, negativePoint);
	SetupRotationMatrix(mRotate, rRot);

	mMat = mForward * mRotate * mBackward;
	return LT_OK;
}

LTRESULT ILTMath::AlignRotation(LTRotation &rOutRot, LTVector &vVector, LTVector &vUp)
{
	LTMatrix theMat;
	LTVector ref[3];

	// Get a frame of reference.
	gr_BuildFrameOfReference(&vVector, &vUp, &ref[0], &ref[1], &ref[2]);

	theMat.SetBasisVectors(&ref[0], &ref[1], &ref[2]);
	MatrixToRotation(&theMat, &rOutRot);
	return LT_OK;
}

LTRESULT ILTMath::EulerRotateX(LTRotation &rRot, float amount)
{
	LTVector vecs[3];

	gr_GetRotationVectors(&rRot, &vecs[0], &vecs[1], &vecs[2]);
	return RotateAroundAxis(rRot, vecs[0], amount);
}

LTRESULT ILTMath::EulerRotateY(LTRotation &rRot, float amount)
{
	LTVector vecs[3];

	gr_GetRotationVectors(&rRot, &vecs[0], &vecs[1], &vecs[2]);
	return RotateAroundAxis(rRot, vecs[1], amount);
}

LTRESULT ILTMath::EulerRotateZ(LTRotation &rRot, float amount)
{
	LTVector vecs[3];

	gr_GetRotationVectors(&rRot, &vecs[0], &vecs[1], &vecs[2]);
	return RotateAroundAxis(rRot, vecs[2], amount);
}

LTRESULT ILTMath::RotateAroundAxis(LTRotation &rRot, LTVector &vAxis, float amount)
{
	LTMatrix rotation, mat;

	// Get the quaternion.
	RotationToMatrix(&rRot, &mat);

	// Setup a rotation matrix and apply it.
	gr_SetupRotationAroundVector(&rotation, vAxis, amount);
	mat = rotation * mat;
	
	MatrixToRotation(&mat, &rRot);
	return LT_OK;
}

LTRESULT ILTMath::GetRotationVectorsFromMatrix(LTMatrix &mMat, 
		LTVector &vRight, LTVector &vUp, LTVector &vForward)
{
	mMat.GetBasisVectors(&vRight, &vUp, &vForward);
	return LT_OK;
}

// Undoes SetupEuler: the roll from the right vector against the level
// one, then, rolled back, the pitch, then the yaw.
LTRESULT ILTMath::GetEulerAngles(LTRotation &rRot, float *pitch, float *yaw, float *roll)
{
	LTRotation rot;
	LTVector vUp, vLevelUp, vRight, vLevelForward, vLevelRight, vForward;
	float fCos, fSin;

	rot = rRot;
	GetRotationVectors(rot, vRight, vUp, vForward);

	vLevelRight = vForward.Cross(LTVector(0.0f, 1.0f, 0.0f));
	vLevelRight.Norm();
	vLevelUp = vLevelRight.Cross(vForward);

	if(roll)
	{
		fCos = vRight.Dot(vLevelRight);
		fSin = vRight.Dot(vLevelUp);
		if(fCos > 1.0f) fCos = 1.0f; else if(fCos < -1.0f) fCos = -1.0f;
		if(fSin > 1.0f) fSin = 1.0f; else if(fSin < -1.0f) fSin = -1.0f;

		*roll = (float)acos(fCos);
		if(fSin < 0.0f)
			*roll = -*roll;
	}

	AlignRotation(rot, vForward, vLevelUp);
	GetRotationVectors(rot, vRight, vUp, vForward);

	vLevelUp.Init(0.0f, 1.0f, 0.0f);
	vLevelForward = vLevelUp.Cross(vRight);
	vLevelForward.Norm();

	if(pitch)
	{
		fCos = vLevelForward.Dot(vForward);
		fSin = vForward.Dot(vLevelUp);
		if(fCos > 1.0f) fCos = 1.0f; else if(fCos < -1.0f) fCos = -1.0f;
		if(fSin > 1.0f) fSin = 1.0f; else if(fSin < -1.0f) fSin = -1.0f;

		*pitch = (float)acos(fCos);
		if(fSin > 0.0f)
			*pitch = -*pitch;
	}

	AlignRotation(rot, vLevelForward, vLevelUp);
	GetRotationVectors(rot, vRight, vUp, vForward);

	if(yaw)
	{
		fCos = vForward.Dot(LTVector(0.0f, 0.0f, 1.0f));
		fSin = vForward.Dot(LTVector(1.0f, 0.0f, 0.0f));
		if(fCos > 1.0f) fCos = 1.0f; else if(fCos < -1.0f) fCos = -1.0f;
		if(fSin > 1.0f) fSin = 1.0f; else if(fSin < -1.0f) fSin = -1.0f;

		*yaw = (float)acos(fCos);
		if(fSin < 0.0f)
			*yaw = -*yaw;
	}

	return LT_OK;
}

LTRESULT ILTMath::GetEulerAngles(LTRotation &rRot, LTVector &vAngles)
{
	return GetEulerAngles(rRot, &vAngles.x, &vAngles.y, &vAngles.z);
}
