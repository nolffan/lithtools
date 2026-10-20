//------------------------------------------------------------------
//
//	FILE	  : PreSurface.h
//
//	PURPOSE	  : Defines the CPreSurface class, which is a surface description
//              (basically a polygon without the vertices) for world polies.
//
//	CREATED	  : February 12 1997
//
//	COPYRIGHT : Microsoft 1997 All Rights Reserved
//
//------------------------------------------------------------------

#ifndef __PRESURFACE_H__
	#define __PRESURFACE_H__

	#include "PreTexture.h"

	class CPrePlane;
	class CPrePoly;
	class CEditPoly;
	class CEditBrush;

	// The 2001 class, laid out from its constructor (0x4306e0),
	// SetupLMVectors (0x430790) and the code that reads it. m_Unknown_*
	// are members whose use is not known yet.
	class CPreSurface : public CGLLNode
	{
		public:

							CPreSurface();

			BOOL			ShouldLightmap() const;
			void			SetupLMVectors();

			// Brushes with a detail level: BspGen puts them below the
			// normal brushes, BrushToWorld lets them touch normal brushes
			// face to face without counting as intersecting.
			BOOL			IsDetail() const;

			inline uint32	GetLMPlaneIndex() const		{return m_Flags >> 29;}

		public:

			// The brush it came from (hull making reports it as a problem brush).
			CEditBrush		*m_pBrush;
			uint32			m_Unknown_0c[5];

			// Vectors defining LIGHTMAP texture space on this surface.
			// These are the INVERSE texture vectors (just like in the de_world structures).
			// ie: these are what are used to calculate texture coordinates.
			PVector			P, Q;

			// These are the texture vectors with inverse magnitude.
			PVector			InverseP, InverseQ;

			uint32			m_Unknown_50[13];

			const char		*m_pTextureName;

			// Plane for this surface.
			CPrePlane		*m_pPlane;

			// talon: the texture's flags (gettextureflags); the 2001 surface has
			// no lightmap grid size of its own.
			uint16			m_TextureFlags;

			// Surface flags (flags defined in surface.h).
			uint32			m_Flags;

			// Index of the surface into the used surface list.. only used during saving.
			uint32			m_UseIndex;

			uint8			m_bDetail;

			//the ambient light of the surface (RGB)
			uint8			m_Ambient[3];

			// Used when saving to get rid of unused surfaces.
			uint8			m_bUsed;

			uint32			m_Unknown_a0;
			uint32			m_Unknown_a4;
	};


#endif  // __PRESURFACE_H__

