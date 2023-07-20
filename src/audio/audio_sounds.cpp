#include "audio.h"


extern ConVar snd_audio_stream_available;
extern ConVar snd_read_chunk_size;
extern ConVar snd_read_mult;


bool AudioSystem::LoadSoundInternal( AudioStream* stream )
{
	IPLerror ret;
	// SDL_AudioStream will convert it to stereo for us
	//IPLAudioFormat inAudioFormat = (stream->channels == 1 ? g_formatMono: g_formatStereo);
	// IPLAudioFormat inAudioFormat = g_formatStereo;
	//IPLAudioFormat inAudioFormat = g_formatMono;


	// ret = iplCreatePanningEffect(renderer, g_formatStereo, g_formatStereo, &stream->effect);
	// ret = iplCreateVirtualSurroundEffect(renderer, g_formatStereo, g_formatStereo, &stream->effect);

	// if ( (!stream->inWorld && stream->channels != 2) || stream->rate != aAudioSpec.freq || stream->format != aAudioSpec.format )
	{
		stream->audioStream = SDL_NewAudioStream( stream->format, stream->channels, stream->rate, aAudioSpec.format, 2, aAudioSpec.freq );
		if ( stream->audioStream == nullptr )
		{
			Log_ErrorF( gLC_Aduio, "SDL_NewAudioStream failed: %s\n", SDL_GetError() );
			return false;
		}
	}

	// final output audio buffer
	ret = iplAudioBufferAllocate( aCtx, 2, CH_OUT_BUFFER_SIZE, &stream->outBuffer );
	if ( !HandleIPLErr( ret, "Error creating outBuffer" ) )
		return false;

	return true;
}


// Checks If This a Valid Audio Stream, if not, throw a warning and return nullptr.
AudioStream* AudioSystem::GetStream( Handle streamHandle )
{
	if ( streamHandle == InvalidHandle )
		return nullptr;

	AudioStream* stream = *aStreams.Get( streamHandle );

	if ( !stream )
		Log_WarnF( gLC_Aduio, "Invalid Stream Handle: %u\n", (size_t)streamHandle );

	return stream;
}


// TODO: this preloading does work, but it's only valid for playing it once
// only because we remove data from it's SDL_AudioStream during playback
// we'll need to store the audio converted from SDL_AudioStream in a separate location
// and then store a long for the index in the audio data array for reading audio

// probably have ReadAudio check if we're preloaded or not, if it is,
// then it returns data from the preload cache instead of reading from the file

bool AudioSystem::PreloadSound( Handle sSound )
{
	AudioStream* stream = GetStream( sSound );

	if ( !stream )
		return false;

	while ( true )
	{
		ChVector< float > rawAudio;
		rawAudio.reserve( snd_read_chunk_size );

		long read = stream->codec->Read( stream, snd_read_chunk_size, rawAudio );

		// should just put silence in the stream if this is true
		if ( read < 0 )
			break;

		// End of File
		if ( read == 0 )
		{
			if ( !(stream->aEffects & AudioEffect_Loop) )
				break;

			if ( !stream->GetInt( EAudio_Loop_Enabled ) )
				break;

			// go back to the starting point
			if ( stream->codec->Seek( stream, stream->GetFloat( EAudio_Loop_StartTime ) ) == -1 )
				return false;  // failed to seek back, stop the sound

			read = stream->codec->Read( stream, snd_read_chunk_size, rawAudio );

			// should just put silence in the stream if this is true
			if ( read < 0 )
				break;
		}

		// NOTE: "read" NEEDS TO BE 4 TIMES THE AMOUNT FOR SOME REASON, no clue why
		if ( SDL_AudioStreamPut( stream->audioStream, rawAudio.data(), read * snd_read_mult ) == -1 )
		{
			Log_MsgF( gLC_Aduio, "SDL_AudioStreamPut - Failed to put samples in stream: %s\n", SDL_GetError() );
			return false;
		}
	}

	stream->preloaded = true;
	return true;
}


bool AudioSystem::PlaySound( Handle sStream )
{
	if ( sStream == InvalidHandle )
		return false;

	AudioStream* stream = *aStreams.Get( sStream );

	if ( stream == nullptr )
		return false;

	if ( vec_contains( aStreamsPlaying, sStream ) )
	{
		Log_ErrorF( gLC_Aduio, "Sound is already playing: \"%s\"\n", stream->name.c_str() );
		return true;
	}

	aStreamsPlaying.push_back( sStream );
	return true;
}


void AudioSystem::FreeSound( Handle sStream )
{
	if ( sStream == InvalidHandle )
		return;

	AudioStream* stream = *aStreams.Get( sStream );

	if ( stream == nullptr )
		return;

	vec_remove_if( aStreamsPlaying, sStream );

	// TODO: use the memory pool again (though i think i can set it in steam audio?)
	iplAudioBufferFree( aCtx, &stream->outBuffer );

	//aStreamPool.Free( stream->inBufferAudio );
	//aStreamPool.Free( stream->midBufferAudio );
	//aStreamPool.Free( stream->outBufferAudio );

	stream->codec->Close( stream );
	aStreams.Remove( sStream );
}


bool AudioSystem::UpdateStream( AudioStream* stream )
{
	if ( !stream->preloaded )
	{
		if ( !ReadAudio( stream ) )
			return false;
	}

	if ( !ApplyEffects( stream ) )
		return false;

	return true;
}


