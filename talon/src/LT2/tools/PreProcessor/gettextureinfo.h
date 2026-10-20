
// This module sets up a list of texture info structures for each texture in a level,
// then you can search the list for info on each texture.

#ifndef __GETTEXTUREINFO_H__
#define __GETTEXTUREINFO_H__


	// talon: the 2001 texture light, from its constructor (inlined at
	// 0x413535) and the LIGHTDEFS parser. The names are ours.
	class TLightDef
	{
		public:

					TLightDef()
					{
						m_bCentered			= FALSE;
						m_bDirectional		= FALSE;
						m_VOffset			= 0.0f;
						m_UOffset			= 0.0f;
						m_ZOffset			= 0.0f;
						m_Radius			= 100.0f;

						m_InnerColor.Init(255, 255, 255);
						m_OuterColor.Init(0, 0, 0);
					}

			//only created at the center of the polygon
			BOOL	m_bCentered;

			//a directional light, not an omni
			BOOL	m_bDirectional;

			//UV coordinates of lights.
			PReal	m_UOffset;
			PReal	m_VOffset;

			//the amount to extrude texture light along normal of polygon
			PReal	m_ZOffset;

			PReal	m_Radius;

			PVector	m_InnerColor;
			PVector	m_OuterColor;
	};


	class TInfo : public CGLLNode
	{
		public:

					TInfo()
					{
						m_pTextureName = "";
					}

					~TInfo()
					{
						DeleteAndClearArray(m_LightDefs);
					}

			uint32					m_Width, m_Height;
			uint16					m_Flags;
			char					*m_pTextureName;
			CMoArray<TLightDef*>	m_LightDefs;
	};


	// Builds the list of TInfos for the textures.
	uint32 GetWorldTextureInfo(CPreWorld *pWorld, CGLinkedList<TInfo*> &theList);
	uint32 GetTextureInfo(CPreMainWorld *pWorld, CGLinkedList<TInfo*> &theList);

	// Find a TInfo in the list.
	TInfo* FindTInfo(CGLinkedList<TInfo*> &theList, const char *pTextureName);


#endif  // __GETTEXTUREINFO_H__






