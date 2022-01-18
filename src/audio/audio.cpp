#include "audio.h"
#include "util.h"
#include "core/systemmanager.h"
#include "types/transform.h"

#if ENABLE_AUDIO
#include "codec_vorbis.h"
#include "codec_wav.h"
#include "codec_libav.h"
#endif

#include <SDL2/SDL.h>
#include <glm/gtx/transform.hpp>
#include <filesystem>
#include <thread>         // std::thread
#include <mutex>

// debugging
#include "igui.h"

AudioSystem* audio = new AudioSystem;

namespace fs = std::filesystem;

// WHY DOES THIS BREAK WHEN I CHANGE IT FROM 2048??? I HATE WORKING WITH AUDIO
constexpr size_t SOUND_BYTES = 2048;
//constexpr size_t SOUND_BYTES = 4096;
constexpr size_t SOUND_RATE = 48000;
constexpr size_t THR_MAX_STREAMS = 32;

CONVAR( snd_volume, 0.5 );
CONVAR( snd_volume_3d, 1 );
CONVAR( snd_volume_2d, 1 );
CONVAR( snd_buffer_size, 2 );

#if ENABLE_AUDIO
IPLAudioFormat g_formatMono = {};
IPLAudioFormat g_formatStereo = {};

extern "C" {
	DLL_EXPORT void* cframework_get() {
		return audio;
	}
}

bool HandleSteamAudioReturn(IPLerror ret, const char* msg)
{
	if (ret == IPL_STATUS_OUTOFMEMORY)
		Print( "[AudioSystem] %s: %d - Out Of Memory\n", msg, (int)ret );

	else if (ret == IPL_STATUS_INITIALIZATION)
		Print( "[AudioSystem] %s: %d - An error occurred while initializing an external dependency\n", msg, (int)ret );

	else if (ret == IPL_STATUS_FAILURE)
		Print( "[AudioSystem] %s: %d - Failure\n", msg, (int)ret );

	return ret == IPL_STATUS_SUCCESS;
}
#endif


AudioSystem::AudioSystem(  ) : BaseAudioSystem(  )
{
}


AudioSystem::~AudioSystem(  )
{
}

#define AUDIO_THREAD 0

#if AUDIO_THREAD

std::mutex g_audioMutex;
std::condition_variable g_cv;

#endif

void AudioSystem::Init(  )
{
#if ENABLE_AUDIO
	SDL_AudioSpec wantedSpec;

	wantedSpec.callback = NULL;
	wantedSpec.userdata = NULL;
	wantedSpec.channels = 2;
	wantedSpec.freq = SOUND_RATE;
	wantedSpec.samples = SOUND_BYTES;
	// aAudioSpec.samples = SOUND_BYTES / 2;
	wantedSpec.silence = 0;
	wantedSpec.size = 0;

	wantedSpec.format = AUDIO_F32;
	// aAudioSpec.format = AUDIO_S16;

	// TODO: be able to switch this on the fly
	// aOutputDeviceID = SDL_OpenAudioDevice( NULL, 0, &aAudioSpec, NULL, 0 );
	// aOutputDeviceID = SDL_OpenAudioDevice( NULL, 0, &aAudioSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE );
	aOutputDeviceID = SDL_OpenAudioDevice( NULL, 0, &wantedSpec, &aAudioSpec, SDL_AUDIO_ALLOW_ANY_CHANGE );
	if ( aOutputDeviceID == 0 )
	{
		Print( "[AudioSystem] SDL_OpenAudioDevice failed: %s\n", SDL_GetError() );
		return;
	}
	
	// this shouldn't be needed, but doesn't work without it, yay
	// aMasterStream = SDL_NewAudioStream(AUDIO_S16, aAudioSpec.channels, aAudioSpec.freq, aAudioSpec.format, aAudioSpec.channels, aAudioSpec.freq);
	aMasterStream = SDL_NewAudioStream(AUDIO_F32, aAudioSpec.channels, aAudioSpec.freq, aAudioSpec.format, aAudioSpec.channels, aAudioSpec.freq);

	InitSteamAudio();
	
	// why won't an std::vector work here ????
	mixBufferAudio = new float[SOUND_BYTES * 2];
	SDL_memset( mixBufferAudio, 0, sizeof( mixBufferAudio ) );

	// mixBufferContext.interleavedBuffer = mixBuffer.data();
	mixBufferContext.interleavedBuffer = mixBufferAudio;
	mixBufferContext.numSamples = aAudioSpec.samples; // * aAudioSpec.channels;
	mixBufferContext.format = g_formatStereo;

	// Load built-in codecs
	// these other 2 codecs don't work properly for some reason
#if ENABLE_VORBIS
	RegisterCodec( new CodecVorbis );
#endif

#if ENABLE_WAV
	RegisterCodec( new CodecWav );
#endif

	// experimental fallback, yet it works the best when actually playing sounds, smh
#if ENABLE_LIBAV
	RegisterCodec( new CodecLibAV );
#endif

	SDL_PauseAudioDevice( aOutputDeviceID, 0 );

#if AUDIO_THREAD

	std::thread* update = new std::thread(
	[&]()
	{
		while ( true )
		{
			std::unique_lock<std::mutex> lock(g_audioMutex);

			//std::lock_guard<std::mutex> guard(g_audioMutex);

			if ( aStreams.size() )
			{
				MixAudio();
				QueueAudio();

				unmixedBuffers.clear();
			}

			g_cv.notify_one();
		}
	}
	);
	
#endif

#endif
}


