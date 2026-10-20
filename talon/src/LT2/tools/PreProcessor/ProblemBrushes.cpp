// talon: a module of its own in the image, between PreWorld and Processing:
// the lists the brush checks fill and SelectProblemBrushes and
// SelectOutOfBoundsBrushes select (the names are ours).

#include "bdefs.h"
#include "processing.h"

CMoArray<CEditBrush*>	g_ProblemBrushes;
CMoArray<CEditBrush*>	g_OutOfBoundsBrushes;



// ------------------------------------------------------ //
// Processing.cpp - does all the main processing for
// the preprocessor.
// ------------------------------------------------------ //
