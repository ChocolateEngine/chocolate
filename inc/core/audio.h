#pragma once

#include "../shared/system.h"
#include "../shared/baseaudio.h"

#if SDL_MIXER
#include <SDL2/SDL_mixer.h>
#endif

const int RATE = 44100;

class AudioSystem : public BaseAudioSystem
{
protected:
#if SDL_MIXER
	Mix_Music* apMusic;
#endif
	/* Load and play music from a given file path.  */
	void 		PlayMusic( const char* spMusicPath );
public:
	/* Initialize the audio format.  */
	explicit        AudioSystem(  );
					~AudioSystem(  );
};
