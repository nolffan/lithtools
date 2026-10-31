#ifndef __LTBASEDEFS_H__
#define __LTBASEDEFS_H__

#ifndef __STDLIB_H__
#include <stdlib.h>
#define __STDLIB_H__
#endif

#ifndef __STRING_H__
#include <string.h>
#define __STRING_H__
#endif

#ifndef __LTASSERT_H__
#include "ltassert.h"
#endif

#ifndef __LITHTECH_H__
#include "lithtech.h"
#endif

#ifndef __LTCODES_H__
#include "ltcodes.h"
#endif

#ifndef __COORDINATE_FRAME_H__
#include "coordinate_frame.h"
#endif

#define LTNULL 0
#define LTTRUE 1
#define LTFALSE 0

#define BEGIN_EXTERNC() extern "C" \
    {
#define END_EXTERNC() };

#if defined(__LINUX)
#define _MAX_PATH 256
#define VSPRINTF(s,n,m,e) vsprintf (s,m,e)
#else
#define VSPRINTF(s,n,m,e) _vsnprintf (s,n,m,e)
#endif

#ifdef __LINUX
inline int stricmp(const char* string1, const char* string2)
{ return strcasecmp(string1, string2); }
#include <ctype.h>
inline char* strupr(char* s)
{ while (*s) { *s = toupper(*s); ++s; } return s; }
#endif

class ILTBaseClass;
#define LPBASECLASS ILTBaseClass*
class IAggregate;
#define LPAGGREGATE IAggregate*



/*!
If the dest buffer is a static buffer, use LTStrCpy; otherwise, do
a normal string copy.
*/
#define SAFE_STRCPY(dest, src) {\
    if (sizeof(dest) > 4)\
    {\
        LTStrCpy(dest, src, sizeof(dest));\
    }\
    else\
    {\
        strcpy(dest, src);\
    }\
}

/*!
Safe string copy (strncpy doesn't always null terminate, but this does).
*/
inline void LTStrCpy(char *pDest, const char *pSrc, unsigned long destBytes) {
    if (destBytes == 0)
        return;

    strncpy(pDest, pSrc, destBytes - 1);
    pDest[destBytes - 1] = 0;
}

/*!
Used in GetFileList/FreeFileList.
*/
#define TYPE_FILE       1
#define TYPE_DIRECTORY  2

struct FileEntry {
    int m_Type;             //! Directory or file?
    char *m_pBaseFilename;   //! pic1.pcx
    char *m_pFullFilename;   //! interface/bitmaps/pic1.pcx
    FileEntry   *m_pNext;
};

/*!
Used in the counter functions.
*/
struct LTCounter {
    unsigned long m_Data[4];
};

/*!
Maximum number of skins per model.
*/
#define MAX_MODEL_TEXTURES      32
#define MAX_MODEL_RENDERSTYLES  8

//! The most trackers that can be added to a single model.
const uint32 MAX_TRACKERS_PER_MODEL = 16;

#define DEFINE_HANDLE_TYPE(name) \
    typedef struct {int __nothing;} name##_t, *name;


#define MAX_CS_FILENAME_LEN 127

/*!
HMESSAGE defines
*/

/*!
Maximum number of bytes in a communications message.
*/
#define MAX_PACKET_LEN      1100

/*!
Maximum number of bytes a single object can save into MID_SAVEOBJECT message.
*/
#define MAX_SAVEOBJECT_LEN  8192

/*!
Model state flags.
*/

//! The (nonlooping) model has finished playing the animation.
#define MS_PLAYDONE     1

/*!
HDECOLOR stuff.
*/
typedef uint32 HLTCOLOR;
#define COLOR_TRANSPARENCY_MASK 0x80000000

/*!
Gets r, g, and b as 0-255 integers.
*/
#define GETR(val) (((val) >> 16) & 0xFF)
#define GETG(val) (((val) >> 8) & 0xFF)
#define GETB(val) ((val) & 0xFF)

/*!
r, g, and b, are 0-255 integers.
*/
#define SETRGB(r,g,b) (((uint32)(r) << 16) | ((uint32)(g) << 8) | ((uint32)(b)))


/*!
r, g, and b are 0-1 floats.
*/
#define SETRGB_F(r,g,b) \
    (((uint32)((r)*255.0f) << 16) | ((uint32)((g)*255.0f) << 8) | ((uint32)((b)*255.0f)))


/*!
Set up a color with transparency.
*/
#define SETRGB_T(r,g,b)     (SETRGB((r),(g),(b)) | COLOR_TRANSPARENCY_MASK)
#define SETRGB_FT(r,g,b)    (SETRGB_F((r),(g),(b)) | COLOR_TRANSPARENCY_MASK)




class ILTMessage;
typedef ILTMessage*     HMESSAGEREAD;
typedef ILTMessage*     HMESSAGEWRITE;





/*!
Handle definitions.  It defines them in this weird way so that
you can't accidentally cast between them.
*/
DEFINE_HANDLE_TYPE(HCLASS)
DEFINE_HANDLE_TYPE(HSURFACE)
DEFINE_HANDLE_TYPE(HLTFONT)
DEFINE_HANDLE_TYPE(HSTRING)
DEFINE_HANDLE_TYPE(HLTSOUND)
DEFINE_HANDLE_TYPE(HCLIENT);
DEFINE_HANDLE_TYPE(HATTACHMENT);
DEFINE_HANDLE_TYPE(HNETSERVICE);
DEFINE_HANDLE_TYPE(HCONSOLEVAR);

/*!
Here is the layout of all message IDs.
*/

/*!
Server->Client (0-255):
DirectEngine reserves 0-15 and defines:
*/
#define STC_BPRINT 0   //! Used for debugging in order to print a message in the console.

/*!
Client->Server (0-255):
DirectEngine reserves 0-15.
*/

/*!
Object->Object (0-65k)
0-1000 reserved.
1000-2000 shared general messages (damage, I see you, etc...)
2000-2500 Riot
3500-4000 Blood 2
4000-4500 Dreadon
*/

/*!
This is used all over to classify which side of a polygon something's on.
*/
typedef enum {
    BackSide=0,
    FrontSide=1,
    Intersect=2
} PolySide;

#define MAX_FORCEUPDATE_OBJECTS 200
struct ForceUpdate {
    HOBJECT *m_Objects; //! This array is MAX_FORCEUPDATE_OBJECTS large.
    uint32 m_nObjects;
};

#define MAX_OCS_CLASSNAME_LEN   64

/*!
User-assigned identifier used to reference anim trackers in API calls.
Used with SMP networking where the engine creates and manages tracker
instances.
*/
typedef uint8 ANIMTRACKERID;
//! Reserved ID for the main tracker which all models have.
const ANIMTRACKERID MAIN_TRACKER = 0xFF;



/*!
ObjectCreateStruct::m_CreateFlags
*/

enum
{
/*!
The default behavior -- if the files this object needs are not loaded,
the engine will load them up.  If this flag is not set, the object
won't be visible until its file is loaded (from a ThreadLoadFile or an
object created with this flag set).  This is currently only supported
by models and their textures.

\note If this is set and the needed file is being loaded in a thread,
it will wait for the thread to finish loading.
*/
    OCS_AUTOLOAD       = (1<<0),

};



