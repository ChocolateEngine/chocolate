#include "audio.h"
#include "core/core.h"
#include "types/transform.h"

#include "codec_vorbis.h"
#include "codec_wav.h"
#include "codec_libav.h"

#include <SDL2/SDL.h>
#include <glm/gtx/transform.hpp>
#include <filesystem>
#include <thread>         // std::thread
#include <mutex>

// debugging
#include "igui.h"

LOG_REGISTER_CHANNEL2( Aduio, LogColor::Green );
LOG_REGISTER_CHANNEL2( OpenAL, LogColor::Green );

AudioSystem* audio = new AudioSystem;

// WHY DOES THIS BREAK WHEN I CHANGE IT FROM 8192??? I HATE WORKING WITH AUDIO
//constexpr size_t FRAME_SIZE = 2048;
// constexpr size_t SAMPLE_SIZE = 8192;
// constexpr size_t FRAME_SIZE = sizeof(float);  // uh, technically, this is actually (samples * width * channels)
constexpr size_t READ_SIZE = 2048;
constexpr size_t SOUND_RATE = 48000;

Handle gDefaultChannel = audio->RegisterChannel( "Default" );

std::mutex g_audioMutex;


CONVAR( snd_volume_3d, 1 );
CONVAR( snd_volume_2d, 1 );
CONVAR( snd_buffer_size, 4 );
CONVAR( snd_read_mult, 4 );  // 4
CONVAR( snd_audio_stream_available, 2 );

//IPLAudioFormat g_formatMono = {};
//IPLAudioFormat g_formatStereo = {};

