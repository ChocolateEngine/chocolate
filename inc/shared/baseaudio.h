#pragma once

#include "../shared/system.h"

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <SDL2/SDL_audio.h>


class BaseCodec;
class BaseAudioSystem;


// maybe change to Sound?
// also hide a few more things from normal game code probably? idk
struct AudioStream
{
	bool Valid() { return codec; }

	BaseCodec *codec = nullptr;
	void* data;  // data for the codec only

	const char* name;
	unsigned int frameSize = 0;
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

	// TODO: set these up
#if 0
	bool loop = false;      // should we loop the file when it ends?
	size_t time = 0;        // time in ms in the file
	size_t startTime = 0;   // time in ms to start playback
	size_t endTime = 0;     // time in ms to end playback
	float speed = 1.0f;
#endif

	bool paused = false;
	bool inWorld = true;  // is this a sound in the world? if so, run steam audio effects on it

	glm::vec3 pos;
};


class BaseCodec
{
protected:
	virtual                 ~BaseCodec() = default;

public:
	virtual bool            Init() = 0;
	virtual const char*     GetName() = 0;
	virtual bool            CheckExt( const char* ext ) = 0;

	virtual bool            OpenStream( const char* soundPath, AudioStream *stream ) = 0;
	virtual long            ReadStream( AudioStream *stream, size_t size, float* data ) = 0;
	virtual void            CloseStream( AudioStream *stream ) = 0;

	virtual void            SetAudioSystem( BaseAudioSystem* system ) { apAudio = system; };
	BaseAudioSystem*        apAudio;
};


class BaseAudioSystem: public BaseSystem
{
public:
	virtual void            SetListenerTransform( const glm::vec3& pos, const glm::quat& rot ) = 0;
	virtual void            SetPaused( bool paused ) = 0;
	virtual void            SetGlobalSpeed( float speed ) = 0;

	virtual bool            LoadSound( const char* soundPath, AudioStream** stream ) = 0;
	virtual bool            PlaySound( AudioStream *stream ) = 0;
	virtual void            FreeSound( AudioStream** stream ) = 0;

	virtual bool            RegisterCodec( BaseCodec *codec ) = 0;
};



