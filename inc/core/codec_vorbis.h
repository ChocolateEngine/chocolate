#pragma once

#include "../core/audio.h"

#define ENABLE_VORBIS 0

#if ENABLE_VORBIS
class CodecVorbis: public BaseCodec
{
protected:
	virtual ~CodecVorbis() = default;

public:
	virtual bool            Init() override;
	virtual const char*     GetName() override { return "Vorbis Audio Codec"; }
	virtual bool            CheckExt( const char* ext ) override;

	virtual bool            OpenStream( const char* soundPath, AudioStream *stream ) override;
	virtual long            ReadStream( AudioStream *stream, size_t size, float* data ) override;
	virtual void            CloseStream( AudioStream *stream ) override;
};
#endif

