#include "../../inc/core/audio.h"

void audio_c::play_mus
	( const char* musPath )
{
	mus = Mix_LoadMUS( musPath );
	Mix_PlayMusic( mus, -1 );
}

audio_c::audio_c
	(  )
{
	mus = NULL;
	Mix_OpenAudio( RATE, MIX_DEFAULT_FORMAT, 2, 2048 );
	play_mus( "audio/music/neon02.ogg" );
}

audio_c::~audio_c
	(  )
{
	if ( !mus )
	{
		Mix_FreeMusic( mus );
	}
}
