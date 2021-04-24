#ifndef AUDIO_H
#define AUDIO_H

#include "system.h"
#include <SDL2/SDL_mixer.h>

const int RATE = 44100;

class audio_c : public system_c
{
	protected:

	Mix_Music* mus;

	void play_mus( const char* musPath );

	public:

	audio_c
		(  );
	~audio_c
		(  );
};

#endif
