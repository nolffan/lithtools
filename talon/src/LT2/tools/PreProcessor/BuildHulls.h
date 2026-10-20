// ------------------------------------------------------------------ //
//	Hull making (the file and function names are ours): portalizes the
//	BSP into the leaves the PVS is made from.
// ------------------------------------------------------------------ //

#ifndef __BUILDHULLS_H__
#define __BUILDHULLS_H__

	class CPreWorld;
	class CEditBrush;

	class CHullInfo
	{
		public:

			// Called with the fraction of the nodes done.
			void					(*m_ProgressFn)(float fPercent);
			uint32					m_nNodesDone;
			uint32					m_nNodes;

			// Brushes whose polies could not be split cleanly.
			CMoArray<CEditBrush*>	m_ProblemBrushes;
	};

	// Returns 0, or nonzero on an error.
	uint32 MakeHulls(CHullInfo *pInfo, CPreWorld *pWorld, void (*ProgressFn)(float fPercent));

#endif
