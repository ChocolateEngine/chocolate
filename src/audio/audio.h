#pragma once

#include "system.h"
#include "iaudio.h"

#include <SDL2/SDL.h>

#if ENABLE_AUDIO
#include <phonon.h>
#endif /* ENABLE_AUDIO  */


class BaseCodec;
class IAudioEffect;


struct AudioStream
{
	bool Valid() { return valid; }

	BaseCodec *codec = nullptr;
	void* data;  // data for the codec only

	std::string name;
	size_t frame = 0;

	unsigned char channels;
	size_t rate;
	SDL_AudioFormat format = AUDIO_F32;
	unsigned int size;
	unsigned int samples;
	unsigned int bits;
	unsigned char width;  // ???

	float vol = 1.f;
	float radius = 100.0f;
	float falloff = 16.f;

	bool loop = false;      // should we loop the file when it ends?
	//double time = 0;        // time in seconds in the file
	double startTime = 0;   // time in seconds to start playback
	//double endTime = 0;     // time in seconds to end playback
	//float speed = 1.0f;

	// must be set before playing a sound
	AudioEffects effects = AudioEffect_None;
	std::vector< IAudioEffect* > aEffects{};

	bool paused = false;
	bool valid = false;
	bool preloaded = false;  // bruh

	// audio stream to store audio from the codec and covert it
	SDL_AudioStream *audioStream = nullptr;

	// stored audio data
	std::vector< float > preloadedAudio;

	// --------------------------------------------------
	// 3d sound
	glm::vec3 pos;

	// maybe std::vector will work here? try later
	//float *inBufferAudio = nullptr;
	//float *midBufferAudio = nullptr;
	//float *outBufferAudio = nullptr;

	IPLAudioBuffer outBuffer{};

	// IPLhandle directSoundEffect = {};
	// IPLhandle binauralEffect = {};

	// IPLBinauralEffect               apBinauralEffect = nullptr;
};


// ===========================================================================
// Audio Effects


class IAudioEffect
{
	//virtual void RunEffect( std::vector< float* >& buffer ) = 0;
};


class AudioEffectWorld: public IAudioEffect
{
public:
	//void RunEffect( std::vector< float* >& buffer );

private:
	glm::vec3 pos;
	IPLAudioBuffer buffer;
};

struct AudioEffectLoop: public IAudioEffect
{
	bool loop = false;      // should we loop the file when it ends?
	double startTime = 0;   // time in seconds to start playback
							//double endTime = 0;     // time in seconds to end playback
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
	virtual long            Read( AudioStream *stream, size_t size, std::vector<float> &data ) = 0;
	virtual int             Seek( AudioStream *stream, double pos ) = 0;
	virtual void            Close( AudioStream *stream ) = 0;

	virtual void            SetAudioSystem( BaseAudioSystem* system ) { apAudio = system; };
	BaseAudioSystem*        apAudio;
};


// ===========================================================================
// Audio System


class AudioSystem : public BaseAudioSystem
{
public:
	                                AudioSystem();
	                                ~AudioSystem();

	// -------------------------------------------------------------------------------------
	// General Audio System Functions
	// -------------------------------------------------------------------------------------

