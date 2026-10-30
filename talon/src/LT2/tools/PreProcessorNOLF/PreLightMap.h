//------------------------------------------------------------------
//
//	FILE	  : PreLightMap.h
//
//	PURPOSE	  : Defines the CPreLightMap class, which represents
//              a single light map for a polygon.
//
//	CREATED	  : September 30 1996
//
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------

// talon: the 2001 CFullLightMap, from the lightmap maker's use of it.
// CPreLightMap is NOLF's (0x43ee40..0x43f2b0; AllBlack 0x43eec0).

#ifndef __PRELIGHTMAP_H__
	#define __PRELIGHTMAP_H__


	// Includes....
	#include "bdefs.h"
	#include "preprocessorbase.h"


	// talon: the 2001 one, as far as the lightmap maker shows it (its
	// destructor, 0x41d330).
	class CPreLightMap
	{
	public:

						CPreLightMap();

		// talon: no copy in the image: icl expands it everywhere and the link
		// drops the unreferenced one.
		void			Term();

		// TRUE if every pixel is black (0x42d8b0).
		BOOL			AllBlack();

		// talon: fill it from NOLF's 16-bit (555) pixels, compressed as a
		// lightmap (0x43ef50) or as a shadow map (0x43f0a0). The names are ours.
		BOOL			CompressLightMap(uint32 width, uint32 height, uint16 *pData);
		BOOL			CompressShadowMap(uint32 width, uint32 height, uint16 *pData);

		// talon: back to pixels (0x43f280) or to a shadow map's bytes (0x43f250).
		BOOL			Decompress(uint16 *pData);
		BOOL			DecompressShadowMap(uint8 *pData);

	public:

		uint32			m_Width, m_Height;
		uint32			m_Unknown_08;
		CMoArray<uint8>	m_Data;
	};


	// Used temporarily for each polygon, then put into a packed CPreLightMap.
	class CFullLightMap
	{

		public:

								CFullLightMap()
								{
									m_Width = m_Height = 0;
								}

			void				SetSize( uint32 width, uint32 height )
			{
				if((width*height) > m_Map.GetSize())
				{
					m_Map.SetSize(width*height);
				}

				m_Width		= width;
				m_Height	= height;
			}

			// Fills the map with one color (0x42db80).
			void				ColorFill( CVector color );

		public:

			uint32				m_Width, m_Height;

			// Set when the map is only the ambient color (the name is ours).
			BOOL				m_bAmbientOnly;

			// The size of this is m_Width*m_Height.
			CMoArray<PVector>	m_Map;

			// Helper.. boundaries of the poly.
			PVector				m_Min, m_Max;

	};



#endif  // __PRELIGHTMAP_H__
