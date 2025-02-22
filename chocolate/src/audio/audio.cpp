#include "audio.h"

#include "codec_libav.h"
#include "codec_vorbis.h"
#include "codec_wav.h"
#include "core/core.h"
#include "types/transform.h"

#include <SDL2/SDL.h>
#include <filesystem>
#include <glm/gtx/transform.hpp>
#include <mutex>
#include <thread>  // std::thread

// debugging
#include "igui.h"

LOG_CHANNEL_REGISTER( Aduio, ELogColor_Green );

AudioSystem* audio = new AudioSystem;

CONVAR_FLOAT( snd_volume, 0.5, CVARF_ARCHIVE, "Global Volume" );
CONVAR_FLOAT( snd_volume_3d, 1, CVARF_ARCHIVE, "Volume for 3D Audio" );
CONVAR_FLOAT( snd_volume_2d, 1, CVARF_ARCHIVE, "Volume for 2D Audio (No Spatial Audio Effects)" );

CONVAR_INT( snd_buffer_size, 4096, "" );
CONVAR_FLOAT( snd_read_mult, 4, "" );  // 4
CONVAR_INT( snd_read_chunk_size, 1024, "" );
CONVAR_INT( snd_audio_stream_available, FRAME_SIZE * 2, "" );

CONVAR_FLOAT( snd_phonon_spatial_blend, 1.f, "" );
CONVAR_INT( snd_phonon_lerp_type, 0, "" );
CONVAR_FLOAT( snd_phonon_directivity, 1.f, "" );

//IPLAudioFormat g_formatMono = {};
//IPLAudioFormat g_formatStereo = {};

ch_handle_t                   gDefaultChannel = audio->RegisterChannel( "Default" );

static ModuleInterface_t gInterfaces[] = {
	{ audio, IADUIO_NAME, IADUIO_VER }
};

extern "C"
{
	DLL_EXPORT ModuleInterface_t* ch_get_interfaces( u8& srCount )
	{
		srCount = 1;
		return gInterfaces;
	}
}

bool HandleIPLErr( IPLerror ret, const char* msg )
{
	if ( ret == IPL_STATUS_OUTOFMEMORY )
		Log_ErrorF( gLC_Aduio, "%s: %d - Out Of Memory\n", msg, (int)ret );

	else if ( ret == IPL_STATUS_INITIALIZATION )
		Log_ErrorF( gLC_Aduio, "%s: %d - An error occurred while initializing an external dependency\n", msg, (int)ret );

	else if ( ret == IPL_STATUS_FAILURE )
		Log_ErrorF( gLC_Aduio, "%s: %d - Failure\n", msg, (int)ret );

	return ret == IPL_STATUS_SUCCESS;
}


// ===========================================================================


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


const char* gAudioDataStr[] = {
	"EAudioEffectData_None",

	"EAudio_Loop_Enabled",
	"EAudio_Loop_StartTime",
	"EAudio_Loop_EndTime",

	"EAudio_World_Pos",
	"EAudio_World_Falloff",
	"EAudio_World_Radius",
	"EAudio_World_ConeInnerAngle",
	"EAudio_World_ConeOuterAngle",
	"EAudio_World_ConeOuterGain",

	"EAudioEffectData_Count",
};


static_assert( CH_ARR_SIZE( gAudioDataStr ) == EAudioEffectData_Count );


const char* EffectData2Str( EAudioEffectData sEffect )
{
	if ( sEffect > EAudioEffectData_Count )
		return "INVALID EFFECT DATA TYPE";

	return gAudioDataStr[ sEffect ];
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
			return "glm::vec3";

		case AudioVar::Invalid:
			return "INVALID (0)";

		default:
			return "UNKNOWN AUDIO VAR TYPE";
	}
}


// ===========================================================================


AudioSystem::AudioSystem() :
	IAudioSystem()
{
}


AudioSystem::~AudioSystem()
{
}

#define AUDIO_THREAD 1

#if AUDIO_THREAD

std::mutex g_audioMutex;

#endif

