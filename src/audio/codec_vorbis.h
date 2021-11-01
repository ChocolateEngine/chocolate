#pragma once

#include "audio.h"

#if ENABLE_VORBIS
class CodecVorbis: public BaseCodec
{
public:
	virtual ~CodecVorbis(){};

public:
	virtual bool            Init() override;
	virtual const char*     GetName() override { return "Vorbis Audio Codec"; }
	virtual bool            CheckExt( const char* ext ) override;

	virtual bool            Open( const char* soundPath, AudioStreamInternal *stream ) override;
	//virtual long            ReadPacket( AudioStreamInternal *stream, std::vector<float> &data );
	virtual long            Read( AudioStreamInternal *stream, size_t size, std::vector<float> &data ) override;
	virtual int             Seek( AudioStreamInternal *stream, double pos ) override;
	virtual void            Close( AudioStreamInternal *stream ) override;
};
#endif

