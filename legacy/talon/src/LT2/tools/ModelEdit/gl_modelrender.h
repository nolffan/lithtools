
#ifndef __GL_MODELRENDER_H__
#define __GL_MODELRENDER_H__

#include "model.h"
#include "dtxmgr.h"
#include <GL/gl.h>
#include <GL/glu.h>

typedef void* GLM_CONTEXT;

#define MAX_GLM_LIGHTS		8
#define NUM_ANIM_INFOS		4	// Max number of animations that can be tracked at once.


// ------------------------------------------------------------------------
// GLMContext
// current GL rendering context. 
// ------------------------------------------------------------------------
class GLMContext
{
public:
	HWND	m_hWnd;
	HDC		m_hDC;
	HGLRC	m_hglrc;
	CMoArray<GLuint> m_Textures;
	GLuint m_nCurTexture;

	float m_bgColor[3]; // gl background clear color

} ;

// ------------------------------------------------------------------------
// DrawStruct
// Model-Scene parameters.
// ------------------------------------------------------------------------
struct DrawStruct
{
	
public:
	
	BOOL		SetupTransformMaker(TransformMaker *pMaker);

	// Returns how many (consecutive) AnimTrackers in m_Times are valid.
	DWORD		NumValidTimes();


public:

	Model*		GetModel()	{return m_Times[0]->m_TimeRef.m_pModel;}

	// talon: 2001's layout (0x15c bytes), read off gl_modelrender's and
	// CRenderWnd's uses: no LOD distance, movement encoding, OBBs, vertex
	// weights or texture switch, and the light colors at the end.

	// The exact place to draw from.
	AnimTracker	*m_Times[NUM_ANIM_INFOS];		// 0x00

	BOOL	m_bNormals;							// 0x10
	DWORD	m_iLOD;								// 0x14

	// VIEW PARAMS
	float	m_FOV;								// 0x18 (gl_modelrender doesn't read it)
	CVector	m_ViewerPos;						// 0x1c
	CVector	m_LookAt;							// 0x28

	BOOL	m_bWireframe;						// 0x34
	
	// LIGHT PARAMS
	CVector	m_LightPositions[MAX_GLM_LIGHTS];	// 0x38
	int		m_nLights;							// 0x98

	BOOL	m_bDims;							// 0x9c
	CVector m_DimsColor;						// 0xa0
	CVector m_ModelBoxColor;					// 0xac
	BOOL	m_bAnimBox;							// 0xb8
	CVector m_AnimBoxColor;						// 0xbc
	
	// --------------------------------------
	// DRAW FLAGS 
	// --------------------------------------
	// Draw the model bright (without lighting).
	BOOL	m_bDrawBright;						// 0xc8

	// Draw the skeleton.
	BOOL	m_bDrawSkeleton;					// 0xcc
	// draw the original mesh
	BOOL	m_bDrawOriginalModel;				// 0xd0

	// Show sockets and/or attachments.
	BOOL	m_bDrawSockets;						// 0xd4
	BOOL	m_bDrawAttachments;					// 0xd8
	BOOL	m_bSolidAttachments;				// 0xdc
	
	// calculate the internal radius of the model while rendering
	BOOL	m_bCalcRadius;						// 0xe0
	// filled in with the model radius if m_bCalcRadius is set
	float	m_fModelRadius;						// 0xe4
	// if set this will cause it to calculate radius AND draw the animation
	BOOL	m_bCalcAndDraw;						// 0xe8

	// Show profile info.
	BOOL	m_bProfile;							// 0xec

	BOOL	m_bShowNormalRef;					// 0xf0

	BYTE	*m_SelectedPieces; // Should be as big as Model::NumPieces().	0xf4
	BYTE	*m_SelectedNodes; // Should be as big as Model::NumNodes().	0xf8

	// RGB 0-255
	CVector	m_LightColors[MAX_GLM_LIGHTS];		// 0xfc
};

						   
// Represents a transformed vertex.
// talon: 2001's is 0x18 bytes, the position and the normal (DrawPiece,
// 0x4054f8; DrawModelPolies' vertex stride, 0x404e58).
class TVert
{
public:
	inline TVert&	operator=(TVert other)
	{
		m_vPos = other.m_vPos;
		m_vNormal = other.m_vNormal;
		return *this;
	}
	
	LTVector		m_vPos;
	LTVector		m_vNormal;
};

// ------------------------------------------------------------------------
// Free Floatting functions 
// ------------------------------------------------------------------------

// Init and term.
// Note: your window must have WS_CLIPCHILDREN and WS_CLIPSIBLINGS set.
GLM_CONTEXT CreateGLMContext(void *hWnd);
void DeleteGLMContext(GLM_CONTEXT hContext);

// The drawing function.
// Draws the model centered at the origin.
// pEyePoint is the position of the viewer.
// pLookAt is where it looks at (it should probably be (0,0,0)).
void DrawModel(GLM_CONTEXT hContext, DrawStruct *pStruct);

void DrawCoordSys(DMatrix *pMat, float arrowLength);

// ------------------------------------------------------------------------
// setup viewing parameters, sets up lookat, lights, wireframe params etc
// for gl (talon: 2001's reads the DrawStruct DrawModel was given)
void SetupViewingParameters( GLMContext *pContext );

// ------------------------------------------------------------------------
// Set the texture to draw with.
BOOL SetGLMTexture(GLM_CONTEXT hContext, TextureData *pTexture, DWORD nTextureNum = 0);

// set the background color for the gl-window.
void SetGLMBackgroundColor( GLM_CONTEXT , COLORREF );
void SetGLMBackgroundColor( GLM_CONTEXT hContext, float *color );

/* The coordinate system this uses...

				 +y
					  -z
				 ^	 /
				 |	/
				 | /
				 |/
	-x ----------|----------> +x
				/|
			   / |
			  +z |
*/

// updates the draw-buffer vertex and animation .
BOOL AlternateGetVertexPositions(	Model *pModel,
 									GVPStruct *pStruct, 
									BOOL bSetupTransforms,
									BOOL bNormals,
									BOOL bDrawOriginalModel,
									void **pNextVert = NULL );

#endif  // __GL_MODELRENDER_H__
