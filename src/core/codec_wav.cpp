#include "../../inc/core/codec_wav.h"
#include "../../inc/shared/util.h"

#include <SDL2/SDL.h>
#include <filesystem>

#if ENABLE_WAV
#include "../../thirdparty/libwav/include/wav.h"



bool CodecWav::Init(  )
{
	return true;
}


bool CodecWav::CheckExt( const char* ext )
{
	return (_stricmp( ".wav", ext) == 0);
}


bool CodecWav::OpenStream( const char* soundPath, AudioStream *stream )
{
    WavFile* wav = wav_open( soundPath, WAV_OPEN_READ );
    if ( wav == NULL )
    {
        Print( "[CodecWav] Failed to open sound file \"%s\"\n", soundPath );
        return false;
    }

    stream->data = wav;
    stream->rate = wav_get_sample_rate(wav);
    stream->bits = wav_get_valid_bits_per_sample(wav);
    stream->width = 2;
    stream->channels = wav_get_num_channels(wav);
    stream->frameSize = stream->channels * 32;  // Float 32

    stream->format = AUDIO_S16;

    return true;
}


long CodecWav::ReadStream( AudioStream *stream, size_t size, float* data )
{
    WavFile* wav = (WavFile*)stream->data;

    size_t ret = wav_read(wav, data, size);

    return ret;
}


void CodecWav::CloseStream( AudioStream *stream )
{
    wav_close((WavFile*)stream->data);
}
#endif
