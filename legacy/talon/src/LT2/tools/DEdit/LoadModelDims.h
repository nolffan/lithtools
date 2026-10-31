// LoadModelDims.h - the 2001 model dims reader (0x42da61).  The module and
// function names are ours (talon); its strings sit between load_pcx's and
// ltquatbase's, so the 2001 file sorts between them.

#ifndef __LOADMODELDIMS_H__
#define __LOADMODELDIMS_H__

	// Reads a model's dimensions: from the "Dimensions" chunk (or the first
	// animation's first keyframe box) of an old "MonolithExport Model File
	// v6", else by loading the model and taking its first animation's dims.
	// Called by CProjectBar::GetModelDims (0x4048b0).
	BOOL GetModelDimsFromFile(DStream *pStream, CVector *pDims);

#endif
