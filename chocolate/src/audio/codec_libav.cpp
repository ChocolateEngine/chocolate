#include "codec_libav.h"
#include "core/util.h"

#if ENABLE_LIBAV

extern "C"
{
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
	#include <libswscale/swscale.h>
	#include <libswresample/swresample.h>
	#include <libavdevice/avdevice.h>
}

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_thread.h>

#define SDL_AUDIO_BUFFER_SIZE 1024
#define MAX_AUDIO_FRAME_SIZE 192000


class PacketQueue
{
public:
	PacketQueue();
	~PacketQueue();

	void Init();
	int Put(AVPacket *pkt);
	int Get(AVPacket *pkt, int block);

	AVPacketList *first_pkt = nullptr;
	AVPacketList *last_pkt = nullptr;

	int nb_packets = 0;
	int size = 0;
	SDL_mutex *mutex = nullptr;
	SDL_cond *cond = nullptr;
	bool quit = false;
};


class AudioFile
{
public:
	AudioFile() {}
	~AudioFile() { Close(); }

	int                  Load(const std::string& file);
	void                 Close();

	int                  GetAudioStreamIndex();
	int                  DecodeFrame( int buf_size );
	void                 ReadPackets( size_t amount );
	long                 Read( AudioStream *stream, size_t size, float* data );

	AVFormatContext      *streamInfo = NULL;
	AVCodecContext       *audioCtxOrig = NULL;
	AVCodecContext       *audioCtx = NULL;
	AVCodec              *aCodec = NULL;

	SwrContext           *swrCtx = NULL;
	SwrContext           *swrMixCtx = NULL;

	PacketQueue          audioq;

	bool                 loaded = false;
	bool                 paused = false;

	uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
	unsigned int audio_buf_size = 0;
	unsigned int audio_buf_index = 0;
};


int TestPlaybackCallback(const std::string& filePath);

// compatibility with newer API
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif


PacketQueue::PacketQueue()
{
}

PacketQueue::~PacketQueue()
{
}


void PacketQueue::Init()
{
	mutex = SDL_CreateMutex();
	cond = SDL_CreateCond();
}


int PacketQueue::Put( AVPacket *pkt )
{
	AVPacketList *pkt1;
	if (av_dup_packet(pkt) < 0)
	{
		return -1;
	}

	pkt1 = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if (!pkt1)
		return -1;

	pkt1->pkt = *pkt;
	pkt1->next = NULL;

	//SDL_LockMutex(mutex);

	if (!last_pkt)
		first_pkt = pkt1;
	else
		last_pkt->next = pkt1;
	last_pkt = pkt1;
	nb_packets++;
	size += pkt1->pkt.size;
	//SDL_CondSignal(cond);

	//SDL_UnlockMutex(mutex);
	return 0;
}


// static int return?
int PacketQueue::Get( AVPacket *pkt, int block )
{
	AVPacketList *pkt1;
	int ret;

	//SDL_LockMutex(mutex);

	for(;;)
	{
		if(quit)
		{
			ret = -1;
			break;
		}

		pkt1 = first_pkt;
		if (pkt1)
		{
			first_pkt = pkt1->next;

			if (!first_pkt)
				last_pkt = NULL;

			nb_packets--;
			size -= pkt1->pkt.size;
			*pkt = pkt1->pkt;
			av_free(pkt1);
			ret = 1;
			break;
		}
		else //if (!block)
		{
			ret = 0;
			break;
		}
		/*else
		{
			SDL_CondWait(cond, mutex);
		}*/
	}

	//SDL_UnlockMutex(mutex);
	return ret;
}


