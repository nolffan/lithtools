// talon: the T-junction fixer (TJunctions.cpp, 0x4479c0..; the name ours).

#ifndef __TJUNCTIONS_H__
#define __TJUNCTIONS_H__

	class CPreWorld;

	// The T-junction fixer (0x4479c0; built and destroyed by 0x43f810, its
	// destructor 0x43f960). FixTJunctions returns nothing (0x4488c9: eax is
	// not set on the way out) and never touches this; Processing.cpp's
	// declaration says BOOL.
	// Its lists' element type is not known (FixTJunctions never touches
	// them; the name is ours): their vtables (0x47578c, 0x47575c) are used
	// by nothing else, so it is no type the rest of Processor has
	// CMoArrays of (CMoArray<CPrePoly*>'s is 0x4746b8, CPreSubBsp::m_Polies's,
	// 0x433b02), though their entries are the same code.
	class CTJUnknown;

	class CTJunctionFixer
	{
	public:
		void								FixTJunctions(CPreWorld *pWorld, BOOL bSplitPolys);

		uint32								m_Unknown_00;
		CMoArray< CMoArray<CTJUnknown*> >	m_Unknown_04;	// vtable 0x47578c
		CMoArray<CTJUnknown*>				m_Unknown_18;	// vtable 0x47575c
	};

#endif