bool AudioSystem::ReadAudio( AudioStream* stream )
{
	int read = SDL_AudioStreamAvailable( stream->audioStream );
	if ( read >= snd_audio_stream_available )
		return true;

	ChVector< float > rawAudio;
	rawAudio.reserve( snd_read_chunk_size );

	read = stream->codec->Read( stream, snd_read_chunk_size, rawAudio );

	// should just put silence in the stream if this is true
	if ( read < 0 )
		return true;

	// End of File
	if ( read == 0 )
	{
		if ( !( stream->aEffects & AudioEffect_Loop ) )
			return false;

		if ( !stream->GetInt( EAudio_Loop_Enabled ) )
			return false;

		// go back to the starting point
		if ( stream->codec->Seek( stream, stream->GetFloat( EAudio_Loop_StartTime ) ) == -1 )
			return false;  // failed to seek back, stop the sound

		read = stream->codec->Read( stream, snd_read_chunk_size, rawAudio );

		// should just put silence in the stream if this is true
		if ( read < 0 )
			return true;
	}

	// NOTE: "read" NEEDS TO BE 4 TIMES THE AMOUNT FOR SOME REASON, no clue why
	if ( SDL_AudioStreamPut( stream->audioStream, rawAudio.data(), read * snd_read_mult ) == -1 )
	{
		Log_WarnF( gLC_Aduio, "SDL_AudioStreamPut - Failed to put samples in stream: %s\n", SDL_GetError() );
		return false;
	}

	return true;
}


// -------------------------------------------------------------------------------------
// Audio Stream Functions
// -------------------------------------------------------------------------------------


// Is This a Valid Audio Stream?
bool AudioSystem::IsValid( Handle stream )
{
	if ( stream == InvalidHandle )
		return false;

	return aStreams.Get( stream );
}


// Audio Stream Volume ranges from 0.0f to 1.0f
void AudioSystem::SetVolume( Handle handle, float vol )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return;

	stream->vol = vol;
}


float AudioSystem::GetVolume( Handle handle )
{
	AudioStream* stream = GetStream( handle );
	if ( !stream )
		return 0.f;

	return stream->vol;
}


bool AudioSystem::Seek( Handle streamHandle, double pos )
{
	AudioStream* stream = GetStream( streamHandle );

	if ( !stream )
		return false;

	return ( stream->codec->Seek( stream, pos ) == 0 );
}


// Audio Volume Channels (ex. General, Music, Voices, Commentary, etc.)
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


// TODO: actually be able to change input sample rate live,
// right now it's impossible due to how SDL_AudioStream works
// need to make my own sample rate converter somehow probably
//bool AudioSystem::SetSampleRate( Handle stream, float vol ) override;
//float AudioSystem::GetSampleRate( Handle stream ) override;


// ============================================================================
// Audio Stream Vars
// ============================================================================


const char* EffectData2Str( EAudioEffectData sEffect );
const char* AudioVar2Str( AudioVar var );


AudioEffectVar* AudioStream::GetVar( EAudioEffectData sName )
{
	for ( auto var : aVars )
	{
		if ( var->aName == sName )
			return var;
	}

	return nullptr;
}


bool AudioStream::RemoveVar( EAudioEffectData sName )
{
	for ( size_t i = 0; i < aVars.size(); i++ )
	{
		if ( aVars[ i ]->aName == sName )
		{
			delete aVars[ i ];
			vec_remove_index( aVars, i );
			return true;
		}
	}

	Log_WarnF( gLC_Aduio, "Trying to Remove non-existent AudioVar on Audio Stream: %s\n", EffectData2Str( sName ) );
	return false;
}


#define SET_VAR( func, type )                                                                     \
  for ( auto var : aVars )                                                                        \
  {                                                                                               \
	if ( var->aName == sName )                                                                     \
	{                                                                                             \
	  if ( var->aType != type )                                                                   \
	  {                                                                                           \
		Log_WarnF( gLC_Aduio, "Audio Var Does not take type of \"%s\"\n", AudioVar2Str( type ) ); \
	  }                                                                                           \
	  else                                                                                        \
	  {                                                                                           \
		var->func( sData );                                                                        \
	  }                                                                                           \
	  return;                                                                                     \
	}                                                                                             \
  }                                                                                               \
  Log_WarnF( gLC_Aduio, "Audio Var Not Defined for Effect Data: %s\n", EffectData2Str( sName ) )


void AudioStream::SetVar( EAudioEffectData sName, float sData )
{
	SET_VAR( SetFloat, AudioVar::Float );
}
void AudioStream::SetVar( EAudioEffectData sName, int sData )
{
	SET_VAR( SetInt, AudioVar::Int );
}
void AudioStream::SetVar( EAudioEffectData sName, const glm::vec3& sData )
{
	SET_VAR( SetVec3, AudioVar::Vec3 );
}

#undef SET_VAR

static glm::vec3 vec3_zero{};

#define GET_VAR( func, ret )                                                                       \
  AudioEffectVar* var = GetVar( sName );                                                            \
  if ( var == nullptr )                                                                            \
  {                                                                                                \
	Log_WarnF( gLC_Aduio, "Audio Var Not Defined for Effect Data: %s\n", EffectData2Str( sName ) ); \
	return ret;                                                                                    \
  }                                                                                                \
  return var->func( ret )

float AudioStream::GetFloat( EAudioEffectData sName )
{
	GET_VAR( GetFloat, 0.f );
}
int AudioStream::GetInt( EAudioEffectData sName )
{
	GET_VAR( GetInt, 0 );
}
const glm::vec3& AudioStream::GetVec3( EAudioEffectData sName )
{
	GET_VAR( GetVec3, vec3_zero );
}

#undef GET_VAR