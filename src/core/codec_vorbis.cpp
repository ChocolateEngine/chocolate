#include "../../inc/core/codec_vorbis.h"
#include "../../inc/shared/util.h"

#include <SDL2/SDL.h>
#include <filesystem>

#if ENABLE_VORBIS

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>


struct CodecVorbisData
{
    OggVorbis_File* oggFile = nullptr;
    FILE* file = nullptr;
    float **buffer = nullptr;
    long result = 0;
};


bool CodecVorbis::Init(  )
{
    return true;
}


bool CodecVorbis::CheckExt( const char* ext )
{
	return (_stricmp( ".ogg", ext) == 0);
}


bool CodecVorbis::OpenStream( const char* soundPath, AudioStream *stream )
{
    FILE *soundFileHandle;
    errno_t err = fopen_s(&soundFileHandle, soundPath, "rb");
    if (err != 0)
    {
        Print( "File does not exist: \"%s\"\n", soundPath );
        return false;
    }

    OggVorbis_File* oggFile = MALLOC_NEW( OggVorbis_File );
    vorbis_info *ovfInfo;

    // check if valid
    if (ov_open_callbacks(soundFileHandle, oggFile, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0)
    {
        Print( "Not a valid ogg file: \"{}\"\n", soundPath );
        ov_clear(oggFile);
        return false;
    }

    if (!ov_seekable(oggFile))
    {
        Print( "Stream not seekable: \"{}\"\n", soundPath );
        ov_clear(oggFile);
        return false;
    }

    ovfInfo = ov_info(oggFile, 0);
    if (!ovfInfo)
    {
        Print( "Unable to get stream info: \"{}\".\n", soundPath );
        ov_clear(oggFile);
        return false;
    }

    long numStreams = ov_streams(oggFile);
    if (numStreams != 1)
    {
        Print( "More than one ({0}) stream in \"{1}\".\n", numStreams, stream->name );
        ov_clear(oggFile);
        return false;
    }

    CodecVorbisData* vorbisData = MALLOC_NEW( CodecVorbisData );
    vorbisData->file = soundFileHandle;
    vorbisData->oggFile = oggFile;
    vorbisData->buffer = nullptr;
    vorbisData->result = 0;

    stream->data = (void*)vorbisData;
    stream->rate = ovfInfo->rate;
    stream->channels = ovfInfo->channels;
    stream->bits = 16;
    stream->width = 2;
    //stream->frameSize = stream->channels * 32;  // Float 32
    //stream->format = AUDIO_S16;

    return true;
}


static void
vorbis_interleave(float *dest, const float *const*src,
		  unsigned nframes, unsigned channels)
{
	for (const float *const*src_end = src + channels;
	     src != src_end; ++src, ++dest) {
		float *d = dest;
		for (const float *s = *src, *s_end = s + nframes;
		     s != s_end; ++s, d += channels)
			*d = *s;
	}
}


//extern size_t SOUND_BYTES;


#define NQR_INT16_MAX 32767.f
#define int16_to_float32(s) ((float) (s) / NQR_INT16_MAX)
#define float32_to_int16(s) ((float) (s) * NQR_INT16_MAX)


CONVAR( vorbis_packet_size, 1024 );


long CodecVorbis::ReadPacket( AudioStream *stream, std::vector<float> &data )
{
    CodecVorbisData *vorbisData = (CodecVorbisData*)stream->data;
    OggVorbis_File *oggFile = vorbisData->oggFile;

    // AAAA
    static int bitStream = 0;

    size_t totalFramesRead = 0;

    long result;
    float** buffer = nullptr;

    result = ov_read_float(oggFile, &buffer, vorbis_packet_size, &bitStream);

    if (result == 0)
        return totalFramesRead;  // EOF

    else if (result < 0)
        return result;  // error in the stream.

    for (int i = 0; i < result; ++i)
    {
        for(int ch = 0; ch < stream->channels; ch++)
        {
            data.push_back(buffer[ch][i]);
            totalFramesRead++;
        }
    }

    return totalFramesRead;
}


long CodecVorbis::ReadStream( AudioStream *stream, size_t size, std::vector<float> &data )
{
    CodecVorbisData *vorbisData = (CodecVorbisData*)stream->data;
    OggVorbis_File *oggFile = vorbisData->oggFile;

    //float buffer[8192];
    //const int frames_per_buffer = 8192 / stream->channels;
   // const unsigned frame_size = sizeof(buffer[0]) * stream->channels;

    //long &result = vorbisData->result;
    // static int currentSection;
    int bitStream = 0;

    size_t totalFramesRead = 0;

    //size_t remain = size;
    size_t remain = size * stream->channels;
    //size_t remain = size / stream->channels;
    long result;
    float** buffer = nullptr;

    while(1)
    {
        result = ov_read_float(oggFile, &buffer, remain, &bitStream);

        if (result == 0)
            return totalFramesRead;  // EOF

        else if (result < 0)
            return result;  // error in the stream.

        // for (int i = 0; i < count; ++i)
        for (int i = 0; i < result; ++i)
        {
            for(int ch = 0; ch < stream->channels; ch++)
            {
                //data[totalFramesRead] = buffer[ch][i];
                data.push_back(buffer[ch][i]);
                totalFramesRead++;

                if (remain != 0)
                    remain--;
                else
                    Print("WHAT!!!\n");
            }
        }

        if (totalFramesRead >= size)
            break;
    }

    // remain doesn't have to be 0???
    return totalFramesRead;
}


void CodecVorbis::CloseStream( AudioStream *stream )
{
    CodecVorbisData* vorbisData = (CodecVorbisData*)stream->data;
    ov_clear(vorbisData->oggFile);
    fclose(vorbisData->file);
}

#endif