bool AudioSystem::RegisterCodec( BaseCodec *codec )
{
#if ENABLE_AUDIO
	if ( aCodecs.empty() || std::find(aCodecs.begin(), aCodecs.end(), codec) == aCodecs.end() )
	{
		aCodecs.push_back(codec);

		// maybe mix these 2 functions together?
		codec->SetAudioSystem( this );
		codec->Init(  ); 

		Print( "[AudioSystem] Loaded Audio Codec - \"%s\"\n", codec->GetName() );
		return true;
	}

	Print( "[AudioSystem] Audio Codec already loaded - \"%s\"\n", codec->GetName() );
#endif
	return false;
}


#if ENABLE_AUDIO
bool AudioSystem::InitSteamAudio()
{
	IPLerror ret = iplCreateContext( [](char* message){ Print("[AudioSystem] Steam Audio: %s", message); }, nullptr, nullptr, &context );
	if ( !HandleSteamAudioReturn( ret, "Error creating steam audio context" ) )
		return false;

	// shared variables
	IPLRenderingSettings settings{ aAudioSpec.freq, aAudioSpec.samples, IPL_CONVOLUTIONTYPE_PHONON };

	// Create binaural renderer
	IPLHrtfParams hrtfParams{ IPL_HRTFDATABASETYPE_DEFAULT, nullptr, nullptr };

	ret = iplCreateBinauralRenderer( context, settings, hrtfParams, &renderer );
	if ( !HandleSteamAudioReturn( ret, "Error creating binaural renderer" ) )
		return false;

	// Create environment (comments copied over from phonon)
	// stored in the class because im probably gonna setup some system to tweak this with convars later
	simulationSettings.ambisonicsOrder = 0; // increases the amount of directional detail; must be between 0 and 3
	simulationSettings.irDuration = 0; // delay between a sound being emitted and the last audible reflection; 0.5 to 4.0
	simulationSettings.maxConvolutionSources = 0; // allows more sound sources to be rendered with sound propagation effects, at the cost of increased memory consumption
	simulationSettings.bakingBatchSize = 0; // only used if sceneType is set to IPL_SCENETYPE_RADEONRAYS
	simulationSettings.irradianceMinDistance = 0; // Increasing this number reduces the loudness of reflections when standing close to a wall; decreasing this number results in a more physically realistic model
	simulationSettings.numBounces = 0; // 1 to 32
	simulationSettings.numDiffuseSamples = 0; // 32 to 4096
	simulationSettings.numOcclusionSamples = 0; // 32 to 512
	simulationSettings.numRays = 0; // 1024 to 131072

	ret = iplCreateEnvironment( context, nullptr, simulationSettings, nullptr, nullptr, &environment );
	if ( !HandleSteamAudioReturn( ret, "Error creating audio environment" ) )
		return false;

	// setup audio formats
	g_formatMono.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
	g_formatMono.channelLayout = IPL_CHANNELLAYOUT_MONO;
	g_formatMono.numSpeakers = 0;
	g_formatMono.channelOrder = IPL_CHANNELORDER_INTERLEAVED; //LRLRLR...

	g_formatStereo.channelLayoutType = IPL_CHANNELLAYOUTTYPE_SPEAKERS;
	g_formatStereo.channelLayout = IPL_CHANNELLAYOUT_STEREO;
	g_formatStereo.numSpeakers = 0;
	g_formatStereo.channelOrder = IPL_CHANNELORDER_INTERLEAVED;

	envRenderer = new IPLhandle;
	ret = iplCreateEnvironmentalRenderer( context, environment, settings, g_formatStereo, nullptr, nullptr, &envRenderer );
	if ( !HandleSteamAudioReturn( ret, "Error creating environmental renderer" ) )
		return false;

	return true;
}
#endif



