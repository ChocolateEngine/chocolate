#pragma once

#include "core/resources.hh"
#include "system.h"

#include <SDL2/SDL_audio.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>


constexpr int MAX_AUDIO_STREAMS  = 64;
constexpr int MAX_AUDIO_CHANNELS = 32;


using AudioEffect                = unsigned char;

enum : AudioEffect
{
	AudioEffect_None  = 0,

	AudioEffect_Loop  = ( 1 << 0 ),
	AudioEffect_World = ( 1 << 1 ),

	// AudioEffect_Loop,
	// AudioEffect_World,

	//AudioEffect_Panning = (1 << 2),
	//AudioEffect_Direct = (1 << 3),
	//AudioEffect_Binaural = (1 << 4),
};

// constexpr AudioEffects AudioEffectPreset_World = AudioEffect_Direct | AudioEffect_Binaural;
// constexpr AudioEffects AudioEffectPreset_World = AudioEffect_World;


// maybe use the term "AudioEmitter" and "AudioListener"?


// still don't like this
enum AudioEffectData
{
	AudioData_None = 0,

	// ===================================
	// Loop Effect

	Audio_Loop_Enabled,    // bool (use int)
	Audio_Loop_StartTime,  // float
	Audio_Loop_EndTime,    // float

	// ===================================
	// World Effect (totally not copy and paste of openal)

	Audio_World_Pos,             // glm::vec3
	Audio_World_Velocity,        // glm::vec3
	Audio_World_Falloff,         // float, [0.0 -  ], default 1.0
	Audio_World_MaxDist,         // float, [0.0 -  ], default FLT_MAX
	Audio_World_ConeInnerAngle,  // float, [0 - 360], default 360
	Audio_World_ConeOuterAngle,  // float, [0 - 360], default 360
	Audio_World_ConeOuterGain,   // float, [0 - 1.0], default 0
};


// prototyping idea
#if 0


// or this, 
SetEffectData( stream, AudioEffect_Loop, AudioEffectLoop_Enabled, 1 );
SetEffectData( stream, AudioEffect_World, AudioEffectWorld_Pos, {10, 10, 10} );

#endif


// implement in game code
class IAudioOccluder
{
};


class IAudioSystem : public ISystem
{
  public:
	// -------------------------------------------------------------------------------------
	// General Audio System Functions (no global volume because that's up the ConVar snd_volume)
	// -------------------------------------------------------------------------------------

