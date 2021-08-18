#include "../../inc/core/audio.h"

void AudioSystem::PlayMusic( const char* spMusicPath )
{
#if SDL_MIXER
	apMusic = Mix_LoadMUS( spMusicPath );
	Mix_PlayMusic( apMusic, -1 );
#endif
}

AudioSystem::AudioSystem(  ) : BaseSystem(  )
{
	aSystemType = AUDIO_C;

#if SDL_MIXER
	apMusic = NULL;
	Mix_OpenAudio( RATE, MIX_DEFAULT_FORMAT, 2, 2048 );
#endif
}

AudioSystem::~AudioSystem(  )
{
#if SDL_MIXER
	if ( !apMusic )
	{
		Mix_FreeMusic( apMusic );
	}
#endif
}
