#pragma once
#if ENABLE_LIBAV

#include "../core/audio.h"
class CodecLibAV: public BaseCodec
{
protected:
	virtual ~CodecLibAV() = default;

public:
	virtual bool            Init() override;
	virtual const char*     GetName() override { return "libavcodec Audio Codec"; }
	virtual bool            CheckExt( const char* ext ) override;

	virtual bool            OpenStream( const char* soundPath, AudioStream *stream ) override;
	virtual long            ReadStream( AudioStream *stream, size_t size, float* data ) override;
	virtual void            CloseStream( AudioStream *stream ) override;
};
#endif