bool AudioSystem::Init()
{
	SDL_AudioSpec wantedSpec;

	wantedSpec.callback = NULL;
	wantedSpec.userdata = NULL;
	wantedSpec.channels = 2;
	wantedSpec.freq     = SOUND_RATE;
	wantedSpec.samples  = FRAME_SIZE;
	// aAudioSpec.samples = FRAME_SIZE / 2;
	wantedSpec.silence  = 0;
	wantedSpec.size     = 0;

	wantedSpec.format   = AUDIO_F32;
	// aAudioSpec.format = AUDIO_S16;

	// TODO: be able to switch this on the fly
	// aOutputDeviceID = SDL_OpenAudioDevice( NULL, 0, &aAudioSpec, NULL, 0 );
	// aOutputDeviceID = SDL_OpenAudioDevice( NULL, 0, &aAudioSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE );
	aOutputDeviceID     = SDL_OpenAudioDevice( NULL, 0, &wantedSpec, &aAudioSpec, SDL_AUDIO_ALLOW_ANY_CHANGE );
	if ( aOutputDeviceID == 0 )
	{
		// NOTE: technically this isn't a fatal error, and should be moved elsewhere so you can pick the output audio device while running
		Log_MsgF( gLC_Aduio, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError() );
		return true;
	}

	InitSteamAudio();

	apMixBufferAudio.resize( CH_OUT_BUFFER_SIZE );

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
	  [ & ]()
	  {
		  while ( true )
		  {
			  PROF_SCOPE_NAMED( "Audio Update Thread" );

			  if ( aPaused || aStreamsPlaying.size() == 0 )
			  {
				  SDL_Delay( 5 );
				  continue;
			  }
			  else
			  {
				  sys_sleep( 0.5 );
			  }

			  // TODO: use this for functions to modify how the stream is played during playback
			  // std::unique_lock<std::mutex> lock(g_audioMutex);

			  int        remainingAudio = SDL_GetQueuedAudioSize( aOutputDeviceID );
			  static int read           = remainingAudio;

			  if ( remainingAudio < snd_buffer_size )
			  {
				  // this might work well for multithreading
				  for ( int i = 0; i < aStreamsPlaying.size(); i++ )
				  {
					  AudioStream* stream = nullptr;
					  if ( !aStreams.Get( aStreamsPlaying[ i ], &stream ) )
					  {
						  Log_Error( "Failed to get stream that's currently playing\n" );
						  FreeSound( aStreamsPlaying[ i ] );
						  vec_remove_index( aStreamsPlaying, i );
						  i--;
						  continue;
					  }

					  if ( stream->paused )
						  continue;

					  if ( !UpdateStream( stream ) )
					  {
						  FreeSound( aStreamsPlaying[ i ] );
						  i--;
						  continue;
					  }
				  }
			  }

			  if ( aStreamsPlaying.size() )
			  {
				  MixAudio();
				  QueueAudio();

				  aUnmixedBuffers.clear();
			  }

			  // g_cv.notify_one();
		  }
	  } );

#endif

	return true;
}


bool AudioSystem::RegisterCodec( IAudioCodec* codec )
{
	if ( aCodecs.empty() || std::find( aCodecs.begin(), aCodecs.end(), codec ) == aCodecs.end() )
	{
		aCodecs.push_back( codec );

		// maybe mix these 2 functions together?
		codec->apAudio = this;
		codec->Init();

		Log_MsgF( gLC_Aduio, "Loaded Audio Codec - \"%s\"\n", codec->GetName() );
		return true;
	}

	Log_MsgF( gLC_Aduio, "Audio Codec already loaded - \"%s\"\n", codec->GetName() );
	return false;
}


IPLHRTF          gpHrtf = nullptr;

IPLAudioSettings gAudioSettings{
	.samplingRate = 48000,
	.frameSize    = FRAME_SIZE
};


bool AudioSystem::InitSteamAudio()
{
	IPLContextSettings contextSettings{};
	contextSettings.logCallback = []( IPLLogLevel level, const char* message )
	{
		Log_MsgF( gLC_Aduio, "Steam Audio LVL %d: %s\n", (int)level, message );
	};

	contextSettings.version = STEAMAUDIO_VERSION;

	IPLerror ret            = iplContextCreate( &contextSettings, &aCtx );
	if ( !HandleIPLErr( ret, "Error creating steam audio context" ) )
		return false;

	IPLHRTFSettings hrtfSettings{};
	hrtfSettings.type = IPL_HRTFTYPE_DEFAULT;

	ret               = iplHRTFCreate( aCtx, &gAudioSettings, &hrtfSettings, &gpHrtf );
	if ( !HandleIPLErr( ret, "Error creating HTRF" ) )
		return false;

	ret = iplAudioBufferAllocate( aCtx, 2, CH_OUT_BUFFER_SIZE, &aMixBuffer );
	if ( !HandleIPLErr( ret, "Error creating steam audio context" ) )
		return false;

	IPLDirectEffectSettings directSettings{};
	directSettings.numChannels = 2;

	if ( !HandleIPLErr( iplDirectEffectCreate( aCtx, &gAudioSettings, &directSettings, &apDirectEffect ), "Error creating direct effect" ) )
		return false;

	IPLBinauralEffectSettings binauralSettings{};
	binauralSettings.hrtf = gpHrtf;

	if ( !HandleIPLErr( iplBinauralEffectCreate( aCtx, &gAudioSettings, &binauralSettings, &apBinauralEffect ), "Error creating binaural effect" ) )
		return false;

	return true;
}


