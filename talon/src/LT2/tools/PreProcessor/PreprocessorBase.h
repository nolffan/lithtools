//------------------------------------------------------------------
//
//	FILE	  : PreprocessorBase.h
//
//	PURPOSE	  : Defines all the base stuff for the preprocessor.
//
//	CREATED	  : December 10 1996
//
//	COPYRIGHT : Microsoft 1996 All Rights Reserved
//
//------------------------------------------------------------------

#ifndef __PREPROCESSOR_BASE_H__
#define __PREPROCESSOR_BASE_H__

	#ifdef PRE_FLOAT
		typedef float				PReal;
	#else
		typedef double				PReal;
	#endif

	typedef _CVector<PReal>		PVector;

	class PVertex : public _CVector<PReal>
	{
		public:
			void operator=(const PVertex &other) {_CVector<PReal>::operator=(other);}
	};

	typedef CMoArray<PVertex>	PVertexArray;

#endif  // __PREPROCESSOR_BASE_H__