/*!
This structure is used when creating objects.  When you want to
create an object, you call ILTServer::CreateObject with one of these.
The structure you pass in is passed to the object's PostPropRead function,
where it can override whatever it wants to.
*/
struct ObjectCreateStruct {
    ObjectCreateStruct() {
        Clear();
    }

    void Clear() {
        int i;

        m_ObjectType = 0;
        m_ContainerCode = 0;
        m_CreateFlags = OCS_AUTOLOAD;
        m_Flags = 0;
        m_Flags2 = 0;
        m_Pos.Init();
        m_Scale.Init(1.0f, 1.0f, 1.0f);
        m_Rotation.Init();
        m_UserData = 0;
        m_ClassName[0] = 0;
        m_Filename[0] = 0;

        for (i=0; i < MAX_MODEL_TEXTURES; i++) {
            m_SkinNames[i][0] = 0;
        }
        for (i=0; i < MAX_MODEL_RENDERSTYLES; i++) {
            m_RenderStyleNames[i][0] = 0;
        }

        m_Name[0] = 0;
        m_NextUpdate = 0.0f;
        m_fDeactivationTime = 0.0f;
    }

/*!
Main info.
*/
    uint16 m_ObjectType;

/*!
Container code if it's a container.  It's in here because
you can only set it at creation time.
*/
    uint16 m_ContainerCode;

//! Combination of OCS_ flags above.
    uint32 m_CreateFlags;

    uint32 m_Flags;
    uint32 m_Flags2;

    LTVector m_Pos;
    LTVector m_Scale;
    LTRotation m_Rotation;


//! User data.
    uint32 m_UserData;

/*!
NOTE: this is ONLY used during MID_PRECREATE messages.  The engine fills this in
before calling MID_PRECREATE so the object can get its class info during the
MID_PRECREATE call.  Otherwise it is not used at all.
*/
    char m_ClassName[MAX_OCS_CLASSNAME_LEN];

/*!
This is the model, sound, or sprite filename.
It also can be the WorldModel name.
This can be zero-length when it's not needed.
*/
    char m_Filename[MAX_CS_FILENAME_LEN+1];  //!


    union {
/*!
This can be zero-length. If you set it for an
OT_MODEL, it's the skin filename.
*/
        char m_SkinName[MAX_CS_FILENAME_LEN+1];  //!
        char m_SkinNames[MAX_MODEL_TEXTURES][MAX_CS_FILENAME_LEN+1];
    };

    union {
/*!
This can be zero-length. If you set it for an
OT_MODEL, it's the skin filename.
*/
        char m_RenderStyleName[MAX_CS_FILENAME_LEN+1];   //!
        char m_RenderStyleNames[MAX_MODEL_RENDERSTYLES][MAX_CS_FILENAME_LEN+1];
    };

/*!
Server only info.
*/

//! This object's name.
    char m_Name[MAX_CS_FILENAME_LEN+1];

//! This will be the object's starting NextUpdate.
    float m_NextUpdate;

//! Amount of time before object deactivates self.
    float m_fDeactivationTime;
};



/*!
Initialize an ObjectCreateStruct.
*/
#define INIT_OBJECTCREATESTRUCT(theStruct) \
{ \
    (theStruct).Clear();\
}

/*!
Model hook.
*/

//! Use the texture for this model (if any).
#define MHF_USETEXTURE      1

struct ModelHookData
{
    HLOCALOBJ m_hObject;

//! Combination of MHF_ flags above.
    unsigned long m_Flags;

/*!
The model's object flags.  You can modify them
in here without changing the model's flags
permanently.
*/
    unsigned long m_ObjectFlags;

//! RGB 0-255
    LTVector *m_LightAdd;

//! RGB 0-255
    LTVector *m_ObjectColor;
};


/*!
Model instance hook data.
*/
#define MIH_CLIPPLANE       (1<<0)

class ModelInstanceHookData
{
public:
    uint32 m_Flags;

/*!
Extra clip plane in world space, used if MIH_CLIPPLANE is set.
*/
    LTPlane m_ClipPlane;
};



/*!
One of these can be associated with each model object and this function will be
called before rendering the model.

\param hObj The model object about to be rendered.
\param pData You fill this in.
\param flags Currently unused.
*/
typedef void (*ModelInstanceHookFn)(HLOCALOBJ hObj, ModelInstanceHookData *pData, uint32 flags);

/*!
For Canvases.
*/
typedef void (*CanvasDrawFn)(HLOCALOBJ hObj, void *pUser);

/*!
Render modes are what are used to describe a video mode/video card.
*/
struct RMode
{
    LTBOOL m_bHardware;

//! This is what the DLLs use to identify a card.
    char m_InternalName[128];

//! This is a 'friendly' string describing the card.
    char m_Description[128];

    uint32 m_Width, m_Height, m_BitDepth;

    RMode *m_pNext;
};

/*!
The ID of the description string contained in render DLLs.
*/
#define RDLL_DESCRIPTION_STRINGID   5

/*!
The blend modes for rendering optimized surfaces.
*/
enum LTSurfaceBlend {
    LTSURFACEBLEND_ALPHA, //! Alpha blend (lerp(src, dest, alpha))
    LTSURFACEBLEND_SOLID, //! Solid (src)
    LTSURFACEBLEND_ADD, //! Add (src + dest)
    LTSURFACEBLEND_MULTIPLY, //! Multiply (src * dest)
    LTSURFACEBLEND_MULTIPLY2, //! Multiply * 2 (src * dest * 2)
    LTSURFACEBLEND_MASK, //! Mask ((1 - src) * dest)
    LTSURFACEBLEND_MASKADD //! Mask & Add (src + (1 - src) * dest)
};


/*!
Rendering callback for dynamic particle volume effects.
Returns false if it needs to be called again to complete rendering.
*/
typedef bool (*VolumeEffectDPUpdateFn)(void* pUser, void* pVertexBufferData, uint32 nVBNumVerts, uint32 nPrevFilledVerts, uint32& nNumFilledVerts);

/*!
Initialization structure for a volume effect.  This is passed to ILTClient::SetupVolumeEffect()
*/
struct VolumeEffectInfo
{
	// the specific effect type to be used for this volume effect
	enum EffectType
	{
		kUninitialized,						// an uninitialized effect
		kDynamicParticles					// a trilist is passed across and filled in by the object (snow, rain)
	};

	// lighting to be used for dynamic particles
	enum DynamicParticleLighting
	{
		kNone,								// light values will not be modified
		kConstant,							// the light constant will be used
		kFirstVertNonDirectional			// only the first vert of a primitive will be lit
	};

	EffectType m_EffectType;
	LTVector m_Dims;

