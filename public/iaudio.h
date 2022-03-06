#pragma once

#include "system.h"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL2/SDL_audio.h>


class BaseCodec;
class BaseAudioSystem;


enum
{
	AudioEffect_None = (1 << 1),
	//AudioEffect_Panning = (1 << 2),
	AudioEffect_Direct = (1 << 3),
	AudioEffect_Binaural = (1 << 4),
};

constexpr size_t AudioEffectPreset_World = AudioEffect_Direct | AudioEffect_Binaural;


// maybe change to Sound?
// also hide a few more things from normal game code probably? idk
struct AudioStream
{
	bool Valid() { return valid; }

	std::string name;
	size_t frame = 0;

	unsigned char channels;
	size_t rate;
	SDL_AudioFormat format = AUDIO_F32;
	unsigned int size;
	unsigned int samples;
	unsigned int bits;
	unsigned char width;  // ???

	float vol = 1.f;
	float radius = 100.0f;
	float falloff = 16.f;

	bool loop = false;      // should we loop the file when it ends?
	//double time = 0;        // time in seconds in the file
	double startTime = 0;   // time in seconds to start playback
	//double endTime = 0;     // time in seconds to end playback
	//float speed = 1.0f;

	// must be set before playing a sound
	size_t effects = AudioEffect_None;

	bool paused = false;
	bool valid = false;
	bool preloaded = false;  // bruh

	glm::vec3 pos;
};


class BaseAudioSystem: public BaseSystem
{
public:
	virtual void            SetListenerTransform( const glm::vec3& pos, const glm::quat& rot ) = 0;
	virtual void            SetListenerTransform( const glm::vec3& pos, const glm::vec3& ang ) = 0;
	virtual void            SetPaused( bool paused ) = 0;
	virtual void            SetGlobalSpeed( float speed ) = 0;

	/* Load a sound from a path */
	virtual Handle          LoadSound( std::string soundPath ) = 0;

	/* Load the entire sound into an audio buffer instead of streaming it from the disk on playback */
	virtual bool            PreloadSound( AudioStream *stream ) = 0;

	/* Start playback of a sound */
	virtual bool            PlaySound( Handle stream ) = 0;

	/* Free a sound */
	virtual void            FreeSound( Handle stream ) = 0;

	/* UNTESTED: seek to different point in the audio file */
	virtual int             Seek( AudioStream *streamPublic, double pos ) = 0;

	//virtual bool            RegisterCodec( BaseCodec *codec ) = 0;
};



