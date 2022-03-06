#pragma once

#define ENABLE_AUDIO 69420

#include "system.h"
#include "iaudio.h"

#include <SDL2/SDL.h>

#if ENABLE_AUDIO
#include <phonon.h>
#endif /* ENABLE_AUDIO  */

class BaseCodec;

struct AudioStreamInternal: public AudioStream
{
#if ENABLE_AUDIO
	BaseCodec *codec = nullptr;
	void* data;  // data for the codec only

	// other stuff
	SDL_AudioFormat format = AUDIO_F32;
	unsigned int frameSize = 0;
	unsigned int size;
	unsigned int samples;
	unsigned int bits;
	unsigned char width;  // ???

	// audio stream to store audio from the codec and covert it
	SDL_AudioStream *audioStream = nullptr;

	// maybe std::vector will work here? try later
	float *inBufferAudio = nullptr;
	float *midBufferAudio = nullptr;
	float *outBufferAudio = nullptr;

	IPLAudioBuffer inBuffer;
	IPLAudioBuffer midBuffer;
	IPLAudioBuffer outBuffer;

	IPLhandle directSoundEffect = {};
	IPLhandle binauralEffect = {};
#endif
};


class BaseCodec
{
protected:
	virtual                 ~BaseCodec() = default;

public:
	virtual bool            Init() = 0;
	virtual const char*     GetName() = 0;
	virtual bool            CheckExt( const char* ext ) = 0;

	virtual bool            Open( const char* soundPath, AudioStreamInternal *stream ) = 0;
	virtual long            Read( AudioStreamInternal *stream, size_t size, std::vector<float> &data ) = 0;
	virtual int             Seek( AudioStreamInternal *stream, double pos ) = 0;
	virtual void            Close( AudioStreamInternal *stream ) = 0;

	virtual void            SetAudioSystem( BaseAudioSystem* system ) { apAudio = system; };
	BaseAudioSystem*        apAudio;
};


class AudioSystem : public BaseAudioSystem
{
public:
	                                AudioSystem();
	                                ~AudioSystem();

	virtual void                    SetListenerTransform( const glm::vec3& pos, const glm::quat& rot ) override;
	virtual void                    SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang ) override;
	virtual void                    SetPaused( bool paused ) override;
	virtual void                    SetGlobalSpeed( float speed ) override;

	virtual Handle                  LoadSound( std::string soundPath ) override;
	virtual bool                    PreloadSound( AudioStream *stream ) override;
	virtual bool                    PlaySound( Handle sStream ) override;
	virtual void                    FreeSound( Handle sStream ) override;
	virtual int                     Seek( AudioStream *streamPublic, double pos ) override;

	bool                            LoadSoundInternal( AudioStreamInternal *stream );

	//virtual bool                    RegisterCodec( BaseCodec *codec ) override;
	bool                            RegisterCodec( BaseCodec *codec );

#if ENABLE_AUDIO
	bool                            UpdateStream( AudioStreamInternal* stream );
	bool                            ReadAudio( AudioStreamInternal* stream );
	bool                            ApplyEffects( AudioStreamInternal* stream );
	bool                            MixAudio();
	bool                            QueueAudio();

	bool                            InitSteamAudio();
	int                             ApplySpatialEffects( AudioStreamInternal* stream, float* data, size_t frameSize );
#endif

	virtual void                 	Init(  ) override;
	virtual void                 	Update( float frameTime ) override;

	// --------------------------------------------------
	// steam audio settings

#if ENABLE_AUDIO
	IPLhandle                       rendererBinaural = nullptr;

	IPLhandle                       context {nullptr};

	IPLhandle                       computeDevice {nullptr};
	IPLhandle                       scene {nullptr};
	IPLhandle                       probeManager {nullptr};
	IPLhandle                       environment {nullptr};
	IPLSimulationSettings           simulationSettings {};

	IPLhandle                       envRenderer = {};
	IPLhandle                       renderer = {};

	// memory buffer for phonon rendering final mix
	float*                          mixBufferAudio = nullptr;
	IPLAudioBuffer                  mixBufferContext;
	std::vector<IPLAudioBuffer>     unmixedBuffers;

	// --------------------------------------------------

	std::vector< BaseCodec* >       aCodecs;
	
	MemPool                                 aStreamPool;
	ResourceManager< AudioStreamInternal* > aStreams;         // all streams loaded into memory
	std::vector< Handle >                   aStreamsPlaying;  // streams currently playing audio for

	SDL_AudioDeviceID               aOutputDeviceID;
	SDL_AudioSpec                   aAudioSpec;
	SDL_AudioStream*                aMasterStream = nullptr;
#endif

	glm::vec3                       aListenerPos = {0, 0, 0};
	glm::quat                       aListenerRot = {0, 0, 0, 0};
	bool                            aPaused = false;
	float                           aSpeed = 1.f;
};

