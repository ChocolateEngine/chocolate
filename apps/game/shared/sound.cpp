#include "sound.h"


// idk what these values should really be tbh
// will require a lot of fine tuning tbh
// CONVAR_CMD( snd_doppler_scale, 0.2 )
// {
// 	// if ( audio )
// 	// 	audio->SetDopplerScale( snd_doppler_scale );
// }
// 
// // openal default is 343.3, but that won't work for this, we need to find a better value for source engine scale
// // maybe look at the xash3d openal soft stuff for this value?
// // unless we revert from source engine scale stuff? idk
// CONVAR_CMD( snd_travel_speed, 6000 )
// {
// 	//if ( audio )
// 	//	audio->SetSoundTravelSpeed( snd_travel_speed );
// }


static EntSys_Sound gEntSys_SoundSystem;


CH_STRUCT_REGISTER_COMPONENT( CSound, sound, EEntComponentNetType_Both, ECompRegFlag_None )
{
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_StdString, std::string, aPath, path, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aVolume, volume, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aFalloff, falloff, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aRadius, radius, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_U32, u32, aEffects, effects, ECompRegFlag_None );
	//CH_REGISTER_COMPONENT_VAR2( EEntNetField_Float, float, aVelocity, velocity, ECompRegFlag_None );
	CH_REGISTER_COMPONENT_VAR2( EEntNetField_Bool, bool, aStartPlayback, startPlayback, ECompRegFlag_None );

	CH_REGISTER_COMPONENT_SYS2( EntSys_Sound, gEntSys_SoundSystem );
}


// ======================================================================


void EntSys_Sound::ComponentAdded( Entity sEntity, void* spData )
{
#if CH_CLIENT
	if ( !audio )
		return;

	auto sound = ch_pointer_cast< CSound >( spData );
#endif
}


void EntSys_Sound::ComponentRemoved( Entity sEntity, void* spData )
{
#if CH_CLIENT
	if ( !audio )
		return;

	auto sound = ch_pointer_cast< CSound >( spData );

	if ( sound->aHandle && audio->IsValid( sound->aHandle ) )
	{
		audio->FreeSound( sound->aHandle );
		sound->aHandle = CH_INVALID_HANDLE;
	}
#endif
}


void EntSys_Sound::ComponentUpdated( Entity sEntity, void* spData )
{
#if CH_CLIENT
	if ( !audio )
		return;

	auto sound = ch_pointer_cast< CSound >( spData );
#endif
}


void EntSys_Sound::Update()
{
#if CH_CLIENT
	if ( !audio )
		return;

	PROF_SCOPE();

	// blech
	// audio->SetDopplerScale( snd_doppler_scale );
	// audio->SetSoundTravelSpeed( snd_travel_speed );

	for ( Entity entity : aEntities )
	{
		auto sound = Ent_GetComponent< CSound >( entity, "sound" );

		if ( !sound )
			continue;

		if ( sound->aHandle == CH_INVALID_HANDLE )
		{
			if ( sound->aPath.Get().size() )
				sound->aHandle = audio->OpenSound( sound->aPath.Get() );
			else
				continue;
		}

		if ( !audio->IsValid( sound->aHandle ) )
		{
			sound->aHandle = CH_INVALID_HANDLE;
			continue;
		}

		// Awful
		if ( sound->aStartPlayback && !sound->aStartedPlayback )
		{
			if ( !audio->PlaySound( sound->aHandle ) )
			{
				audio->FreeSound( sound->aHandle );
				sound->aStartPlayback = false;
				sound->aHandle        = CH_INVALID_HANDLE;
			}
			else
			{
				sound->aStartedPlayback = true;
			}
		}

		audio->SetVolume( sound->aHandle, sound->aVolume );

		// --------------------------------------------------

		if ( sound->aEffects & AudioEffect_Loop )
		{
			if ( !audio->HasEffects( sound->aHandle, AudioEffect_Loop ) )
				audio->AddEffects( sound->aHandle, AudioEffect_Loop );
		}
		else
		{
			if ( audio->HasEffects( sound->aHandle, AudioEffect_Loop ) )
				audio->RemoveEffects( sound->aHandle, AudioEffect_Loop );
		}

		// --------------------------------------------------

		if ( sound->aEffects & AudioEffect_World )
		{
			glm::mat4 matrix;
			Entity_GetWorldMatrix( matrix, entity );

			if ( !audio->HasEffects( sound->aHandle, AudioEffect_World ) )
				audio->AddEffects( sound->aHandle, AudioEffect_World );

			audio->SetEffectData( sound->aHandle, EAudio_World_Pos, Util_GetMatrixPosition( matrix ) );
			audio->SetEffectData( sound->aHandle, EAudio_World_Falloff, sound->aFalloff );
			audio->SetEffectData( sound->aHandle, EAudio_World_Radius, sound->aRadius );

			// TODO: auto calculate velocity
			// maybe have the entity system do it, and add GetWorldVelocity() and GetLocalVelocity()
			// though, why not just have the audio system do it? hmm
			// audio->SetEffectData( sound->aHandle, EAudio_World_Velocity, sound->aVelocity );
		}
		else
		{
			if ( audio->HasEffects( sound->aHandle, AudioEffect_World ) )
				audio->RemoveEffects( sound->aHandle, AudioEffect_World );
		}

		// --------------------------------------------------
	}
#endif
}

