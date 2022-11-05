#pragma once

#include "core/resources.hh"
#include "system.h"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL2/SDL_audio.h>


class BaseCodec;
class BaseAudioSystem;

using AudioEffects = unsigned char;

enum: AudioEffects
{
	AudioEffect_None = (1 << 1),
	//AudioEffect_Panning = (1 << 2),
	//AudioEffect_Direct = (1 << 3),
	//AudioEffect_Binaural = (1 << 4),
	AudioEffect_World = (1 << 2),
};

// constexpr AudioEffects AudioEffectPreset_World = AudioEffect_Direct | AudioEffect_Binaural;
constexpr AudioEffects AudioEffectPreset_World = AudioEffect_World;


// prototyping idea
#if 0
struct AudioComponentSpatial
{
	/* Sound Position in World (TODO: MOVE TO A COMPONENT) */
	virtual void                    SetPos( Handle stream, const glm::vec3& pos ) = 0;
	virtual const glm::vec3&        GetPos( Handle stream ) = 0;

	glm::vec3 pos;
	float radius = 100.0f;
	float falloff = 16.f;
};
#endif


class BaseAudioSystem: public BaseSystem
{
public:
	// -------------------------------------------------------------------------------------
	// General Audio System Functions
	// -------------------------------------------------------------------------------------

	virtual void                    SetListenerTransform( const glm::vec3& pos, const glm::quat& rot ) = 0;
	virtual void                    SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang ) = 0;
	virtual void                    SetPaused( bool paused ) = 0;
	virtual void                    SetGlobalSpeed( float speed ) = 0;

	/* Load a sound from a path */
	virtual Handle                  LoadSound( std::string soundPath ) = 0;

	/* Load a sound from audio data from a SoundInfo struct (is this worth setting up?) */
	// virtual Handle               LoadSoundFromData( const SoundInfo& soundInfo ) = 0;

	/* Load the entire sound into an audio buffer instead of streaming it from the disk on playback */
	virtual bool                    PreloadSound( Handle stream ) = 0;

	/* Start playback of a sound */
	virtual bool                    PlaySound( Handle stream ) = 0;

	/* Free a sound */
	virtual void                    FreeSound( Handle stream ) = 0;

	//virtual bool                    RegisterCodec( BaseCodec *codec ) = 0;

	// -------------------------------------------------------------------------------------
	// Audio Stream Functions
	// -------------------------------------------------------------------------------------

	/* Is This a Valid Audio Stream? */
	virtual bool                    IsValid( Handle stream ) = 0;

	/* Audio Stream Volume ranges from 0.0f to 1.0f */
	virtual void                    SetVolume( Handle stream, float vol ) = 0;
	virtual float                   GetVolume( Handle stream ) = 0;

	/* Audio Stream Volume ranges from 0.0f to 1.0f */
	//virtual bool                    SetSampleRate( Handle stream, float vol ) = 0;
	//virtual float                   GetSampleRate( Handle stream ) = 0;

	/* Sound Position in World (TODO: MOVE TO A COMPONENT) */
	virtual void                    SetWorldPos( Handle stream, const glm::vec3& pos ) = 0;
	virtual const glm::vec3&        GetWorldPos( Handle stream ) = 0;

	/* Sound Loop Parameters (make a component?) */
	virtual void                    SetLoop( Handle stream, bool loop ) = 0;
	virtual bool                    DoesSoundLoop( Handle stream ) = 0;  // um

	/* Audio Volume Channels (ex. General, Music, Voices, Commentary, etc.) */
	// virtual void                    SetChannel( Handle stream, int channel ) = 0;
	// virtual int                     GetChannel( Handle stream ) = 0;
	
	virtual void                    SetEffects( Handle stream, AudioEffects effect ) = 0;
	virtual AudioEffects            GetEffects( Handle stream ) = 0;

	/* UNTESTED: seek to different point in the audio file */
	virtual bool                    Seek( Handle stream, double pos ) = 0;
	
	// -------------------------------------------------------------------------------------
	// Audio Stream Components
	// -------------------------------------------------------------------------------------

	// TODO: setup an audio component system internally
	// will need to think about it's design more

	// virtual Handle                  CreateComponent( AudioComponentType type ) = 0;
	// virtual void                    AddComponent( Handle stream, Handle component, AudioComponentType type ) = 0;
	// virtual Handle                  GetComponent( Handle stream, AudioComponentType type ) = 0;
};


#define IADUIO_NAME "Aduio"
#define IADUIO_HASH 1