static ModuleInterface_t gInterfaces[] = {
	{ audio, IADUIO_NAME, IADUIO_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* cframework_GetInterfaces( size_t& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}


// ============================================================================
// Util Funcs
// ============================================================================


const char* AlErrorToStr( ALCenum err )
{
	if ( err == 0 )
		return "AL_NO_ERROR";

	if ( err == AL_INVALID_NAME )
		return "AL_INVALID_NAME";

	if ( err == AL_INVALID_ENUM )
		return "AL_INVALID_ENUM";

	if ( err == AL_INVALID_VALUE )
		return "AL_INVALID_VALUE";

	if ( err == AL_INVALID_OPERATION )
		return "AL_INVALID_OPERATION";

	if ( err == AL_OUT_OF_MEMORY )
		return "AL_OUT_OF_MEMORY";

	return "Unknown OpenAL Error";
}


bool HandleALErr( const char* spMsg )
{
	ALCenum error = alGetError();

	if ( error == AL_NO_ERROR )
		return true;

	// alcCon

	Log_ErrorF( gLC_OpenAL, "%s: %s - %d\n", spMsg, AlErrorToStr( error ), error );
	return false;
}


void LogALErr( const char* spMsg )
{
	ALCenum error = alGetError();

	Log_ErrorF( gLC_OpenAL, "%s: %s - %d\n", spMsg, AlErrorToStr( error ), error );
}


void LogALErrF( const char* spFmt, ... )
{
	ALCenum error = alGetError();

	va_list args;
	va_start( args, spFmt );
	int len = std::vsnprintf( nullptr, 0, spFmt, args );

	if ( len < 0 )
	{
		Log_Error( gLC_OpenAL, "vsnprintf failed?\n\n" );
		return;
	}

	std::string msg( std::size_t( len ) + 1, '\0' );
	std::vsnprintf( msg.data(), msg.size(), spFmt, args );
	msg.resize( len );
	va_end( args );

	Log_ErrorF( gLC_OpenAL, "%s: %s - %d\n", msg.c_str(), AlErrorToStr( error ), error );
}


ALint GetBuffer( AudioStream* stream )
{
	if ( !stream )
		return 0;

	ALint buffer;
	alGetSourcei( stream->aSource, AL_BUFFERS_QUEUED, &buffer );
	return buffer;
}


ALuint GetAudioFormat( AudioStream* stream )
{
	if ( stream->channels == 2 )
		return AL_FORMAT_STEREO_FLOAT32;
	else
		return AL_FORMAT_MONO_FLOAT32;


	if ( stream->width == 1 )
	{
		if ( stream->channels == 2 )
			return AL_FORMAT_STEREO8;

		return AL_FORMAT_MONO8;
	}

	if ( stream->width == 2 )
	{
		if ( stream->channels == 2 )
			return AL_FORMAT_STEREO16;

		return AL_FORMAT_MONO16;
	}

	return AL_FORMAT_MONO_FLOAT32;
}


// NOTE: technically this is specific to sidury, but i can find a solution later when cleaning this up
void ToAlVec( const glm::vec3& in, float out[3] )
{
	// -aListenerPos.x, aListenerPos.z, aListenerPos.y
	out[0] = -in.x;
	out[1] = in.z;
	out[2] = in.y;
}


const char* gAudioEffectStr[] = {
	"None",
	"World",
	"Loop",
};


const char* Effect2Str( AudioEffect effect )
{
	switch ( effect )
	{
		case AudioEffect_None:
			return "None";

		case AudioEffect_World:
			return "World";

		case AudioEffect_Loop:
			return "Loop";

		default:
			return "UNKNOWN AUDIO EFFECT";
	}
}


const char* EffectData2Str( AudioEffectData effect )
{
	switch ( effect )
	{
		case AudioData_None:
			return "AudioData_None";

		// ===================================
		// Loop Effect

		case Audio_Loop_Enabled:
			return "Audio_Loop_Enabled";

		case Audio_Loop_StartTime:
			return "Audio_Loop_StartTime";

		case Audio_Loop_EndTime:
			return "Audio_Loop_EndTime";

		// ===================================
		// World Effect

		case Audio_World_Pos:
			return "Audio_World_Pos";

		case Audio_World_Falloff:
			return "Audio_World_Falloff";

		// ===================================

		default:
			return "UNKNOWN AUDIO EFFECT";
	}
}


const char* AudioVar2Str( AudioVar var )
{
	switch ( var )
	{
		case AudioVar::Float:
			return "float";

		case AudioVar::Int:
			return "int";

		case AudioVar::Vec3:
			return "vec3";

		case AudioVar::Invalid:
			return "INVALID (0)";

		default:
			return "UNKNOWN AUDIO VAR";
	}
}


// ============================================================================
// ConVar Funcs
// ============================================================================


CONVAR_CMD( snd_volume, 0.5 )
{
	alListenerf( AL_GAIN, snd_volume.GetFloat() );
}

CONVAR_CMD( snd_hrtf, 1 )
{
}


// ============================================================================
// Audio Stream Vars
// ============================================================================


AudioEffectVar* AudioStream::GetVar( AudioEffectData name )
{
	for ( auto var : aVars )
	{
		if ( var->aName == name )
			return var;
	}

	return nullptr;
}

template <typename T>
AudioEffectVar* AudioStream::CreateVar( AudioEffectData name, T data )
{
	AudioEffectVar *var = new AudioEffectVar( name, data );
	aVars.push_back( var );
	return var;
}


bool AudioStream::RemoveVar( AudioEffectData name )
{
	for ( size_t i = 0; i < aVars.size(); i++ )
	{
		if ( aVars[i]->aName == name )
		{
			delete aVars[i];
			vec_remove_index( aVars, i );
			return true;
		}
	}

	Log_WarnF( gLC_Aduio, "Trying to Remove non-existent AudioVar on Audio Stream: %s\n", EffectData2Str( name ) );
	return false;
}


#define SET_VAR( func, type ) \
	for ( auto var : aVars ) { \
		if ( var->aName == name ) { \
			if ( var->aType != type ) { \
				Log_WarnF( gLC_Aduio, "Audio Var Does not take type of \"%s\"\n", AudioVar2Str(type) ); \
			} else { \
				var->func( data ); \
			} \
			return; \
		} \
	} \
	Log_WarnF( gLC_Aduio, "Audio Var Not Defined for Effect Data: %s\n", EffectData2Str(name) )


void AudioStream::SetVar( AudioEffectData name, float data )              { SET_VAR( SetFloat, AudioVar::Float );   }
void AudioStream::SetVar( AudioEffectData name, int data )                { SET_VAR( SetInt,   AudioVar::Int );     }
void AudioStream::SetVar( AudioEffectData name, const glm::vec3 &data )   { SET_VAR( SetVec3,  AudioVar::Vec3 );    }

#undef SET_VAR

static glm::vec3 vec3_zero{};

#define GET_VAR( func, ret ) \
	AudioEffectVar *var = GetVar( name ); \
	if ( var == nullptr ) { \
		Log_WarnF( gLC_Aduio, "Audio Var Not Defined for Effect Data: %s\n", EffectData2Str(name) ); \
		return ret; \
	} \
	return var->func( ret )

float               AudioStream::GetFloat(   AudioEffectData name )     { GET_VAR( GetFloat, 0.f );         }
int                 AudioStream::GetInt(     AudioEffectData name )     { GET_VAR( GetInt, 0 );             }
const glm::vec3&    AudioStream::GetVec3(    AudioEffectData name )     { GET_VAR( GetVec3, vec3_zero );    }

#undef GET_VAR


// ============================================================================
// Audio System
// ============================================================================


AudioSystem::AudioSystem(  ) : IAudioSystem(  )
{
}


AudioSystem::~AudioSystem(  )
{
}


bool AudioSystem::Init()
{
    /*
	 *    Preallocate 128 MB of memory for audio processing.
	 */
	// aStreamPool.Resize( 128000000 );
	// aStreamPool.SetStepSize( 1000000 );

	aStreams.Allocate( MAX_AUDIO_STREAMS * 2 );
	aChannels.Allocate( MAX_AUDIO_CHANNELS );

	SDL_AudioSpec wantedSpec;

	wantedSpec.callback = NULL;
	wantedSpec.userdata = NULL;
	wantedSpec.channels = 2;
	wantedSpec.freq = SOUND_RATE;
	wantedSpec.samples = READ_SIZE * 4;  // width(2) * channels(2)
	// aAudioSpec.samples = SAMPLE_SIZE / 2;
	wantedSpec.silence = 0;
	wantedSpec.size = 0;

	wantedSpec.format = AUDIO_F32;
	// aAudioSpec.format = AUDIO_S16;

#if 0
	// TODO: be able to switch this on the fly
	// aOutputDeviceID = SDL_OpenAudioDevice( NULL, 0, &aAudioSpec, NULL, 0 );
	// aOutputDeviceID = SDL_OpenAudioDevice( NULL, 0, &aAudioSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE );
	aOutputDeviceID = SDL_OpenAudioDevice( NULL, 0, &wantedSpec, &aAudioSpec, SDL_AUDIO_ALLOW_ANY_CHANGE );
	if ( aOutputDeviceID == 0 )
	{
		Log_Msg( gLC_Aduio, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError() );
		return;
	}
#endif

	InitOpenAL();
	
	// why won't an std::vector work here ????
	//apMixBufferAudio = new float[SAMPLE_SIZE * 2];
	//SDL_memset( apMixBufferAudio, 0, sizeof( apMixBufferAudio ) );

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

	// SDL_PauseAudioDevice( aOutputDeviceID, 0 );

	std::thread* update = new std::thread(
	[&]()
	{
		while ( true )
		{
			if ( aPaused || aStreamsPlaying.size() == 0 )
			{
				SDL_Delay( 5 );
				continue;
			}
			else
			{
				sys_sleep( 0.5 );
			}

			// NOTHING WORKS YET
			// continue;

			g_audioMutex.lock();

			// TODO: use this for functions to modify how the stream is played during playback
			// std::unique_lock<std::mutex> lock(g_audioMutex);

			// int remainingAudio = SDL_GetQueuedAudioSize( aOutputDeviceID );
			int remainingAudio = 0;
			static int read = remainingAudio;

			float pos[3];
			ToAlVec( aListenerPos, pos );
			alListener3f( AL_POSITION, pos[0], pos[1], pos[2] );

			HandleALErr( "Failed to Set Listener Position" );
			//if ( !HandleALErr( "Failed to Set Listener Position" ) )
			//	continue;

			float vel[3];
			ToAlVec( aListenerVel, vel );
			alListener3f( AL_VELOCITY, vel[0], vel[1], vel[2] );

			HandleALErr( "Failed to Set Listener Velocity" );

			// if ( remainingAudio < SAMPLE_SIZE * snd_buffer_size.GetFloat() )
			{
				// this might work well for multithreading
				for ( int i = 0; i < aStreamsPlaying.size(); i++ )
				{
					auto stream = *aStreams.Get( aStreamsPlaying[i] );
					if ( stream->paused )
						continue;

					if ( !UpdateStream( stream ) )
					{
						// funny
						ALint bufferCount;
						alGetSourcei( stream->aSource, AL_BUFFERS_QUEUED, &bufferCount );

						if ( bufferCount > 0 )
							continue;

						FreeSound( aStreamsPlaying[i] );
						i--;
						continue;
					}
				}
			}

			if ( aStreamsPlaying.size() )
			{
				MixAudio();
				QueueAudio();

				// unmixedBuffers.clear();
			}

			// alcProcessContext( apOutContext );

			g_audioMutex.unlock();

			// g_cv.notify_one();
		}
	}
	);

	return true;
}


bool AudioSystem::RegisterCodec( BaseCodec *codec )
{
	if ( aCodecs.empty() || std::find(aCodecs.begin(), aCodecs.end(), codec) == aCodecs.end() )
	{
		aCodecs.push_back(codec);

		// maybe mix these 2 functions together?
		codec->apAudio = this;
		codec->Init(  ); 

		Log_MsgF( gLC_Aduio, "Loaded Audio Codec - \"%s\"\n", codec->GetName() );
		return true;
	}

	Log_WarnF( gLC_Aduio, "Audio Codec already loaded - \"%s\"\n", codec->GetName() );
	return false;
}


void GetALDevices( std::vector< std::string >& results )
{
	const ALCchar *names = alcGetString( nullptr, ALC_ALL_DEVICES_SPECIFIER );

	while ( names && *names )
	{
		results.emplace_back( names );
		names += strlen( names ) + 1;
	}
}


void snd_device_dropdown(
	const std::vector< std::string >& args,    // arguments currently typed in by the user
	std::vector< std::string >& results )      // results to populate the dropdown list with
{
	const ALCchar *names = alcGetString( nullptr, ALC_ALL_DEVICES_SPECIFIER );

	std::string result = "\"";  // dumb
	while ( names && *names )
	{
		results.push_back( (result + names) + "\"" );
		names += strlen( names ) + 1;
	}
}


// TODO: set a dropdown for this once snd_list_devices works
CONCMD_DROP( snd_init_device, snd_device_dropdown )
{
	g_audioMutex.lock();

	if ( args.size() >= 1 )
		audio->OpenOutputDevice( args[0].c_str() );
	else
		audio->OpenOutputDevice();

	g_audioMutex.unlock();
}


CONCMD( snd_list_devices )
{
	const ALCchar *names = alcGetString( nullptr, ALC_ALL_DEVICES_SPECIFIER );

	Log_Msg( gLC_Aduio, "Audio Devices:\n" );

	while ( names && *names )
	{
		Log_MsgF( gLC_Aduio, "\t%s\n", names );
		names += strlen( names ) + 1;
	}
}


CONCMD( snd_openal_info )
{
	Log_Msg( gLC_Aduio, "OpenAL Info:\n" );
	Log_MsgF( gLC_Aduio, "\tVendor:           %s\n", alGetString( AL_VENDOR ) );
	Log_MsgF( gLC_Aduio, "\tVersion:          %s\n", alGetString( AL_VERSION ) );
	Log_MsgF( gLC_Aduio, "\tRenderer:         %s\n", alGetString( AL_RENDERER ) );
	Log_MsgF( gLC_Aduio, "\tOutput Device:    %s\n", alcGetString( audio->apOutDevice, ALC_ALL_DEVICES_SPECIFIER ) );

	// List All Output Devices
	const ALCchar *names = alcGetString( nullptr, ALC_ALL_DEVICES_SPECIFIER );

	Log_Msg( gLC_Aduio, "\tAudio Devices:\n" );

	while ( names && *names )
	{
		Log_MsgF( gLC_Aduio, "\t\t%s\n", names );
		names += strlen( names ) + 1;
	}

	// List Extensions
	Log_Msg( gLC_Aduio, "\n" );
	Log_MsgF( gLC_Aduio, "\tAL Extensions:    %s\n\n", alGetString( AL_EXTENSIONS ) );
	Log_MsgF( gLC_Aduio, "\tALC Extensions:   %s\n\n", alcGetString( audio->apOutDevice, ALC_EXTENSIONS ) );
}


bool AudioSystem::OpenOutputDevice( const char* name )
{
	if ( apOutDevice )
		CloseOutputDevice();

	// Get Default Audio Device and open it
	// const ALCchar* defaultDeviceStr = alcGetString( nullptr, ALC_DEFAULT_DEVICE_SPECIFIER );

	// Open Default Audio Device
	apOutDevice = alcOpenDevice( name );

	if ( !apOutDevice )
	{
		if ( name == nullptr )
			LogALErr( "Failed to Open Default Audio Output Device" );
		else
			LogALErrF( "Failed to Open Audio Output Device \"%s\"", name );

		return false;
	}

	apOutContext = alcCreateContext( apOutDevice, NULL );

	ALCboolean ret = alcMakeContextCurrent( apOutContext );

	Log_Dev( gLC_Aduio, 1, "Opened Default Audio Output Device\n" );

	return ret;
}


bool AudioSystem::CloseOutputDevice()
{
	alcMakeContextCurrent( NULL );

	if ( apOutContext )
		alcDestroyContext( apOutContext );

	if ( apOutDevice )
	{
		ALCboolean ret = alcCloseDevice( apOutDevice );

		if ( !ret )
		{
			LogALErr( "Failed to Close Audio Output Device" );
			return false;
		}
	}

	return true;
}


bool gEAX = false;


#if 0
enum class DistanceModel {
	InverseClamped  = AL_INVERSE_DISTANCE_CLAMPED,
	LinearClamped   = AL_LINEAR_DISTANCE_CLAMPED,
	ExponentClamped = AL_EXPONENT_DISTANCE_CLAMPED,
	Inverse  = AL_INVERSE_DISTANCE,
	Linear   = AL_LINEAR_DISTANCE,
	Exponent = AL_EXPONENT_DISTANCE,
	None = AL_NONE,
};
#endif


bool AudioSystem::InitOpenAL()
{
	if ( !OpenOutputDevice( nullptr ) )
		return false;

	// Check for EAX 2.0 support
	gEAX = alIsExtensionPresent("EAX2.0");

	// Generate Buffers
	alGetError(); // clear error code

	// apBuffers = (ALCuint*)malloc( sizeof( ALCuint ) * MAX_AUDIO_STREAMS );

	alGenSources( MAX_AUDIO_STREAMS, apSources );

	if ( !HandleALErr( "Failed to Gen Sources" ) )
		return false;

	alGenBuffers( MAX_AUDIO_STREAMS, apBuffers );

	if ( !HandleALErr( "Failed to Gen Buffers" ) )
		return false;

	// uh
	// alDistanceModel( AL_LINEAR_DISTANCE );
	alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );

	alListenerf( AL_GAIN, snd_volume.GetFloat() );

	return true;
}


// TEMP
CONVAR( snd_min_distance, 120 );

// excess error handling lol
bool AudioSystem::LoadSoundInternal( AudioStream *stream )
{
	// attach buffer to source 
	// alSourcei( apSources[aBufferCount], AL_BUFFER, apBuffers[aBufferCount] );

	stream->format = GetAudioFormat( stream );

	if ( stream->channels == 2 )
		stream->format = AL_FORMAT_STEREO_FLOAT32;
	else
		stream->format = AL_FORMAT_MONO_FLOAT32;

	// lets use this for now
	stream->aBuffers.resize( MAX_SOURCE_BUFFERS );
	alGenBuffers( MAX_SOURCE_BUFFERS, stream->aBuffers.data() );

	if ( !HandleALErr( "Failed to Gen Buffers" ) )
		return false;

	stream->aSource = apSources[aBufferCount];

	alSourcef( stream->aSource, AL_PITCH, 1.0f );

	if ( !HandleALErr( "Failed to Set Source Pitch" ) )
		return false;

	alSourcef( stream->aSource, AL_REFERENCE_DISTANCE, snd_min_distance.GetFloat() );

	if ( !HandleALErr( "Failed to Set Source Reference Distance" ) )
		return false;

	alSourcef( stream->aSource, AL_SOURCE_RELATIVE, AL_TRUE );

	if ( !HandleALErr( "Failed to Set Source to Relative" ) )
		return false;

	stream->aChannel = gDefaultChannel;

	// if ( !HandleALErr( "Failed to Attach Buffer to Source" ) )
	//	return false;

#if 0
	// TODO: REMOVE THIS
	stream->audioStream = SDL_NewAudioStream( AUDIO_F32, stream->channels, stream->rate, AUDIO_F32, 2, 48000 );
	if (stream->audioStream == nullptr)
	{
		Log_Error( gLC_Aduio, "SDL_NewAudioStream failed: %s\n", SDL_GetError() );
		return false;
	}
#endif

	// Jump to start time
	// if ( stream->startTime > 0 )
	//	stream->codec->Seek( stream, stream->startTime );

	return true;
}


/* Checks If This a Valid Audio Stream, if not, throw a warning and return nullptr. */
AudioStream* AudioSystem::GetStream( Handle streamHandle )
{
	if ( streamHandle == InvalidHandle )
		return nullptr;

	AudioStream *stream = *aStreams.Get( streamHandle );

	if ( !stream )
		Log_WarnF( gLC_Aduio, "Invalid Stream Handle: %u\n", (size_t)streamHandle );

	return stream;
}



void AudioSystem::SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang )
{
	aListenerPos = pos;
	aListenerRot = ang;
}


