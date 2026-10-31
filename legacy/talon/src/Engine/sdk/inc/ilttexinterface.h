/*! The ILTTexture interface allows the game to modify textures. */

#ifndef __ILTTEXINTERFACE_H__
#define __ILTTEXINTERFACE_H__

#ifndef __LTBASETYPES_H__
#include "ltbasetypes.h"
#endif

#ifndef __LTMODULE_H__
#include "ltmodule.h"
#endif

#ifndef __LTBASETYPES_H__
#include "ltbasetypes.h"
#endif

/*!  Some of these types are not supported on all platforms.  For the
PS2, the \b A value represents a value between 0.0 and 2.0 where 128 =
1.0 and 255 = 2.0 */


enum ETextureType
{
    TEXTURETYPE_INVALID     = 0,    // an error
    TEXTURETYPE_ARGB8888    = 1,    // PC Only
    TEXTURETYPE_ABGR8888    = 2,    // PS2 Only 
    TEXTURETYPE_ARGB8888_P8 = 3,    // PC Only
    TEXTURETYPE_ABGR8888_P8 = 4,    // PS2 Only
    TEXTURETYPE_ARGB4444    = 5,    // PC Only
    TEXTURETYPE_ARGB1555    = 6,    // PC Only
    TEXTURETYPE_RGB565      = 7,    // PC Only
    TEXTURETYPE_DXT1        = 8,    // PC Only
    TEXTURETYPE_DXT3        = 9,    // PC Only
    TEXTURETYPE_DXT5        = 10,   // PC Only
};

enum ETexRefType
{
	TEXREFTYPE_SINGLE		= 0,
	TEXREFTYPE_DETAIL		= 1,
	TEXREFTYPE_DETAILALPHA	= 2,
	TEXREFTYPE_ENVMAP		= 3,
	TEXREFTYPE_ENVMAPALPHA	= 4
};

enum ETextureMod {
    TEXTURE_DATACHANGED           = 0,
    TEXTURE_PALETTECHANGED         = 1,
    TEXTURE_DATAANDPALETTECHANGED  = 2,
};

 
const uint32 TEXTUREFLAG_FULLBRIGHT     = (1<<0);
const uint32 TEXTUREFLAG_PREFER16BIT    = (1<<1);
const uint32 TEXTUREFLAG_NOSYSCACHE     = (1<<6);       // Tells the engine  to not keep a system memory copy of the texture.
const uint32 TEXTUREFLAG_PREFER4444     = (1<<7);       // If in 16-bit mode, use a 4444 texture for this.
const uint32 TEXTUREFLAG_PREFER5551     = (1<<8);       // Use 5551 if 16-bit.
const uint32 TEXTUREFLAG_32BITSYSCOPY   = (1<<9);       // If there is a sys copy - don't convert it to device specific format (keep it 32 bit).


class   SharedTexture;
typedef SharedTexture* HTEXTURE;


/*!  
The ILTTexInterface interface exposes texture manipulation functionality 
to the game code. It enables direct access to texture data. This 
direct access means that the texture won't be converted to a new format, 
and will therefore result in faster texture rendering.

Define a holder to get this interface like this:
\code
define_holder(ILTTexInterface, your_var);
\endcode
*/

