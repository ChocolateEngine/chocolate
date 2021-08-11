#ifndef AUDIO_H
#define AUDIO_H

#include "../shared/system.h"

#if SDL_MIXER
#include <SDL2/SDL_mixer.h>
#endif

const int RATE = 44100;

class audio_c : public system_c
{
	protected:

#if SDL_MIXER
	Mix_Music* mus;
#endif

	void play_mus( const char* musPath );

	public:

	audio_c
		(  );
	~audio_c
		(  );
};

#endif
