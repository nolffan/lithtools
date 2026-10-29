// talon: the 2001 lightmap maker (0x415770..0x41e020), written from the
// image with Build 69's as a guide. Its layout is the constructor's
// (0x415930, called in DoProcessing on a stack object of 0x19c bytes); the
// names of members are ours where Build 69 has nothing like them, and
// m_Unknown_* are members whose use is not known yet.

#ifndef __LIGHTMAP_MAKER_H__
#define __LIGHTMAP_MAKER_H__

	#include "bdefs.h"
	#include "preworld.h"
	#include "classbind.h"
	#include "world_tree.h"
	#include "iltprelight.h"
	#include "prelightmap.h"
	#include <map>
	#include <string>
	#include "worldtreehelper.h"

	class CEditRegion;
	class TLightDef;
	class CBaseEditObj;


	// A light (talon: the 2001 one, 0x4c bytes, from its constructor
	// inlined in GetLightDefs and SetupFrameLights).
	class CLightDef
	{
	public:

						CLightDef()
						{
							m_InnerColor.Init(0.0f, 0.0f, 0.0f);
							m_OuterColor.Init(0.0f, 0.0f, 0.0f);
							m_Pos.Init(0.0f, 0.0f, 0.0f);
							m_MaxDist = m_MaxDistSqr = 0.0f;
							m_bClip = TRUE;
							m_FOV = MATH_HALFPI;
							m_BrightScale = 1.0f;
							m_pObject = NULL;
						}

		PVector			m_Direction;
		PVector			m_Pos;
		PVector			m_InnerColor;	// Colors.
		PVector			m_OuterColor;
		PReal			m_FOV;			// Of a directional light, in radians.
		PReal			m_BrightScale;
		PReal			m_MaxDist;
		PReal			m_MaxDistSqr;
		BOOL			m_bClip;		// Should this light clip against walls?
		BOOL			m_bDirectional;
		CBaseEditObj	*m_pObject;
	};


	// Lights by name (0x484ee8, LightMapMaker_p1.cpp; the name is ours).
	typedef std::map<std::string, CLightDef*> TLightNameMap;
	extern TLightNameMap g_LightsByName;


	// A world model as the lighting sees it: in the lightmap maker's world
	// tree, so lights can find the world models they touch (0x415860).
	class LMMWorld : public WorldTreeObj
	{
	public:

						LMMWorld();

		// WorldTreeObj.
		virtual void	GetBBox(LTVector *pMin, LTVector *pMax)
		{
			*pMin = m_vMin;
			*pMax = m_vMax;
		}

	public:

		CPreWorld		*m_pWorld;

		BOOL			m_bUsingTransform;
		LTMatrix		m_mTransform;
		LTMatrix		m_mInverseTransform;

		// The world model's box.
		PVector			m_vMin, m_vMax;
	};


	// The ILTMath the game code is handed (mathlt_impl, 0x424210..).
	class CLTMath : public ILTMath
	{
	};


	// talon: a WorldTreeHelper for its world tree (the frame code), and the
	// ILTPreLight the objects' PreLight functions get.
	class CLightMapMaker : public WorldTreeHelper, public ILTPreLight
	{
	public:

							CLightMapMaker();
							~CLightMapMaker();

		// Called from DoProcessing (0x415cb0).
		BOOL				Init(CPreMainWorld *pWorld, BOOL bClipLight,
								void (*ProgressFn)(float fPercent));

		void				Term();									// 0x416050

		// Lights the world: the base light animation, then the objects'
		// (0x4169b0, the name is ours).
		BOOL				Process();

		// m_WorldTree's subdivision (0x416030).
		static LTBOOL		CreateNodesCB(WorldTree *pTree, WorldTreeNode *pNode, void *pUser);

		// The sunlight on a surface facing dir (0x416160).
		PVector				CalcSunLightSample(PVector dir);

		// Sizes pMap for pPoly and fills it with its ambient light; FALSE if
		// the poly isn't lightmapped (0x416270).
		BOOL				InitPolyLightmapFill(CPrePoly *pPoly, CFullLightMap *pMap);

		BOOL				SetupNewLightAnim(const char *pName);	// 0x416590
		BOOL				AddAnimFrame();							// 0x416810

		// Runs the objects' PreHook_Light (0x416a40, the name is ours).
		BOOL				DoObjectPreLights();

		// The threads' function (0x416b00; P3 starts the threads with it).
		static void			ThreadCB(void *pUser);

		void				ThreadMain();
		BOOL				ThreadLightPoly(uint32 iPoly);
		BOOL				ThreadGetNextJob(uint32 &iPoly);
		void				ThreadSignalError();


		// Adds a light's color to a poly's full lightmap (0x417180, the name
		// is ours). pPositions holds the world position of each texel.
		BOOL				ApplyLightToPoly(CLightDef *pLight, CPrePoly *pPoly, CMoArray<PVector> *pPositions);

		// Adds the StaticSunLight to a poly's full lightmap (0x418260).
		BOOL				ApplySunLightToPoly(CPrePoly *pPoly, CMoArray<PVector> *pPositions);

		// Whether the sun reaches vPos (0x418610): nothing between it and
		// the sky, or what is between is sky.
		BOOL				DoesSunLightHit(const PVector &vPos);

		// Lights the current frame's polies with the threads (0x418b70).
		BOOL				CalculateLightFrame();

		// The poly's lightmap size, from its texture size (0x418ea0).
		void				GetPolyLightmapSize(CPrePoly *pPoly);

		// Marks every poly as in no light animation yet (0x418f80, the name is ours).
		void				ClearPolyAnimRefs();

		// m_VertNormals, for each world model (0x418fb0, the name is ours).
		BOOL				SetupVertNormals();

		// The poly's vertex colors, packed (0x419bd0, the name is ours).
		BOOL				GetPolyVertexColors(CPrePoly *pPoly, CMoArray<uint32> &colors);

		// Adds a light for a texture light on pPoly at pos (0x419cf0).
		void				AddLightDef(TLightDef *pLightDef, PVector pos, CPrePoly *pPoly);

		// Lights the poly's vertices (0x419340).
		BOOL				LightPolygon(CPrePoly *pPoly);

		// The texture lights: a light for each LIGHTDEFS light on each poly
		// (0x419e20).
		void				GetTextureLights();

		// Reads the Light, DirLight and StaticSunLight objects (0x41a870).
		void				GetLightDefs();

		// The sunlight, ambient and lights of a light animation frame
		// (0x41af00, the name is ours).
		BOOL				SetupFrameLights(PreLightAnimFrameInfo *pFrame);

		BOOL				ShouldBlockLight(CPreWorld *pWorld);			// 0x41b240

		// Places the world models as a frame has them (0x41b2d0).
		BOOL				SetupWorldModels(PreWorldModelInfo *pInfos, uint32 nInfos);

		void				ClearLightDefs();								// 0x41b960

		BOOL				LightTouchesPoly(CLightDef *pLight, CPrePoly *pPoly);	// 0x41bd50

		// Loads object.lto into m_hClassModule (0x41cec0, the name is ours).
		BOOL				LoadClassModule();


		// Clamps each color to 255 (0x41be50).
		void				ClampMapValues(CFullLightMap *pMap, uint32 nSamplesU, uint32 nSamplesV);

		// Averages each 2x2 of samples (0x41bed0).
		void				DownSample(CFullLightMap *pIn, CFullLightMap *pOut);

		// Packs the samples and compresses them into pOut (0x41c070).
		BOOL				ConvertMap(CFullLightMap *pIn, CPreLightMap *pOut);

		// The light at a vertex: the ambient light and every light that
		// reaches it, clipped when bClip (0x41ba10, the name is ours).
		void				LightVertex(CPolyVert *pVert, BOOL bClip);

		// Where the poly's lightmap samples are, moved out of walls (0x41c160,
		// the name is ours).
		BOOL				CalcSamplePositions(CPrePoly *pPoly, uint32 nSub, CMoArray<PVector> *pPositions);

		// Removes the current animation's polies that are black in every frame
		// (0x41cf50).
		BOOL				RemoveExcessPolies();

		// WorldTreeHelper.
		virtual uint32		IncFrameCode();									// 0x41d380
		virtual uint32		GetFrameCode();									// 0x41d390

		// talon: two virtuals of its own after WorldTreeHelper's (0x41d6b0,
		// 0x41d6e0), as ILTMath's first two. The names are ours.
		virtual LTRESULT	GetRotationVectors(LTRotation &rot, LTVector &right, LTVector &up, LTVector &forward);
		virtual LTRESULT	InterpolateRotation(LTRotation &rDest, LTRotation &rot1, LTRotation &rot2, float t);

		// ILTPreLight.
		virtual LTRESULT	CreateLightAnim(const char *pName, PreLightAnimFrameInfo *pFrames,
							uint32 nFrames, LTBOOL bShadowMap);			// 0x41d3a0
		virtual LTRESULT	FindObject(const char *pName, HPREOBJECT &hObject);	// 0x41d500
		virtual LTRESULT	GetClassName(HPREOBJECT hObject, char *pClassName, uint32 maxCount);	// 0x41d570
		virtual LTRESULT	GetPropGeneric(HPREOBJECT hObject, const char *pName, GenericProp *pProp);	// 0x41d5d0
		virtual LTRESULT	Parse(ConParse *pParse);						// 0x41d650
		virtual LTRESULT	CPrint(const char *pMsg, ...);					// 0x41d670
		virtual LTRESULT	GetWorldModelRuntimePos(HPREOBJECT hObj, LTVector &vPos);	// 0x41d710

		// Frees object.lto (0x41cf30, the name is ours).
		void				UnloadClassModule();

	public:

		// (0x00 WorldTreeHelper, 0x04 ILTPreLight, 0x08 ILTPreLight::m_pMathLT.)

		// The threads' critical section (thd_CreateCriticalSection).
		void					*m_ThreadCS;				// 0x0c
		uint32					m_iCurThreadPoly;			// 0x10
		BOOL					m_bThreadError;				// 0x14

		// object.lto, for the objects' PreLight functions.
		HCLASSMODULE			m_hClassModule;				// 0x18

		CPreMainWorld			*m_pWorld;					// 0x1c

		WorldTree				m_WorldTree;				// 0x20
		CMoArray<LMMWorld>		m_LMMWorlds;				// 0x9c
		CMoArray<CPrePoly*>		m_Polies;					// 0xb0

		// Init's third argument: DoProcessing passes a function (0x43f110),
		// the progress (ThreadGetNextJob).
		void					(*m_ProgressFn)(float fPercent);	// 0xc4
		PVector					m_AmbientLight;				// 0xc8
		BOOL					m_bClipLight;				// 0xd4

		// The StaticSunLight's.
		BOOL					m_bStaticSunLight;			// 0xd8
		PVector					m_SunLightInner;			// 0xdc
		PVector					m_SunLightOuter;			// 0xe8
		PVector					m_SunLightDir;				// 0xf4
		PReal					m_SunLightBrightScale;		// 0x100
		PReal					m_SunLightBias;				// 0x104
		// Half a lightmap texel back, when oversampling (Init; the names are
		// ours).
		PReal					m_ShiftU;					// 0x108
		PReal					m_ShiftV;					// 0x10c
		BOOL					m_bOverSample;				// 0x110

		CMoArray<CLightDef*>	m_LightDefs;				// 0x114
		CMoArray<CFullLightMap*>	m_FullLightMaps;		// 0x128
		CFullLightMap			m_Unknown_13c;
		CPreLightAnim			*m_pCurAnim;				// 0x174
		CPreLightFrame			*m_pCurFrame;				// 0x178
		PReal					m_fSunRayLength;			// 0x17c
		// Each world model's vertex normals (the average of the normals of
		// the polies on each point), by point index (the name is ours).
		CMoArray< CMoArray<PVector> >	m_VertNormals;		// 0x180
		// WorldTreeHelper.
		uint32					m_FrameCode;				// 0x194

		CLTMath					m_MathLT;					// 0x198
	};


#endif  // __LIGHTMAP_MAKER_H__
