#ifndef __PROCESSING_H__
#define __PROCESSING_H__


	class CPreWorld;
	class CEditBrush;
	class CBspHullMaker;
	class CWorldNode;
	class IPackerOutput;


	// ------------------------------------------------------ //
	// Here's all the functions and variables the 
	// system-dependent stuff gets to handle.
	// ------------------------------------------------------ //

	#define MAX_THREADS					40

	// The 2001 layout: where the option parser (0x43e7e0) and the code
	// that reads them put each field. m_Unknown_* are fields whose use is
	// not known yet; the Processing module will name them.
	class CProcessorGlobs
	{
	public:

		uint32	m_Unknown_00[3];

		// Set to TRUE if it's making a world model (so the debug output can
		// be toned down a little).
		BOOL	m_bMakingWorldModel;

		// -MaxBounds (default 400000).
		PReal	m_MaxBounds;

		// These should be set by the caller.
		char	**m_Argv;
		uint32	m_Argc;
		char	m_InputFile[MAX_PATH]; // Input filename (like worlds\test1.ed).
		uint8	m_Unknown_120[0x1ac - 0x120];
		char	m_ProjectDir[0x274 - 0x1ac];

		BOOL	m_bTextureFlags;
		BOOL	m_bSelectProblemBrushes;
		BOOL	m_bSelectOutOfBoundsBrushes;
		BOOL	m_bRaySelect;
		BOOL	m_bSelectHullProblemBrushes;
		BOOL	m_bRemoveProblemObjects;
		char	m_LeakFile[256];
		char	m_PortalFile[256];
		uint32	m_Unknown_48c[3];

		BOOL	m_bPortals;				// cleared by -NoPortals
		BOOL	m_bReplaceTextures;
		BOOL	m_bBsp;
		BOOL	m_bIgnoreHidden;
		PReal	m_SplitWeight, m_BalanceWeight;
		BOOL	m_bAIVolumes;
		BOOL	m_bLight;
		BOOL	m_bClipLight;			// cleared by -nocliplight
		BOOL	m_bLightAnimations;
		BOOL	m_bVerticesOnly;		// -onlylightverts
		BOOL	m_bLambertian;
		BOOL	m_bOptimize;
		BOOL	m_bFast;
		uint32	m_MaxDetail;
		BOOL	m_bLogFile;
		uint32	m_nThreads;
		uint32	m_Unknown_4dc;
		BOOL	m_bFixTJunc;
		BOOL	m_bNoSnapNoCSG;			// NoSnap and hull maker brushes take no part in CSG
		BOOL	m_bBalanceBSPs;			// cleared by -NoBalanceBSPs

		// The processor will fill these in.
		uint32	m_nInputPolies, m_nInputVertices, m_nInputObjects;
		uint32	m_nOutputPolies, m_nOutputVertices, m_TreeDepth;
		uint32	m_LMDataSize;
	};

	extern CProcessorGlobs *g_pGlobs;


	// ------------------------------------------------------ //
	// Functions the Processing module implements.
	// ------------------------------------------------------ //

	//different types of messages that can be displayed
	enum EStatusTextType
	{
		eST_Error,		//used to signal errors that would indicate bugs in the level
		eST_Warning,	//used to indicate items that should be brought to the users attention but does not invalidate level
		eST_Normal		//used for displaying what the processor is doing
	};

	// Initializes the global structure
	void InitProcessOptions(CProcessorGlobs *pGlobs, const char* pszInputFile);

	// Do all the processing.
	void DoProcessing(CProcessorGlobs *pGlobs);

	void DrawStatusText(const char *pMsg, ...);
	// Draws text if not working on a world model.
	void DrawStatusTextIfMainWorld(const char *pMsg, ...);

	// Makes a surface in pWorld for each brush poly.
	void GenerateSurfaces(GenList<CEditBrush*> &brushes, CPreWorld *pWorld);

	//given a node, it will look at the processor settings and determine if it should be ignored
	bool ShouldIgnoreNode(const CWorldNode* pNode);


	// ------------------------------------------------------ //
	// Functions the system-dependent stuff implements.
	// ------------------------------------------------------ //

	// Functions for the back end user interface see IPackerOutput for more details
	void	ActivateTask(const char* pszTaskName);
	void	ActivateSubTask(const char* pszSubTaskName);
	void	SetProgressBar(float fPercent);

	// Question types and return values.
	#define QUES_YES		1
	#define QUES_NO			2
	#define QUES_CANCEL		4

	// Type is a combination of the above flags.
	// Returns one of the above flags.
	uint32 AskQuestion(const char *pQuestion, uint32 type);

	// Does a union on all brush geometry and converts to world geometry.
	bool MergeBrushes(
		GenList<CEditBrush*> &brushes,
		CPreWorld *pWorld);

	// Make a BSP.
	bool MakeBsp(CPreWorld *pWorld);

#endif  // __PROCESSING_H__