// OpenSound?
AudioStream* AudioSystem::LoadSound( const char* soundPath )
{
#if ENABLE_AUDIO
	// check for too many streams
	if ( aStreams.size() == THR_MAX_STREAMS )
	{
		Print( "[AudioSystem] At Max Audio Streams: \"%d\"\n", THR_MAX_STREAMS );
		return nullptr;
	}

	if ( !FileExists( soundPath ) )
	{
		Print( "[AudioSystem] Sound does not exist: \"%s\"\n", soundPath );
		return nullptr;
	}

	std::string ext = fs::path(soundPath).extension().string();

	AudioStreamInternal* stream = new AudioStreamInternal;
	stream->name = soundPath;

	for ( BaseCodec* codec: aCodecs )
	{
		if ( !codec->CheckExt(ext.c_str()) )
			continue;

		if ( !codec->Open(soundPath, stream) )
			continue;

		stream->codec = codec;
		stream->valid = true;
		aStreams.push_back(stream);
		return stream;
	}

	Print("[AudioSystem] Could not load sound: \"%s\"\n", soundPath);
	delete stream;
#endif
	return nullptr;
}


bool AudioSystem::PlaySound( AudioStream *streamPublic )
{
#if !ENABLE_AUDIO
	return false;
#else
	AudioStreamInternal* stream = (AudioStreamInternal*)streamPublic;

	IPLerror ret;
	// SDL_AudioStream will convert it to stereo for us
	//IPLAudioFormat inAudioFormat = (stream->channels == 1 ? g_formatMono: g_formatStereo);
	IPLAudioFormat inAudioFormat = g_formatStereo;
	//IPLAudioFormat inAudioFormat = g_formatMono;

	if ( stream->effects & AudioEffect_Direct )
	{
		ret = iplCreateDirectSoundEffect(envRenderer, inAudioFormat, g_formatStereo, &stream->directSoundEffect);
		if (!HandleSteamAudioReturn(ret, "Error creating direct sound effect"))
			return false;
	}

	if ( stream->effects & AudioEffect_Binaural )
	{
		ret = iplCreateBinauralEffect(renderer, g_formatStereo, g_formatStereo, &stream->binauralEffect);
		if (!HandleSteamAudioReturn(ret, "Error creating binaural effect"))
			return false;
	}

	// ret = iplCreatePanningEffect(renderer, g_formatStereo, g_formatStereo, &stream->effect);
	// ret = iplCreateVirtualSurroundEffect(renderer, g_formatStereo, g_formatStereo, &stream->effect);

	// if ( (!stream->inWorld && stream->channels != 2) || stream->rate != aAudioSpec.freq || stream->format != aAudioSpec.format )
	{
		// stream->audioStream = SDL_NewAudioStream(AUDIO_F32, stream->channels, stream->rate, AUDIO_F32, 2, aAudioSpec.freq);
		// stream->audioStream = SDL_NewAudioStream(AUDIO_F32, stream->channels, stream->rate / 2, AUDIO_F32, 2, aAudioSpec.freq);
		// stream->audioStream = SDL_NewAudioStream(AUDIO_F32, stream->channels, stream->rate, AUDIO_F32, 2, aAudioSpec.freq);
		stream->audioStream = SDL_NewAudioStream(stream->format, stream->channels, stream->rate, aAudioSpec.format, 2, aAudioSpec.freq);
		if (stream->audioStream == nullptr)
		{
			Print( "[AudioSystem] SDL_NewAudioStream failed: %s\n", SDL_GetError() );
			return false;
		}
	}

	// hmm, how am i going to handle this with different effects...
	stream->inBufferAudio = new float[SOUND_BYTES * 2];
	stream->midBufferAudio = new float[SOUND_BYTES * 2];
	stream->outBufferAudio = new float[SOUND_BYTES * 2];

	SDL_memset( stream->inBufferAudio, 0, sizeof( stream->inBufferAudio ) );
	SDL_memset( stream->midBufferAudio, 0, sizeof( stream->midBufferAudio ) );
	SDL_memset( stream->outBufferAudio, 0, sizeof( stream->outBufferAudio ) );

	stream->inBuffer.interleavedBuffer = stream->inBufferAudio;
	stream->midBuffer.interleavedBuffer = stream->midBufferAudio;
	stream->outBuffer.interleavedBuffer = stream->outBufferAudio;

	stream->inBuffer.format = inAudioFormat;
	stream->midBuffer.format = g_formatStereo;
	stream->outBuffer.format = g_formatStereo;

	// Jump to start time
	if ( stream->startTime > 0 )
		stream->codec->Seek( stream, stream->startTime );

	return true;
#endif
}