class ILTTexInterface : public IBase {
public:
    //set the interface version number
    interface_version(ILTTexInterface, 0);

/*!  
\param hTexture     (return) Texture handle.
\param pFilename    Texture filename.

\result \b LT_ERROR - Texture not found.
\result \b LT_OK - Texture found.

Retrieves a handle to an existing texture given a filename.     

Used for: Special FX.
*/
    virtual LTRESULT FindTextureFromName(HTEXTURE &hTexture, 
        const char *pFilename) = 0;

/*!
\param hTexture     The address of the converted texture.
\param pFilename    Points to a char array containing the relative path to the resource directory in which the .DTX file is located.

\return \b LT_MISSINGFILE - \em pFilename is invalid (can't find file).
\return \b LT_ERROR - Internal error.
\return \b LT_OK - Successful.

Creates a texture from a .DTX file. A .DTX file is created using the DEdit tool (see the Game Content Creation Guide).
If the .DTX file was created with the DTX_32BITSYSCOPY bit set, the texture will not be converted (the system memory copy will remain RGBA_8888).
To determine the format type of the new texture, use ILTTexInterface::GetTextureType.

Used for: Special FX.
*/
    virtual LTRESULT CreateTextureFromName(HTEXTURE &hTexture, 
        const char *pFilename) = 0;

/*!
\param hTexture                 (return) Texture handle.
\param eTextureType             Type (see TEXTURETYPE_ enum, above).
\param TextureFlags             Flags (see TEXTUREFLAG_ values, above).
\param pData                    Pointer to the texture data.
\param nWidth                   Width.
\param nHeight                  Height.
\param nAutoGenMipMaps          Request auto creation of nAutoGenMipMaps 
                                number of mip maps. They will be down 
                                sampled. Set this param to 0 to autogen 
                                all the way down to 1x1.

\return \b LT_ERROR - Texture could not be created.
\return \b LT_OK - Successful.
        
Creates a texture from the passed in data.

Used for: Special FX.
*/
    virtual LTRESULT CreateTextureFromData(HTEXTURE &hTexture,
        ETextureType eTextureType, uint32 TextureFlags, 
        uint8 *pData, uint32 nWidth, uint32 nHeight,
        uint32 nAutoGenMipMaps = 1) = 0;

/*!
\param hTexture         (return) Texture handle.
\param eTextureType     Type (see TEXTURETYPE_ enum, above).
\param TextureFlags     Flags (see TEXTUREFLAG_ values, above).
\param pData            Pointer to the texture data.
\param pPalette         Pointer to the texture palette.
\param nWidth           Width.
\param nHeight          Height.
\param nAutoGenMipMaps  Request auto creation of nAutoGenMipMaps number of 
                        mip maps. They will be down sampled. Set this param 
                        to 0 to autogen all the way down to 1x1.

\return \b LT_ERROR - Texture could not be created.
\return \b LT_OK - Successful.
        
Creates a texture from the passed in data and palette.

Used for: Special FX.
*/
    virtual LTRESULT CreateTextureFromData(HTEXTURE &hTexture,
        ETextureType eTextureType, uint32 TextureFlags, 
        uint8 *pData, uint8 *pPalette, uint32 nWidth, uint32 nHeight,
        uint32 nAutoGenMipMaps = 1) = 0;

/*!
\param hTexture         (return) Texture handle.
\param eTextureType     Type (see TEXTURETYPE_ enum, above).
\param TextureFlags     Flags (see TEXTUREFLAG_ values, above).
\param nWidth           Width.
\param nHeight          Height.
\param nAutoGenMipMaps  Request auto creation of nAutoGenMipMaps number of 
                        mip maps. Set this param to 0 to autogen all the way 
                        down to 1x1.

\return \b LT_OK - Successful.
        
Creates an empty texture

Used for: Special FX.
*/
    virtual LTRESULT CreateEmptyTexture(HTEXTURE &hTexture,
        ETextureType eTextureType, uint32 TextureFlags, 
        uint32 nWidth, uint32 nHeight, uint32 nAutoGenMipMaps = 1) = 0;


/*!
\param hTexture         Texture handle.
\param nAutoGenMipMaps  Request auto creation of nAutoGenMipMaps number of 
                        mip maps. They will be down sampled. Passing Zero 
                        does all that mips that have been created.

\return \b LT_OK - Successful.

Auto-Generate mipmaps. Note: You can only generate mipmaps that have 
already been created.
        
Used for: Special FX.
*/
    virtual LTRESULT AutoGenMipMaps(const HTEXTURE hTexture,
        uint32 nAutoGenMipMaps = 1) = 0;

/*!
\param hTexture     Texture handle.
\param bReadOnly    Parameter to indicates whether the data is read only or read/write. To modify the texture data, bReadOnly must be set to false.
\param pData        (return) Texture pixel data pointer
\param nPitch       (return) Size (in bytes) of a row of pixel data.
\param nMipMap      The mipmap you would like to lock.

\return \b LT_OK - Successful.

Fetch a pointer to the texture RGB raw data.  You can modify this as
you see fit. Howerver if you do not lock it as readonly, you must do a 
FlushTextureData call before anything that uses the texture tries to 
render.  For efficiency, you are given direct access to the texture 
data.  The effect of trying to render before flushing the data is 
platform dependant, however, is most likely not what you would desire.
        
Used for: Special FX.
*/
    virtual LTRESULT GetTextureData(const HTEXTURE hTexture,
        bool bReadOnly, uint8* &pData,  uint32 &nPitch, 
        uint32 nMipMap = 0) = 0;

/*!
\param hTexture     Texture handle.
\param bReadOnly    Parameter to indicates whether the data is read only or read/write. To modify the texture data, bReadOnly must be set to false.
\param pData        (return) Texture pixel data pointer
\param pPalette     (return) Texture palette pointer
\param nPitch       (return) Size (in bytes) of a row of pixel data.
\param nMipMap      The mipmap you would like to lock (base 0).

\return \b LT_OK - Successful.
        
Fetch a pointer to the texture RGB raw data.  You can modify this as
you see fit. Howerver if you do not lock it as readonly, you must do a 
FlushTextureData call before anything that uses the texture tries to 
render.  For efficiency, you are given direct access to the texture 
data.  The effect of trying to render before flushing the data is 
platform dependant, however, is most likely not what you would desire.

Used for: Special FX.
*/
    virtual LTRESULT GetTextureData(const HTEXTURE hTexture,
        bool bReadOnly, uint8* &pData, uint8* &pPalette, 
        uint32 &nPitch, uint32 nMipMap = 0) = 0;

/*!
\param hTexture     Texture handle.
\param eChanged     Indicate is the texture pixel data and/or palette has changed.
\param nMipMap      Mipmap to unlock.

\return \b LT_OK - Successful.

Let the engine know that we are finished modifing the texture

Used for: Special FX.
*/
    virtual LTRESULT FlushTextureData (const HTEXTURE hTexture, 
        ETextureMod eChanged = TEXTURE_DATAANDPALETTECHANGED,
        uint32 nMipMap = 0) = 0;

/*!
\param hTexture     Texture handle.
\param nWidth       (return) Texture width.
\param nHeight      (return) Texture height.

\return \b LT_OK - Successful.

Get information about the texture.
        
Used for: Special FX.
*/
    virtual LTRESULT GetTextureDims(const HTEXTURE hTexture, 
        uint32 &nWidth, uint32 &nHeight) = 0;

/*!
\param hTexture     Texture handle.
\param TextureFlags (return) The textures flags.

\return \b LT_OK - Successful.
        
Get information about the texture.

Used for: Special FX.
*/
    virtual LTRESULT GetTextureFlags(const HTEXTURE hTexture, 
        uint32 &TextureFlags) = 0;

/*!
\param hTexture     Texture handle.
\param eTextureType (return) The texture type.

\return \b LT_OK - Successful.
        
Get information about the texture.

Used for: Special FX.
*/
    virtual LTRESULT GetTextureType(const HTEXTURE hTexture, 
        ETextureType &eTextureType) = 0;

/*!
\param hTexture     Texture handle.
\param nMinMipMaps  (return) The smallest mip map to use for the texture. The value is actually a number of mip maps from the base texture (which has a value of 0).For example, the third mip map for a texture has a value of 3.
\param nMaxMipMaps  (return) The largest mip map to use for the texture. The value is actually a number of mip maps from the base texture.

\return \b LT_OK - Successful.
        
Get information about the texture.
        
Used for: Special FX.
*/
    virtual LTRESULT GetTextureMipMapRange(const HTEXTURE hTexture, 
        uint32 &nMaxMipMaps,uint32 &nMinMipMaps) = 0;

/*!
\param hTexture     Texture handle.
\param nMinMipMaps  Sets the smallest mip map to use for the texture. The value is actually a number of mip maps from the base texture (which has a value of 0).For example, the third mip map for a texture has a value of 3.
\param nMaxMipMaps  Sets the largest mip map to use for the texture. The value is actually a number of mip maps from the base texture.

\return \b LT_OK - Successful.
        
Set the texture's range of useable mipmaps. You may not set the range beyond the number of 
currently created mipmaps.
        
Used for: Special FX.
*/
    virtual LTRESULT SetTextureMipMapRange(const HTEXTURE hTexture, 
        uint32 nMaxMipMaps,uint32 nMinMipMaps) = 0;

/*!
\param hTexture     Texture handle.
\param nMipMaps     (return) Number of created mipmaps.

\return \b LT_OK - Successful.
        
Get information about the texture.

Used for: Special FX.
*/
    virtual LTRESULT GetTextureNumMipMaps(const HTEXTURE hTexture, 
        uint32 &nMipMaps) = 0;

/*!
\param hTexture         Texture handle.
\param hRefTex          (return) Detail texture handle.
\param eDetailTexType   (return) Detail texture type.

\return \b LT_OK - Successful.

Get a new texture handle which is a reference to the detail texture
that the passed in texture reference is using
        
Used for: Special FX.
*/
    virtual LTRESULT GetDetailTexture(const HTEXTURE hTexture,
        HTEXTURE &hRefTex, ETexRefType &eDetailTexType) = 0;

/*!
\param hTexture         Texture handle.
\param hRefTex          Detail texture handle.
\param eDetailTexType   Detail texture type.

\return \b LT_OK - Successful.

Sets the detail texture for the passed in texture reference
        
Used for: Special FX.
*/
    virtual LTRESULT SetDetailTexture(HTEXTURE &hTexture,
        const HTEXTURE hRefTex, ETexRefType eDetailTexType) = 0;

/*!
\param hTexture     Texture handle.

\return \b LT_OK - Successful.

Returns a Boolean value indicating whether or not the texture was rendered in the last frame. If it returns false, the texture is not in camera and need not be updated.

Used for: Special FX.
*/
    virtual bool WasTextureDrawnLastFrame(const HTEXTURE hTexture) = 0;

/*!
\param hTexture     Texture handle.

\return \b LT_OK - Successful.

You should call ReleaseTextureHandle after you are done with your texture
so that it can be freed.  If it is not also being used by the engine, it 
will be freed.

Used for: Special FX.
*/
    virtual bool ReleaseTextureHandle(const HTEXTURE hTexture) = 0;
};


#endif  //! __ILTTEXINTERFACE_H__
