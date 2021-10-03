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

    stream->data = (void*)vorbisData;
    stream->rate = ovfInfo->rate;
    stream->channels = ovfInfo->channels;
    stream->bits = 16;
    stream->width = 2;
    stream->frameSize = stream->channels * 32;  // Float 32

    return true;
}


long CodecVorbis::ReadStream( AudioStream *stream, size_t size, float* data )
{
    OggVorbis_File *oggFile = (OggVorbis_File*)((CodecVorbisData*)stream->data)->oggFile;
    // static int currentSection;
    int bitStream;

    size_t totalFramesRead = 0;

    //size_t remain = size; // stream->channels;
    size_t remain = size / stream->channels;
    long result;
    float** buffer = nullptr;

    while(1)
    {
        // uhhhh what if we have more than needed
        result = ov_read_float(oggFile, &buffer, remain, &bitStream);
        if (result == 0)
        {
            return totalFramesRead;  // EOF
        }
        else if (result < 0)
        {
            return result;  // error in the stream.
        }

        remain -= result;
        // totalFramesRead += result;

        // for (int i = 0; i < count; ++i)
        for (int i = 0; i < result; ++i)
        {
            for(int ch = 0; ch < stream->channels; ch++)
            {
                data[totalFramesRead] = buffer[ch][i];
                totalFramesRead++;
            }
        }

        if (remain <= 0)
            break;

        // buffer += result;
    }

    /*for (int ch = 0; ch < stream->channels; ch++)
    {
        for(int i = 0; i < result; ++i)
        {
            data[totalFramesRead] = buffer[ch][i];
            totalFramesRead++;
        }
    }*/

    return totalFramesRead;
}


void CodecVorbis::CloseStream( AudioStream *stream )
{
    CodecVorbisData* vorbisData = (CodecVorbisData*)stream->data;
    ov_clear(vorbisData->oggFile);
    fclose(vorbisData->file);
}

#endif

