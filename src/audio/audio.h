#pragma once

#include "system.h"
#include "iaudio.h"

#include <SDL2/SDL.h>

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>


#define FLOAT_AUDIO 1

constexpr int MAX_SOURCE_BUFFERS = 4;

class BaseCodec;
class IAudioEffect;


extern Handle gDefaultChannel;


// uh idk make something here where we know that we are done updating the audio buffers,
// but we know that the stream is still being processed
enum: char
{
	AudioFl_None = 0,
	AudioFl_Processing = (1 << 0),
	AudioFl_NeedsUpdating = (1 << 1),
	AudioFl_AtEOF = (1 << 2),
};

using AudioFlags = char;


enum AudioState: char
{
	AudioState_Playing,
	AudioState_Paused,
	AudioState_Stopped,
};




// attempt 3: MaterialVar like idea
// this does work, but is it really necessary for this?
// it might have no actual benefit and only just add some overhead
enum class AudioVar
{
	Invalid = 0,
	Float,
	Int,
	Vec3,
};


class AudioEffectVar
{
private:
	AudioEffectVar( AudioEffectData name, AudioVar type ):
		aName( name ), aType( type )
	{
	}

public:
	AudioEffectVar( AudioEffectData name, float data ):
		AudioEffectVar( name, AudioVar::Float )
	{ aDataFloat = data; }

	AudioEffectVar( AudioEffectData name, int data ):
		AudioEffectVar( name, AudioVar::Int )
	{ aDataInt = data; }

	AudioEffectVar( AudioEffectData name, const glm::vec3 &data ):
		AudioEffectVar( name, AudioVar::Vec3 )
	{ aDataVec3 = data; }

	AudioEffectData aName;
	AudioVar        aType;

	union
	{
		float       aDataFloat;
		int         aDataInt;
		glm::vec2   aDataVec2;
		glm::vec3   aDataVec3;
		glm::vec4   aDataVec4;
	};

	inline void                 SetFloat( float val )                        { aType = AudioVar::Float; aDataFloat = val; }
	inline void                 SetInt( int val )                            { aType = AudioVar::Int; aDataInt = val; }
	inline void                 SetVec3( const glm::vec3 &val )              { aType = AudioVar::Vec3; aDataVec3 = val; }

	inline float                GetFloat( float fallback = 0.f )             { return (aType == AudioVar::Float) ? aDataFloat : fallback; }
	inline int                  GetInt( int fallback = 0 )                   { return (aType == AudioVar::Int) ? aDataInt : fallback; }
	inline const glm::vec3&     GetVec3( const glm::vec3 &fallback )         { return (aType == AudioVar::Vec3) ? aDataVec3 : fallback; }
};


struct AudioStream
{
	// ============================================================
	// Audio Codec

	BaseCodec*      codec = nullptr;
	void*           data;  // data for the codec only

	// ============================================================
	// Audio Data

	std::string     name;
	size_t          frame = 0;
	unsigned char   channels;
	size_t          rate;
	int             format = AL_FORMAT_STEREO_FLOAT32;
	unsigned int    size;
	unsigned int    samples;
	unsigned int    bits;
	unsigned char   width;  // ???

	// ============================================================
	// Audio System Info

	bool            paused    = false;
	bool            valid     = false;
	bool            preloaded = false; // hmm
	float           vol       = 1.f;

	// Index in Audio Source List (kinda dumb imo, idk)
	// size_t          index = 0;
	ALint           aSource = 0;

	// Audio Playback Channel
	Handle          aChannel = gDefaultChannel;

	// buffer testing
	std::vector< ALuint > aBuffers;
	ALuint                aBufferIndex = 0;

	// ============================================================
	// Audio Effects - must be set before playing a sound

	template <typename T>
	AudioEffectVar*     CreateVar( AudioEffectData name, T data );

	bool                RemoveVar( AudioEffectData name );

	AudioEffectVar*     GetVar( AudioEffectData name );

	void                SetVar( AudioEffectData name, float data );
	void                SetVar( AudioEffectData name, int data );
	void                SetVar( AudioEffectData name, const glm::vec3 &data );

