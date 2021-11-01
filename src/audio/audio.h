#pragma once

#include "iaudio.h"

#include <SDL2/SDL.h>
#include <vector>

#include "phonon.h"

class BaseCodec;

struct AudioStreamInternal: public AudioStream
{
	BaseCodec *codec = nullptr;
	void* data;  // data for the codec only

	// other stuff
	SDL_AudioFormat format = AUDIO_F32;
	unsigned int frameSize = 0;
	unsigned int size;
	unsigned int samples;
	unsigned int bits;
	unsigned char width;  // ???

	// audio stream to store audio from the codec and covert it
	SDL_AudioStream *audioStream = nullptr;

	// maybe std::vector will work here? try later
	float *inBufferAudio = nullptr;
	float *midBufferAudio = nullptr;
	float *outBufferAudio = nullptr;

	IPLAudioBuffer inBuffer;
	IPLAudioBuffer midBuffer;
	IPLAudioBuffer outBuffer;

	IPLhandle directSoundEffect = {};
	IPLhandle binauralEffect = {};
};


class BaseCodec
{
protected:
	virtual                 ~BaseCodec() = default;

public:
	virtual bool            Init() = 0;
	virtual const char*     GetName() = 0;
	virtual bool            CheckExt( const char* ext ) = 0;

	virtual bool            Open( const char* soundPath, AudioStreamInternal *stream ) = 0;
	virtual long            Read( AudioStreamInternal *stream, size_t size, std::vector<float> &data ) = 0;
	virtual int             Seek( AudioStreamInternal *stream, double pos ) = 0;
	virtual void            Close( AudioStreamInternal *stream ) = 0;
};

extern "C"
{

	//void                    SetListenerTraRot( const glm::vec3& pos, const glm::quat& rot );
	void                    SetListenerTransformAng( const glm::vec3* pos, const glm::vec3* ang );
	void                    SetPaused( bool paused );
	void                    SetGlobalSpeed( float speed );

	bool                    LoadSound( const char* soundPath, AudioStream** stream );
	bool                    PlaySound( AudioStream *stream );
	void                    FreeSound( AudioStream** stream );
	int                     Seek( AudioStream *streamPublic, double pos );

//virtual bool                    RegisterCodec( BaseCodec *codec );
	bool                            RegisterCodec( BaseCodec *codec );

	bool                            UpdateStream( AudioStreamInternal* stream );
	bool                            MixAudio();
	bool                            QueueAudio();

	bool                            InitSteamAudio();
	int                             ApplySpatialEffects( AudioStreamInternal* stream, float* data, size_t frameSize );

/* Initialize system.  */
	void                 	Init(  );

/* Initialize system.  */
	void                 	Update( float frameTime );

// --------------------------------------------------
// steam audio settings

}
