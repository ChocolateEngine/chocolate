#pragma once

#include "../core/audio.h"

#define ENABLE_WAV 0

#if ENABLE_WAV
class CodecWav: public BaseCodec
{
protected:
	virtual ~CodecWav() = default;

public:
	virtual bool            Init() override;
	virtual const char*     GetName() override { return "Wav Audio Codec"; }
	virtual bool            CheckExt( const char* ext ) override;

	virtual bool            OpenStream( const char* soundPath, AudioStream *stream ) override;
	virtual long            ReadStream( AudioStream *stream, size_t size, float* data ) override;
	virtual void            CloseStream( AudioStream *stream ) override;
};
#endif