void AudioSystem::SetListenerVelocity( const glm::vec3& vel )
{
	aListenerVel = vel;
}


// void AudioSystem::SetListenerOrient( float orient[6] )
void AudioSystem::SetListenerOrient( const glm::vec3& forward, const glm::vec3& up )
{
	// std::memcpy( aListenerOrient, orient, 6 );

	// *aListenerOrient = *orient;

	// BAD, but oh well
	float forwardAl[3], upAl[3];
	ToAlVec( forward, forwardAl );
	ToAlVec( up, upAl );

	ALfloat orient[6] = {
		forwardAl[0],
		forwardAl[1],
		forwardAl[2],
		upAl[0],
		upAl[1],
		upAl[2],
	};

	alListenerfv( AL_ORIENTATION, orient );

	HandleALErr( "Failed to Set Listener Orientation" );
}


void AudioSystem::SetDopplerScale( float scale )
{
	alDopplerFactor( scale );

	HandleALErr( "Failed to Set Doppler Factor" );
}


void AudioSystem::SetSoundSpeed( float speed )
{
	alSpeedOfSound( speed );

	HandleALErr( "Failed to Set Speed of Sound" );
}


void AudioSystem::SetPaused( bool paused )
{
	aPaused = paused;
}


void AudioSystem::SetGlobalSpeed( float speed )
{
	aSpeed = speed;
}