	float               GetFloat( AudioEffectData name );
	int                 GetInt( AudioEffectData name );
	const glm::vec3&    GetVec3( AudioEffectData name );

	AudioEffect aEffects = AudioEffect_None;
	// std::vector< AudioEffect > aEffects = {};

	std::vector< AudioEffectVar* > aVars = {};

	// AudioEffects effects = AudioEffect_None;
	// std::vector< IAudioEffect* > aEffects{};
	
	// typeid hash - data
	// std::unordered_map< AudioEffects, IAudioEffect* > aEffects{};
};


// ===========================================================================
// Base Codec


class BaseCodec
{
protected:
	virtual                 ~BaseCodec() = default;

public:
	virtual bool            Init() = 0;
	virtual const char*     GetName() = 0;
	virtual bool            CheckExt( const char* ext ) = 0;

	virtual bool            Open( const char* soundPath, AudioStream *stream ) = 0;

#if FLOAT_AUDIO
	virtual int             Read( AudioStream *stream, size_t size, std::vector<float> &data ) = 0;
#else
	virtual int             Read( AudioStream *stream, size_t size, std::vector<char> &data ) = 0;
#endif

	virtual int             Seek( AudioStream *stream, double pos ) = 0;
	virtual void            Close( AudioStream *stream ) = 0;

	virtual void            SetAudioSystem( BaseAudioSystem* system ) { apAudio = system; };
	BaseAudioSystem*        apAudio;
};


// ===========================================================================
// Audio System


struct AudioChannel
{
	std::string     aName;
	float           aVol = 1.f;
};


class AudioSystem : public BaseAudioSystem
{
public:
	                                AudioSystem();
	                                ~AudioSystem();

	// -------------------------------------------------------------------------------------
	// General Audio System Functions
	// -------------------------------------------------------------------------------------