	// dynamic particle specific information
	VolumeEffectDPUpdateFn m_DPUpdateFn;	// function that fills in the vertex buffer
	void* m_DPUserData;						// user data pointer passed back to the update function
	DynamicParticleLighting m_DPLighting;	// lighting type to be used
	uint32 m_DPLightConstant;				// constant light value to be used if lighting is kConstant
	uint32 m_DPMaxNumParticles;				// maximum number of particles that may be created (may be used to size VB)
	char* m_DPTextureName;					// filename of the texture to be used for this particle system
};


/*!
Device input for device tracking
*/
#define MAX_INPUT_BUFFER_SIZE   32

enum
{
    DEVICETYPE_KEYBOARD    = 1,
    DEVICETYPE_MOUSE       = 2,
    DEVICETYPE_JOYSTICK    = 4,
    DEVICETYPE_UNKNOWN     = 8
};



enum
{
//! unknown control type
    CONTROLTYPE_UNKNOWN    = 0,

//! horizontal axis, such as left-right motion of a mouse
    CONTROLTYPE_XAXIS      = 1,

//! vertical axis, such as forward-backward motion of a mouse
    CONTROLTYPE_YAXIS      = 2,

//! z-axis, such as a wheel on a mouse or a throttle on a joystick
    CONTROLTYPE_ZAXIS      = 3,

//! rotation around the x-axis
    CONTROLTYPE_RXAXIS     = 4,

//! rotation around the y-axis
    CONTROLTYPE_RYAXIS     = 5,

//! rotation around the z-axis
    CONTROLTYPE_RZAXIS     = 6,

//! a slider axis
    CONTROLTYPE_SLIDER     = 7,

//! a mouse button
    CONTROLTYPE_BUTTON     = 8,

//! a key on a keyboard
    CONTROLTYPE_KEY        = 9,

//! point-of-view indicator or "hat"
    CONTROLTYPE_POV        = 10
};



struct DeviceInput {

//! type of input device (DEVICETYPE_ defines above)
    uint32 m_DeviceType;

//! name of input device
    char m_DeviceName[128];

//! type of control that changed (CONTROLTYPE_ defines above)
    uint32 m_ControlType;

//! name of control that changed
    char m_ControlName[64];

//! object instance code for the control that changed (DIK_ code for control)
	uint16 m_ControlCode;

//! input value for the control
    uint32 m_InputValue;
};



/*!
Device Object for listing objects on a particular device
*/
struct DeviceObject {

//! type of input device (DEVICETYPE_ defines above)
    uint32 m_DeviceType;

//! name of input device ("Keyboard", "Mouse", etc.)
    char m_DeviceName[128];

//! type of object (CONTROLTYPE_ defines above)
    uint32 m_ObjectType;

//! name of object ("x-axis", "button 0", etc.)
    char m_ObjectName[64];

//! low end of range for this object
    float m_RangeLow;

//! high end of range for this object
    float m_RangeHigh;

    DeviceObject *m_pNext;
};



/*!
Device Binding info for GetDeviceBindings()
*/

#define MAX_ACTIONNAME_LEN 30
#define INPUTNAME_LEN 100

struct GameAction {

//! action number from config file
    int32 nActionCode;

//! name of action from config file
    char strActionName[MAX_ACTIONNAME_LEN];

//! low range if used (zero if not used)
    float nRangeLow;

//! high range if used (zero if not used)
    float nRangeHigh;

//! next in list
    GameAction  *pNext;

};

struct DeviceBinding {

//! name of device for this trigger
    char strDeviceName[INPUTNAME_LEN];

//! name of this trigger (device object name)
    char strTriggerName[INPUTNAME_LEN];

//! real name of this trigger ("##21" instead of "Space bar")
    char strRealName[INPUTNAME_LEN];

//! scale to multiply input by
    float nScale;

//! min for range scale
    float nRangeScaleMin;

//! max for range scale
    float nRangeScaleMax;

//! offset of cent value for input data
    float nRangeScalePreCenterOffset;

//! list of actions bound to this trigger
    GameAction *pActionHead;

//! next in list
    DeviceBinding *pNext;
};

/*!
Structure used by a driver to describe a net service (such as ipx or tcp/ip).
*/
#define MAX_NETSERVICENAME_LEN  128
class NetService
{
public:
    HNETSERVICE m_handle;
    uint32 m_dwFlags; //! Combination of NETSERVICE_ flags.
    LTGUID m_guidService;
    char m_sName[MAX_NETSERVICENAME_LEN];
    NetService *m_pNext;
};




/*!
Structure used by a driver to describe a specific net session.
*/
#define MAX_NETSESSIONNAME_LEN  4096
#define MAX_NETPASSWORD_LEN     64
#define MAX_HOSTIP_LEN          32
#define NO_HOST_PORT            0xFFFF

class NetSession {
public:
    NetSession() {Clear();}
    virtual ~NetSession() {}

    void Clear() {
        m_dwFlags = 0;
        m_dwMaxPlayers = 0;
        m_dwCurPlayers = 0;
        m_Ping = 0;
        m_sName[0] = 0;
        m_sPassword[0] = 0;
        m_HostIP[0] = 0;
        m_HostPort = NO_HOST_PORT;
        m_pNext = LTNULL;
    }

    uint32 m_dwFlags;
    LTGUID m_guidApp;
    LTGUID m_guidInst;
    uint32 m_dwMaxPlayers;
    uint32 m_dwCurPlayers;
    uint32 m_Ping; //! Ping time in milliseconds.

/*!
Host info.  0 length string and NO_HOST_PORT if not on TCP/IP.
*/
    char m_HostIP[MAX_HOSTIP_LEN];
    uint32 m_HostPort;

    char m_sName[MAX_NETSESSIONNAME_LEN];
    char m_sPassword[MAX_NETPASSWORD_LEN];
    NetSession *m_pNext;
};

/*!
Structure used by to instruct a driver to create/host a new session.
*/
struct NetHost {
    uint32 m_Port; //! Port if TCP/IP.  If zero, it uses the default.
    uint32 m_dwFlags;
    uint32 m_dwMaxPlayers;
    char m_sName[MAX_NETSESSIONNAME_LEN];
    char m_sPassword[MAX_NETPASSWORD_LEN];
};

/*!
Used to determine information in GetStandingOn or GetLastCollision.
*/
struct CollisionInfo
{
/*!
The blocking plane of whatever the object collided with.  If both
object's are non-solid, then this will have a normal vector of mag 0.
*/
    LTPlane m_Plane;

/*!
The object collided with.
*/
    HOBJECT m_hObject;

/*!
If the m_hObject is the world, then m_hPoly contains a handle to the
polygon the object collided with, otherwise it is equal to LTNULL.
*/
    HPOLY m_hPoly;

/*!
Stopping velocity.  Engine will automatically apply this velocity to stop
object from continuing to collide with blocker.
*/
    LTVector m_vStopVel;
};




/*!
Sound 3D Provider

Used with GetSoundSW3DProviderList, GetSoundHW3DProviderList and
ReleaseSound3DProviderList.
*/



/*!
Sound 3D provider IDs.
*/

enum
{
    