void AudioSystem::SetOccluder( IAudioOccluder* spOccluder )
{
	apOccluder = spOccluder;
}


IAudioOccluder* AudioSystem::GetOccluder()
{
	return apOccluder;
}


// -------------------------------------------------------------------------------------
// Audio Channels
// -------------------------------------------------------------------------------------


// probably fine to just set the const char directly
// but how do i limit the channel name size?
Handle AudioSystem::RegisterChannel( const char* name )
{
	// TODO: probably use the memory pool for this
	AudioChannel* channel = new AudioChannel;
	channel->aName = name;
	channel->aVol = 1.f;

	Handle handle = aChannels.Add( channel );
	return handle;
}


Handle AudioSystem::GetChannel( const std::string& name )
{
	for ( size_t i = 0; i < aChannels.aHandles.size(); i++ )
	{
		AudioChannel* channel = nullptr;
		if ( !aChannels.Get( aChannels.aHandles[ i ], &channel ) )
		{
			Log_Error( gLC_Aduio, "We have an invalid channel handle saved? wtf??\n" );
			continue;
		}

		if ( channel->aName == name )
			return aChannels.aHandles[ i ];
	}

	return InvalidHandle;
}

static std::string gInvalidChannelName = "INVALID_AUDIO_CHANNEL";

