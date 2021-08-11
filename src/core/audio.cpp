#include "../../inc/core/audio.h"

void audio_c::play_mus
	( const char* musPath )
{
#if SDL_MIXER
	mus = Mix_LoadMUS( musPath );
	Mix_PlayMusic( mus, -1 );
#endif
}

audio_c::audio_c
	(  )
{
	systemType = AUDIO_C;

#if SDL_MIXER
	mus = NULL;
	Mix_OpenAudio( RATE, MIX_DEFAULT_FORMAT, 2, 2048 );
	play_mus( "audio/music/neon02.ogg" );
#endif
}

audio_c::~audio_c
	(  )
{
#if SDL_MIXER
	if ( !mus )
	{
		Mix_FreeMusic( mus );
	}
#endif
}
