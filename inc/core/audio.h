#pragma once

#include "../shared/system.h"
#include "../shared/baseaudio.h"

#include <SDL2/SDL.h>
#include <phonon.h>


#define ENABLE_AUDIO 0


struct AudioStreamInternal: public AudioStream
{
#if ENABLE_AUDIO
	// audio stream for any conversion potentially needed
	SDL_AudioStream *convertStream = nullptr;

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


class AudioSystem : public BaseAudioSystem
{
public:
	                                AudioSystem();
	                                ~AudioSystem();

	virtual void                    SetListenerTransform( const glm::vec3& pos, const glm::quat& rot ) override;
	virtual void                    SetPaused( bool paused ) override;
	virtual void                    SetGlobalSpeed( float speed ) override;

	virtual bool                    LoadSound( const char* soundPath, AudioStream** stream ) override;
	virtual bool                    PlaySound( AudioStream *stream ) override;
	virtual void                    FreeSound( AudioStream** stream ) override;

	virtual bool                    RegisterCodec( BaseCodec *codec ) override;

#if ENABLE_AUDIO
	bool                            UpdateStream( AudioStreamInternal* stream );
	bool                            MixAudio();
	bool                            QueueAudio();

	bool                            InitSteamAudio();
	int                             ApplySpatialEffects( AudioStreamInternal* stream, float* data, size_t frameSize );
#endif

	/* Initialize system.  */
	virtual void                 	Init(  ) override;

	/* Initialize system.  */
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
	std::vector< AudioStreamInternal* > aStreams;

	SDL_AudioDeviceID               aOutputDeviceID;
	SDL_AudioSpec                   aAudioSpec;
	SDL_AudioStream*                aMasterStream = nullptr;
#endif

	glm::vec3                       aListenerPos = {0, 0, 0};
	glm::quat                       aListenerRot = {0, 0, 0, 0};
	bool                            aPaused = false;
	float                           aSpeed = 1.f;
};