const std::string& AudioSystem::GetChannelName( Handle handle )
{
	AudioChannel* channel = GetChannelData( handle );
	if ( !channel )
		return gInvalidChannelName;

	return channel->aName;
}


float AudioSystem::GetChannelVolume( Handle handle )
{
	AudioChannel* channel = GetChannelData( handle );
	if ( !channel )
		return 0.f;

	return channel->aVol;
}


void AudioSystem::SetChannelVolume( Handle handle, float vol )
{
	AudioChannel* channel = GetChannelData( handle );
	if ( !channel )
		return;

	channel->aVol = vol;
}


AudioChannel* AudioSystem::GetChannelData( Handle handle )
{
	if ( handle == InvalidHandle )
		return nullptr;

	AudioChannel* channel = nullptr;
	if ( !aChannels.Get( handle, &channel ) )
	{
		Log_ErrorF( gLC_Aduio, "Invalid Audio Channel Handle: %zd\n", (size_t)handle );
		return nullptr;
	}

	return channel;
}


// -------------------------------------------------------------------------------------
// Audio Stream Functions
// -------------------------------------------------------------------------------------


Handle AudioSystem::LoadSound( std::string_view sSoundPath )
{
	if ( aBufferCount >= 64 )
	{
		Log_ErrorF( gLC_Aduio, "Can't Load Sound, At Max Audio Streams (%d): %s\n", MAX_AUDIO_STREAMS, sSoundPath.data() );
		return InvalidHandle;
	}

	std::string soundPathAbs = FileSys_FindFile( sSoundPath.data() );

	if ( soundPathAbs.empty() )
	{
		Log_ErrorF( gLC_Aduio, "Sound does not exist: \"%s\"\n", sSoundPath.data() );
		return CH_INVALID_HANDLE;
	}

	std::string  ext    = FileSys_GetFileExt( sSoundPath );

	// TODO: probably allocate streams in a memory pool, and be game controlled
	AudioStream* stream = new AudioStream;
	stream->name        = soundPathAbs;

	for ( BaseCodec* codec: aCodecs )
	{
		if ( !codec->CheckExt( ext ) )
			continue;

		if ( !codec->Open( soundPathAbs.c_str(), stream ) )
			continue;

		stream->codec = codec;

		aBufferCount++;
		if ( !(stream->valid = LoadSoundInternal( stream )) )
		{
			aBufferCount--;
			Log_ErrorF( gLC_Aduio, "Could not load sound: \"%s\"\n", sSoundPath.data() );
			delete stream;
			return InvalidHandle;
		}
		else
		{
			Handle h = aStreams.Add( stream );
			return h;
		}
	}

	Log_ErrorF( gLC_Aduio, "Could not load sound: \"%s\"\n", sSoundPath.data() );
	delete stream;
	return InvalidHandle;
}


// TODO: have this check if the sound is preloaded, and read from the preloaded audio buffer instead
// though this is used in PreloadSound, so we would need to have a bool so it knows it's called during preloading
// unless you have a better idea in mind in the future
int ReadAudioInternal( AudioStream* stream, ChVector< float >& rawAudio )
{
	int read = stream->codec->Read( stream, READ_SIZE, rawAudio );

	// End of File
	if ( read == 0 )
	{
		// AL_LOOPING ?
		if ( !(stream->aEffects & AudioEffect_Loop) )
			return 0;

		if ( !stream->GetInt( Audio_Loop_Enabled ) )
			return 0;

		// go back to the starting point
		if ( stream->codec->Seek( stream, stream->GetFloat( Audio_Loop_StartTime ) ) == -1 )
			return 0; // failed to seek back, stop the sound

		read = stream->codec->Read( stream, READ_SIZE, rawAudio );
	}

	return read;
}


// TODO: this preloading does work, but it's only valid for playing it once
// only because we remove data from it's SDL_AudioStream during playback
// we'll need to store the audio converted from SDL_AudioStream in a separate location
// and then store a long for the index in the audio data array for reading audio

// probably have ReadAudio check if we're preloaded or not, if it is,
// then it returns data from the preload cache instead of reading from the file

bool AudioSystem::PreloadSound( Handle streamHandle )
{
	Log_Warn( gLC_Aduio, "PRELOADING SOUNDS NOT SETUP YET FOR OPENAL!!!!\n" );
	return false;

#if 0
	AudioStream *stream = GetStream( streamHandle );

	if ( !stream )
		return false;

	while ( true )
	{
		std::vector<float> rawAudio;

		int read = ReadAudioInternal( stream, rawAudio );

		if ( read == 0 )
			break;

#if 0
		// NOTE: "read" NEEDS TO BE 4 TIMES THE AMOUNT FOR SOME REASON, no clue why
		if ( SDL_AudioStreamPut( stream->audioStream, rawAudio.data(), read * snd_read_mult ) == -1 )
		{
			Log_Msg( gLC_Aduio, "SDL_AudioStreamPut - Failed to put samples in stream: %s\n", SDL_GetError() );
			return false;
		}
#endif
	}

	stream->preloaded = true;
	return true;
#endif
}


bool AudioSystem::PlaySound( Handle sStream )
{
	if ( sStream == InvalidHandle )
		return false;

	AudioStream *stream = *aStreams.Get( sStream );

	if ( stream == nullptr )
		return false;

	aStreamsPlaying.push_back( sStream );

	// alSourcePlay( apSources[stream->index] );

	// ALint buffer;
	// ALint source = apSources[stream->index];

	// alGetSourcei( apSources[stream->index], AL_BUFFERS_QUEUED, &buffer );

	// alBufferi( buffer, AL_POSITION, 0 )

	// um
	// ALuint buffers[] = {buffer};

	// alSourceQueueBuffers( source, 1, buffers );

	return true;
}