void AudioSystem::FreeSound( AudioStream** streamPublic )
{
#if ENABLE_AUDIO
	AudioStreamInternal* stream = (AudioStreamInternal*)*streamPublic;

	if ( vec_contains(aStreams, stream) )
	{
		stream->codec->Close( stream );
		vec_remove(aStreams, stream);

		delete[] stream->inBufferAudio;
		delete[] stream->midBufferAudio;
		delete[] stream->outBufferAudio;

		delete stream;

		*streamPublic = nullptr;
	}
#endif
}


int AudioSystem::Seek( AudioStream *streamPublic, double pos )
{
#if !ENABLE_AUDIO
	return -1;
#else
	AudioStreamInternal* stream = (AudioStreamInternal*)streamPublic;
	return stream->codec->Seek( stream, pos );
#endif
}


void AudioSystem::SetListenerTransform( const glm::vec3& pos, const glm::quat& rot )
{
	aListenerPos = pos;
	aListenerRot = rot;
}


void AudioSystem::SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang )
{
	aListenerPos = pos;
	aListenerRot = AngToQuat(ang);
}


void AudioSystem::SetPaused( bool paused )
{
	aPaused = paused;
}


void AudioSystem::SetGlobalSpeed( float speed )
{
	aSpeed = speed;
}


