/*!  This file defines the ILTMessage class, which handles
reading/writing of messages.  */

#ifndef __ILTMESSAGE_H__
#define __ILTMESSAGE_H__


#ifndef __ILTBASEDEFS_H__
#include "ltbasedefs.h"
#endif

#ifndef __STDARG_H__
#include <stdarg.h>
#define __STDARG_H__
#endif


#define OPERATOR_READ(type, fn) \
    ILTMessage &operator>>(type &val) {fn(val); return *this;}

#define OPERATOR_WRITE(type, fn) \
    ILTMessage &operator<<(type &val) {fn(val); return *this;}




/*!  Message status flags.  */

enum
{
    LMSTAT_READOVERFLOW =    (1<<0),
    LMSTAT_WRITEOVERFLOW =   (1<<1)
};

class ILTMessage {
public:

/*! 
Free the message.

Used for: Messaging.
*/
    virtual LTRESULT Release() = 0;        

/*! 
\return A byte (8 bits) read from the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.  
*/
    inline uint8 ReadByte();

/*! 
\return A word (16 bits) read from the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.  
*/
    inline uint16 ReadWord();

/*! 
\return A dword (32 bits) read from the message.

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.  
*/
    inline uint32 ReadDWord();

/*! 
\return A 32 bit float read from the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.  
*/
    inline float ReadFloat();

/*! 
\return An \b HSTRING read from the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.  
*/
    inline HSTRING ReadHString();

/*! 
\return An \b HOBJECT read from the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.  
*/
    inline HOBJECT ReadObject();

/*! 
\return An \b LTVector read from the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.  
*/
    inline LTVector ReadVector();

/*! 
\return An \b LTVector, which was compressed, read from the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.  
*/
    inline LTVector ReadCompVector();

/*! 
\return An \b LTVector, which was a compressed position, read from the
message.

Used for: Messaging.  
*/
    inline LTVector ReadCompPos();

/*! 
\return An \b LTRotation read from the message. 

Used for: Messaging.
*/
    inline LTRotation ReadRotation();

/*!
\return An \b LTRotation which was compressed read from the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.  
*/
    inline LTRotation ReadCompRotation();

/*! 
\return An \b ILTMessage read from the message. 

Used for: Messaging.
*/
    inline ILTMessage *ReadMessage();

/*! 
\param val (return) The unsigned 8 bit value read from the message.
\return Returns a pointer to the message object.

This operator reads a byte (8 bits) from the message and 
writes it into the variable.

Used for: Messaging.
*/
    inline ILTMessage &operator>>(uint8 &val);

/*! 
\param val (return) The unsigned 16 bit value read from the message.
\return Returns a pointer to the message object.

This operator reads a word (16 bits) from the message and 
writes it into the variable. 

Used for: Messaging.
*/
    inline ILTMessage &operator>>(uint16 &val);

/*! 
\param val (return) The unsigned 32 bit value read from the message.
\return Returns a pointer to the message object.

This operator reads a dword (32 bits) from the message and 
writes it into the variable. 

Used for: Messaging.
*/
    inline ILTMessage &operator>>(uint32 &val);

/*! 
\param val (return) The 8 bit value read from the message.
\return Returns a pointer to the message object.

This operator reads a byte (8 bits) from the message and 
writes it into the variable.

Used for: Messaging.
*/
    inline ILTMessage &operator>>(int8 &val);

/*! 
\param val (return) The 16 bit value read from the message.
\return Returns a pointer to the message object.

This operator reads a word (16 bits) from the message and 
writes it into the variable. 

Used for: Messaging.
*/
    inline ILTMessage &operator>>(int16 &val);

/*! 
\param val (return) The 32 bit value read from the message.
\return Returns a pointer to the message object.

This operator reads a dword (32 bits) from the message and 
writes it into the variable. 

Used for: Messaging.
*/
    inline ILTMessage &operator>>(int32 &val);

/*! 
\param val (return) The 32 bit float value read from the message.
\return Returns a pointer to the message object.

This operator reads a 32 bit float from the message and 
writes it into the variable. 

Used for: Messaging.
*/
    inline ILTMessage &operator>>(float &val);

/*! 
\param val (return) The \b LTVector object read from the message.
\return Returns a pointer to the message object.

This operator reads an \b LTVector from the message and 
writes it into the variable. 

Used for: Messaging.
*/
    inline ILTMessage &operator>>(LTVector &val);

/*! 
\param val (return) The \b LTRotation object read from the message.
\return Returns a pointer to the message object.

This operator reads an \b LTRotation from the message and 
writes it into the variable. 

Used for: Messaging.
*/
    inline ILTMessage &operator>>(LTRotation &val);

/*! 
\param val (return) The \b HOBJECT read from the message.
\return Returns a pointer to the message object.

This operator reads an \b HOBJECT from the message and 
writes it into the variable. 

Used for: Messaging.
*/
    inline ILTMessage &operator>>(HOBJECT &val);

/*! 
\param val The unsigned 8 bit value to write to the message.
\return Returns a pointer to the message object.

This operator writes a byte (8 bits) to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(uint8 &val);

/*! 
\param val The unsigned 16 bit value to write to the message.
\return Returns a pointer to the message object.

This operator writes a word (16 bits) to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(uint16 &val);

/*! 
\param val The unsigned 32 bit value to write to the message.
\return Returns a pointer to the message object.

This operator writes a dword (32 bits) to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(uint32 &val);

/*! 
\param val The 8 bit value to write to the message.
\return Returns a pointer to the message object.

This operator writes a byte (8 bits) to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(int8 &val);

/*! 
\param val The 16 bit value to write to the message.
\return Returns a pointer to the message object.

This operator writes a word (16 bits) to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(int16 &val);

/*! 
\param val The 32 bit value to write to the message.
\return Returns a pointer to the message object.

This operator writes a dword (32 bits) to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(int32 &val);

/*! 
\param val The 32 bit float value to write to the message.
\return Returns a pointer to the message object.

This operator writes 32 bit float to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(float &val);

/*! 
\param val The LTVector object to write to the message.
\return Returns a pointer to the message object.

This operator writes an \b LTVector to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(LTVector &val);

/*! 
\param val The LTRotation object to write to the message.
\return Returns a pointer to the message object.

This operator writes an \b LTRotation to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(LTRotation &val);

/*! 
\param val The HOBJECT to write to the message.
\return Returns a pointer to the message object.

This operator writes an \b HOBJECT to the message.

Used for: Messaging.
*/
    inline ILTMessage &operator<<(HOBJECT &val);

/*! 
\param val (return) The unsigned 8 bit value read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads a byte (8 bits) from the message into 
the variable. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT ReadByteFL(uint8 &val) = 0;

/*! 
\param val (return) The unsigned 16 bit value read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads a word (16 bits) from the message into 
the variable. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT ReadWordFL(uint16 &val) = 0;

/*! 
\param val (return) The unsigned 32 bit value read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads a dword (32 bits) from the message into 
the variable. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT ReadDWordFL(uint32 &val) = 0;

/*! 
\param val (return) The 32 bit float value read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads a 32 bit float from the message into 
the variable. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT ReadFloatFL(float &val) = 0;

/*! 
\param pData The buffer into which to read the string.
\param maxBytes The size of the string buffer.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads a string from the message into the variable. 

Used for: Messaging.
*/
    virtual LTRESULT ReadStringFL(char *pData, uint32 maxBytes) = 0;

/*! 
\param hString (return) The HSTRING object read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads an \b HSTRING from the message into the variable. 

Used for: Messaging.
*/
    virtual LTRESULT ReadHStringFL(HSTRING &hString) = 0;
        
/*! 
\param pMsg The buffer into which to read the string.
\param msgBufSize The size of the string buffer.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads an \b HSTRING into the string buffer 
(this is more allocation friendly).

\note If \b msgBufSize > 0, the string is guaranteed to be null-terminated 
even if an error is returned.

Used for: Messaging.
*/
    virtual LTRESULT ReadHStringAsStringFL(char *pMsg, uint32 msgBufSize) = 0;

/*! 
\param pData  The buffer into which to read the raw data.
\param len The size of the buffer.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads raw data of the given length from the 
message into the variable. 

Used for: Messaging.
*/
    virtual LTRESULT ReadRawFL(void *pData, uint32 len) = 0;

/*! 
\param vec (return) The LTVector object read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads an \b LTVector from the message into the variable. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT ReadVectorFL(LTVector &vec) = 0;

/*! 
\param vec (return) The LTVector object read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads an \b LTVector which was compressed from the 
message into the variable. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT ReadCompVectorFL(LTVector &vec) = 0;

/*! 
\param vec (return) The LTVector object read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads an \b LTVector which was a compressed position 
from the message into the variable. 

Used for: Messaging.
*/
    virtual LTRESULT ReadCompPosFL(LTVector &vec) = 0;

/*! 
\param rot (return) The LTRotation object read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads an \b LTRotation from the message into 
the variable. 

Used for: Messaging.
*/
    virtual LTRESULT ReadRotationFL(LTRotation &rot) = 0;

/*! 
\param rot (return) The LTRotation object read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads an \b LTRotation which was compressed from 
the message into the variable. 

Used for: Messaging.
*/
    virtual LTRESULT ReadCompRotationFL(LTRotation &rot) = 0;

/*! 
\param pMsg (return) The ILTMessage object read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads an \b ILTMessage from the message into the 
variable. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT ReadMessageFL(ILTMessage *&pMsg) = 0;

/*! 
\param hObj (return) The HOBJECT read from the message.
\return Returns \b LT_ERROR if you read past the end; otherwise returns \b LT_OK.

This function reads an \b HOBJECT from the message into the variable. 

\note
This could very well return \b NULL if the object sent doesn't 
still exist. The object will \em not be there if you send a message 
with the object in it right after creating it.  The server must 
process at least one update on an object before you can send it 
to the client.

\par The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT ReadObjectFL(HOBJECT &hObj) = 0;
        
/*! 
\param valThe unsigned 8 bit value to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes a byte (8 bits) to the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT WriteByte(uint8 val) = 0;

/*! 
\param val The unsigned 16 bit value to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes a word (16 bits) to the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT WriteWord(uint16 val) = 0;

/*! 
\param val The unsigned 32 bit value to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes a dword (32 bits) to the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT WriteDWord(uint32 val) = 0;

/*! 
\param val The 32 bit float value to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes a 32 bit float to the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT WriteFloat(float val) = 0;

/*! 
\param pData A pointer to string to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes a null-terminated string to the message. 

Used for: Messaging.
*/
    virtual LTRESULT WriteString(const char *pData) = 0;

/*! 
\param vec The LTVector object to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an LTVector (12 bytes) to the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT WriteVector(LTVector &vec) = 0;

/*! 
\param vec The LTVector object to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an LTVector to the message and 
compresses it (9 bytes).

Used for: Messaging.
*/
    virtual LTRESULT WriteCompVector(LTVector &vec) = 0;

/*! 
\param vec The LTVector object to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an LTVector which represents a position to 
the message and compresses it (6 bytes).

Used for: Messaging.
*/
    virtual LTRESULT WriteCompPos(LTVector &vec) = 0;

/*! 
\param rot The LTRotation object to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an LTRotation (16 bytes) to the message. 

\note The stream operators << and >> are the recommended way to read
and write messages.

Used for: Messaging.
*/
    virtual LTRESULT WriteRotation(LTRotation &rot) = 0;

/*! 
\param rot The LTRotation object to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an \b LTRotation to the message and 
compresses it (3-6 bytes).

Used for: Messaging.
*/
    virtual LTRESULT WriteCompRotation(LTRotation &rot) = 0;

/*! 
\param pData Pointer to the raw data to write to the message.
\param len The amount of data to write.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes raw data of the given length to the message.

Used for: Messaging.
*/
    virtual LTRESULT WriteRaw(const void *pData, uint32 len) = 0;

/*! 
\param msg The ILTMessage object to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an \b ILTMessage to the message.

Used for: Messaging.
*/
    virtual LTRESULT WriteMessage(ILTMessage &msg) = 0;

/*! 
\param hString The HSTRING object to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an \b HSTRING to the message.

Used for: Messaging.
*/
    virtual LTRESULT WriteHString(HSTRING hString) = 0;

/*! T
\param messageCode The resouce identifier of the string to write.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes a string formatted from a string resource (from
cres.dll or sres.dll) to the message.

Used for: Messaging.  
*/
    virtual LTRESULT WriteHStringFormatted(int messageCode, ...) = 0;

/*! 
\param messageCode The resouce identifier of the string to write.
\param pList Dynamic argument list.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an \b HSTRING to the message specified as a
va_list.

Used for: Messaging.
*/
    virtual LTRESULT WriteHStringArgList(int messageCode, va_list *pList) = 0;

/*! 
\param pStr A pointer to string to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an \b HSTRING to the message (but saves an 
allocation of an \b HSTRING).

Used for: Messaging.
*/
    virtual LTRESULT WriteStringAsHString(char *pStr) = 0;

/*! 
\param hObj The HOBJECT to write to the message.
\return Returns \b LT_ERROR if the buffer is full; otherwise returns \b LT_OK.

This function writes an \b HOBJECT to the message.

\note This only works for objects created by the server. If you put an
object in here created by the client, then the receiver will get \b NULL
out. If the object has been removed on the server before this message
gets read, the receiver will wind up with a different object handle,
so be careful.

Used for: Messaging.  
*/
    virtual LTRESULT WriteObject(HOBJECT hObj) = 0;

/*! 
\return Returns LT_OK.

This function resets the read position of the message.
The position also automatically resets when you read to the end 
of the message.

\note This function has the same effect as calling Seek(0)

Used for: Messaging.
*/
    virtual LTRESULT ResetPos() = 0;

/*! 
\param nOffset	Absolute offset in the message to move to
\return Returns \b LT_ERROR if the seek command is outside of the range of the 
\return buffer; otherwise returns LT_OK.

Moves the read/write position

Used for: Messaging.
*/
    virtual LTRESULT Seek(uint32 nOffset) = 0;

/*! 
\return Returns the current offset of the read/write pointer

Reports the current read/write position

Used for: Messaging.
*/
    virtual uint32 Tell() = 0;

/*! 
\return Returns the length of the data currently in the message

Reports the current length of the message

Used for: Messaging.
*/
    virtual uint32 Length() = 0;

/*! 
\param flags (return) A 32 bit flag field to be filled by the function.
\return Returns LT_OK.

This function fills in the parameter with flags which indicate
the status of the message. The flags will be filled in with a 
combination of the \b LMSTAT_ flags above.

Used for: Messaging.
*/
    virtual LTRESULT GetStatus(uint32 &flags) = 0;
};