// OpenSound?
ch_handle_t AudioSystem::OpenSound( std::string_view sSoundPath )
{
	// check for too many streams
	/*if ( aStreams.size() == MAX_STREAMS )
	{
		Log_Msg( gLC_Aduio, "At Max Audio Streams: \"%d\"\n", MAX_STREAMS );
		return nullptr;
	}*/

	ch_string_auto soundPathAbs = FileSys_FindFile( sSoundPath.data(), sSoundPath.size() );

	if ( !soundPathAbs.data )
	{
		Log_ErrorF( gLC_Aduio, "Sound does not exist: \"%s\"\n", sSoundPath.data() );
		return CH_INVALID_HANDLE;
	}

	ch_string_auto ext    = FileSys_GetFileExt( sSoundPath.data(), sSoundPath.size() );

	// TODO: probably allocate streams in a memory pool, and be game controlled
	AudioStream*   stream = new AudioStream;
	stream->name          = soundPathAbs.data;

	for ( IAudioCodec* codec : aCodecs )
	{
		if ( !codec->CheckExt( ext.data ) )
			continue;

		if ( !codec->Open( soundPathAbs.data, stream ) )
			continue;

		stream->codec = codec;

		if ( !( stream->valid = LoadSoundInternal( stream ) ) )
		{
			Log_ErrorF( gLC_Aduio, "Could not load sound: \"%s\"\n", sSoundPath.data() );
			delete stream;
			return CH_INVALID_HANDLE;
		}
		else
		{
			ch_handle_t h = aStreams.Add( stream );
			return h;
		}
	}

	Log_ErrorF( gLC_Aduio, "Could not load sound: \"%s\"\n", sSoundPath.data() );
	delete stream;
	return CH_INVALID_HANDLE;
}


void AudioSystem::SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang )
{
	aListenerPos = pos;
	aListenerRot = glm::radians( ang );
	aListenerAng = ang;
}


void AudioSystem::SetListenerVelocity( const glm::vec3& vel )
{
	aListenerVel = vel;
}