void AudioSystem::Update( float frameTime )
{
#if ENABLE_AUDIO
	// not really needed tbh
	// SDL_PauseAudioDevice( aOutputDeviceID, aPaused );

	if ( aPaused )
		return;

#if AUDIO_THREAD
	std::unique_lock<std::mutex> lock(g_audioMutex);
	g_cv.wait(lock);
#else
	unmixedBuffers.clear();
#endif

	//gui->DebugMessage( 14, "Audio Streams: %u", aStreams.size() );

	int remainingAudio = SDL_GetQueuedAudioSize( aOutputDeviceID );
	static int read = remainingAudio;

	// if remaining audio is too low, update streams
	// maybe a bad idea for new streams?
	if ( remainingAudio < SOUND_BYTES * snd_buffer_size.GetFloat() )
	{
		// this might work well for multithreading
		for (int i = 0; i < aStreams.size(); i++)
		{
			if (aStreams[i]->paused)
				continue;

			if ( !UpdateStream( aStreams[i] ) )
			{
				FreeSound( (AudioStream**)&aStreams[i] );
				i--;
				continue;
			}
		}
	}

#if !AUDIO_THREAD
	if ( aStreams.size() )
	{
		MixAudio();
		QueueAudio();
	}
#endif
#endif
}


#if ENABLE_AUDIO

CONVAR( snd_read_mult, 4 );  // 4
CONVAR( snd_audio_stream_available, 2 );

bool AudioSystem::UpdateStream( AudioStreamInternal* stream )
{
	int read = SDL_AudioStreamAvailable( stream->audioStream );
	if ( read < SOUND_BYTES * snd_audio_stream_available.GetFloat() )
	{
		std::vector<float> rawAudio;

		read = stream->codec->Read( stream, SOUND_BYTES, rawAudio );

		// should just put silence in the stream if this is true
		if (read < 0)
			return true;

		// End of File
		if (read == 0)
		{
			if ( !stream->loop )
				return false;

			// go back to the starting point
			if ( stream->codec->Seek( stream, stream->startTime ) == -1 )
				return false;  // failed to seek back, stop the sound

			read = stream->codec->Read( stream, SOUND_BYTES, rawAudio );

			// should just put silence in the stream if this is true
			if (read < 0)
				return true;
		}

		// NOTE: "read" NEEDS TO BE 4 TIMES THE AMOUNT FOR SOME REASON, no clue why
		if ( SDL_AudioStreamPut( stream->audioStream, rawAudio.data(), read*snd_read_mult ) == -1 )
		{
			Print( "[AudioSystem] SDL_AudioStreamPut - Failed to put samples in stream: %s\n", SDL_GetError() );
			return false;
		}
	}

	// this is in bytes, not samples!
	std::vector<float> outAudio;
	outAudio.resize( SOUND_BYTES );
	//outAudio = rawAudio;
	read = SDL_AudioStreamGet( stream->audioStream, outAudio.data(), SOUND_BYTES );
	//read = SOUND_BYTES;

	if (read == -1)
	{
		Print( "[AudioSystem] SDL_AudioStreamGet - Failed to get converted data: %s\n", SDL_GetError() );
		return false;
	}

	// apply stream volume
	for (int i = 0; i < read; i++)
		outAudio[i] *= stream->vol;

	// TODO: split these effects up
	if ( stream->effects & AudioEffectPreset_World )
	{
		ApplySpatialEffects( stream, outAudio.data(), read );
	}
	else
	{
		SDL_memset( stream->outBufferAudio, 0, sizeof( stream->outBufferAudio ) );

		// copy over samples
		for (int i = 0; i < read; i++)
			stream->outBufferAudio[i] = outAudio[i] * snd_volume_2d.GetFloat();
	}

	unmixedBuffers.push_back(stream->outBuffer);

	stream->frame += read;
	return true;
}


#define USE_MASTER_STREAM 0


bool AudioSystem::MixAudio()
{
	if (!unmixedBuffers.size())
		return true;

	iplMixAudioBuffers( unmixedBuffers.size(), unmixedBuffers.data(), mixBufferContext );

#if USE_MASTER_STREAM
	if ( SDL_AudioStreamPut( aMasterStream, unmixedBuffers[0].interleavedBuffer, SOUND_BYTES ) == -1 )
	{
		Print( "SDL_AudioStreamPut - Failed to put samples in stream: %s\n", SDL_GetError() );
		return false;
	}
#endif

	return true;
}


