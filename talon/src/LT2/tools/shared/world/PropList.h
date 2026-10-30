//
//   (c) 1998-1999 Monolith Productions, Inc.  All Rights Reserved
//
// ---------------------------------------------------------------
//
//------------------------------------------------------------------
//
//	FILE	  : PropList.h
//
//	PURPOSE	  : Defines CPropList and related classes.
//
//	CREATED	  : December 1 1996
//
//
//------------------------------------------------------------------

#ifndef __PROPLIST_H__
	#define __PROPLIST_H__


	class CEditProjectMgr;
	class CLTANode;
	class CLTANodeBuilder;
	class CLTAFile;


	// Property types.
	#include "ltserverobj.h"
	#include "ltbasedefs.h"	// talon: the property types live here before ltproperty.h


	#define MAX_PROPNAME_LEN		30
	#define MAX_STRINGPROP_LEN		MAX_GP_STRING_LEN
	#define MAX_EDITPROP_LEN		80



	typedef void (*StringListFillFn)( void *pData, CMoArray<char*> &fillIn );




	// One class for each property type...
	class CBaseProp
	{
		public:
	
								CBaseProp( const char *pName );

			virtual void		LoadData( CAbstractIO &file )	{}
#ifdef DIRECTEDITOR_BUILD
			// talon: DEdit's 2001 vtable (0x4dade8): LoadData, LoadDataLTA,
			// SaveData, Copy, CreateSameKind, SetupString, SetupGenericProp
			// and SameAs (below; the derived classes' SameAs take their own
			// type and override nothing).
			virtual void		LoadDataLTA( CLTANode* pNode )	{}
#endif
			virtual void		SaveData( CAbstractIO &file )	{}
			virtual void		Copy( CBaseProp *pOther );
			virtual CBaseProp*	CreateSameKind()				{ return new CBaseProp(NULL); }
			virtual void		SetupString(char *pStr) {}
			virtual void		SetupGenericProp(GenericProp *pProp) {}

			// Access to member variables
			int					GetType()	{ return m_Type; }
			long				GetFlags()	{ return m_PropFlags; }
			char				*GetName()	{ return m_Name; }

			// talon: not virtual in 2001: CBaseProp's vtable (0x475b38) has six
			// slots -- LoadData, SaveData, Copy, CreateSameKind, SetupString,
			// SetupGenericProp -- and no LTA ones or DataSize
#ifdef DIRECTEDITOR_BUILD
			virtual
#endif
			bool						SameAs(CBaseProp* pProp) 
			{
				if( m_Type == pProp->m_Type )
					if( m_PropFlags == pProp->m_PropFlags )
						if( _stricmp(m_Name, pProp->m_Name) == 0 )
							return true;
				return false;
			}
				

			// Member data
		public:

			// This is the property type from serverobj_de.h.
			int				m_Type;
			unsigned long	m_PropFlags;
			char			m_Name[MAX_PROPNAME_LEN];
			PropDef*		m_pPropDef;
	};

	class CStringProp : public CBaseProp
	{
		public:
						
						CStringProp(  const char *pName  );
			
			void		LoadData( CAbstractIO &file );
			void		SaveData( CAbstractIO &file );
#ifdef DIRECTEDITOR_BUILD
				void		LoadDataLTA( CLTANode* pNode );
#endif
			void		Copy( CBaseProp *pOther );
			virtual		CBaseProp*	CreateSameKind()				{ return new CStringProp(NULL); }
			void		SetupString(char *pStr);
			virtual void		SetupGenericProp(GenericProp *pProp);

			void		SetString(const char *lpszString);
			char		*GetString()								{ return m_String; }

			char		m_String[MAX_STRINGPROP_LEN+1];

#ifdef DIRECTEDITOR_BUILD
			bool	SameAs(CStringProp* pProp) 
#else
			bool	SameAs(CBaseProp* pProp) 
#endif
			{
				if( CBaseProp::SameAs(pProp) )
				{
					if( _stricmp(m_String, ((CStringProp*)pProp)->m_String) == 0 )
					{
						return true;
					}
				}
				return false;
			}

	};

	class CVectorProp : public CBaseProp
	{
		public:

						CVectorProp(  const char *pName  );
			
			void		LoadData( CAbstractIO &file );
			void		SaveData( CAbstractIO &file );
#ifdef DIRECTEDITOR_BUILD
				void		LoadDataLTA( CLTANode* pNode );
#endif
			void		Copy( CBaseProp *pOther );
			virtual		CBaseProp*	CreateSameKind()				{ return new CVectorProp(NULL); }
			void		SetupString(char *pStr);
			virtual void		SetupGenericProp(GenericProp *pProp);

			void		SetVector(CVector v)					{ m_Vector=v; }
			CVector		m_Vector;

#ifdef DIRECTEDITOR_BUILD
			bool	SameAs(CVectorProp* pProp) 
#else
			bool	SameAs(CBaseProp* pProp) 
#endif
			{
				if( CBaseProp::SameAs(pProp) )
				{
					if( m_Vector == ((CVectorProp*)pProp)->m_Vector )
					{
						return true;
					}
				}
				return false;
			}

	};

	class CRotationProp : public CBaseProp
	{
	public:

					CRotationProp(  const char *pName  );

		void		LoadData( CAbstractIO &file );
		void		SaveData( CAbstractIO &file );
#ifdef DIRECTEDITOR_BUILD
			void		LoadDataLTA( CLTANode* pNode );
#endif
		void		Copy( CBaseProp *pOther );
		virtual CBaseProp*	CreateSameKind() { return new CRotationProp(NULL); }
		void		SetupString(char *pStr);
		virtual void		SetupGenericProp(GenericProp *pProp);

		void		InitEulerAngles()				{m_EulerAngles.Init();}
		LTVector&	GetEulerAngles()				{return m_EulerAngles;}
		void		SetEulerAngles(LTVector vec)	{m_EulerAngles = vec;}

#ifdef DIRECTEDITOR_BUILD
		bool	SameAs(CRotationProp* pProp) 
#else
		bool	SameAs(CBaseProp* pProp) 
#endif
		{
			if( CBaseProp::SameAs(pProp) )
			{
				if( GetEulerAngles() == ((CRotationProp*)pProp)->GetEulerAngles() )
				{
					return true;
				}
			}
			return false;
		}


	// They used to be stored as DRotations..
	private:
		DVector		m_EulerAngles;
	};

	class CColorProp : public CVectorProp
	{
		public:

			CColorProp(  const char *pName  );

			void				SetColor(CVector vColor)		{ SetVector(vColor); }
			virtual CBaseProp*	CreateSameKind()				{ return new CColorProp(NULL); }

#ifdef DIRECTEDITOR_BUILD
			bool	SameAs(CColorProp* pProp) 
#else
			bool	SameAs(CBaseProp* pProp) 
#endif
			{
				if( CVectorProp::SameAs(pProp) )
				{
					return true;
				}
				return false;
			}
	};

	class CRealProp : public CBaseProp
	{
		public:

						CRealProp( const char *pName );

			void		LoadData( CAbstractIO &file );
			void		SaveData( CAbstractIO &file );
#ifdef DIRECTEDITOR_BUILD
				void		LoadDataLTA( CLTANode* pNode );
#endif
			void		Copy( CBaseProp *pOther );
			void		SetupString(char *pStr);
			virtual void		SetupGenericProp(GenericProp *pProp);

			virtual		CBaseProp*	CreateSameKind()				{ return new CRealProp(NULL); }

			void		SetValue(float fValue)					{ m_Value=fValue; }
			float		m_Value;

#ifdef DIRECTEDITOR_BUILD
			bool	SameAs(CRealProp* pProp) 
#else
			bool	SameAs(CBaseProp* pProp) 
#endif
			{
				if( CBaseProp::SameAs(pProp) )
				{
					if( m_Value == ((CRealProp*)pProp)->m_Value )
						return true;
				}
				return false;
			}

	};

	class CBoolProp : public CBaseProp
	{
		public:

						CBoolProp( const char *pName );

			void		LoadData( CAbstractIO &file );
			void		SaveData( CAbstractIO &file );
#ifdef DIRECTEDITOR_BUILD
				void		LoadDataLTA( CLTANode* pNode );
#endif
			void		Copy( CBaseProp *pOther );
			void		SetupString(char *pStr);
			virtual void		SetupGenericProp(GenericProp *pProp);

			void		SetValue(bool bValue)					{ m_Value=bValue; }
			virtual		CBaseProp*	CreateSameKind()				{ return new CBoolProp(NULL); }

			char		m_Value;

#ifdef DIRECTEDITOR_BUILD
			bool		SameAs(CBoolProp* pProp) 
#else
			bool		SameAs(CBaseProp* pProp) 
#endif
			{
				if( CBaseProp::SameAs(pProp) )
				{
					if( m_Value == ((CBoolProp*)pProp)->m_Value )
						return true;
				}
				return false;
			}

	};


	// The property list.  Every object has one of these.
	class CPropList
	{
		public:

									CPropList();
									~CPropList();
			
			void					Term();

			CBaseProp*				GetProp(const char *pName, BOOL bAssert=FALSE, uint32 *pIndex=NULL);
#ifndef DIRECTEDITOR_BUILD
			CBaseProp*				GetPropPartial(const char *pName, BOOL bAssert=FALSE, uint32 *pIndex=NULL);
#endif
			CBaseProp*				GetMatchingProp(const char *pName, int type);
			// Note : This returns the first prop that matches ALL of the flags.  (Not just any of them)
			CBaseProp*				GetPropByFlagsAndType(uint32 flags, int type, uint32 *index=NULL);
			
			// Copies property list values from one list to another.  This removes non-matching values.
			void					CopyValues(CPropList *pSource);

			// Copies the matching values (same name and type) from one list to another.
			void					CopyMatchingValues(CPropList *pSourceList);

			// Returns the number of properties
			int						GetSize()			{ return m_Props.GetSize(); }

			// Returns a property at a specific index
			CBaseProp				*GetAt(int nIndex)	{ return m_Props[nIndex]; }

			bool					SameAs(CPropList* pPropList);

#ifndef DIRECTEDITOR_BUILD
			//determines if the property lists are the same, but ignores properties listed in the filter
			bool					SameAs(CPropList* pPropList, uint32 nNumFilters, const char** pszFilter);
#endif

		public:

			CMoArray<CBaseProp*>	m_Props;

	};


#ifdef DIRECTEDITOR_BUILD
// talon: DEdit's (0x44ddc8..0x44de66): no filters, no TBW or LTA save.
class CPropListContainer
{
public:
			CPropListContainer();
	void	Init();
	uint32	AddPropList(CPropList* propList, bool alwaysAdd=false);

	CPropList* GetElem(uint32 index) {return m_propList[index];} 
	void	UnloadFromMemory();
private:
	CMoArray<CPropList*> m_propList;
};
#endif


#endif  // __PROPLIST_H__