    SOUND3DPROVIDERID_NONE                   =  0,
    SOUND3DPROVIDERID_DS3D_HARDWARE          =  1,
    SOUND3DPROVIDERID_DS3D_HARDWARE_EAX      =  2,
    SOUND3DPROVIDERID_DS3D_SOFTWARE          =  3,
	SOUND3DPROVIDERID_DS3D_DEFAULT			 =  4,
    SOUND3DPROVIDERID_UNKNOWN                =  5
};


/*!
Caps bits.
*/

enum
{
    SOUND3DPROVIDER_CAPS_REVERB        = (1<<0)
};

/*!
Contains information about a 3D sound provider.

\see GetSound3DProviderLists(), ReleaseSound3DProviderList()
*/
struct Sound3DProvider {
/*!
One of the SOUND3DPROVIDERID_xxx values. Many of the commonly used 3D providers have been identified with one of these values.  If the provider doesn't have a unique ID, it will be set to SOUND3DPROVIDERID_UNKNOWN. If this is the case, it can be identified using the m_szProvider parameter. Compare the m_szProvider string with the known provider names listed in the Miles Sound System documentation.
*/
    uint32 m_dwProviderID;

/*!
A string descriptor identifying the 3D Sound Provider. To use a 3D sound provider, you must copy this string into the InitSoundInfo structure when initializing the sound engine.
*/
    char m_szProvider[_MAX_PATH + 1];

/*!
A combination of any of the SOUND3DPROVIDER_CAPS_xxxx flags. You can use this to identify some of the features of the 3D provider.
*/
    uint32 m_dwCaps;

/*!
A pointer to the next 3D sound provider in the linked list. A NULL value indicates the end of the list. To step through all providers, keep accessing the Sound3Dprovider::m_pNextProvider member variable until Sound3DProvider::m_pNextProvider is equal to NULL.
*/
    Sound3DProvider *m_pNextProvider;
};

/*!
Reverb properties.

Use this structure with SetReverb and GetReverb.
*/

enum
{
//! m_fVolume field valid
    REVERBPARAM_VOLUME            =  (1<<0),

//! m_dwAcoustics field valid
    REVERBPARAM_ACOUSTICS         =  (1<<1),

//! m_fReflectTime field valid
    REVERBPARAM_REFLECTTIME       =  (1<<2),

//! m_fDecayTime field valid
    REVERBPARAM_DECAYTIME         =  (1<<3),

//! m_fDamping field valid
    REVERBPARAM_DAMPING           =  (1<<4),

    REVERBPARAM_ALL               =  0x1F
};




/*!
These are the valid values for m_dwAcoustics field
*/
enum {
    REVERB_ACOUSTICS_GENERIC,
    REVERB_ACOUSTICS_PADDEDCELL,
    REVERB_ACOUSTICS_ROOM,
    REVERB_ACOUSTICS_BATHROOM,
    REVERB_ACOUSTICS_LIVINGROOM,
    REVERB_ACOUSTICS_STONEROOM,
    REVERB_ACOUSTICS_AUDITORIUM,
    REVERB_ACOUSTICS_CONCERTHALL,
    REVERB_ACOUSTICS_CAVE,
    REVERB_ACOUSTICS_ARENA,
    REVERB_ACOUSTICS_HANGAR,
    REVERB_ACOUSTICS_CARPETEDHALLWAY,
    REVERB_ACOUSTICS_HALLWAY,
    REVERB_ACOUSTICS_STONECORRIDOR,
    REVERB_ACOUSTICS_ALLEY,
    REVERB_ACOUSTICS_FOREST,
    REVERB_ACOUSTICS_CITY,
    REVERB_ACOUSTICS_MOUNTAINS,
    REVERB_ACOUSTICS_QUARRY,
    REVERB_ACOUSTICS_PLAIN,
    REVERB_ACOUSTICS_PARKINGLOT,
    REVERB_ACOUSTICS_SEWERPIPE,
    REVERB_ACOUSTICS_UNDERWATER,
    REVERB_ACOUSTICS_DRUGGED,
    REVERB_ACOUSTICS_DIZZY,
    REVERB_ACOUSTICS_PSYCHOTIC,

    REVERB_ACOUSTICS_COUNT           //! total number of room types
};

struct ReverbProperties {

//! Set the params bits for which fields are valid
    uint32 m_dwParams;

//! 0.0 - 1.0
    float m_fVolume;

//! One of the REVERB_ACOUSTICS_xxx values
    uint32 m_dwAcoustics;

//! 0.0 - 5.0 Time for first reflection
    float m_fReflectTime;

//! 0.1 - 20.0 Determines how quickly reflection diminish
    float m_fDecayTime;

/*!
0.0 - 2.0, == 1.0f is even damping, < 1.0f low frequencies dampen faster;
> 1.0f high frequencies dampen faster
*/
    float m_fDamping;
};

/*!
Sound effects.

The types of sounds available are ambient, local and 3D.  The flags
controlling these types are mutually exclusive.  An ambient sound will
have distance effects of rolloff, but no orientation effects.  A local
sound will have no orientation or distance effects, and will seem as
if it is played inside the player's head.  The 3D sound will have
distance, orientation and doppler effects.

Sounds are played from the beginning once the message reaches the
client.  If it is important that the playback be synchronized with the
server's beginning time, then set PLAYSOUND_TIMESYNC.  Normally, this
is not important.  The server will use it internally if a client comes
in after a sound has already been played.

The server keeps a sound object if any of the following flags are set:
PLAYSOUND_LOOP, PLAYSOUND_ATTACHED, PLAYSOUND_GETHANDLE, PLAYSOUND_TIMESYNC,
PLAYSOUND_TIME. Server-kept sounds will update clients that come and go.
Non-server-kept sounds are sent to the existing clients once, so the
overhead is much less.

Server-kept sounds with PLAYSOUND_GETHANDLE must be removed by the game.
Other server-kept sounds are removed if they time out, or the object they
are attached to is removed.  When a sound is removed from the server,
it tells the clients to remove their copies.

The server tells clients about its sounds if the client object is
within twice the outer radius of the sound.  If it is a local sound,
then the server always tells all of the clients.

Sounds that have a client object in m_hObject and PLAYSOUND_CLIENTLOCAL
or PLAYSOUND_ATTACHED are played with PLAYSOUND_LOCAL for that particular client.

The PLAYSOUND_CLIENT is for client-initiated sounds only.  When playing
client side sounds, the PLAYSOUND_ATTACHED and PLAYSOUND_CLIENTLOCAL
flags and m_wObjectID are ignored.

*/


