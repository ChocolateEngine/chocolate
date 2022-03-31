#pragma once

#include "audio.h"

#if ENABLE_VORBIS
class CodecVorbis: public BaseCodec
{
protected:
	virtual ~CodecVorbis() = default;

public:
	virtual bool            Init() override;
	virtual const char*     GetName() override { return "Vorbis Audio Codec"; }
	virtual bool            CheckExt( const char* ext ) override;

	virtual bool            Open( const char* soundPath, AudioStream *stream ) override;
	//virtual long            ReadPacket( AudioStream *stream, std::vector<float> &data );
#if FLOAT_AUDIO
	virtual int             Read( AudioStream *stream, size_t size, std::vector<float> &data ) override;
#else
	virtual int             Read( AudioStream *stream, size_t size, std::vector<char> &data ) override;
#endif
	virtual int             Seek( AudioStream *stream, double pos ) override;
	virtual void            Close( AudioStream *stream ) override;
};
#endif

