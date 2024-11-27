#pragma once

#include "core/resource.h"

#include <SDL2/SDL_audio.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>


constexpr int MAX_AUDIO_STREAMS  = 64;
constexpr int MAX_AUDIO_CHANNELS = 32;


using AudioEffect                = u32;

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
enum EAudioEffectData : u32
{
	EAudioEffectData_None = 0,

	// ===================================
	// Loop Effect

	EAudio_Loop_Enabled,    // bool (use int)
	EAudio_Loop_StartTime,  // float
	EAudio_Loop_EndTime,    // float

	// ===================================
	// World Effect (totally not copy and paste of openal)

	EAudio_World_Pos,             // glm::vec3
	EAudio_World_Velocity,        // glm::vec3
	EAudio_World_Falloff,         // float, [0.0 -  ], default 1.0
	EAudio_World_Radius,         // float, [0.0 -  ], default FLT_MAX
	EAudio_World_ConeInnerAngle,  // float, [0 - 360], default 360
	EAudio_World_ConeOuterAngle,  // float, [0 - 360], default 360
	EAudio_World_ConeOuterGain,   // float, [0 - 1.0], default 0

	EAudioEffectData_Count,
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

	virtual void               SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang )                = 0;
	virtual void               SetListenerVelocity( const glm::vec3& vel )                                       = 0;
	// virtual void               SetListenerOrient( const glm::vec3& forward, const glm::vec3& up )                = 0;

	// virtual void               SetDopplerScale( float sSpeed )                                                   = 0;
	// virtual void               SetSoundTravelSpeed( float sSpeed )                                               = 0;

	// Pause the entire audio system
	virtual void               SetPaused( bool paused )                                                          = 0;

	// Set a global playback speed on the sound system
	// virtual void               SetGlobalSpeed( float speed )                                                     = 0;

	virtual void               SetOccluder( IAudioOccluder* spOccluder )                                         = 0;
	virtual IAudioOccluder*    GetOccluder()                                                                     = 0;

	// Define your own custom audio occluding interface
	// This disables the built in audio occlusion system and the static audio mesh creation functions (only with steam audio)
	// virtual void               SetCustomOccluder( IAudioOccluder* spOccluder )                                        = 0;
	// virtual IAudioOccluder*    GetCustomOccluder()                                                                    = 0;

	// -------------------------------------------------------------------------------------
	// Audio Channels
	// -------------------------------------------------------------------------------------

	// Create a new audio channel, if one with the same name is already taken, it will return the existing channel
	virtual ch_handle_t             RegisterChannel( const char* spName )                                             = 0;

	// Get's an Audio Channel ch_handle_t by the name of it
	virtual ch_handle_t             GetChannel( std::string_view sName )                                              = 0;

	// Get's an Audio Channel's Name
	virtual const std::string& GetChannelName( ch_handle_t sChannel )                                                 = 0;

	// Get and Set the Volume of this Audio Channel
	virtual float              GetChannelVolume( ch_handle_t sChannel )                                               = 0;
	virtual void               SetChannelVolume( ch_handle_t sChannel, float sVol )                                   = 0;

	// Get and Set whether all sounds playing on this channel are paused or not
	virtual bool               GetChannelPaused( ch_handle_t sChannel )                                               = 0;
	virtual void               SetChannelPaused( ch_handle_t sChannel, bool sPaused )                                 = 0;

	// -------------------------------------------------------------------------------------
	// Sound Playback
	// -------------------------------------------------------------------------------------

	// Preload a entire sound for playback, useful for playing a sound multiple times in a row
	// virtual ch_handle_t             PrecacheSound( std::string_view sSoundPath )                                      = 0;

	// Free a Preloaded sound
	// virtual void               FreePrecachedSound( ch_handle_t sSound )                                               = 0;

	// Load a sound from a path, usable for one time playback only
	virtual ch_handle_t             OpenSound( std::string_view sSoundPath )                                           = 0;

	// Load a sound from a precached sound handle
	// virtual ch_handle_t             OpenSound( ch_handle_t sSound )                                                        = 0;

	// Read an entire opened sound for playback
	virtual bool               PreloadSound( ch_handle_t sSound )                                                      = 0;

	// Load a sound from audio data from a SoundInfo struct (is this worth setting up?)
	// virtual ch_handle_t               LoadSoundFromData( const SoundInfo& soundInfo ) = 0;

	// Play an instance of a sound, you can play a handle multiple times
	virtual bool               PlaySound( ch_handle_t sSound )                                                        = 0;

	// Free a sound */
	virtual void               FreeSound( ch_handle_t sSound )                                                        = 0;

	// Is This a Valid Audio Stream? */
	virtual bool               IsValid( ch_handle_t sSound )                                                          = 0;

	// Audio Stream Volume ranges from 0.0f to 1.0f
	virtual void               SetVolume( ch_handle_t sSound, float vol )                                             = 0;
	virtual float              GetVolume( ch_handle_t sSound )                                                        = 0;

	// Audio Stream Volume ranges from 0.0f to 1.0f
	//virtual bool                    SetSampleRate( ch_handle_t stream, float vol ) = 0;
	//virtual float                   GetSampleRate( ch_handle_t stream ) = 0;

	// Audio Volume Channels (ex. General, Music, Voices, Commentary, etc.)
	virtual void               SetChannel( ch_handle_t sSound, ch_handle_t sChannel )                                       = 0;
	virtual ch_handle_t             GetChannel( ch_handle_t sSound )                                                       = 0;

	// UNTESTED: seek to different point in the audio file
	virtual bool               Seek( ch_handle_t sSound, double sPos )                                                = 0;

	// -------------------------------------------------------------------------------------
	// Audio Effects
	// -------------------------------------------------------------------------------------

	virtual void               AddEffects( ch_handle_t stream, AudioEffect effect )                                   = 0;
	virtual void               RemoveEffects( ch_handle_t stream, AudioEffect effect )                                = 0;
	virtual bool               HasEffects( ch_handle_t stream, AudioEffect effect )                                   = 0;

	virtual bool               SetEffectData( ch_handle_t stream, EAudioEffectData sDataType, int data )              = 0;
	virtual bool               SetEffectData( ch_handle_t stream, EAudioEffectData sDataType, float data )            = 0;
	virtual bool               SetEffectData( ch_handle_t stream, EAudioEffectData sDataType, const glm::vec3& data ) = 0;

	virtual bool               GetEffectData( ch_handle_t stream, EAudioEffectData sDataType, int& data )             = 0;
	virtual bool               GetEffectData( ch_handle_t stream, EAudioEffectData sDataType, float& data )           = 0;
	virtual bool               GetEffectData( ch_handle_t stream, EAudioEffectData sDataType, glm::vec3& data )       = 0;

