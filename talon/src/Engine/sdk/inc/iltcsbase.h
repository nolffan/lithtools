/*!
Base class for ILTClient and ILTServer.  
This is just here for b/w compatibility.
*/

#ifndef __ILTCSBASE_H__
#define __ILTCSBASE_H__


#ifndef __LTBASEDEFS_H__
#include "ltbasedefs.h"
#endif

#ifndef __LTMODULE_H__
#include "ltmodule.h"
#endif

/*!  Forward declarations of base interface classes */

class ILTPhysics;
class ILTModel;
class ILTTransform;
class ILTLightAnim;
class ILTSoundMgr;
class ILTCommon;
class LTObjRef;

class ILTCSBase : public IBase {
public:
    interface_version(ILTCSBase, 0);

/*!
\return Pointer to the \b ILTCommon interface.

Accesses the ILTCommon interface.

Used for:   Interface.
*/
    virtual ILTCommon *Common() = 0;

/*!
\return Pointer to the \b ILTPhysics interface.

Accesses the ILTPhysics interface.

Used for:   Interface.
*/
    virtual ILTPhysics *Physics() = 0;

/*!
\return Pointer to the \b ILTModel interface.

Accesses the ILTModel interface.

Used for:   Interface.
*/
    virtual ILTModel *GetModelLT() = 0;

/*!
\return Pointer to the \b ILTTransform interface.

Accesses the ILTTransform interface.

Used for:   Interface.
*/
    virtual ILTTransform *GetTransformLT() = 0;

/*!
\return Pointer to the \b ILTSoundMgr interface.

Accesses the ILTSoundMgr interface.

Used for:   Interface.
*/
    virtual ILTSoundMgr *SoundMgr() = 0;

/*!
Messaging functions.  All of these functions are obsolete.  Use
the LMessage functions. */

public:
                

/*!
\param hMessage The \b HMESSAGEWRITE written to.
\param hDataMessage The \b HMESSAGEWRITE written from.

Writes a \b HMESSAGEWRITE into an already opened \b HMESSAGEWRITE.

Used for: Messaging.
*/
    virtual LTRESULT    WriteToMessageHMessageWrite(HMESSAGEWRITE hMessage,
        HMESSAGEWRITE hDataMessage);



/*!
\param hMessage The \b HMESSAGEWRITE written to.
\param hDataMessage The \b HMESSAGEREAD written from.

Writes a \b HMESSAGEREAD into an already opened \b HMESSAGEWRITE.

Used for: Messaging.
*/
    virtual LTRESULT    WriteToMessageHMessageRead(HMESSAGEWRITE hMessage, 
        HMESSAGEREAD hDataMessage);



/*!
\param hMessage ???
\param messageCode ???
 
Helper so you don't have to FormatString and FreeString.

Used for: Messaging.
*/
    virtual LTRESULT    WriteToMessageFormattedHString(
        HMESSAGEWRITE hMessage, int messageCode, ...);



/*!
\param hMessage ???
\param hObj ???

\note You can't send object references to the client yet, so the
client can't read object references. You can't write object
references in a \b HMESSAGEWRITE passed in \b MID_SAVEOBJECT.

Used for: Messaging.  */
    virtual LTRESULT    WriteToMessageObject(HMESSAGEWRITE hMessage, 
        HOBJECT hObj);  

/*!
\param hMessage ???
\param hObject ???

Use this only while saving objects (inside \b MID_SAVEOBJECT).

Used for: Messaging.
*/
    virtual LTRESULT    WriteToLoadSaveMessageObject(HMESSAGEWRITE hMessage,
        HOBJECT hObject);


/*!
\param  cMsg	user data to send
\param  sAddr   IP address to target; can be numeric or host name.
\param  port    Port to use
\return \b LT_NOTINITIALIZED if there is no TCP/IP driver or the driver's socket is uninitialized, or \b LT_ERROR if the address cannot be built; otherwise, returns \b LT_OK.

Send a message using the TCP/IP protocol.

\b sAddr is a string containing an IP addresses to check.
If the first character of the address is a numeral (between '0' and '9') it is treated
as a numeric IP address; otherwise it's treated as a host name.

Used for: Networking. 
*/
    virtual LTRESULT SendTo(ILTMessage &cMsg, const char *pAddr, uint16 nPort) = 0;

/*!
Gets the position of the specified object

Used for: Object.
*/
    virtual LTRESULT GetObjectPos(HLOCALOBJ hObj, LTVector *pPos) = 0;

/*!
\param hObj Model instance
\param pAnimName name of animation
\return -1 if the animation doesn't exist or if the object isn't a
model.

Get an animation index from a model.
The animation index is a handle to an animation associated with a model.

Used for: Models and Animation.  */
    virtual HMODELANIM  GetAnimIndex(HOBJECT hObj, char *pAnimName) = 0;



/*!
\param hObj ???
\param hAnim ???

Set the current animation of the object, if it is a model.

Used for: Models and Animation.
*/
    virtual void    SetModelAnimation(HOBJECT hObj, HMODELANIM hAnim) = 0;


/*!
\param hObj ???
\return The animation the model is currently on, or (\b HMODELANIM)-1 if
none.

Used for: Models and Animation.  */
    virtual HMODELANIM  GetModelAnimation(HOBJECT hObj) = 0;



/*!
\param hObj ???
\param bLoop ???

Set the looping state of the model.  The default state is \b TRUE.

Used for: Models and Animation.
*/
    virtual void    SetModelLooping(HOBJECT hObj, LTBOOL bLoop) = 0;


/*!
\param hObj ???
\return ???

Get the looping state of the model.  The default state is \b TRUE.

Used for: Models and Animation.
*/
    virtual LTBOOL  GetModelLooping(HOBJECT hObj) = 0;



/*!
\param hObj ???
\return ???

Start the current animation over.

Used for: Models and Animation.
*/
    virtual LTRESULT    ResetModelAnimation(HOBJECT hObj) = 0;



/*!
\param hObj ???
\return ???

Tell what the playback state of the model is (a combination of the
\b MS_ bits defined in basedefs_de.h).

Used for: Models and Animation.
*/
    virtual uint32  GetModelPlaybackState(HOBJECT hObj) = 0;

/*!
\param pMsg \b printf style format string and variable parameter list.

A better BPrint. It prints right to the console (doesn't do any network 
stuff) and doesn't care if you're connected yet.

Used for: Misc.
*/
    virtual void    CPrint(const char *pMsg, ...) = 0;

/*!
\param pPoint Location to test.
\param pList Array of \b HOBJECT
\param maxListSize Length of the pList array.
\return The number of containers filled in.

Find out what containers contain the given point.

Used for: Misc.
*/
    virtual uint32  GetPointContainers(LTVector *pPoint, HOBJECT *pList, 
        uint32 maxListSize) = 0;

/*!
\param hObj Handle to container object.
\param pCode Address of variable that gets set.
\return \b LTFALSE when the object isn't a container.

Get the container's container code (can only be set in the 
\b ObjectCreateStruct during creation).  

Used for: Misc.
*/
    virtual LTBOOL  GetContainerCode(HOBJECT hObj, uint16 *pCode) = 0;



/*!
\param pFilename Name of the file to open.
\param pStream Address of pointer to an \b ILTStream that gets set.

Open a file up.  Pass in the relative filename.
Free the file by calling DStream::Release().

Used for: Misc.
*/
    virtual LTRESULT OpenFile(const char *pFilename, ILTStream **pStream) = 0;


/*!
\param pszSourceFile Name of the file to open.
\param pszDestFile Where to copy the file to.

Copies a file.  This function is useful for copying files out of the
rez file to a temporary file, so you can do special operations on it like
LoadLibrary.

Used for: Misc.
*/
	virtual LTRESULT CopyFile( const char *pszSourceFile, const char *pszDestFile ) = 0;


/*!
\param pStream Address of pointer to an \b ILTStream that gets set.
\param nCacheSize Stream memory cache block size

Opens a memory stream.
Free the file by calling DStream::Release().

Used for: Misc.
*/
        virtual LTRESULT OpenMemoryStream(ILTStream **pStream, uint32 nCacheSize = 256) = 0;



/*!  String functions.  Strings are reference-counted objects that
cannot be manipulated.  When you create one with FormatString() or
CreateString(), the reference count is 1.  When you copy a string with
CopyString(), the reference count is incremented.  When you free one
with FreeString(), it decrements the reference count. When the
reference count goes to zero, it deletes the string.  If you forget to
free up any strings, LithTech will spit out a message telling you
about it.  */



/*!
\param messageCode Index of format string to use.

In Windows, messageCode comes from your resource DLL.  The messages
need to be formatted with %1!s! %2!s! (the number is the argument 
number and the !s! says its a string).  You can also use !f! and !d!
for floating point and whole number.

Used for: Misc.
*/
    virtual HSTRING FormatString(int messageCode, ...) = 0;
        


/*!
\param hString Handle to the string to copy.

Copy a string. This is much more efficient than CreateString().

Used for: Misc.
*/
    virtual HSTRING CopyString(HSTRING hString) = 0;



/*!
\param pString Null terminated character string.

Create a new \b HSTRING from the given ASCII-terminated \b char*.

Used for: Misc.
*/
    virtual HSTRING CreateString(char *pString) = 0;


/*!
\param hString Handle to the string to free.

Destroy an \b HSTRING.

Used for: Misc.
*/
    virtual void    FreeString(HSTRING hString) = 0;




/*!
\param hString1 Handle of string one.
\param hString2 Handle of string two.

Compare two \b HSTRING strings for equality.

Used for: Misc.
*/
    virtual LTBOOL  CompareStrings(HSTRING hString1, HSTRING hString2) = 0;



/*!
\param hString1 Handle of string one.
\param hString2 Handle of string two.

Compare two \b HSTRING strings for equality.

Used for: Misc.
*/
    virtual LTBOOL  CompareStringsUpper(HSTRING hString1, 
        HSTRING hString2) = 0;



/*!
\param hString Handle of string.

Get the string's data. You should only use this for strings
that you stored off and want to pass to the engine for a filename 
or something.  Most strings could be in some format other than ANSI.

Used for: Misc.
*/
    virtual char*   GetStringData(HSTRING hString) = 0;




/*!
\param hVar Handle of console variable.
\return The floating point value of a console variable, 
or 0 if the \b hVar is \b NULL. 

Used for: Misc.
*/
    virtual float GetVarValueFloat(HCONSOLEVAR hVar) = 0;



/*!
\param hVar Handle of console variable.
\return The string value of a console variable, or \b NULL if the \b hVar is \b NULL.

Used for: Misc.
*/
    virtual char* GetVarValueString(HCONSOLEVAR hVar) = 0;



/*!
\return The current time, in seconds, since the shell started.

Used for: Misc.
*/
    virtual LTFLOAT GetTime() = 0;
        


/*!
\return The time since the last shell update.

Used for: Misc.
*/
    virtual LTFLOAT GetFrameTime() = 0;



/*!
\param hObj ???

Remove the object from the world.  

\note The object will not be removed or deleted until the end of the frame.

Used for: Object.
*/
    virtual LTRESULT RemoveObject(HOBJECT hObj) = 0;

/*!
\param pName Name of the lightgroup

Function to generate the appropriate lightgroup ID given the name of the lightgroup.
\note	Lightgroup names are case-sensitive.

Used for: LightGroup control
*/
	virtual LTRESULT GetLightGroupID(const char *pName, uint32 *pResult) const = 0;

/*!
\param pName Name of the texture effect
\param nStage The stage that is desired

Function to generate the appropriate ID for a texture effect given the name of the
effect and the stage that the parameter belongs to.
\note	Lightgroup names are case-sensitive.

Used for: Texture Effect Variable manipulation
*/
	virtual LTRESULT GetTextureEffectVarID(const char *pName, uint32 nStage, uint32 *pResult) const = 0;

/*!
\param hObj Object to link to
\param pRef LTObjRef object to add to the object's reference list

This function adds a reference link to the object's list, to be called when the
object gets deleted.
\note	This function is called internally by the LTObjRef class.  Do not call this function externally!
*/
	virtual LTRESULT LinkObjRef(HOBJECT hObj, LTObjRef *pRef) = 0;


/*!
\param nNum Index of blind data.

\param nId ID of blind data (will fail if doesn't match).

\param pData A pointer to the blind data is placed here.

\param nSize Size of blind data is placed here.

\return Returns LTTRUE on success.

Retrieve a pointer to a section of blind data
Get a pointer to a section of blind data.  Blind data
should be freed with FreeBlindObjectData after it has been used.

Used for: Misc.
*/
	virtual LTRESULT GetBlindObjectData(uint32 nNum, uint32 nId, uint8*& pData, uint32& nSize) = 0;

/*!
\param nNum Index of blind data.

\param nId ID of blind data (will fail if doesn't match).

\return Returns LTTRUE on success.

Free the memory used by blind data.  Once all blind data has been freed,
support structures will also be freed.

Used for: Misc.
*/
	virtual LTRESULT FreeBlindObjectData(uint32 nNum, uint32 nId) = 0;
};


#endif //! __ILTCSBASE_H__



