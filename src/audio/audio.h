#pragma once

#include "iaudio.h"
#include "system.h"

#include <SDL2/SDL.h>
#include <phonon.h>

LOG_CHANNEL( Aduio );

constexpr size_t FRAME_SIZE         = 256;
constexpr size_t SOUND_RATE         = 48000;
constexpr size_t MAX_STREAMS        = 32;
constexpr size_t CH_OUT_BUFFER_SIZE = 2048;

extern ch_handle_t    gDefaultChannel;

bool             HandleIPLErr( IPLerror ret, const char* msg );

class IAudioCodec;


// ===========================================================================
// Audio Effect Var


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
	AudioEffectVar( EAudioEffectData name, AudioVar type ) :
		name( name ), aType( type )
	{
	}

  public:
	AudioEffectVar( EAudioEffectData name, float data ) :
		AudioEffectVar( name, AudioVar::Float )
	{
		aDataFloat = data;
	}

	AudioEffectVar( EAudioEffectData name, int data ) :
		AudioEffectVar( name, AudioVar::Int )
	{
		aDataInt = data;
	}

	AudioEffectVar( EAudioEffectData name, const glm::vec3& data ) :
		AudioEffectVar( name, AudioVar::Vec3 )
	{
		aDataVec3 = data;
	}

	EAudioEffectData name;
	AudioVar         aType;

	union
	{
		float     aDataFloat;
		int       aDataInt;
		glm::vec2 aDataVec2;
		glm::vec3 aDataVec3;
		glm::vec4 aDataVec4;
	};

	inline void SetFloat( float val )
	{
		aType      = AudioVar::Float;
		aDataFloat = val;
	}
	inline void SetInt( int val )
	{
		aType    = AudioVar::Int;
		aDataInt = val;
	}
	inline void SetVec3( const glm::vec3& val )
	{
		aType     = AudioVar::Vec3;
		aDataVec3 = val;
	}

	inline float            GetFloat( float fallback = 0.f ) { return ( aType == AudioVar::Float ) ? aDataFloat : fallback; }
	inline int              GetInt( int fallback = 0 ) { return ( aType == AudioVar::Int ) ? aDataInt : fallback; }
	inline const glm::vec3& GetVec3( const glm::vec3& fallback ) { return ( aType == AudioVar::Vec3 ) ? aDataVec3 : fallback; }
};


// ===========================================================================
// Audio Stream


struct AudioStream
{
	// ============================================================
	// Audio Codec

	IAudioCodec*         codec = nullptr;
	void*                data;  // data for the codec only

	// ============================================================
	// Audio Data

	std::string          name;
	size_t               frame = 0;
	unsigned char        channels;
	size_t               rate;
	SDL_AudioFormat      format = AUDIO_F32;
	unsigned int         size;
	unsigned int         samples;
	unsigned int         bits;
	unsigned char        width;  // ???

	// ============================================================
	// Audio System Info

	bool                 paused      = false;
	bool                 valid       = false;
	bool                 preloaded   = false;  // hmm
	float                vol         = 1.f;

	// Audio Playback Channel
	ch_handle_t               channel    = gDefaultChannel;

	// audio stream to store audio from the codec and covert it
	SDL_AudioStream*     audioStream = nullptr;

	// stored audio data
	std::vector< float > preloadedAudio;

	// Final output audio
	IPLAudioBuffer       outBuffer{};

	// ============================================================
	// Audio Effects - must be set before playing a sound

	template< typename T >
	AudioEffectVar* CreateVar( EAudioEffectData sName, T sData )
	{
		AudioEffectVar* var = new AudioEffectVar( sName, sData );
		aVars.push_back( var );
		return var;
	}

	bool                           RemoveVar( EAudioEffectData sName );

	AudioEffectVar*                GetVar( EAudioEffectData sName );

	void                           SetVar( EAudioEffectData sName, float sData );
	void                           SetVar( EAudioEffectData sName, int sData );
	void                           SetVar( EAudioEffectData sName, const glm::vec3& sData );

	float                          GetFloat( EAudioEffectData sName );
	int                            GetInt( EAudioEffectData sName );
	const glm::vec3&               GetVec3( EAudioEffectData sName );

	AudioEffect                    aEffects = AudioEffect_None;
	// std::vector< AudioEffect > aEffects = {};

	std::vector< AudioEffectVar* > aVars    = {};

	// AudioEffects effects = AudioEffect_None;
	// std::vector< IAudioEffect* > aEffects{};