#if 0
	// -------------------------
	// World Effect
	virtual bool                    SetWorldPos( ch_handle_t stream, const glm::vec3& data ) = 0;
	virtual bool                    GetWorldPos( ch_handle_t stream, glm::vec3& data ) = 0;

	virtual bool                    SetWorldPos( ch_handle_t stream, const glm::vec3& data ) = 0;
	virtual bool                    GetWorldPos( ch_handle_t stream, glm::vec3& data ) = 0;

	// -------------------------
	// Loop Effect
	virtual bool                    SetLoopEnabled( ch_handle_t stream, bool enabled ) = 0;
	virtual bool                    GetLoopEnabled( ch_handle_t stream ) = 0;

	virtual bool                    SetLoopStartTime( ch_handle_t stream, float time ) = 0;
	virtual float                   GetLoopStartTime( ch_handle_t stream ) = 0;

	virtual bool                    SetLoopEndTime( ch_handle_t stream, float time ) = 0;
	virtual float                   GetLoopEndTime( ch_handle_t stream ) = 0;

	// -------------------------
	// Panning Effect
	virtual bool                    SetPanningEnabled( ch_handle_t stream, bool enabled ) = 0;
	virtual bool                    GetPanningEnabled( ch_handle_t stream ) = 0;

	// -1.f: All Left Channel, 1.f: All Right Channel
	virtual bool                    SetPanningAmount( ch_handle_t stream, float amount ) = 0;
	virtual float                   GetPanningAmount( ch_handle_t stream ) = 0;
#endif
};


#define IADUIO_NAME "Aduio"
#define IADUIO_VER  4