	virtual void               SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang )               = 0;
	virtual void               SetListenerVelocity( const glm::vec3& vel )                                      = 0;
	virtual void               SetListenerOrient( const glm::vec3& forward, const glm::vec3& up )               = 0;

	virtual void               SetDopplerScale( float scale )                                                   = 0;
	virtual void               SetSoundSpeed( float speed )                                                     = 0;

	virtual void               SetPaused( bool paused )                                                         = 0;
	virtual void               SetGlobalSpeed( float speed )                                                    = 0;

	virtual void               SetOccluder( IAudioOccluder* spOccluder )                                        = 0;
	virtual IAudioOccluder*    GetOccluder()                                                                    = 0;

	// Define your own custom audio occluding interface
	// This disables the built in audio occlusion system and the static audio mesh creation functions
	// virtual void               SetCustomOccluder( IAudioOccluder* spOccluder )                                        = 0;
	// virtual IAudioOccluder*    GetCustomOccluder()                                                                    = 0;

	// -------------------------------------------------------------------------------------
	// Audio Channels
	// -------------------------------------------------------------------------------------

	virtual Handle             RegisterChannel( const char* name )                                              = 0;

	virtual Handle             GetChannel( const std::string& name )                                            = 0;
	virtual const std::string& GetChannelName( Handle channel )                                                 = 0;

	virtual float              GetChannelVolume( Handle channel )                                               = 0;
	virtual void               SetChannelVolume( Handle channel, float vol )                                    = 0;

	// -------------------------------------------------------------------------------------
	// Audio Streams
	// -------------------------------------------------------------------------------------

	// Load a sound from a path (change to OpenSound?)
	virtual Handle             LoadSound( std::string_view sSoundPath )                                         = 0;

	// Create a playback instance of a sound
	// virtual Handle             CreateSoundInstance( std::string soundPath )                                     = 0;

	/* Load a sound from audio data from a SoundInfo struct (is this worth setting up?) */
	// virtual Handle               LoadSoundFromData( const SoundInfo& soundInfo ) = 0;

	/* Load the entire sound into an audio buffer instead of streaming it from the disk on playback */
	virtual bool               PreloadSound( Handle stream )                                                    = 0;

	// Play an instance of a sound, you can play a handle multiple times
	virtual bool               PlaySound( Handle stream )                                                       = 0;

	/* Free a sound */
	virtual void               FreeSound( Handle stream )                                                       = 0;

	/* Is This a Valid Audio Stream? */
	virtual bool               IsValid( Handle stream )                                                         = 0;

	/* Audio Stream Volume ranges from 0.0f to 1.0f */
	virtual void               SetVolume( Handle stream, float vol )                                            = 0;
	virtual float              GetVolume( Handle stream )                                                       = 0;

	/* Audio Stream Volume ranges from 0.0f to 1.0f */
	//virtual bool                    SetSampleRate( Handle stream, float vol ) = 0;
	//virtual float                   GetSampleRate( Handle stream ) = 0;

	/* Audio Volume Channels (ex. General, Music, Voices, Commentary, etc.) */
	virtual void               SetChannel( Handle stream, Handle channel )                                      = 0;
	virtual Handle             GetChannel( Handle stream )                                                      = 0;

	/* UNTESTED: seek to different point in the audio file */
	virtual bool               Seek( Handle stream, double pos )                                                = 0;

	// -------------------------------------------------------------------------------------
	// Audio Effects
	// -------------------------------------------------------------------------------------

	virtual void               AddEffect( Handle stream, AudioEffect effect )                                   = 0;
	virtual void               RemoveEffect( Handle stream, AudioEffect effect )                                = 0;
	virtual bool               HasEffect( Handle stream, AudioEffect effect )                                   = 0;

	virtual bool               SetEffectData( Handle stream, AudioEffectData sDataType, int data )              = 0;
	virtual bool               SetEffectData( Handle stream, AudioEffectData sDataType, float data )            = 0;
	virtual bool               SetEffectData( Handle stream, AudioEffectData sDataType, const glm::vec3& data ) = 0;

	virtual bool               GetEffectData( Handle stream, AudioEffectData sDataType, int& data )             = 0;
	virtual bool               GetEffectData( Handle stream, AudioEffectData sDataType, float& data )           = 0;
	virtual bool               GetEffectData( Handle stream, AudioEffectData sDataType, glm::vec3& data )       = 0;

#if 0
	// -------------------------
	// World Effect
	virtual bool                    SetWorldPos( Handle stream, const glm::vec3& data ) = 0;
	virtual bool                    GetWorldPos( Handle stream, glm::vec3& data ) = 0;

	virtual bool                    SetWorldPos( Handle stream, const glm::vec3& data ) = 0;
	virtual bool                    GetWorldPos( Handle stream, glm::vec3& data ) = 0;

	// -------------------------
	// Loop Effect
	virtual bool                    SetLoopEnabled( Handle stream, bool enabled ) = 0;
	virtual bool                    GetLoopEnabled( Handle stream ) = 0;

	virtual bool                    SetLoopStartTime( Handle stream, float time ) = 0;
	virtual float                   GetLoopStartTime( Handle stream ) = 0;

	virtual bool                    SetLoopEndTime( Handle stream, float time ) = 0;
	virtual float                   GetLoopEndTime( Handle stream ) = 0;

	// -------------------------
	// Panning Effect
	virtual bool                    SetPanningEnabled( Handle stream, bool enabled ) = 0;
	virtual bool                    GetPanningEnabled( Handle stream ) = 0;

	// -1.f: All Left Channel, 1.f: All Right Channel
	virtual bool                    SetPanningAmount( Handle stream, float amount ) = 0;
	virtual float                   GetPanningAmount( Handle stream ) = 0;
#endif
};


#define IADUIO_NAME "Aduio"
#define IADUIO_VER  2