int AudioFile::Load(const std::string& file)
{
	if (loaded)
		return 0;

	if (avformat_open_input(&streamInfo, file.c_str(), NULL, NULL) != 0)
	{
		return -1;
	}

	if (avformat_find_stream_info(streamInfo, NULL)<0)
	{
		Close();
		return -1;
	}

	// Dump information about file onto standard error
	av_dump_format(streamInfo, 0, file.c_str(), 0);

	// Find the first audio stream
	int audioStream = GetAudioStreamIndex();

	if (audioStream == -1)
	{
		Close();
		return -1;
	}

	audioCtxOrig = streamInfo->streams[audioStream]->codec;
	aCodec = avcodec_find_decoder(audioCtxOrig->codec_id);
	if (!aCodec)
	{
		Print("[CodecLibAV] Unsupported codec!\n");
		Close();
		return -1;
	}

	// Copy context
	audioCtx = avcodec_alloc_context3(aCodec);
	if (avcodec_copy_context(audioCtx, audioCtxOrig) != 0)
	{
		Print("[CodecLibAV] Couldn't copy codec context\n");
		Close();
		return -1;
	}

	avcodec_open2(audioCtx, aCodec, NULL);

	swrCtx = swr_alloc();
	if (!swrCtx)
	{
		Print("[CodecLibAV] Couldn't alloc SwrContext\n");
		Close();
		return -1;
	}

	av_opt_set_channel_layout(swrCtx, "in_channel_layout", audioCtx->channel_layout, 0);
	av_opt_set_channel_layout(swrCtx, "out_channel_layout", audioCtx->channel_layout, 0);
	av_opt_set_int(swrCtx, "in_sample_rate", audioCtx->sample_rate, 0);
	av_opt_set_int(swrCtx, "out_sample_rate", audioCtx->sample_rate, 0);
	av_opt_set_sample_fmt(swrCtx, "in_sample_fmt", audioCtx->sample_fmt, 0);
	av_opt_set_sample_fmt(swrCtx, "out_sample_fmt", AV_SAMPLE_FMT_FLT, 0);

	if (swr_init(swrCtx))
	{
		Print("[CodecLibAV] Couldn't init SwrContext\n");
		Close();
		return -1;
	}

	loaded = true;

	return 0;
}


void AudioFile::Close()
{
	if (!loaded)
		return;

	loaded = false;

	if (streamInfo)
	{
		avformat_close_input(&streamInfo);
		streamInfo = NULL;
	}

	if (audioCtxOrig)
	{
		avcodec_close(audioCtxOrig);
		// avcodec_free_context(&audioCtxOrig);
		// audioCtxOrig = NULL;
	}

	if (audioCtx)
	{
		avcodec_close(audioCtx);
		// avcodec_free_context(&audioCtx);
		// audioCtx = NULL;
	}

	if (swrCtx)
	{
		swr_free(&swrCtx);
		swrCtx = NULL;
	}

	aCodec = NULL;
}


int AudioFile::GetAudioStreamIndex()
{
	return streamInfo ? av_find_best_stream(streamInfo, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0) : -1;
}


int AudioFile::DecodeFrame( int buf_size )
{
	static AVPacket pkt;
	static uint8_t *audio_pkt_data = NULL;
	static int audio_pkt_size = 0;
	static AVFrame frame;

	//static uint8_t converted_data[(192000 * 3) / 2];
	static uint8_t * converted;

	converted = &audio_buf[0];

	int len1, len2 = 0;
	static int data_size = 0;

	for(;;)
	{
		// while(audio_pkt_size > 0)
		while(audio_pkt_size > 0 || data_size > 0)
		{
			int got_frame = 0;

			if (audio_pkt_size > 0)
			{
				len1 = avcodec_decode_audio4(audioCtx, &frame, &got_frame, &pkt);
				if(len1 < 0)
				{
					/* if error, skip frame */
					audio_pkt_size = 0;
					break;
				}
				audio_pkt_data += len1;
				audio_pkt_size -= len1;
				data_size = 0;
			}

			if (got_frame)
			{
				data_size = av_samples_get_buffer_size(
					NULL,
					audioCtx->channels,
					frame.nb_samples,
					audioCtx->sample_fmt,
					1);

				int outSize = av_samples_get_buffer_size(NULL, audioCtx->channels, frame.nb_samples, AV_SAMPLE_FMT_FLT, 1);
				len2 = swr_convert(swrCtx, &converted, frame.nb_samples, (const uint8_t**)&frame.data[0], frame.nb_samples);
				//memcpy(audio_buf, converted_data, outSize);
				data_size = outSize;

				//assert(data_size <= buf_size);
				// memcpy(audio_buf, frame.data[0], data_size);
			}

			if (data_size <= 0)
			{
				/* No data yet, get more frames */
				continue;
			}

			/* We have data, return it and come back for more later */
			int tmp = data_size;
			data_size -= buf_size;
			return tmp;
		}

		if (pkt.data)
		{
			av_free_packet(&pkt);
		}

		if (audioq.quit)
		{
			return -1;
		}

		int queueRet = audioq.Get(&pkt, 1);

		if (queueRet == 0)
		{
			ReadPackets( buf_size );
		}
		else if (queueRet < 0)
		{
			return -1;
		}

		audio_pkt_data = pkt.data;
		audio_pkt_size = pkt.size;
	}
}