	void                            SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang ) override;
	void                            SetListenerVelocity( const glm::vec3& vel ) override;
	void                            SetListenerOrient( const glm::vec3& forward, const glm::vec3& up ) override;

	void                            SetDopplerScale( float scale ) override;
	void                            SetSoundSpeed( float speed ) override;

	void                            SetPaused( bool paused ) override;
	void                            SetGlobalSpeed( float speed ) override;

	void                            SetOccluder( IAudioOccluder* spOccluder ) override;
	IAudioOccluder*                 GetOccluder() override;

	// -------------------------------------------------------------------------------------
	// Audio Channels
	// -------------------------------------------------------------------------------------

	Handle                          RegisterChannel( const char* name ) override;

	Handle                          GetChannel( const std::string& name ) override;
	const std::string&              GetChannelName( Handle channel ) override;

	float                           GetChannelVolume( Handle channel ) override;
	void                            SetChannelVolume( Handle channel, float vol ) override;

	AudioChannel*                   GetChannelData( Handle channel );

	// -------------------------------------------------------------------------------------
	// Audio Stream Functions
	// -------------------------------------------------------------------------------------

	Handle                          LoadSound( std::string soundPath ) override;
	bool                            PreloadSound( Handle stream ) override;
	bool                            PlaySound( Handle sStream ) override;
	void                            FreeSound( Handle sStream ) override;

	/* Is This a Valid Audio Stream? */
	bool                            IsValid( Handle stream ) override;

	/* Audio Stream Volume ranges from 0.0f to 1.0f */
	void                            SetVolume( Handle stream, float vol ) override;
	float                           GetVolume( Handle stream ) override;

	/* Audio Stream Volume ranges from 0.0f to 1.0f */
	//bool                            SetSampleRate( Handle stream, float vol ) override;
	//float                           GetSampleRate( Handle stream ) override;

	/* Audio Volume Channels (ex. General, Music, Voices, Commentary, etc.) */
	void                            SetChannel( Handle stream, Handle channel ) override;
	Handle                          GetChannel( Handle stream ) override;

	bool                            Seek( Handle streamPublic, double pos ) override;

	// -------------------------------------------------------------------------------------
	// Audio Effects
	// -------------------------------------------------------------------------------------

	void                            AddEffect( Handle stream, AudioEffect effect ) override;
	void                            RemoveEffect( Handle stream, AudioEffect effect ) override;
	bool                            HasEffect( Handle stream, AudioEffect effect ) override;

	// i don't like this aaaa
	// maybe all return types will be bool, false if effect is not there? idk
	// also, how do we know what part of the effect to apply this data to? hmm
	bool                            SetEffectData( Handle stream, AudioEffectData sDataType, int data ) override;
	bool                            SetEffectData( Handle stream, AudioEffectData sDataType, float data ) override;
	bool                            SetEffectData( Handle stream, AudioEffectData sDataType, const glm::vec3& data ) override;

	bool                            GetEffectData( Handle stream, AudioEffectData sDataType, int& data ) override;
	bool                            GetEffectData( Handle stream, AudioEffectData sDataType, float& data ) override;
	bool                            GetEffectData( Handle stream, AudioEffectData sDataType, glm::vec3& data ) override;
	
	// -------------------------------------------------------------------------------------
	// Internal Functions
	// -------------------------------------------------------------------------------------

	bool                            LoadSoundInternal( AudioStream *stream );

	/* Checks If This a Valid Audio Stream, if not, throw a warning and return nullptr. */
	AudioStream*                    GetStream( Handle stream );

	/* Get an effect on an audio stream if it exists, otherwise return nullptr. */
	// IAudioEffect*                   GetEffect( AudioStream* stream, AudioEffects effectEnum );

	/* Same as above, but warns if it doesn't exist */
	// IAudioEffect*                   GetEffectWarn( AudioStream* stream, AudioEffects effectEnum );

	bool                            RegisterCodec( BaseCodec *codec );

	bool                            ApplyVolume( AudioStream* stream );
	bool                            UpdateStream( AudioStream* stream );
	bool                            ReadAudio( AudioStream* stream );
	bool                            ApplyEffects( AudioStream* stream );
	bool                            MixAudio();
	bool                            QueueAudio();

	bool                            OpenOutputDevice( const char* name = nullptr );
	bool                            CloseOutputDevice();

	bool                            InitOpenAL();

	bool                            InitSteamAudio();
	int                             ApplySpatialEffects( AudioStream* stream, float* data, size_t frameSize );

	void                            Init() override;
	void                            Update( float frameTime ) override;

	// --------------------------------------------------
	// openal audio settings

	ALCdevice*                      apOutDevice = nullptr;
	ALCcontext*                     apOutContext = nullptr;

	// will do for voice chat eventually
	// ALCdevice*                      apInDevice = nullptr;
	// ALCcontext*                     apInContext = nullptr;

	// kinda bad
	ALCuint                         apSources[MAX_AUDIO_STREAMS]{};
	ALCuint                         apBuffers[MAX_AUDIO_STREAMS]{};
	int                             aBufferCount = 0;

	// IPLAudioBuffer                  aMixBuffer;
	// std::vector<IPLAudioBuffer>     unmixedBuffers;

	// --------------------------------------------------

	IAudioOccluder*                 apOccluder = nullptr;

	std::vector< BaseCodec* >       aCodecs;
	
	MemPool                                 aStreamPool;
	ResourceManager< AudioStream* >         aStreams;         // all streams loaded into memory
	std::vector< Handle >                   aStreamsPlaying;  // streams currently playing audio for

	ResourceManager< AudioChannel >         aChannels;
	std::vector< Handle >                   aChannelHandles;  // Move to ResourceManager?

	// SDL_AudioDeviceID               aOutputDeviceID;
	// SDL_AudioSpec                   aAudioSpec;

	glm::vec3                       aListenerPos = {0, 0, 0};
	glm::vec3                       aListenerVel = {0, 0, 0};
	glm::quat                       aListenerRot = {0, 0, 0, 0};
	float                           aListenerOrient[6] = {};
	bool                            aPaused = false;
	float                           aSpeed = 1.f;

	//AudioEffect3D*                  aEffect3D = new AudioEffect3D;

	//std::vector< IAudioEffect* >    aEffects;
};

