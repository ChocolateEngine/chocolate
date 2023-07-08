#pragma once

#include "audio.h"

#if ENABLE_WAV
class CodecWav: public IAudioCodec
{
protected:
	virtual ~CodecWav() = default;

public:
	virtual bool            Init() override;
	virtual const char*     GetName() override { return "Wav Audio Codec"; }
	virtual bool            CheckExt( const char* ext ) override;

	virtual bool            Open( const char* soundPath, AudioStreamInternal *stream ) override;
	virtual long            Read( AudioStreamInternal *stream, size_t size, std::vector<float> &data ) override;
	virtual int             Seek( AudioStreamInternal *stream, double pos ) override;
	virtual void            Close( AudioStreamInternal *stream ) override;
};
#endif