//
//Inline function bodies.
//

uint8 ILTMessage::ReadByte() {
    uint8 temp; 
    ReadByteFL(temp); 
    return temp;
}

uint16 ILTMessage::ReadWord() {
    uint16 temp; 
    ReadWordFL(temp); 
    return temp;
}

uint32 ILTMessage::ReadDWord() {
    uint32 temp;
    ReadDWordFL(temp);
    return temp;
}

float ILTMessage::ReadFloat() {
    float temp;
    ReadFloatFL(temp);
    return temp;
}

HSTRING ILTMessage::ReadHString() {
    HSTRING temp;
    ReadHStringFL(temp);
    return temp;
}

HOBJECT ILTMessage::ReadObject() {
    HOBJECT temp;
    ReadObjectFL(temp);
    return temp;
}

LTVector ILTMessage::ReadVector() {
    LTVector temp;
    ReadVectorFL(temp);
    return temp;
}

LTVector ILTMessage::ReadCompVector() {
    LTVector temp;
    ReadCompVectorFL(temp);
    return temp;
}

LTVector ILTMessage::ReadCompPos() {
    LTVector temp;
    ReadCompPosFL(temp);
    return temp;
}

LTRotation ILTMessage::ReadRotation() {
    LTRotation temp;
    ReadRotationFL(temp);
    return temp;
}

