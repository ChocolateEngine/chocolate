#pragma once

#include "main.h"


enum ESoundState
{
	ESoundState_Loaded,
	ESoundState_Playing,
};


struct CSound
{
	// Path to sound to load
	ComponentNetVar< std::string > aPath;

	// Sound Controls
	ComponentNetVar< float >       aVolume        = 1.f;
	ComponentNetVar< float >       aFalloff       = 1.f;
	ComponentNetVar< float >       aRadius        = 1000.f;
	//ComponentNetVar< float >       aVelocity = 0.f;
	ComponentNetVar< u32 >         aEffects       = AudioEffect_None;

	// TODO: Find a better way to do this
	ComponentNetVar< bool >        aStartPlayback   = false;
	bool                           aStartedPlayback = false;

	ch_handle_t                         aHandle        = CH_INVALID_HANDLE;
};


class EntSys_Sound : public IEntityComponentSystem
{
  public:
	EntSys_Sound() {}
	~EntSys_Sound() {}

	void ComponentAdded( Entity sEntity, void* spData ) override;
	void ComponentRemoved( Entity sEntity, void* spData ) override;
	void ComponentUpdated( Entity sEntity, void* spData ) override;
	void Update() override;
};