void AudioSystem::FreeSound( Handle sStream )
{
	if ( sStream == InvalidHandle )
		return;

	AudioStream *stream = *aStreams.Get( sStream );

	if ( stream == nullptr )
		return;

	vec_remove_if( aStreamsPlaying, sStream );

	// TODO: use the memory pool again (though i think i can set it in steam audio?)
	// iplAudioBufferFree( aCtx, &stream->outBuffer );

	//aStreamPool.Free( stream->inBufferAudio );
	//aStreamPool.Free( stream->midBufferAudio );
	//aStreamPool.Free( stream->outBufferAudio );

	alDeleteBuffers( MAX_SOURCE_BUFFERS, stream->aBuffers.data() );

	stream->codec->Close( stream );
	aStreams.Remove( sStream );

	aBufferCount--;
}


/* Is This a Valid Audio Stream? */
bool AudioSystem::IsValid( Handle stream )
{
	if ( stream == InvalidHandle )
		return false;

	return aStreams.Get( stream );
}


/* Audio Stream Volume ranges from 0.0f to 1.0f */
void AudioSystem::SetVolume( Handle handle, float vol )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return;

	// AL_GAIN ?
	stream->vol = vol;
}

float AudioSystem::GetVolume( Handle handle )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return 0.f;

	return stream->vol;
}


// TODO: actually be able to change input sample rate live,
// right now it's impossible due to how SDL_AudioStream works
// need to make my own sample rate converter somehow probably
//bool AudioSystem::SetSampleRate( Handle stream, float vol ) override;
//float AudioSystem::GetSampleRate( Handle stream ) override;


/* Sound Position in World */
#if 0
void AudioSystem::SetWorldPos( Handle handle, const glm::vec3& pos )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return;

	stream->pos = pos;

	ALint source = GetSource( stream );
	alSourcefv( source, AL_POSITION, &pos[0] );

	HandleALErr( "Failed to Set Position" );
}

const glm::vec3& AudioSystem::GetWorldPos( Handle handle )
{
	static constexpr glm::vec3 origin = { 0, 0, 0 };

	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return origin;

	return stream->pos;
}
#endif

/* Audio Volume Channels (ex. General, Music, Voices, Commentary, etc.) */
void AudioSystem::SetChannel( Handle handle, Handle channel )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return;

	stream->aChannel = channel;
}

Handle AudioSystem::GetChannel( Handle handle )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return InvalidHandle;

	return stream->aChannel;
}


bool AudioSystem::Seek( Handle streamHandle, double pos )
{
	AudioStream *stream = GetStream( streamHandle );

	if ( !stream )
		return false;

	return (stream->codec->Seek( stream, pos ) == 0);
}


// -------------------------------------------------------------------------------------
// Audio Effects
// -------------------------------------------------------------------------------------


void AudioSystem::AddEffect( Handle handle, AudioEffect effect )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return;

	if ( stream->aEffects & effect )
	{
		Log_WarnF( gLC_Aduio, "Stream Already has Effect: %s\n", Effect2Str( effect ) );
		return;
	}

	// Create Vars for Effect
	switch ( effect )
	{
		case AudioEffect_World:
			alSourcef( stream->aSource, AL_SOURCE_RELATIVE, AL_FALSE );

			if ( !HandleALErr( "Failed to Set Source to Non-Relative" ) )
				return;

			stream->CreateVar< glm::vec3 >( Audio_World_Pos, {} );
			stream->CreateVar< glm::vec3 >( Audio_World_Velocity, {} );
			stream->CreateVar( Audio_World_Falloff, 1.f );
			stream->CreateVar( Audio_World_ConeInnerAngle, 360.f );
			stream->CreateVar( Audio_World_ConeOuterAngle, 360.f );
			stream->CreateVar( Audio_World_ConeOuterGain, 1.f );

			break;

		case AudioEffect_Loop:
			stream->CreateVar( Audio_Loop_Enabled, 1 );
			stream->CreateVar( Audio_Loop_StartTime, 0.f );
			stream->CreateVar( Audio_Loop_EndTime, -1.f );
			break;

		default:
			Log_Warn( gLC_Aduio, "Trying to Add an Unknown Audio Effect To Stream\n" );
			return;
	}

	stream->aEffects |= effect;
	// stream->aEffects.push_back( effect );
}


void AudioSystem::RemoveEffect( Handle handle, AudioEffect effect )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return;

#if 1
	if ( !(stream->aEffects & effect) )
	{
		Log_WarnF( gLC_Aduio, "Trying to Remove non-existent Audio Effect on Audio Stream: %s\n", Effect2Str( effect ) );
		return;
	}

	stream->aEffects &= ~effect;
#else
	auto it = std::find( stream->aEffects.begin(), stream->aEffects.end(), effect );
	if ( it == stream->aEffects.end() )
	{
		Log_Warn( gLC_Aduio, "Trying to Remove non-existent Audio Effect on Audio Stream: %s\n", Effect2Str( effect ) );
		return;
	}

	vec_remove_index( stream->aEffects, it - stream->aEffects.begin() );
#endif

	// Delete Vars for Effect
	switch ( effect )
	{
		case AudioEffect_World:
			stream->RemoveVar( Audio_World_Pos );
			stream->RemoveVar( Audio_World_Velocity );
			stream->RemoveVar( Audio_World_Falloff );
			stream->RemoveVar( Audio_World_ConeInnerAngle );
			stream->RemoveVar( Audio_World_ConeOuterAngle );
			stream->RemoveVar( Audio_World_ConeOuterGain );
			break;

		case AudioEffect_Loop:
			stream->RemoveVar( Audio_Loop_Enabled );
			stream->RemoveVar( Audio_Loop_StartTime );
			stream->RemoveVar( Audio_Loop_EndTime );
			break;

		default:
			Log_Warn( gLC_Aduio, "Trying to Remove an Unknown Audio Effect on an Audio Stream\n" );
			return;
	}
}