enum
{
/*!
Play sound locally (inside head). Mutually exclusive with
\b PLAYSOUND_AMBIENT and \b PLAYSOUND_3D. Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_LOCAL       =  0x0000,

/*!
Play sound as ambient sound. Mutually exclusive with \b PLAYSOUND_LOCAL
and \b PLAYSOUND_3D. Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_AMBIENT     =  0x0001,

/*!
Play sound as 3D sound. Mutually exclusive with \b PLAYSOUND_AMBIENT
and \b PLAYSOUND_LOCAL. Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_3D          =  0x0002,

/*!
Loop the sound. Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_LOOP        =  0x0004,

/*!
Sound's position and orientation comes from object in the \b PlaySoundInfo structure's
\b m_hObject member variable. Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_ATTACHED    =  0x0008,

/*!
Handle requested. Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_GETHANDLE   =  0x0010,

/*!
Server must time sound. Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_TIME        =  0x0020,

/*!
Control volume (using the \b PlaySoundInfo structure's \b m_nVolume member variable).
Used with the \b PlaySoundInfo structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_CTRL_VOL    =  0x0040,

/*!
Control pitch (using the \b PlaySoundinfo structure's \b m_nPitch member variable). Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_CTRL_PITCH  =  0x0080,

/*!
Allow reverb. Used with the \b PlaySoundInfo structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_REVERB      =  0x0100,

/*!
Client side sound. Used with the \b PlaySoundInfo structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_CLIENT      =  0x0200,

/*!
Playback synchronized with server clock. Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_TIMESYNC    =  0x0400,

/*!
Sound is played with \b PLAYSOUND_LOCAL for the object identified by the \b PlaySoundInfo
structure's \b m_hObject member variable. Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_CLIENTLOCAL =  0x0800,

/*!
This sound will only be played once (reduces memory overhead). Used with the \b PlaySoundInfo
structure's \b m_dwFlags member variable.
*/
    PLAYSOUND_ONCE        =  0x1000,
};



/*!
Defines a variety of options for sounds. The options you choose can drastically change
the performance of the sound rendering. This structure must be populated for each sound
to be played.
To initialize the \b PlaySoundInfo struct to default values, use the \b PLAYSOUNDINFO_INIT
macro.
*/
struct PlaySoundInfo {
/*!
A combination of any of the \b PLAYSOUND_xxxx flags. \b PLAYSOUND_LOCAL,
\b PLAYSOUND_AMBIENT and \b PLAYSOUND_3D are mutually exclusive.
*/
    uint32 m_dwFlags;

/*!
A string containing the path to the sound to play.
*/
    char m_szSoundName[_MAX_PATH + 1];

/*!
The server-side object to which the sound will be attached. This parameter must be
filled out if \b PLAYSOUND_ATTACHED or \b PLAYSOUND_CLIENTLOCAL is set in the \b
\m_dwFlags.
*/
    HOBJECT m_hObject;

/*!
This parameter is filled out by the \b ILTSoundMgr::PlaySound() function if the
\b PLAYSOUND_GETHANDLE is set. The game can use this handle with other \b ILTSoundMgr
functions that require a handle to the sound.
*/
    HLTSOUND m_hSound;



/*!
The priority of the sound. Zero is the lowest priority. The engine uses this priority
to decide which sounds get rendered when all the voices have been taken.
*/
    unsigned char m_nPriority;

/*!
For Ambient or 3D sounds. Clients outside the outer radius will not hear this sound.
Clients within the inner radius will hear the sound at full volume. Clients between
the inner and outer radius will hear an interpolated volume.
*/
    float m_fOuterRadius;



/*!
For Ambient or 3D sounds. Clients outside the outer radius will not hear this sound.
Clients within the inner radius will hear the sound at full volume. Clients between
the inner and outer radius will hear an interpolated volume.
*/
    float m_fInnerRadius;

/*!
The maximum volume of this sound. This value can range between 0 and 100. This value
is ignored unless \b PLAYSOUND_CTRL_VOLUME is set.
*/
    uint8 m_nVolume;

/*!
This is a multiplier to the playback frequency of the sound, which will shift its pitch.
If you had a sound recorded at 22,000 Hz and set its pitch shift to 1.1f, then the
output sound would be 24,2000 Hz and make the sound have a higher pitch. This value
is ignored unless \b PLAYSOUND_CTRL_PITCH is set.
*/
    float m_fPitchShift;

/*!
The 3D position of the sound in world coordinates. This is only needed for Ambient or 3D
sounds and if \b PLAYSOUND_ATTACHED is not used.
*/
    LTVector m_vPosition;

/*!
Game defined user data. The 32bit value will be passed to \b IClientShell::OnPlaySound().\
*/
    uint32 m_UserData;
};

#define PLAYSOUNDINFO_COPY(dest, src) \
    memcpy((void *)&(dest), (void *)&(src), sizeof(PlaySoundInfo));

#define PLAYSOUNDINFO_INIT(x) memset(&x, 0, sizeof(x));



/*!
InitSoundInfo.

Used to initialize the sound engine.
*/



/*!
Maximum number of voices allowed for either sw or 3d
*/
#define INITSOUNDINFO_MAXVOICES         128



/*!
Used for m_dwFlags parameter
*/

enum
{
/*!
Convert all 16-bit sounds to 8-bit sounds when loaded
from disk. Used for the \b m_dwFlags member variable of the InitSoundInfo structure.
*/
    INITSOUNDINFOFLAG_CONVERT16TO8  = (1<<0),

/*!
This tells \b ILTClientSoundMgr::InitSound() to keep track of
any sounds that exist before the InitSound call and reload them after the sound engine
is initialized. This is the best way to reinitialize the sound engine.
Used for the \b m_dwFlags member variable of the InitSoundInfo structure.
*/
    INITSOUNDINFOFLAG_RELOADSOUNDS  = (1<<1)
};

/*!
Engine can fill these flags in the m_dwResults parameter
*/
enum
{
/*!
Indicates that the 3D provider identified by the
\b m_sz3DProvider parameter provides reverb. Used with the \b m_dwResults member
variable of the InitSoundInfo structure.
*/
    INITSOUNDINFORESULTS_REVERB    = (1<<0),
};


/*!
Use this to start a game.
*/

enum
{

//! Start a world and host it (using dialogs).
    STARTGAME_HOST =      0,

//! Start a world and host on TCP/IP.
    STARTGAME_HOSTTCP =   1,

//! Connect to a server using dialogs.
    STARTGAME_CLIENT =    2,

//! Connect to the first TCP/IP game it can find at m_pTCPAddress.
    STARTGAME_CLIENTTCP = 3,

//! Start a normal game.
    STARTGAME_NORMAL =    4,

//! (Used for GetGameMode, means we're not running a world or on a server yet).
    GAMEMODE_NONE =       5
};




//! Game was lobby-launched
enum
{
    SG_LOBBY =           1,
};


#define MAX_SGR_STRINGLEN   100

class StartGameRequest
{
public:
    StartGameRequest() {
        m_Type = STARTGAME_NORMAL;
        m_WorldName[0] = 0;
        m_TCPAddress[0] = 0;
        m_RecordFilename[0] = 0;
        m_PlaybackFilename[0] = 0;
        m_flags = 0;
        m_pGameInfo = LTNULL;
        m_GameInfoLen = 0;
        m_pNetSession = LTNULL;
        m_pClientData = LTNULL;
        m_ClientDataLen = 0;
    }