	virtual void                    SetListenerTransform( const glm::vec3& pos, const glm::quat& rot ) override;
	virtual void                    SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang ) override;
	virtual void                    SetPaused( bool paused ) override;
	virtual void                    SetGlobalSpeed( float speed ) override;

	virtual Handle                  LoadSound( std::string soundPath ) override;
	virtual bool                    PreloadSound( Handle stream ) override;
	virtual bool                    PlaySound( Handle sStream ) override;
	virtual void                    FreeSound( Handle sStream ) override;

	// -------------------------------------------------------------------------------------
	// Audio Stream Functions
	// -------------------------------------------------------------------------------------

	/* Is This a Valid Audio Stream? */
	virtual bool                    IsValid( Handle stream ) override;

	/* Audio Stream Volume ranges from 0.0f to 1.0f */
	virtual void                    SetVolume( Handle stream, float vol ) override;
	virtual float                   GetVolume( Handle stream ) override;

	/* Audio Stream Volume ranges from 0.0f to 1.0f */
	//virtual bool                    SetSampleRate( Handle stream, float vol ) override;
	//virtual float                   GetSampleRate( Handle stream ) override;

	/* Sound Position in World */
	virtual void                    SetWorldPos( Handle stream, const glm::vec3& pos ) override;
	virtual const glm::vec3&        GetWorldPos( Handle stream ) override;

	/* Sound Loop Parameters (make a component?) */
	virtual void                    SetLoop( Handle stream, bool loop ) override;
	virtual bool                    DoesSoundLoop( Handle stream ) override;  // um

	/* Audio Volume Channels (ex. General, Music, Voices, Commentary, etc.) */
	// virtual void                    SetChannel( Handle stream, int channel ) = 0;
	// virtual int                     GetChannel( Handle stream ) = 0;
	
	virtual void                    SetEffects( Handle stream, AudioEffects effect ) override;
	virtual AudioEffects            GetEffects( Handle stream ) override;

	virtual bool                    Seek( Handle streamPublic, double pos ) override;
	
	// -------------------------------------------------------------------------------------
	// Audio Stream Components
	// -------------------------------------------------------------------------------------

	// TODO: setup an audio component system internally
	// will need to think about it's design more

	// virtual Handle                  CreateComponent( AudioComponentType type ) = 0;
	// virtual void                    AddComponent( Handle stream, Handle component, AudioComponentType type ) = 0;
	// virtual Handle                  GetComponent( Handle stream, AudioComponentType type ) = 0;
	
	// -------------------------------------------------------------------------------------
	// Internal Functions
	// -------------------------------------------------------------------------------------

	bool                            LoadSoundInternal( AudioStream *stream );

	/* Checks If This a Valid Audio Stream, if not, throw a warning and return nullptr. */
	AudioStream*                    GetStream( Handle stream );

	//virtual bool                    RegisterCodec( BaseCodec *codec ) override;
	bool                            RegisterCodec( BaseCodec *codec );

	bool                            UpdateStream( AudioStream* stream );
	bool                            ReadAudio( AudioStream* stream );
	bool                            ApplyEffects( AudioStream* stream );
	bool                            MixAudio();
	bool                            QueueAudio();

	bool                            InitSteamAudio();
	int                             ApplySpatialEffects( AudioStream* stream, float* data, size_t frameSize );

	virtual bool                 	Init() override;
	virtual void                 	Update( float frameTime ) override;

	// --------------------------------------------------
	// steam audio settings

	IPLBinauralEffect               apBinauralEffect = nullptr;

	IPLContext                      aCtx {nullptr};

	//IPLhandle                       computeDevice {nullptr};
	//IPLhandle                       scene {nullptr};
	//IPLhandle                       probeManager {nullptr};
	//IPLhandle                       environment {nullptr};
	IPLSimulationSettings           simulationSettings {};

	//IPLhandle                       envRenderer = {};
	//IPLhandle                       renderer = {};

	// memory buffer for phonon rendering final mix
	float*                          apMixBufferAudio = nullptr;
	// IPLAudioBuffer                  mixBufferContext;
	IPLAudioBuffer                  aMixBuffer;
	std::vector<IPLAudioBuffer>     unmixedBuffers;

	// --------------------------------------------------

	std::vector< BaseCodec* >       aCodecs;
	
	MemPool                                 aStreamPool;
	ResourceManager< AudioStream* >         aStreams;         // all streams loaded into memory
	std::vector< Handle >                   aStreamsPlaying;  // streams currently playing audio for

	SDL_AudioDeviceID               aOutputDeviceID;
	SDL_AudioSpec                   aAudioSpec;

	glm::vec3                       aListenerPos = {0, 0, 0};
	glm::quat                       aListenerRot = {0, 0, 0, 0};
	bool                            aPaused = false;
	float                           aSpeed = 1.f;

	//AudioEffect3D*                  aEffect3D = new AudioEffect3D;

	//std::vector< IAudioEffect* >    aEffects;
};