bool AudioSystem::HasEffect( Handle handle, AudioEffect effect )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	return stream->aEffects & effect;
	// return vec_contains( stream->aEffects, effect );
}


// --------------------------------------------------------------------


// i don't like this aaaa
// maybe all return types will be bool, false if effect is not there? idk
// also, how do we know what part of the effect to apply this data to? hmm
bool AudioSystem::SetEffectData( Handle handle, AudioEffectData sDataType, int data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	stream->SetVar( sDataType, data );
	return true;
}


bool AudioSystem::SetEffectData( Handle handle, AudioEffectData sDataType, float data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	stream->SetVar( sDataType, data );

	// hmmm, should move elsewhere probably, idk
	// also this is one of the types that could change live, hmmmmm
	switch ( sDataType )
	{
		case Audio_World_Falloff:
		{
			alSourcef( stream->aSource, AL_ROLLOFF_FACTOR, data );

			if ( !HandleALErr( "Failed to Set Source Rolloff Factor" ) )
				return false;
		}

		case Audio_World_ConeInnerAngle:
		{
			alSourcef( stream->aSource, AL_CONE_INNER_ANGLE, data );

			if ( !HandleALErr( "Failed to Set Source Cone Inner Angle" ) )
				return false;
		}

		case Audio_World_ConeOuterAngle:
		{
			alSourcef( stream->aSource, AL_CONE_OUTER_ANGLE, data );

			if ( !HandleALErr( "Failed to Set Source Cone Outer Angle" ) )
				return false;
		}

		case Audio_World_ConeOuterGain:
		{
			alSourcef( stream->aSource, AL_CONE_OUTER_GAIN, data );

			if ( !HandleALErr( "Failed to Set Source Cone Outer Gain" ) )
				return false;
		}

		default:
			break;
	}

	return true;
}


bool AudioSystem::SetEffectData( Handle handle, AudioEffectData sDataType, const glm::vec3& data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	stream->SetVar( sDataType, data );

	// hmmm, should move elsewhere probably, idk
	// also this is one of the types that could change live, hmmmmm
	switch ( sDataType )
	{
		case Audio_World_Pos:
		{
			ALfloat vec[3];
			ToAlVec( data, vec );
			alSourcefv( stream->aSource, AL_POSITION, &vec[0] );

			if ( !HandleALErr( "Failed to Set Source Position" ) )
				return false;
		}

		case Audio_World_Velocity:
		{
			ALfloat vec[3];
			ToAlVec( data, vec );
			alSourcefv( stream->aSource, AL_VELOCITY, &vec[0] );

			if ( !HandleALErr( "Failed to Set Source Velocity" ) )
				return false;
		}

		default:
			break;
	}

	return true;
}


// --------------------------------------------------------------------


bool AudioSystem::GetEffectData( Handle handle, AudioEffectData sDataType, int& data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	data = stream->GetInt( sDataType );
	return true;
}


bool AudioSystem::GetEffectData( Handle handle, AudioEffectData sDataType, float& data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	data = stream->GetFloat( sDataType );
	return true;
}


bool AudioSystem::GetEffectData( Handle handle, AudioEffectData sDataType, glm::vec3& data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	data = stream->GetVec3( sDataType );
	return true;
}


// -------------------------------------------------------------------------------------
// Internal Functions
// -------------------------------------------------------------------------------------


bool AudioSystem::ApplyVolume( AudioStream* stream )
{
	float finalVol = stream->vol;

	if ( stream->aEffects & AudioEffect_World )
		finalVol *= snd_volume_3d;
	else
		finalVol *= snd_volume_2d;

	if ( stream->aChannel != InvalidHandle )
	{
		AudioChannel* channel = nullptr;
		if ( aChannels.Get( stream->aChannel, &channel ) )
		{
			finalVol *= channel->aVol;
		}
	}

	// apply volume
	alSourcef( stream->aSource, AL_GAIN, finalVol );

	return true;
}


bool AudioSystem::UpdateStream( AudioStream* stream )
{
	ApplyVolume( stream );

	if ( !stream->preloaded )
	{
		if ( !ReadAudio( stream ) )
			return false;
	}

	if ( !ApplyEffects( stream ) )
		return false;

	ALint sourceState = 0;
	alGetSourcei( stream->aSource, AL_SOURCE_STATE, &sourceState );

	if ( sourceState != AL_PLAYING )
	{
		alSourcePlay( stream->aSource );
	}

	return true;
}


CONVAR( snd_max_buffers, 2 );


bool AudioSystem::ReadAudio( AudioStream *stream )
{
	// Unqueue processed buffers

	if ( stream->aBufferIndex == stream->aBuffers.size() - 1 )
	{
		// allocate more buffers i guess? idk lol
		// wait what if i do this
		stream->aBufferIndex = 0;
	}

	ALint processed;
	alGetSourcei( stream->aSource, AL_BUFFERS_PROCESSED, &processed );

	if ( processed > 0 )
	{
		std::vector< ALuint > buffers( processed );
		
		for ( ALint i = 0; i < processed; i++ )
		{
			int index = stream->aBufferIndex - processed + i;

			if ( index < 0 )
				index = -index;

			buffers[i] = stream->aBuffers[index];
		}

		alSourceUnqueueBuffers( stream->aSource, processed, buffers.data() );
	}

	// don't queue too many buffers at a time

	int read = 0;
	alGetSourcei( stream->aSource, AL_BUFFERS_QUEUED, &read );

	if ( read > snd_max_buffers )
		return true;

	ChVector< float > rawAudio;

	read = ReadAudioInternal( stream, rawAudio );

	if ( read <= 0 )
		return false;

	// fill in the rest with zeros
	if ( read != READ_SIZE )
	{
		// GET RID OF THIS
		rawAudio.resize( READ_SIZE );
	}

	ALuint& buffer = stream->aBuffers[stream->aBufferIndex++];

	// um
	ALuint buffers[] = {buffer};
		
	// auto size = (READ_SIZE * stream->width * stream->channels);
	auto size = (READ_SIZE * stream->width * 2);
	alBufferData( buffer, stream->format, rawAudio.data(), size, stream->rate );

	if ( !HandleALErr( "Failed to add audio data to alBuffer" ) )
		return false;

	alSourceQueueBuffers( stream->aSource, 1, buffers );

	if ( !HandleALErr( "Failed to queue buffer to source" ) )
		return false;

#if 0
	// NOTE: "read" NEEDS TO BE 4 TIMES THE AMOUNT FOR SOME REASON, no clue why
	if ( SDL_AudioStreamPut(stream->audioStream, rawAudio.data(), read * snd_read_mult) == -1 )
	{
		Print( "[AudioSystem] SDL_AudioStreamPut - Failed to put samples in stream: %s\n", SDL_GetError() );
		return false;
	}
#endif
	
	return true;
}