	// typeid hash - data
	// std::unordered_map< AudioEffects, IAudioEffect* > aEffects{};
};



// ===========================================================================
// Audio Codec Interface


class IAudioCodec
{
  protected:
	virtual ~IAudioCodec() = default;

  public:
	virtual bool        Init()                                                            = 0;
	virtual const char* GetName()                                                         = 0;
	virtual bool        CheckExt( std::string_view sExt )                                 = 0;

	virtual bool        Open( const char* soundPath, AudioStream* stream )                = 0;
	virtual long        Read( AudioStream* stream, size_t size, ChVector< float >& data ) = 0;
	virtual int         Seek( AudioStream* stream, double pos )                           = 0;
	virtual void        Close( AudioStream* stream )                                      = 0;

	IAudioSystem*       apAudio;
};


// ===========================================================================
// Audio System


struct AudioChannel
{
	std::string name;
	float       aVol    = 1.f;
	bool        aPaused = false;
};


class AudioSystem : public IAudioSystem
{
  public:
	AudioSystem();
	~AudioSystem();

	// -------------------------------------------------------------------------------------
	// General Audio System Functions
	// -------------------------------------------------------------------------------------

	void                          SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang ) override;
	void                          SetListenerVelocity( const glm::vec3& vel ) override;
	//void                          SetListenerOrient( const glm::vec3& forward, const glm::vec3& up ) override;

	//void                          SetDopplerScale( float sSpeed ) override;
	//void                          SetSoundTravelSpeed( float sSpeed ) override;

	// Pause the entire audio system
	void                          SetPaused( bool paused ) override;

	// Set a global playback speed on the sound system
	//void                          SetGlobalSpeed( float speed ) override;

	void                          SetOccluder( IAudioOccluder* spOccluder ) override;
	IAudioOccluder*               GetOccluder() override;

	// Define your own custom audio occluding interface
	// This disables the built in audio occlusion system and the static audio mesh creation functions (only with steam audio)
	// virtual void               SetCustomOccluder( IAudioOccluder* spOccluder )                                        override;
	// virtual IAudioOccluder*    GetCustomOccluder()                                                                    override;

	// -------------------------------------------------------------------------------------
	// Audio Channels
	// -------------------------------------------------------------------------------------

	// Create a new audio channel, if one with the same name is already taken, it will return the existing channel
	ch_handle_t                        RegisterChannel( const char* spName ) override;

	// Get's an Audio Channel ch_handle_t by the name of it
	ch_handle_t                        GetChannel( std::string_view sName ) override;

	// Get's an Audio Channel's Name
	const std::string&            GetChannelName( ch_handle_t sChannel ) override;

	// Get and Set the Volume of this Audio Channel
	float                         GetChannelVolume( ch_handle_t sChannel ) override;
	void                          SetChannelVolume( ch_handle_t sChannel, float sVol ) override;

	// Get and Set whether all sounds playing on this channel are paused or not
	bool                          GetChannelPaused( ch_handle_t sChannel ) override;
	void                          SetChannelPaused( ch_handle_t sChannel, bool sPaused ) override;

	// Internal Function for getting the Audio Channel data
	AudioChannel*                 GetChannelData( ch_handle_t sChannel );

	// -------------------------------------------------------------------------------------
	// Sound Playback
	// -------------------------------------------------------------------------------------

	// Preload a entire sound for playback, useful for playing a sound multiple times in a row
	// ch_handle_t                        PrecacheSound( std::string_view sSoundPath ) override;

	// Free a Preloaded sound
	// void                          FreePrecachedSound( ch_handle_t sSound ) override;

	// Load a sound from a path, usable for one time playback only
	ch_handle_t                        OpenSound( std::string_view sSoundPath ) override;

	// Load a sound from a precached sound handle
	// ch_handle_t                        OpenSound( ch_handle_t sSound ) override;

	// Read an entire opened sound for playback
	bool                          PreloadSound( ch_handle_t sSound ) override;

	// Load a sound from audio data from a SoundInfo struct (is this worth setting up?)
	// virtual ch_handle_t               LoadSoundFromData( const SoundInfo& soundInfo ) = 0;

	// Play an instance of a sound, you can play a handle multiple times
	bool                          PlaySound( ch_handle_t sSound ) override;

	// Free a sound
	void                          FreeSound( ch_handle_t sSound ) override;

	// Is This a Valid Sound?
	bool                          IsValid( ch_handle_t sSound ) override;

	// Sound Volume ranges from 0.0f to 1.0f
	void                          SetVolume( ch_handle_t sSound, float sVol ) override;
	float                         GetVolume( ch_handle_t sSound ) override;

	/* Audio Stream Volume ranges from 0.0f to 1.0f */
	//bool                    SetSampleRate( ch_handle_t stream, float vol ) = 0;
	//float                   GetSampleRate( ch_handle_t stream ) = 0;

	// Audio Volume Channels (ex. General, Music, Voices, Commentary, etc.)
	void                          SetChannel( ch_handle_t sSound, ch_handle_t sChannel ) override;
	ch_handle_t                        GetChannel( ch_handle_t sSound ) override;

	// UNTESTED: seek to different point in the audio file
	bool                          Seek( ch_handle_t sSound, double sPos ) override;

	// -------------------------------------------------------------------------------------
	// Audio Effects
	// -------------------------------------------------------------------------------------

	void                          AddEffects( ch_handle_t stream, AudioEffect effect ) override;
	void                          RemoveEffects( ch_handle_t stream, AudioEffect effect ) override;
	bool                          HasEffects( ch_handle_t stream, AudioEffect effect ) override;

	bool                          SetEffectData( ch_handle_t stream, EAudioEffectData sDataType, int data ) override;
	bool                          SetEffectData( ch_handle_t stream, EAudioEffectData sDataType, float data ) override;
	bool                          SetEffectData( ch_handle_t stream, EAudioEffectData sDataType, const glm::vec3& data ) override;

	bool                          GetEffectData( ch_handle_t stream, EAudioEffectData sDataType, int& data ) override;
	bool                          GetEffectData( ch_handle_t stream, EAudioEffectData sDataType, float& data ) override;
	bool                          GetEffectData( ch_handle_t stream, EAudioEffectData sDataType, glm::vec3& data ) override;

	// -------------------------------------------------------------------------------------
	// Internal Functions
	// -------------------------------------------------------------------------------------

	bool                          LoadSoundInternal( AudioStream* stream );

	/* Checks If This a Valid Audio Stream, if not, throw a warning and return nullptr. */
	AudioStream*                  GetStream( ch_handle_t stream );

	bool                          RegisterCodec( IAudioCodec* codec );

	bool                          UpdateStream( AudioStream* stream );
	bool                          ReadAudio( AudioStream* stream );
	bool                          ApplyEffects( AudioStream* stream );
	bool                          MixAudio();
	bool                          QueueAudio();

	bool                          InitSteamAudio();
	int                           ApplySpatialEffects( AudioStream* stream, float* data, size_t frameSize );

	virtual bool                  Init() override;
	virtual void                  Update( float frameTime ) override;

	// -------------------------------------------------------------------------------------
	// Steam Audio Settings
	// -------------------------------------------------------------------------------------

	IPLDirectEffect               apDirectEffect   = nullptr;
	IPLBinauralEffect             apBinauralEffect = nullptr;

	IPLContext                    aCtx{ nullptr };

	//IPLhandle                       computeDevice {nullptr};
	//IPLhandle                       scene {nullptr};
	//IPLhandle                       probeManager {nullptr};
	//IPLhandle                       environment {nullptr};
	IPLSimulationSettings         aSimulationSettings{};

	//IPLhandle                       envRenderer = {};
	//IPLhandle                       renderer = {};

	// memory buffer for phonon rendering final mix
	ChVector< float >             apMixBufferAudio;
	// IPLAudioBuffer                  mixBufferContext;
	IPLAudioBuffer                aMixBuffer;
	std::vector< IPLAudioBuffer > aUnmixedBuffers;

	// -------------------------------------------------------------------------------------

	std::vector< IAudioCodec* >   aCodecs;
	IAudioOccluder*               apOccluder = nullptr;

	ResourceList< AudioStream* >  aStreams;         // all streams loaded into memory
	std::vector< ch_handle_t >         aStreamsPlaying;  // streams currently playing audio for

	ResourceList< AudioChannel* > aChannels;

	SDL_AudioDeviceID             aOutputDeviceID;
	SDL_AudioSpec                 aAudioSpec;

	glm::vec3                     aListenerPos = {};
	glm::vec3                     aListenerAng = {};
	glm::quat                     aListenerRot = {};
	glm::vec3                     aListenerVel = {};

	bool                          aPaused      = false;
	float                         aSpeed       = 1.f;
};
