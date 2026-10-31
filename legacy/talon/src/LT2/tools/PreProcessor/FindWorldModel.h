#ifndef __FINDWORLDMODEL_H__
#define __FINDWORLDMODEL_H__

	class CEditBrush;
	class CPropList;
	class CEditRegion;

	#define MAX_WORLD_NAME_LEN			64

	class CWorldModelDef
	{
	public:
								CWorldModelDef();

		// Total number of polies in all the brushes.
		uint32					CalcNumPolies();
	
	public:

		// Info flags..
		uint32					m_WorldInfoFlags; // WIF_ flags in de_world.h
		CMoArray<CEditBrush*>	m_Brushes;

		char					m_WorldName[MAX_WORLD_NAME_LEN];
	};


	// Finds all the world models.
	// The first world model is always the main world.
	void FindWorldModels(CEditRegion *pRegion, CMoArray<CWorldModelDef*> &worldModels);

	// talon: the world models of the AIVolumes and MusicVolumes (0x411c00,
	// the name is ours).
	void FindAIVolumes(CEditRegion *pRegion, CMoArray<CWorldModelDef*> &worldModels);

	uint32 GetBrushType(CEditBrush *pBrush);

	// The brush's DetailLevel, at most MAX_SURFACE_DETAIL_LEVEL.
	uint32 GetDetailLevel(CEditBrush *pBrush);


#endif  // __FINDWORLDMODEL_H__