LTRotation ILTMessage::ReadCompRotation() {
    LTRotation temp;
    ReadCompRotationFL(temp);
    return temp;
}


ILTMessage *ILTMessage::ReadMessage() {
    ILTMessage *pMsg;
    ReadMessageFL(pMsg);
    return pMsg;
}

ILTMessage &ILTMessage::operator>>(uint8 &val) {
    ReadByteFL(val);
    return *this;
}

ILTMessage &ILTMessage::operator>>(uint16 &val) {
    ReadWordFL(val);
    return *this;
}

ILTMessage &ILTMessage::operator>>(uint32 &val) {
    ReadDWordFL(val);
    return *this;
}

ILTMessage &ILTMessage::operator>>(int8 &val) {
    ReadByteFL((uint8 &)val);
    return *this;
}

ILTMessage &ILTMessage::operator>>(int16 &val) {
    ReadWordFL((uint16 &)val);
    return *this;
}

ILTMessage &ILTMessage::operator>>(int32 &val) {
    ReadDWordFL((uint32 &)val);
    return *this;
}

ILTMessage &ILTMessage::operator>>(float &val) {
    ReadFloatFL(val);
    return *this;
}

ILTMessage &ILTMessage::operator>>(LTVector &val) {
    ReadVectorFL(val);
    return *this;
}

ILTMessage &ILTMessage::operator>>(LTRotation &val) {
    ReadRotationFL(val);
    return *this;
}

ILTMessage &ILTMessage::operator>>(HOBJECT &val) {
    ReadObjectFL(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(uint8 &val) {
    WriteByte(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(uint16 &val) {
    WriteWord(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(uint32 &val) {
    WriteDWord(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(int8 &val) {
    WriteByte(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(int16 &val) {
    WriteWord(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(int32 &val) {
    WriteDWord(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(float &val) {
    WriteFloat(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(LTVector &val) {
    WriteVector(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(LTRotation &val) {
    WriteRotation(val);
    return *this;
}

ILTMessage &ILTMessage::operator<<(HOBJECT &val) {
    WriteObject(val);
    return *this;
}

#endif //! __ILTMESSAGE_H__