    int m_Type;
    char m_WorldName[MAX_SGR_STRINGLEN];

//! TCP/IP address, if any.
    char m_TCPAddress[MAX_SGR_STRINGLEN];

/*!
Filename to record into, if any (set this to 0 length if you don't want to record).

\note When this is set, the engine starts the server but doesn't run the level; you must
send a message to the server telling it to load the world.
*/
    char m_RecordFilename[MAX_SGR_STRINGLEN];

/*!
The filename of a recorded demo.  If this is filled in, the engine starts a server
and fills in m_WorldName with the world that the demo record uses.  You need to
send a message to the server telling it to load that world.
*/
    char m_PlaybackFilename[MAX_SGR_STRINGLEN];

//! This must be used with STARTGAME_CLIENT.
    NetSession *m_pNetSession;

//! This must be used with STARTGAME_HOST.
    NetHost m_HostInfo;

//! Various flags.
    uint32 m_flags;

/*!
This data is copied over and can be accessed by
the server with ServerDE::GetGameInfo() (if you're
running a local or hosted game).
*/
    void *m_pGameInfo;
    uint32 m_GameInfoLen;

/*!
This data gets sent up and passed into OnClientEnterWorld on the server.
*/
    void *m_pClientData;
    uint32 m_ClientDataLen;
};

/*!
Parsing stuff.
*/
#define PARSE_MAXTOKENS     64
#define PARSE_MAXTOKENSIZE  80



/*!
The new console parsing thing.
*/
class ConParse {
public:
    ConParse() {m_pCommandPos = LTNULL;}
    ConParse(const char *pBuffer) {m_pCommandPos = pBuffer;}

/*!
Sets it up to parse the specified buffer.
*/
    void Init(const char *pBuffer) {m_pCommandPos = pBuffer;}

/*!
The parsed arguments.
*/
    char *m_Args[PARSE_MAXTOKENS];
    int m_nArgs;

/*!
Used internally by the engine.
*/
public:

/*!
Parse the next set of tokens.

\return LTTRUE if it parsed anything.
*/
    LTBOOL Parse();

/*!
Parses until it finds tokens with pLookFor as the first one.
skipCount says how many to skip.
You can use this just like Parse like this:


while(ParseFind("AmbientLight", LTFALSE)) { ... }
*/
    LTBOOL ParseFind(char *pLookFor, LTBOOL bCaseSensitive, uint32 minTokens=1);

private:
    const char *m_pCommandPos;
    char m_ArgBuffer[PARSE_MAXTOKENS*PARSE_MAXTOKENSIZE];
};


template<class T>
class BaseObjArray {
public:

    BaseObjArray() {
        m_pArray = LTNULL;
        m_MaxListSize = 0;
        m_nElements = 0;
    }

    inline bool AddObject(const T& obj) {
        if (m_nElements < m_MaxListSize) {
            m_pArray[m_nElements] = obj;
            m_nElements++;
            return LTTRUE;
        }

        return LTFALSE;
    }

    inline T GetObject(uint32 index) {
        ASSERT(index < m_nElements);
        return m_pArray[index];
    }

    inline void Reset() {
        m_nElements = 0;
    }

    inline uint32 NumObjects() {
        return m_nElements;
    }

protected:
    T       *m_pArray;
    uint32 m_MaxListSize;

/*!
Number of valid objects in m_pList.
*/
    uint32 m_nElements;
};

template<class T, int size>
class ObjArray : public BaseObjArray<T> {
public:
    ObjArray() {
        m_pArray = m_Array;
        m_MaxListSize = size;
    }

private:
    T m_Array[size];
};



/*!
Intersection stuff.
*/

/*!
\return LTTRUE to select this object and LTFALSE to not select it.
*/
typedef LTBOOL (*ObjectFilterFn)(HOBJECT hObj, void *pUserData);

typedef LTBOOL (*PolyFilterFn)(HPOLY hPoly, void *pUserData);

/*!
Pass this in to the IntersectSegment routine.
*/
class IntersectQuery {
public:

    IntersectQuery() {
        m_Flags = 0;
        m_FilterFn = 0;
        m_PolyFilterFn = 0;
        m_pUserData = LTNULL;

        m_From.Init();
        m_To.Init();
        m_Direction.Init();
    }

/*!
Start and end points.
*/
    LTVector m_From;
    LTVector m_To;

/*!
Only used for CastRay, this is the direction the ray should be cast in.
This doesn't need to be normalized.
*/
    LTVector m_Direction;

/*!
A combination of the intersect flags (in ltcodes.h).
*/
    uint32 m_Flags;

/*!
If this is not LTNULL, then it will call this function when it has a
POSSIBLE object intersection (it doesn't know if it really intersects
yet when it calls this).  If you return LTFALSE from this function,
then it will ignore the object and continue on.
*/
    ObjectFilterFn m_FilterFn;

/*!
If this is not LTNULL, then it will call this function when it has a
poly intersection. If you return LTFALSE from this function,
then it will ignore the poly and continue on.
*/
    PolyFilterFn m_PolyFilterFn;

/*!
Passed into pUserData of the filter function.
*/
    void *m_pUserData;
};


struct IntersectInfo {
    IntersectInfo() {
        m_Point.Init();
        m_Plane.m_Normal.Init();
        m_Plane.m_Dist = 0.0f;
        m_hObject = LTNULL;
        m_hPoly = INVALID_HPOLY;
        m_SurfaceFlags = 0;
    }

/*!
Point of intersection.
*/
    LTVector m_Point;

/*!
Plane of intersection.
*/
    LTPlane m_Plane;

/*!
Object it hit.
*/
    HOBJECT m_hObject;

/*!
The polygon it hit (if it's a world poly).
Value is INVALID_HPOLY if it didn't hit one.
*/
    HPOLY m_hPoly;

/*!
Surface flags of what it hit (these aren't object flags, and are
only set if it hit the world or a WorldModel).
*/
    uint32 m_SurfaceFlags;
};

#define ClientIntersectInfo     IntersectInfo
#define ClientIntersectQuery    IntersectQuery

typedef void (*ModelHookFn)(ModelHookData *pData, void *pUser);

/*!
GenericProp -
Here's a list of which property types map to which variables:

PT_STRING

- m_Vec and m_Color if it's formatted like '1 2 3', m_String, m_Long, m_Float, m_Bool.
- If this is 'true' or 'false', it will set m_Long, m_Float, and m_Bool to 0 or 1.

PT_VECTOR   - m_Vec, m_String ('1 2 3')
PT_COLOR    - m_Color, m_String ('1 2 3')
PT_REAL     - m_String, m_Long, m_Float, m_Bool
PT_FLAGS    - m_String, m_Long, m_Float, m_Bool
PT_BOOL     - m_String, m_Long, m_Float, m_Bool
PT_LONGINT - m_String, m_Long, m_Float, m_Bool
PT_ROTATION - m_Rotation and m_Vec (with euler rotation).
*/

#define MAX_GP_STRING_LEN 256

struct GenericProp {
    LTVector m_Vec;
    LTVector m_Color;

    char m_String[MAX_GP_STRING_LEN+1];
    LTRotation m_Rotation;
    int32 m_Long;
    float m_Float;
    LTBOOL m_Bool;
};

/*!
Helper functions.
*/

