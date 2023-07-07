#include "codec_vorbis.h"
#include "util.h"

#include <SDL2/SDL.h>
#include <filesystem>

#if ENABLE_VORBIS

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>


LOG_REGISTER_CHANNEL( Vorbis, LogColor::Green );

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


bool CodecVorbis::CheckExt( std::string_view sExt )
{
	return sExt == "ogg";
}


bool CodecVorbis::Open( const char* soundPath, AudioStream *stream )
{
    FILE *soundFileHandle = fopen(soundPath, "rb");
    if (!soundFileHandle)
    {
        Log_MsgF( gVorbisChannel, "File does not exist: \"%s\"\n", soundPath );
        return false;
    }

    OggVorbis_File* oggFile = MALLOC_NEW( OggVorbis_File );
    vorbis_info *ovfInfo;

    // check if valid
    if (ov_open_callbacks(soundFileHandle, oggFile, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0)
    {
        Log_MsgF( gVorbisChannel, "Not a valid ogg file: \"%s\"\n", soundPath );
        ov_clear(oggFile);
        return false;
    }

    if (!ov_seekable(oggFile))
    {
        Log_MsgF( gVorbisChannel, "Stream not seekable: \"%s\"\n", soundPath );
        ov_clear(oggFile);
        return false;
    }

    ovfInfo = ov_info(oggFile, 0);
    if (!ovfInfo)
    {
        Log_MsgF( gVorbisChannel, "Unable to get stream info: \"%s\".\n", soundPath );
        ov_clear(oggFile);
        return false;
    }

    long numStreams = ov_streams(oggFile);
    if (numStreams != 1)
    {
        Log_MsgF( gVorbisChannel, "More than one (%s) stream in \"%s\".\n", numStreams, stream->name.c_str() );
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


/*CONVAR( vorbis_packet_size, 1024 );


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
}*/


long CodecVorbis::Read( AudioStream* stream, size_t size, ChVector< float >& data )
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
		data.resize( data.size() + result );
        for ( int i = 0; i < result; ++i )
        {
            for(int ch = 0; ch < stream->channels; ch++)
            {
                //data[totalFramesRead] = buffer[ch][i];
                // data.push_back( buffer[ch][i] );
				data[ totalFramesRead++ ] = buffer[ ch ][ i ];

                if (remain != 0)
                    remain--;
                else
                    Log_Msg( gVorbisChannel, "WHAT!!!\n");
            }
        }

        if (totalFramesRead >= size)
            break;
    }

    // remain doesn't have to be 0???
    return totalFramesRead;
}


int CodecVorbis::Seek( AudioStream *stream, double pos )
{
    CodecVorbisData* vorbisData = (CodecVorbisData*)stream->data;
    OggVorbis_File *oggFile = vorbisData->oggFile;

    long seekable = ov_seekable( oggFile );
    if ( !seekable )
        return -1;

    return ov_time_seek( oggFile, pos );
}


void CodecVorbis::Close( AudioStream *stream )
{
    CodecVorbisData* vorbisData = (CodecVorbisData*)stream->data;
    ov_clear(vorbisData->oggFile);
    fclose(vorbisData->file);
}

#endif

