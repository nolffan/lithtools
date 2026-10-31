// ------------------------------------------------------------------ //
//	Hull making (the file and function names are ours): portalizes the
//	BSP into the leaves the PVS is made from.
// ------------------------------------------------------------------ //

#ifndef __BUILDHULLS_H__
#define __BUILDHULLS_H__

	class CPreWorld;
	class CEditPoly;

	class CHullInfo
	{
		public:

			// Called with the fraction of the nodes done.
			void					(*m_ProgressFn)(float fPercent);
			uint32					m_nNodesDone;
			uint32					m_nNodes;

			// Brush polies that could not be split cleanly (talon: their
			// surfaces' polies: the array's vtable is CMoArray<CEditPoly*>'s,
			// 0x47480c, 0x43f616).
			CMoArray<CEditPoly*>	m_ProblemPolies;
	};

	// Returns 0, or nonzero on an error.
	uint32 MakeHulls(CHullInfo *pInfo, CPreWorld *pWorld, void (*ProgressFn)(float fPercent));

#endif