void AudioFile::ReadPackets( size_t amount )
{
	AVPacket packet;

	static size_t amountRead = 0;

	// Read frames
	while ( av_read_frame( streamInfo, &packet ) >= 0 )
	{
		if ( packet.stream_index == GetAudioStreamIndex() )
		{
			amountRead += packet.size; // ????
			audioq.Put( &packet );
		}
		else
		{
			av_packet_unref( &packet );
		}

		if ( amountRead >= amount )
		{
			amountRead -= amount;
			break;
		}
	}
}


long AudioFile::Read( AudioStream *stream, size_t size, float* data )
{
	int len1 = 0;
	int audio_size = 0;

	//static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
	//static unsigned int audio_buf_size = 0;
	//static unsigned int audio_buf_index = 0;

	while (size > 0)
	{
		if (audio_buf_index >= audio_buf_size)
		{
			ReadPackets( size );

			/* We have already sent all our data; get more */
			// audio_size = file->DecodeFrame(sizeof(audio_buf));
			audio_size = DecodeFrame( 8192 );
			if (audio_size < 0)
			{
				/* If error, output silence */
				audio_buf_size = 256; // arbitrary?
				memset(audio_buf, 0, audio_buf_size);
			}
			else
			{
				audio_buf_size = audio_size; // 2;
			}

			audio_buf_index = 0;
		}

		len1 = audio_buf_size - audio_buf_index;
		if(len1 > size)
			len1 = size;

		memcpy(data, (uint8_t *)audio_buf + audio_buf_index, len1);
		size -= len1;
		data += len1;
		audio_buf_index += len1;
	}

	return len1;
}


bool CodecLibAV::Init()
{
	return true;
}



// we can just check this when loading the file,
// if it fails, it just goes to the next codec anyway
bool CodecLibAV::CheckExt( const char* ext )
{
	// return (_stricmp( ".wav", ext) == 0);
	return true;
}


bool CodecLibAV::OpenStream( const char* soundPath, AudioStream *stream )
{
	AudioFile* file = new AudioFile;

	int ret = file->Load( soundPath );
	if ( ret == -1 )
		return false;
	
	stream->data = file;
	stream->channels = file->audioCtx->channels;
	stream->width = file->audioCtx->width;
	stream->rate = file->audioCtx->sample_rate;
	stream->bits = file->audioCtx->bits_per_coded_sample;  // or raw sample? idk

	stream->frameSize = file->audioCtx->frame_size;
	//stream->frameSize = stream->channels * 32;  // Float 32

	return true;
}


long CodecLibAV::ReadStream( AudioStream *stream, size_t size, float* data )
{
	AudioFile *file = (AudioFile *)stream->data;
	return file->Read( stream, size, data );
}


