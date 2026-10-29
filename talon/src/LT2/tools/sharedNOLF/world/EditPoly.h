//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : EditPoly.h
//
//	PURPOSE	  : Defines the CEditPoly class.
//
//	CREATED	  : October 5 1996
//
//
//------------------------------------------------------------------

#ifndef __EDITPOLY_H__
	#define __EDITPOLY_H__


	// Includes....
	#include "basepoly.h"
	#include "editregion.h"
	#include "texturedplane.h"


	// Defines....
	class CRegionView;
	class CPrePoly;
	class CLTANode;
	class CLTAFile;


	// talon: the 2001 CEditPoly, 0x88 bytes (`push $0x88` at 0x40db72,
	// 0x40dda2): one texture, its fields in the poly (its constructor sets
	// the name "Default", P (1,0,0), Q (0,0,1), O (0,0,0), 0x40f858..), and
	// its triangles (0x5c, a CMoArray<CBasePoly*>, vtable 0x474884, made by
	// UpdateTriangles). Its vtable (0x474880) is CopyAttributes alone
	// (0x40fe10); the destructor is not virtual (CEditBrush::Term calls it
	// directly, 0x40d97b). The .ed order (0x40fb30, 0x40fc70, 0x40fcf0): the
	// base poly, O P Q, 0x70, the texture name, 0x84, the three bytes
	// 0x58..0x5a. m_pUser1 is at 0x7c (EditPolyToPrePoly, 0x402bd8); 0x84 is
	// OR'd into the surface flags (RaySelecter.cpp, 0x445663) and is the
	// poly's own surface flags (GenerateSurfaces); the three bytes at 0x58
	// are the surface's ambient light (GenerateSurfaces). 0x80 is never
	// initialized.
	class CEditPoly : public CBasePoly
	{
		public:

			enum	{ NUM_TEXTURES	= 2 };

								CEditPoly();
								CEditPoly( CEditPoly *pCopyFrom );
								CEditPoly( CEditBrush *pBrush );
								~CEditPoly();
		
			void				Term();


		private:

			void				CommonConstructor( );

		// Loading/saving.
		public:

			BOOL				LoadEditPolyLTA( CLTANode* pNode, CStringHolder *pStringHolder );
			void				SaveEditPolyLTA( CLTAFile* pFile, uint32 level );

			// talon: 2001 (0x40fb30): the arguments as CEditBrush's LoadTBW
			// passes them; the third is never read.
			BOOL				LoadEditPolyTBW( CAbstractIO &file, CStringHolder *pStringHolder, uint32 unused );
			void				SaveEditPolyTBW( CAbstractIO &file );	// 0x40fc70
			// talon: 2001 (0x40fcf0, Build 69's CTexturedPlane name): O, P, Q,
			// 0x70 and the texture name, which SaveEditPolyTBW writes after the
			// CBasePoly (0x40fc81) and CEditBrush::SaveTBW on its own (0x40dcfd).
			void				SaveTextureInfoTBW( CAbstractIO &file );

		// Functionality.
		public:

			void				CopyAttributes( CBasePoly *pOther, CStringHolder *pStringHolder=NULL );
			
			// Decrements all indices >= index.
			void				DecrementPoints( uint32 index );

			// Tells if and where the ray intersects the poly.
			BOOL				IntersectRay( CEditRay &ray, CReal &t, BOOL bBackface );

			BOOL				CopyEditPoly( CEditPoly *pPoly, BOOL bCopyIndices=TRUE, CStringHolder *pStringHolder=NULL );

			// talon: 2001 (0x40fec0, the name ours; called at 0x411225):
			// triangulates a copy of the poly into m_Triangles.
			void				UpdateTriangles();
			
			void				Flip();
			
			void				SetupBaseTextureSpace(uint32 nTex);

			void				SetTextureSpace(uint32 nTex, const LTVector& newO, const LTVector& newP, const LTVector& newQ);

			// sets up OPQs based on uv coordinates of the first 3 verts
			bool				SetUVTextureSpace(uint32 nTex, const float* coords, const int texWidth, const int texHeight );

			//determines if this polygon is concave or not
			bool				IsConcave();

			//determines if this polygon lies entirely within a single plane
			bool				IsCoplanar();

			//determines the surface area of the polygon
			CReal				GetSurfaceArea();

			//retrieves the normal that should be used for various texture operations
			LTVector			GetTextureNormal()			{return Normal();}

		// talon: the 2001 data (one texture; the names of the unknown
		// members by offset).
		public:

			uint32				m_Unknown_2c;
			uint32				m_Unknown_30;
			LTVector			O, P, Q;			// 0x34: the texture vectors
			uint8				m_Ambient[3];		// 0x58: the surface's ambient light
			CMoArray<CBasePoly*>	m_Triangles;	// 0x5c
			uint32				m_Unknown_70;
			char				*m_pTextureName;	// 0x74
			uint32				m_Unknown_78;

			// User data pointer .. used by the preprocessor to point to its surface.
			void				*m_pUser1;			// 0x7c
			uint32				m_Unknown_80;
			uint32				m_SurfaceFlags;		// 0x84: the poly's own surface flags
	};


	typedef CMoArray<CEditPoly*> CEditPolyArray;


#endif  // __EDITPOLY_H__