void AudioSystem::SetPaused( bool paused )
{
	aPaused = paused;
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
// idk
// -------------------------------------------------------------------------------------


void AudioSystem::Update( float frameTime )
{
}


bool AudioSystem::ApplyEffects( AudioStream* stream )
{
	// this is in bytes, not samples!
	ChVector< float > outAudio;
	outAudio.resize( CH_OUT_BUFFER_SIZE );

	// wtf is this doing
	int read = SDL_AudioStreamGet( stream->audioStream, outAudio.data(), outAudio.size() );

	if ( read == -1 )
	{
		Log_ErrorF( gLC_Aduio, "SDL_AudioStreamGet - Failed to get converted data: %s\n", SDL_GetError() );
		return false;
	}

	// no audio data available
	if ( read == 0 )
		return false;

	if ( read < outAudio.size() )
	{
		// add 0's to it so steam audio is happy
		for ( size_t i = read; i < outAudio.size(); i++ )
		{
			outAudio[ i ] = 0.f;
		}
	}

	// apply stream volume
	for ( int i = 0; i < read; i++ )
		outAudio[ i ] *= stream->vol;

	SDL_memset( stream->outBuffer.data[ 0 ], 0, outAudio.size() / 2 );
	SDL_memset( stream->outBuffer.data[ 1 ], 0, outAudio.size() / 2 );

	// TODO: split these effects up
	if ( stream->aEffects & AudioEffect_World )
	{
		ApplySpatialEffects( stream, outAudio.data(), outAudio.size() );
	}
	else
	{
		for ( uint32_t i = 0, j = 0; i < outAudio.size() / 2; i++ )
		{
			for ( uint32_t ch = 0; ch < 2; ch++ )
			{
				stream->outBuffer.data[ ch ][ i ] = outAudio[ j++ ] * snd_volume_2d;
			}
		}
	}

	aUnmixedBuffers.push_back( stream->outBuffer );

	stream->frame += read;

	return true;
}


bool AudioSystem::MixAudio()
{
	if ( !aUnmixedBuffers.size() )
		return true;

	// clear the mix buffer of previous audio data first
	SDL_memset( aMixBuffer.data[ 0 ], 0, CH_OUT_BUFFER_SIZE / 2 );
	SDL_memset( aMixBuffer.data[ 1 ], 0, CH_OUT_BUFFER_SIZE / 2 );

	for ( IPLAudioBuffer& buffer : aUnmixedBuffers )
		iplAudioBufferMix( aCtx, &buffer, &aMixBuffer );

	return true;
}


bool AudioSystem::QueueAudio()
{
	if ( !aUnmixedBuffers.size() )
		return true;

	// iplAudioBufferInterleave( aCtx, &aMixBuffer, apMixBufferAudio );

	apMixBufferAudio.resize( aMixBuffer.numSamples );

	// interleave manually so we can control global audio volume
	// unless we can set volume on the output device without changing it on the system
	for ( uint32_t i = 0, j = 0; i < apMixBufferAudio.size() / 2; i++ )
	{
		for ( uint32_t ch = 0; ch < 2; ch++ )
		{
			// apMixBufferAudio[j++] = unmixedBuffers[0].data[ch][i] * snd_volume;
			apMixBufferAudio[ j++ ] = aMixBuffer.data[ ch ][ i ] * snd_volume;
		}
	}

	if ( SDL_QueueAudio( aOutputDeviceID, apMixBufferAudio.data(), apMixBufferAudio.size() ) == -1 )
	{
		Log_ErrorF( gLC_Aduio, "SDL_QueueAudio Error: %s\n", SDL_GetError() );
		return false;
	}

	return true;
}


// NOTE: technically this is specific to sidury, but i can find a solution later when cleaning this up
static IPLVector3 ToPhonon( const glm::vec3& vector )
{
	return { vector.x, vector.z, -vector.y };
}


static float CalculateDistanceAttenutation( const glm::vec3& listener, const glm::vec3& source, float radius, float falloffPower )
{
	float distance = glm::length( listener - source );

	if ( distance >= radius )
		return 0;

	// what is this actually doing
	return glm::pow( glm::clamp( 1.f - ( distance * 1.f / radius ), 0.f, 1.f ), falloffPower );
}


int AudioSystem::ApplySpatialEffects( AudioStream* stream, float* data, size_t frameCount )
{
	glm::vec3      worldPos      = stream->GetVec3( EAudio_World_Pos );
	float          worldDistance = stream->GetFloat( EAudio_World_Radius );
	float          worldFalloff  = stream->GetFloat( EAudio_World_Falloff );

	IPLVector3     direction     = ToPhonon( aListenerRot * glm::normalize( worldPos - aListenerPos ) );
	IPLfloat32     distanceAtten = CalculateDistanceAttenutation( aListenerPos, worldPos, worldDistance, worldFalloff );

	if ( distanceAtten == 0.f )
		return 0;

	IPLerror       err = IPL_STATUS_SUCCESS;

	// TODO ONCE WORKING: don't allocate and free multiple times an audio update
	// probably move this to an effect class or some job class thing, idk
	// and then just store it in a memory pool, and then just use memset on it to clear it if needed
	IPLAudioBuffer tempInBuffer{};
	IPLAudioBuffer tempMidBuffer{};
	IPLAudioBuffer tempOutBuffer{};

	err = iplAudioBufferAllocate( aCtx, 2, frameCount, &tempInBuffer );

	if ( CH_IF_ASSERT( err == IPL_STATUS_SUCCESS ) )
	{
		Log_Error( gLC_Aduio, "Failed to allocate input phonon buffer\n" );
		return 0;
	}

	err = iplAudioBufferAllocate( aCtx, 2, frameCount, &tempMidBuffer );

	if ( CH_IF_ASSERT( err == IPL_STATUS_SUCCESS ) )
	{
		iplAudioBufferFree( aCtx, &tempInBuffer );
		Log_Error( gLC_Aduio, "Failed to allocate mid phonon buffer\n" );
		return 0;
	}

	err = iplAudioBufferAllocate( aCtx, 2, frameCount, &tempOutBuffer );

	if ( CH_IF_ASSERT( err == IPL_STATUS_SUCCESS ) )
	{
		iplAudioBufferFree( aCtx, &tempInBuffer );
		iplAudioBufferFree( aCtx, &tempMidBuffer );
		Log_Error( gLC_Aduio, "Failed to allocate output phonon buffer\n" );
		return 0;
	}

	// copy over samples
	for ( uint32_t i = 0, j = 0; i < frameCount / 2; i++ )
	{
		for ( uint32_t ch = 0; ch < 2; ch++ )
		{
			tempInBuffer.data[ ch ][ i ] = data[ j++ ];

			CH_ASSERT( tempInBuffer.data[ ch ][ i ] < 1 );
			CH_ASSERT( tempInBuffer.data[ ch ][ i ] > -1 );
		}
	}

	IPLDirectEffectParams directParams{};

	// one day we can add IPL_DIRECTEFFECTFLAGS_APPLYOCCLUSION
	directParams.flags = (IPLDirectEffectFlags)( IPL_DIRECTEFFECTFLAGS_APPLYDISTANCEATTENUATION | IPL_DIRECTEFFECTFLAGS_APPLYDIRECTIVITY );
	
	directParams.distanceAttenuation = distanceAtten;
	directParams.directivity         = snd_phonon_directivity;

	CH_ASSERT( _heapchk() == _HEAPOK );

	iplDirectEffectApply( apDirectEffect, &directParams, &tempInBuffer, &tempMidBuffer );

	IPLBinauralEffectParams binauralParams{};
	binauralParams.direction     = direction;
	binauralParams.interpolation = snd_phonon_lerp_type ? IPL_HRTFINTERPOLATION_BILINEAR : IPL_HRTFINTERPOLATION_NEAREST;
	binauralParams.spatialBlend  = snd_phonon_spatial_blend;
	binauralParams.hrtf          = gpHrtf;

	iplBinauralEffectApply( apBinauralEffect, &binauralParams, &tempMidBuffer, &tempOutBuffer );

	CH_ASSERT( _heapchk() == _HEAPOK );

	for ( uint32_t i = 0; i < frameCount / 2; i++ )
	{
		for ( uint32_t ch = 0; ch < 2; ch++ )
		{
			stream->outBuffer.data[ ch ][ i ] = tempOutBuffer.data[ ch ][ i ] * snd_volume_3d;
		}
	}

	CH_ASSERT( _heapchk() == _HEAPOK );

	iplAudioBufferFree( aCtx, &tempInBuffer );

	CH_ASSERT( _heapchk() == _HEAPOK );

	iplAudioBufferFree( aCtx, &tempMidBuffer );

	CH_ASSERT( _heapchk() == _HEAPOK );

	iplAudioBufferFree( aCtx, &tempOutBuffer );

	CH_ASSERT( _heapchk() == _HEAPOK );

	return 0;
}

// -------------------------------------------------------------------------------------
// Audio Channels
// -------------------------------------------------------------------------------------


// probably fine to just set the const char directly
// but how do i limit the channel name size?
ch_handle_t AudioSystem::RegisterChannel( const char* name )
{
	// TODO: probably use the memory pool for this
	AudioChannel* channel = new AudioChannel;
	channel->name        = name;
	channel->aVol         = 1.f;

	ch_handle_t handle         = aChannels.Add( channel );
	return handle;
}


ch_handle_t AudioSystem::GetChannel( std::string_view name )
{
	for ( size_t i = 0; i < aChannels.aHandles.size(); i++ )
	{
		AudioChannel* channel = nullptr;
		if ( !aChannels.Get( aChannels.aHandles[ i ], &channel ) )
		{
			Log_Error( gLC_Aduio, "We have an invalid channel handle saved? wtf??\n" );
			continue;
		}

		if ( channel->name == name )
			return aChannels.aHandles[ i ];
	}

	return CH_INVALID_HANDLE;
}

static std::string gInvalidChannelName = "INVALID_AUDIO_CHANNEL";

const std::string& AudioSystem::GetChannelName( ch_handle_t handle )
{
	AudioChannel* channel = GetChannelData( handle );
	if ( !channel )
		return gInvalidChannelName;

	return channel->name;
}


float AudioSystem::GetChannelVolume( ch_handle_t handle )
{
	AudioChannel* channel = GetChannelData( handle );
	if ( !channel )
		return 0.f;

	return channel->aVol;
}


void AudioSystem::SetChannelVolume( ch_handle_t handle, float vol )
{
	AudioChannel* channel = GetChannelData( handle );
	if ( !channel )
		return;

	channel->aVol = vol;
}


bool AudioSystem::GetChannelPaused( ch_handle_t handle )
{
	AudioChannel* channel = GetChannelData( handle );
	if ( !channel )
		return false;

	return channel->aPaused;
}


void AudioSystem::SetChannelPaused( ch_handle_t handle, bool sPaused )
{
	AudioChannel* channel = GetChannelData( handle );
	if ( !channel )
		return;

	channel->aPaused = sPaused;
}


AudioChannel* AudioSystem::GetChannelData( ch_handle_t handle )
{
	if ( handle == CH_INVALID_HANDLE )
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
// Audio Effects
// -------------------------------------------------------------------------------------


void AudioSystem::AddEffects( ch_handle_t sHandle, AudioEffect sEffects )
{
	AudioStream* stream = GetStream( sHandle );
	if ( !stream )
		return;

	if ( sEffects & AudioEffect_World && !( stream->aEffects & AudioEffect_World ) )
	{
		stream->aEffects |= AudioEffect_World;

		stream->CreateVar< glm::vec3 >( EAudio_World_Pos, {} );
		stream->CreateVar< glm::vec3 >( EAudio_World_Velocity, {} );
		stream->CreateVar( EAudio_World_Falloff, 1.f );
		stream->CreateVar( EAudio_World_Radius, 1000.f );
		stream->CreateVar( EAudio_World_ConeInnerAngle, 360.f );
		stream->CreateVar( EAudio_World_ConeOuterAngle, 360.f );
		stream->CreateVar( EAudio_World_ConeOuterGain, 1.f );
	}

	if ( sEffects & AudioEffect_Loop && !( stream->aEffects & AudioEffect_Loop ) )
	{
		stream->aEffects |= AudioEffect_Loop;

		stream->CreateVar( EAudio_Loop_Enabled, 1 );
		stream->CreateVar( EAudio_Loop_StartTime, 0.f );
		stream->CreateVar( EAudio_Loop_EndTime, -1.f );
	}
}


void AudioSystem::RemoveEffects( ch_handle_t sHandle, AudioEffect sEffects )
{
	AudioStream* stream = GetStream( sHandle );
	if ( !stream )
		return;

	// Delete Vars for Effect
	if ( sEffects & AudioEffect_World && stream->aEffects & AudioEffect_World )
	{
		stream->aEffects &= ~AudioEffect_World;

		stream->RemoveVar( EAudio_World_Pos );
		stream->RemoveVar( EAudio_World_Velocity );
		stream->RemoveVar( EAudio_World_Falloff );
		stream->RemoveVar( EAudio_World_Radius );
		stream->RemoveVar( EAudio_World_ConeInnerAngle );
		stream->RemoveVar( EAudio_World_ConeOuterAngle );
		stream->RemoveVar( EAudio_World_ConeOuterGain );
	}

	if ( sEffects & AudioEffect_Loop && stream->aEffects & AudioEffect_Loop )
	{
		stream->aEffects &= ~AudioEffect_Loop;

		stream->RemoveVar( EAudio_Loop_Enabled );
		stream->RemoveVar( EAudio_Loop_StartTime );
		stream->RemoveVar( EAudio_Loop_EndTime );
	}
}


bool AudioSystem::HasEffects( ch_handle_t handle, AudioEffect effect )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	return stream->aEffects & effect;
}


// --------------------------------------------------------------------


// i don't like this aaaa
// maybe all return types will be bool, false if effect is not there? idk
// also, how do we know what part of the effect to apply this data to? hmm
bool AudioSystem::SetEffectData( ch_handle_t handle, EAudioEffectData sDataType, int data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	stream->SetVar( sDataType, data );
	return true;
}


bool AudioSystem::SetEffectData( ch_handle_t handle, EAudioEffectData sDataType, float data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	stream->SetVar( sDataType, data );
	return true;
}


bool AudioSystem::SetEffectData( ch_handle_t handle, EAudioEffectData sDataType, const glm::vec3& data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	stream->SetVar( sDataType, data );
	return true;
}


// --------------------------------------------------------------------


bool AudioSystem::GetEffectData( ch_handle_t handle, EAudioEffectData sDataType, int& data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	data = stream->GetInt( sDataType );
	return true;
}


bool AudioSystem::GetEffectData( ch_handle_t handle, EAudioEffectData sDataType, float& data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	data = stream->GetFloat( sDataType );
	return true;
}


bool AudioSystem::GetEffectData( ch_handle_t handle, EAudioEffectData sDataType, glm::vec3& data )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return false;

	data = stream->GetVec3( sDataType );
	return true;
}