void CodecLibAV::CloseStream( AudioStream *stream )
{
	AudioFile *file = (AudioFile *)stream->data;

	if ( file->loaded )
		file->Close();

	delete (AudioFile*)stream->data;
}




#if 0
void AudioCallback(void *userdata, Uint8 *stream, int len)
{
	AudioFile *file = (AudioFile *)userdata;

	int len1, audio_size;

	static uint8_t audio_buf[(MAX_AUDIO_FRAME_SIZE * 3) / 2];
	static unsigned int audio_buf_size = 0;
	static unsigned int audio_buf_index = 0;

	while (len > 0)
	{
		if (audio_buf_index >= audio_buf_size)
		{
			/* We have already sent all our data; get more */
			// audio_size = file->DecodeFrame(sizeof(audio_buf));
			audio_size = file->DecodeFrame(8192);
			if (audio_size < 0)
			{
				/* If error, output silence */
				audio_buf_size = 256; // arbitrary?
				memset(audio_buf, 0, audio_buf_size);
			}
			else
			{
				audio_buf_size = audio_size; // 2;
			}

			audio_buf_index = 0;
		}

		len1 = audio_buf_size - audio_buf_index;
		if(len1 > len)
			len1 = len;

		memcpy(stream, (uint8_t *)audio_buf + audio_buf_index, len1);
		len -= len1;
		stream += len1;
		audio_buf_index += len1;
	}
}


int TestPlaybackCallback(const std::string& filePath)
{
	AudioFile* file = new AudioFile;
	if (file->Load(filePath) < 0)
	{
		return -1;
	}

	// Set audio settings from codec info
	SDL_AudioSpec wantedSpec, spec;

	SDL_memset(&spec, 0, sizeof(spec));

	wantedSpec.freq = file->audioCtx->sample_rate;  // something is very wrong here, i shouldn't need to divide by 2 (also sounds like shit)
	// WORKING ONE HERE
	// wantedSpec.format = AUDIO_F32MSB; // AUDIO_S16SYS;
	//wantedSpec.format = AUDIO_F32; // AUDIO_S16SYS;
	wantedSpec.format = AUDIO_S16; // AUDIO_S16SYS;
	//wantedSpec.channels = 2; //file->aCodecCtx->channels;
	wantedSpec.channels = file->audioCtx->channels;
	wantedSpec.silence = 0;
	wantedSpec.samples = SDL_AUDIO_BUFFER_SIZE;
	wantedSpec.callback = AudioCallback;
	wantedSpec.userdata = file;

	// if (SDL_OpenAudio(&wantedSpec, &spec) < 0)
	SDL_AudioDeviceID dev;
	// SDL_AUDIO_ALLOW_ANY_CHANGE allows this to work
	dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
	//dev = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &spec, 0);
	/*{
		fprintf(stderr, "SDL_OpenAudioDevice: %s\n", SDL_GetError());
		return -1;
	}*/

	// g_masterStream = SDL_NewAudioStream(AUDIO_S16, 2, aCodecCtx->sample_rate, AUDIO_S16, 2, aCodecCtx->sample_rate);

	// audio_st = pFormatCtx->streams[index]
	audioq.Init();
	// SDL_PauseAudio(0);
	SDL_PauseAudioDevice(dev, 0);

	SDL_Event event;
	AVPacket packet;

	// Read frames and save first five frames to disk
	while (av_read_frame(file->streamInfo, &packet) >= 0)
	{
		if (packet.stream_index == file->GetAudioStreamIndex())
		{
			audioq.Put(&packet);
		}
		else
		{
			av_packet_unref(&packet);
		}

		SDL_PollEvent(&event);
		switch(event.type)
		{
			case SDL_QUIT:
				quit = 1;
				SDL_Quit();
				exit(0);
				break;
			default:
				break;
		}
	}

	// lazy hack to wait for audio playback to finish
	SDL_Delay(file->streamInfo->duration / 1000);
	file->Close();
	delete file;

	return 0;
}
#endif

#endif