bool AudioSystem::ApplyEffects( AudioStream *stream )
{

	return true;

#if 0

	// this is in bytes, not samples!
	std::vector<float> outAudio;
	outAudio.resize( SAMPLE_SIZE );
	//outAudio = rawAudio;

	// wtf is this doing
	// long read = SDL_AudioStreamGet( stream->audioStream, outAudio.data(), SAMPLE_SIZE );
	long read = 0;
	//read = SAMPLE_SIZE;

	if ( read == -1 )
	{
		Log_Msg( gLC_Aduio, "SDL_AudioStreamGet - Failed to get converted data: %s\n", SDL_GetError() );
		return false;
	}

	// no audio data available
	if ( read == 0 )
		return false;

	// apply stream volume
	for ( int i = 0; i < read; i++ )
		outAudio[i] *= stream->vol;

	// SDL_memset( stream->outBuffer.data[0], 0, SAMPLE_SIZE/2 );
	// SDL_memset( stream->outBuffer.data[1], 0, SAMPLE_SIZE/2 );

	// TODO: split these effects up
	if ( stream->aEffects & AudioEffect_World )
	{
		ApplySpatialEffects( stream, outAudio.data(), read );
	}
	else
	{
		// SDL_memset( stream->outBufferAudio, 0, sizeof( stream->outBufferAudio ) );
		// SDL_memset( stream->outBuffer.data, 0, sizeof(stream->outBuffer.data));

		// copy over samples
		// auto data = stream->outBuffer.data;

		for ( uint32_t i = 0, j = 0; i < SAMPLE_SIZE / 2; i++ )
		{
			for ( uint32_t ch = 0; ch < 2; ch++ )
			{
				// stream->outBuffer.data[ch][i] = outAudio[j++] * snd_volume_2d.GetFloat();
			}
		}
	}

	// unmixedBuffers.push_back( stream->outBuffer );

	stream->frame += read;
#endif

	return true;
}


bool AudioSystem::MixAudio()
{
#if 1
#else
	if (!unmixedBuffers.size())
		return true;

	// clear the mix buffer of previous audio data first
	SDL_memset( aMixBuffer.data[0], 0, SAMPLE_SIZE/2 );
	SDL_memset( aMixBuffer.data[1], 0, SAMPLE_SIZE/2 );

 	for ( auto& buffer: unmixedBuffers )
		iplAudioBufferMix( aCtx, &buffer, &aMixBuffer );
#endif

	return true;
}


bool AudioSystem::QueueAudio()
{
#if 0
	// if (!unmixedBuffers.size())
	// 	return true;
	
	// iplAudioBufferInterleave( aCtx, &aMixBuffer, apMixBufferAudio );

	// interleave manually so we can control global audio volume
	// unless we can set volume on the output device without changing it on the system
	for ( uint32_t i = 0, j = 0; i < SAMPLE_SIZE / 2; i++ )
	{
		for ( uint32_t ch = 0; ch < 2; ch++ )
		{
			// apMixBufferAudio[j++] = unmixedBuffers[0].data[ch][i] * snd_volume.GetFloat();
			// apMixBufferAudio[j++] = aMixBuffer.data[ch][i] * snd_volume.GetFloat();
		}
	}

	if ( SDL_QueueAudio( aOutputDeviceID, apMixBufferAudio, SAMPLE_SIZE ) == -1 )
	{
		Log_Msg( gLC_Aduio, "SDL_QueueAudio Error: %s\n", SDL_GetError() );
		return false;
	}
#endif

	return true;
}


/*IPLVector3 toPhonon(const glm::vec3& vector)
{
	return { vector.x, vector.z, -vector.y };
}*/

float calculateDistanceAttenutation( const glm::vec3& listener, const glm::vec3& source, float radius, float falloffPower )
{
	float distance = glm::length(listener - source);

	if (distance >= radius)
		return 0;

	// what is this actually doing
	return glm::pow(glm::clamp(1.f - (distance * 1.f / radius), 0.f, 1.f), falloffPower);
}


CONVAR( snd_phonon_spatial_blend, 0.5f );
CONVAR( snd_phonon_lerp_type, 0 );


int AudioSystem::ApplySpatialEffects( AudioStream* stream, float* data, size_t frameCount )
{
#if 0
	// TODO ONCE WORKING: don't allocate and free multiple times an audio update
	// probably move this to an effect class or some job class thing, idk
	// and then just store it in a memory pool, and then just use memset on it to clear it if needed

	// copy over samples
	for ( uint32_t i = 0, j = 0; i < SAMPLE_SIZE / 2; i++ )
	{
		for ( uint32_t ch = 0; ch < 2; ch++ )
		{
			// tempInBuffer.data[ch][i] = data[j++] * snd_volume_3d.GetFloat();
		}
	}

	// IPLVector3 direction = toPhonon(aListenerRot * glm::normalize(stream->pos - aListenerPos));


	for ( uint32_t i = 0, j = 0; i < SAMPLE_SIZE / 2; i++ )
	{
		for ( uint32_t ch = 0; ch < 2; ch++ )
		{
			// stream->outBuffer.data[ch][i] = tempOutBuffer.data[ch][i];
		}
	}
#endif

	return 0;
}


void AudioSystem::Update( float frameTime )
{
}