/*!
\return Returns LTTRUE if the point is touching or inside the box.
*/
inline LTBOOL base_IsPtInBox(LTVector *pPt, LTVector *pBoxMin, LTVector *pBoxMax) {
    return (pPt->x >= pBoxMin->x) && (pPt->y >= pBoxMin->y) && (pPt->z >= pBoxMin->z) &&
        (pPt->x <= pBoxMax->x) && (pPt->y <= pBoxMax->y) && (pPt->z <= pBoxMax->z);
}


/*!
Just test on XZ.
*/
inline LTBOOL base_IsPtInBoxXZ(LTVector *pPt, LTVector *pBoxMin, LTVector *pBoxMax) {
    return (pPt->x >= pBoxMin->x) && (pPt->z >= pBoxMin->z) &&
        (pPt->x <= pBoxMax->x) && (pPt->z <= pBoxMax->z);
}

/*!
Object flags.
*/
enum
{
    FLAG_VISIBLE =            (1<<0),

//! Does this model cast shadows?
    FLAG_SHADOW =             (1<<1),

//! Biases the Z towards the view so a sprite doesn't clip as much.
    FLAG_SPRITEBIAS =         (1<<2),

/*!
If this is set, it draws a model in 2 passes.  In the
second pass, it scales down the color with ColorR,
ColorG, and ColorB.  This is used to tint the skins
in multiplayer.  Note: it uses powers of 2 to determine
scale so the color scale maps like this:

> 253 = 1.0
> 126 = 0.5
> 62  = 0.25
> 29  = 0.12
otherwise 0
*/
// FLAG_MODELTINT Is no longer supported.
    FLAG_MODELTINT = (1<<2),

//! Should this light cast shadows (slower)?
    FLAG_CASTSHADOWS =        (1<<3),

//! Sprites only.


    FLAG_ROTATABLESPRITE =	(1<<3),

#ifndef DOXYGEN_SHOULD_SKIP_THIS
	// minor fix for a common misspelling
	#define FLAG_ROTATEABLESPRITE	FLAG_ROTATABLESPRITE
#endif // DOXYGEN_SHOULD_SKIP_THIS



//! Models: draw with a detail texture.
    FLAG_DETAILTEXTURE =      (1<<3),

//! Particle systems only.
    FLAG_UPDATEUNSEEN =       (1<<3),


/*!
If this is set, the engine will update
particles even when they're invisible.
You should check FLAG_WASDRAWN
on any particle systems you're iterating
over so you don't update invisible ones.
*/

//! Use the 'fastlight' method for this light.
    FLAG_SOLIDLIGHT =         (1<<4),

    FLAG_MODELWIREFRAME =     (1<<4),

/*!
The engine sets this if a particle system
or PolyGrid was drawn.  You can use this to
determine whether or not to do some expensive
calculations on it.
*/
    FLAG_WASDRAWN =           (1<<4),

//! Disable Z read/write on sprite (good for lens flares).
    FLAG_SPRITE_NOZ =        (1<<4),

//! Shrinks the sprite as the viewer gets nearer.
    FLAG_GLOWSPRITE =             (1<<5),

//! Lights only - tells it to only light the world.
    FLAG_ONLYLIGHTWORLD =         (1<<5),

//! Environment map the model.
    FLAG_ENVIRONMENTMAP =         (1<<5),

//! For PolyGrids - says to only use the environment map (ignore main texture).
    FLAG_ENVIRONMENTMAPONLY =     (1<<5),

//! Lights only - don't light backfacing polies.
    FLAG_DONTLIGHTBACKFACING =    (1<<6),

/*!
This is used for objects that are really close to the camera and would
normally be clipped out of the view. PV weapons and effects attached
to them are the best example. This tells the renderer to do some tricks
to clip their near Z much closer to the eyepoint. This can be used for:
- models
- sprites without FLAG_ROTATEABLESPRITE
- canvases
*/
    FLAG_REALLYCLOSE =            (1<<6),

//! Disables fog on WorldModels, Sprites, Particle Systems and Canvases only.
    FLAG_FOGDISABLE =         (1<<7),

//! Does a 200ms transition between model animations.
    FLAG_ANIMTRANSITION =     (1<<7),

//! Lights only - tells it to only light objects (and not the world).
    FLAG_ONLYLIGHTOBJECTS =   (1<<7),

/*!
LT normally compresses the position and rotation info
to reduce packet size.  Some things must be exact
(like some WorldModels) so this will
enlarge the packets for better accuracy.
*/
    FLAG_FULLPOSITIONRES =    (1<<8),

/*!
Just use the object's color and global light scale.
(Don't affect by area or by dynamic lights).
*/
    FLAG_NOLIGHT =            (1<<9),
    
//! Don't draw this object if we're using software rendering.
    FLAG_HARDWAREONLY =       (1<<10),

//! Draw in portals even if the object is invisible.
    FLAG_PORTALVISIBLE =      (1<<10),

/*!
Uses minimal network traffic to represent rotation
(1 byte instead of 3, but only rotates around the Y axis).

Mutually exclusive with FLAG_FULLPOSITIONRES (FLAG_FULLPOSITIONRES
forces full uncompressed rotation distribution, and FLAG_YROTATION
is ignored)
*/
    FLAG_YROTATION =          (1<<11),

//! Object is hit by raycasts.
    FLAG_RAYHIT =             (1<<12),

//! Object can't go thru other solid objects.
    FLAG_SOLID =              (1<<13),

//! Use simple box physics on this object (used for WorldModels and containers).
    FLAG_BOXPHYSICS =         (1<<14),

//! Particle systems: disables use of global light scale.
    FLAG_NOGLOBALLIGHTSCALE = (1<<14),

//! This object is solid on the server and nonsolid on the client.
    FLAG_CLIENTNONSOLID =     (1<<15),

};






/*!
Which flags the client knows about.
*/
#define CLIENT_FLAGMASK (FLAG_VISIBLE|FLAG_SHADOW|FLAG_MODELTINT|\
                         FLAG_ROTATEABLESPRITE|FLAG_SOLIDLIGHT|\
                         FLAG_REALLYCLOSE|FLAG_SPRITE_NOZ|FLAG_FOGDISABLE|\
                         FLAG_FULLPOSITIONRES|FLAG_NOLIGHT|FLAG_ENVIRONMENTMAP|\
                         FLAG_HARDWAREONLY|FLAG_YROTATION|FLAG_RAYHIT|\
                         FLAG_SOLID|FLAG_BOXPHYSICS|FLAG_CLIENTNONSOLID)


/*! 
This indicates all of the available flags for modification.  (Useful for SetObjectFlags.)
*/
#define FLAGMASK_ALL (0xFFFFFFFF)

/*!
Server only flags.
*/
//*
//! Gets touch notification.
#define FLAG_TOUCH_NOTIFY       (1<<16)

//! Gravity is applied.
#define FLAG_GRAVITY            (1<<17)

//! Steps up stairs.
#define FLAG_STAIRSTEP          (1<<18)

//! Object can pass through world
#define FLAG_GOTHRUWORLD        (1<<21)

