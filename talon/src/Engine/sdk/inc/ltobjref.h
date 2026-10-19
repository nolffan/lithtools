#ifndef __LTOBJREF_H__
#define __LTOBJREF_H__

#ifndef __LTBASETYPES_H__
#include "ltbasetypes.h"
#endif

#ifndef __LTLINK_H__
#include "ltlink.h"
#endif

#include "ltassert.h"

class ILTBaseClass;

//////////////////////////////////////////////////////////////////////////////
// Object reference type implementation

// The base object reference class
class LTObjRef : public LTLink
{
public:
	// ctor's, dtor's
	LTObjRef() { m_pData = 0;  TieOff(); }
	LTObjRef(HOBJECT hObj) { TieOff(); Link(hObj); }
	virtual ~LTObjRef() { Unlink(); }

	LTObjRef &operator=(const LTObjRef &cOther) {
		if (cOther.m_pData == m_pData)
			return *this;
		Unlink();
		Link(reinterpret_cast<HOBJECT>(cOther.m_pData));
		return *this;
	}

	// Functions to make it act like an HOBJECT
	LTObjRef &operator=(HOBJECT hObj) {
		if (hObj == reinterpret_cast<HOBJECT>(m_pData))
			return *this;
		Unlink();
		Link(hObj);
		return *this;
	}
	operator HOBJECT() const { return reinterpret_cast<HOBJECT>(m_pData); }

	// The callback from the engine for when the object gets deleted
	virtual void OnObjDelete() { Unlink(); }
private:
	// Link to the specified object
	void Link(HOBJECT hObj);
	// Unlink from the specified object
	void Unlink();

	// The actual object is stored in the parent class's m_pData member
};


// Object message notification object reference class
class LTObjRef_Notify : public LTObjRef
{
public:
	// ctor's
	LTObjRef_Notify() : LTObjRef(), m_pNotifyObj(0) {}
	LTObjRef_Notify(HOBJECT hObj) : LTObjRef(hObj), m_pNotifyObj(0) {}

	// Accessors for the object to receive notification
	void SetNotifyObj(ILTBaseClass *pObj) { m_pNotifyObj = pObj; }
	ILTBaseClass *GetNotifyObj() const { return m_pNotifyObj; }

	virtual void OnObjDelete();

private:
	ILTBaseClass *m_pNotifyObj;
};

// ----------------------------------------------------------------------- //
//
//	ILTObjRefReceiver
//
//	PURPOSE:	For a class to get a notification when a LTObjRefNotifier gets
//				deleted, you must implement this interface.  OnLinkBroken
//				will get called, and you can compare the LTObjRefNotifier
//				passed in against the member variables to do any special
//				processing.
//
// ----------------------------------------------------------------------- //
class LTObjRefNotifier;
class ILTObjRefReceiver
{
	public:
	
		// Implementing classes will have this function called
		// when HOBJECT ref points to gets deleted.
		virtual void OnLinkBroken( LTObjRefNotifier& ref ) = 0;
};

// ----------------------------------------------------------------------- //
//
//	LTObjRefNotifier
//
//	PURPOSE:	This version of LTObjRef provides a way to get notifications
//				when the referred to HOBJECT is deleted.  The containing
//				class must implement ILTObjRefReceiver.
//
// ----------------------------------------------------------------------- //
class LTObjRefNotifier : public LTObjRef
{
	public:

		LTObjRefNotifier( ) { m_pReceiver = NULL; }
		LTObjRefNotifier( HOBJECT hObj ) : LTObjRef( hObj ) { m_pReceiver = NULL; }
		LTObjRefNotifier( ILTObjRefReceiver& receiver ) { m_pReceiver = &receiver; }

		LTObjRefNotifier& operator=( LTObjRefNotifier const& cOther )
		{
			LTObjRef::operator=( cOther );
			return *this;
		}

		// Functions to make it act like an HOBJECT
		LTObjRefNotifier &operator=( HOBJECT hObj )
		{
			LTObjRef::operator=( hObj );
			return *this;
		}


		// Call back from engine when object gets deleted.
		virtual void OnObjDelete()
		{
			// Tell the receiver their link is broken.
			if( m_pReceiver )
			{
				m_pReceiver->OnLinkBroken( *this );
			}
			else
			{
				ASSERT( !"LTObjRefNotifier:  Receiver not set." );
			}

			// Call up to our parent.
			LTObjRef::OnObjDelete( );
		}

		// Set/Get the containing object.
		void SetReceiver( ILTObjRefReceiver& receiver ) { m_pReceiver = &receiver; }
		ILTObjRefReceiver* GetReceiver( ) const { return m_pReceiver; }

	private:

		// Back pointer to the Container.
		ILTObjRefReceiver*	m_pReceiver;
};


#endif //__LTOBJREF_H__