bool AudioSystem::QueueAudio()
{
#if USE_MASTER_STREAM
	// this is in bytes, not samples!
	int gotten = SDL_AudioStreamGet( aMasterStream, converted, SOUND_BYTES );

	if (gotten == 0)
		return true;

	if (gotten == -1)
	{
		Print( "SDL_AudioStreamGet - Failed to get converted data: %s\n", SDL_GetError() );
		return false;
	}

	// apply global volume to stream
	for (uint32_t i = 0; i < gotten; i++)
	{
		converted[i] *= snd_volume.GetFloat();
	}

	if ( SDL_QueueAudio( aOutputDeviceID, converted, gotten ) == -1 )
	{
		Print( "SDL_QueueAudio Error: %s\n", SDL_GetError() );
		return false;
	}
#else

	if (!unmixedBuffers.size())
		return true;
	
	// apply global volume to stream
	for (uint32_t i = 0; i < SOUND_BYTES; i++)
	{
		mixBufferContext.interleavedBuffer[i] *= snd_volume.GetFloat();
	}

	if ( SDL_QueueAudio( aOutputDeviceID, mixBufferContext.interleavedBuffer, SOUND_BYTES ) == -1 )
	{
		Print( "[AudioSystem] SDL_QueueAudio Error: %s\n", SDL_GetError() );
		return false;
	}

#endif

	return true;
}


IPLVector3 toPhonon( const glm::vec3& vector )
{
	return { vector.x, vector.z, -vector.y };
}

float calculateDistanceAttenutation( const glm::vec3& listener, const glm::vec3& source, float radius, float falloffPower )
{
	float distance = glm::length(listener - source);

	if (distance >= radius)
		return 0;

	return glm::pow(glm::clamp(1.f - (distance * 1.f / radius), 0.f, 1.f), falloffPower);
}


int AudioSystem::ApplySpatialEffects( AudioStreamInternal* stream, float* data, size_t frameCount )
{
	SDL_memset( stream->inBufferAudio, 0, sizeof( stream->inBufferAudio ) );
	SDL_memset( stream->midBufferAudio, 0, sizeof( stream->midBufferAudio ) );
	SDL_memset( stream->outBufferAudio, 0, sizeof( stream->outBufferAudio ) );

	stream->inBuffer.numSamples = frameCount;
	stream->midBuffer.numSamples = frameCount;
	stream->outBuffer.numSamples = frameCount;

	// copy over samples
	//for (uint32_t i = 0; i < frameCount * stream->channels; i++)
	for (uint32_t i = 0; i < frameCount; i++)
		stream->inBufferAudio[i] = data[i] * snd_volume_3d.GetFloat();

	IPLDirectSoundPath soundPath = {};
	soundPath.distanceAttenuation = calculateDistanceAttenutation( aListenerPos, stream->pos, stream->radius, stream->falloff );

	IPLVector3 direction = toPhonon(aListenerRot * glm::normalize(stream->pos - aListenerPos));
	// uh
	//soundPath.direction = direction;

	// sound is too far away to hear
	if ( soundPath.distanceAttenuation == 0.f )
		return 0;

	IPLDirectSoundEffectOptions directSoundOptions = {IPL_TRUE, IPL_FALSE, IPL_FALSE, IPL_DIRECTOCCLUSION_NONE};

	// TODO: split each effect up into it's own function in the audio system
	// also this isn't working at the moment so that's cool
	iplApplyDirectSoundEffect(
		stream->directSoundEffect,
		stream->inBuffer,
		soundPath,
		directSoundOptions,
		stream->midBuffer
	);

	//iplGetDirectSoundPath

	//gui->DebugMessage( 11, "Sound Dir: %s", Vec2Str({direction.x, direction.y, direction.z}).c_str() );

	// now, why does this sound so glitchy?
	iplApplyBinauralEffect(
		stream->binauralEffect,
		renderer,
		stream->midBuffer,
		direction,
		IPL_HRTFINTERPOLATION_BILINEAR,
		stream->outBuffer
	);

	return 0;
}

#endif