//! Don't follow the object this object is standing on.
#define FLAG_DONTFOLLOWSTANDING (1<<23)

//Use this whenever possible as it saves cycles.

//! Object won't slide agaist polygons
#define FLAG_NOSLIDING          (1<<25)

//Uses much (10x) faster physics for collision detection, but the
//object is a point (dims 0,0,0).  Standing info is not set when
//this flag is set.
#define FLAG_POINTCOLLIDE       (1<<26)
//*/

//! The object won't get get MID_MODELSTRINGKEY messages unless it sets this flag.
#define FLAG_MODELKEYS          (1<<19)

//! Save and restore this object when switching worlds.
#define FLAG_KEEPALIVE          (1<<20)

//! Force client updates even if the object is OT_NORMAL or invisible.
#define FLAG_FORCECLIENTUPDATE  (1<<24)

//! Remove this object automatically if it gets outside the world.
#define FLAG_REMOVEIFOUTSIDE    (1<<27)

/*!
Force the engine to optimize this object
as if the FLAG_OPTIMIZEOBJECT flags were
cleared.  This can be used if you have a visible
object that's an attachment but it doesn't need
touch notifies or raycast hits (like a gun-in-hand).
*/
#define FLAG_FORCEOPTIMIZEOBJECT    (1<<28) //!

/*!
Will get an MID_AFFECTPHYSICS message for each object
that is inside its volume.
*/
#define FLAG_CONTAINER              (1<<29)

/*!
Internal flags.  Descriptions are there to help the DE developers remember what
they're there for, NOT for general use!
*/

//! (Did the renderer see the object).
#define FLAG_INTERNAL1          (1<<30)

//! (Used by ClientDE::FindObjectsInSphere).
#define FLAG_INTERNAL2          (1<<31)

#define FLAG_LASTFLAG           FLAG_INTERNAL2


/*!
If you clear these flags (flags &= ~FLAG_OPTIMIZEMASK) and the object doesn't have
a special effect message, the engine never even iterates over the object for movement,
raycasting, visibility, etc.  Use this whenever you can.
*/
#define FLAG_OPTIMIZEMASK (FLAG_VISIBLE|FLAG_SOLID|FLAG_TOUCH_NOTIFY|\
    FLAG_RAYHIT|FLAG_FORCECLIENTUPDATE|/*FLAG_GOTHRUWORLD|*/FLAG_CONTAINER)

/*!
FLAG2_ defines.
*/
enum
{
// WARNING: only the first 16 bits of FLAG2 are distributed to the
// client.  If you add a new flag in the high 16 bits, the client will
// never hear about it!

//! This object is invisible in portals.
    FLAG2_PORTALINVISIBLE =   (1<<0),

//! For WorldModels and models - use chromakeying to draw.
    FLAG2_CHROMAKEY =         (1<<1),

//! For models and sprites - do additive blending.
    FLAG2_ADDITIVE =          (1<<2),

//! For models and sprites. Multiplied color blending.
    FLAG2_MULTIPLY =          (1<<3),

//Use a y-axis aligned cylinder to collide with the BSP.
    FLAG2_CYLINDER_CD =	      (1<<4),

//NOTE:  Same flag as above for backward compatibility.
    FLAG2_PLAYERCOLLIDE =     (1<<4),

/*!
For models - re-calculates the directional lighting
when the object moves.
*/
    FLAG2_DYNAMICDIRLIGHT =   (1<<5),

/*!
Don't render this object thru the normal stuff,
only render it when processing sky objects.
*/
    FLAG2_SKYOBJECT =         (1<<6),


/*!
Treat this world model as a fog volume object
used for rendering fog volumes.
*/
    FLAG2_FOGOBJECT =         (1<<7),

/*!
!forces an object to be considered translucent
during rendering. This should be used when
an object has parts that are translucent, but
overall the alpha is completely opaque.
*/
    FLAG2_FORCETRANSLUCENT =  (1<<10),

/*!
Add a default collision object when the object is
created.  (Implemented in BaseClass/BaseClassClient)
*/
    FLAG2_COLLISIONOBJECT =	(1<<12),

/*!
When the object's position is updated through a call to
ILTPhysics::SetPosition or ILTPhysics::SetRotation, perform a non-solid
collision update on the object's associated collision
object.  If the flag is not set, the associated collision
object will just be zapped to the new position/rotation.
(Implemented in BaseClass/BaseClassClient)
*/
    FLAG2_NONSOLIDCOLLISIONUPDATE =	(1<<13),
};


/*!
Different object types.  Some can only be created on the client.
*/
enum
{
//! Invisible object.  Note, client's aren't told about
    OT_NORMAL =           0,

//! Model object.
    OT_MODEL =            1,

//! WorldModel.
    OT_WORLDMODEL =       2,

//! Sprite.
    OT_SPRITE =           3,

//! Dynamic light.
    OT_LIGHT =            4,

//! Camera.
    OT_CAMERA =           5,

//! Particle system.
    OT_PARTICLESYSTEM =   6,

//! Poly grid.
    OT_POLYGRID =         7,

//! Line system.
    OT_LINESYSTEM =       8,

//! Container.
    OT_CONTAINER =        9,

//! Canvas (game code renders it).
    OT_CANVAS =           10,

//! Volume effect.
	OT_VOLUMEEFFECT =     11,

/*!
NOTE: the high bit of the object type is reserved
for the engine's networking.
*/
    NUM_OBJECTTYPES =     12,
};

/*!
Size defines used for Parse functions
Use these to size your argument buffer and argument pointer
buffer accordingly.
*/
#define PARSE_MAXARGS       30
#define PARSE_MAXARGLEN     256

/*!
Useful for objects who need to call Term and return a value
(like for loading functions).
*/

#define TERM_RET(__ret) \
    { Term(); return __ret; }

/*!
Maximum number of sky objects.
*/
#define MAX_SKYOBJECTS  30

//*!
//Helpful math definitions.
#define MATH_PI             3.141592653589793f
#define MATH_HALFPI         1.570796326795f
#define MATH_CIRCLE         6.283185307178f
#define MATH_ONE_OVER_PI    0.3183098861839f
#define MATH_EPSILON        0.00000000001f
#define MATH_DEGREES_TO_RADIANS(x) ((x) *  0.01745329251994f)
#define MATH_RADIANS_TO_DEGREES(x) ((x) *  57.2957795130967f)

#define MATH_ONE_OVER_255   0.003921568627451f
#define MATH_ONE_OVER_128	0.0078125f


#define LTDIFF(a,b) (((a) < (b)) ? ((b) - (a)) : ((a) - (b)))
#define LTMIN(a,b) ((a) < (b) ? (a) : (b))
#define LTMAX(a,b) ((a) > (b) ? (a) : (b))
#define LTABS(a) ((a) > 0 ? (a) : -(a))
#define LTCLAMP(a, min, max) ((a) < (min) ? (min) : ((a) > (max) ? (max) : (a)))
#define LTLERP(min, max, t) ((min) + ((max) - (min)) * (t))

#define INLINE_FN __inline
//*/

#endif  //! __LTBASEDEFS_H__
