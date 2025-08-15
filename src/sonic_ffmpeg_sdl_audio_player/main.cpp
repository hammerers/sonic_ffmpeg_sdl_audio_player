#include<iostream>
using namespace std;
#include"xdecode.h"
#include"xdemux.h"
#include"avqueue.h"
#include"sonic.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <sdl/SDL.h>

}
#pragma comment(lib,"SDL2.lib")
#ifdef main
#undef main
#endif

float speed = 1.0;//默认1.0
char* file_name;

//ffmpeg
XDemux demux;
XDecode decode;
AVFormatContext* format_ctx = nullptr;
AVCodecContext* codec_ctx = nullptr;
SwrContext* swr_ctx = nullptr;
AVQueue<AVFrame*> frame_q;

//SDL
SDL_AudioSpec wanted_spec, obtained_spec;

//sonic
sonicStream sonic = nullptr;

AVSampleFormat SDLToFFmpegFormat(Uint16 sdl_format) {
	switch (sdl_format) {
	case AUDIO_S16LSB:
		return AV_SAMPLE_FMT_S16;
	case AUDIO_S32LSB:
		return AV_SAMPLE_FMT_S32;
	case AUDIO_F32LSB:
		return AV_SAMPLE_FMT_FLT;
	default:
		return AV_SAMPLE_FMT_NONE;
	}
}
void printAudioSpec(const SDL_AudioSpec& obtained_spec) {
	std::cout << "Sample Rate (freq): " << obtained_spec.freq << std::endl;
	std::cout << "Format (format): " << (SDL_AudioFormat)obtained_spec.format << std::endl;
	std::cout << "Channels (channels): " << (int)obtained_spec.channels << std::endl;
	std::cout << "Silence Value (silence): " << (int)obtained_spec.silence << std::endl;
	std::cout << "Samples per buffer (samples): " << obtained_spec.samples << std::endl;
	std::cout << "Callback function address: " << obtained_spec.callback << std::endl;
	std::cout << "User data address: " << obtained_spec.userdata << std::endl;
}
void AudioCallback(void* userdata, Uint8* stream, int len) {
	SDL_memset(stream, 0, len);
	//AVQueue<AVFrame*>* frame_q= static_cast<AVQueue<AVFrame*>*>(userdata);
	AVFrame* frame = nullptr;
	int filled = 0;
	while (filled < len) {
		int available = sonicSamplesAvailable(sonic);
		if (available>0) {
			int read_sample = std::min(available, (len - filled) / (obtained_spec.channels * av_get_bytes_per_sample(SDLToFFmpegFormat(obtained_spec.format))));
			sonicReadFloatFromStream(sonic, (float*)(stream), read_sample);
			filled += read_sample * obtained_spec.channels * av_get_bytes_per_sample(SDLToFFmpegFormat(obtained_spec.format));
			continue;
		}
		frame_q.Pop(frame);
		sonicWriteFloatToStream(sonic, (float*)frame->data[0], frame->nb_samples);
		av_frame_unref(frame);
	}
	
	av_frame_free(&frame);
}
int main(int argc,char*argv[]) {
	cout << "播放mp4音频" << endl;
	cout << "格式: sonic_ffmpeg_sdl_audio_player.exe [文件路径] [播放速度]"<<endl;
	cout << "样例: sonic_ffmpeg_sdl_audio_player.exe ./1080p.mp4 1.5" << endl;
	if (argc < 2)return -1;
	file_name = argv[1];
	if (argc >= 3)speed = atof(argv[2]);
	cout << "file_name: " << file_name << " speed: " << speed << endl;

	//FFMPEG 解封装
	format_ctx = XDemux::Open(file_name);
	if (!format_ctx) {
		cerr << "XDemux::Open error" << endl;
		return -2;
	}
	demux.set_c(format_ctx);
	demux.PrintInfo();
	//FFMPEG 解码
	codec_ctx=decode.Create(demux.audio_codec_id(), false);
	if(!codec_ctx) {
		cerr << "XDecode::Create error" << endl;
		return -2;
	}
	demux.CopyPara(demux.audio_index(), codec_ctx);
	decode.set_c(codec_ctx);
	if (!decode.Open()) {
		cerr << "XDecode::Open error" << endl;
		return -2;
	}

	wanted_spec.freq = codec_ctx->sample_rate;
	wanted_spec.format = AUDIO_F32LSB;
	wanted_spec.channels = (Uint8)codec_ctx->channels;
	wanted_spec.silence = 0;
	wanted_spec.samples = 1024;
	wanted_spec.callback = AudioCallback;
	wanted_spec.userdata = &frame_q;
	// SDL 初始化 
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
		return -2;
	}
		if (SDL_OpenAudio(&wanted_spec, &obtained_spec) < 0) {
			std::cerr << "Could not open audio: " << SDL_GetError() << std::endl;
			return -2;
		}
		printAudioSpec(obtained_spec);

		swr_ctx=swr_alloc_set_opts(nullptr,
			av_get_default_channel_layout(obtained_spec.channels),
			SDLToFFmpegFormat(obtained_spec.format),
			obtained_spec.freq,
			av_get_default_channel_layout(codec_ctx->channels),
			(AVSampleFormat)codec_ctx->sample_fmt,
			codec_ctx->sample_rate,
			0, nullptr
		);
		if (!swr_ctx || swr_init(swr_ctx)<0) {
			std::cerr << "Could not init swr: " << SDL_GetError() << std::endl;
			return -2;
		}

		//SONIC 初始化
		sonic=sonicCreateStream(codec_ctx->sample_rate, codec_ctx->channels);
		if (!sonic) {
			std::cerr << "Could not init sonic " << std::endl;
			return -2;
		}
		sonicSetSpeed(sonic, speed);
		sonicSetPitch(sonic, 1.0f);
		sonicSetVolume(sonic, 1.0f);
		sonicSetChordPitch(sonic, 0);
		//循环解封装解码
		AVPacket pkt;
		AVFrame* frame = av_frame_alloc();
		for (;;) {
			if (!demux.Read(&pkt)) {
				break;
			}
			if (demux.audio_index() >= 0 && pkt.stream_index == demux.audio_index()) {
				if (!decode.Send(&pkt)) {
					cerr << "decode::Send error" << endl;
					continue;
				}
				AVFrame* resampled_frame = av_frame_alloc();
				while (decode.Recv(frame)) {
					//入队前进行重采样
					
					resampled_frame->sample_rate = obtained_spec.freq;
					resampled_frame->format = SDLToFFmpegFormat(obtained_spec.format);
					resampled_frame->channels = obtained_spec.channels;
					resampled_frame->channel_layout = av_get_default_channel_layout(obtained_spec.channels);

					int dst_nb_samples = obtained_spec.samples;
					resampled_frame->nb_samples = dst_nb_samples;
					int re = av_frame_get_buffer(resampled_frame, 4);
					if (re != 0) {
						av_frame_free(&resampled_frame);
						cerr << "av_frame_get_buffer error "<<re << endl;
					
					}
					int actually_sample = swr_convert(swr_ctx, resampled_frame->data, dst_nb_samples, (const uint8_t**)frame->data, frame->nb_samples);
					if (actually_sample < 0) {
						cerr << "swr_convert error" << endl;
					
					}
					resampled_frame->nb_samples = actually_sample;
					frame_q.Push(av_frame_clone(resampled_frame));
					av_frame_unref(resampled_frame);
					
					SDL_PauseAudio(0);
				}
				av_frame_free(&resampled_frame);
			}
			
			av_packet_unref(&pkt);
		}
		avcodec_send_packet(codec_ctx, nullptr);
		while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
			frame_q.Push(frame);
		}
		//清理
		frame_q.Stop();
		frame_q.Clear();
		av_frame_unref(frame);
		av_frame_free(&frame);
		sonicDestroyStream(sonic);
		demux.set_c(nullptr);
		decode.set_c(nullptr);
		SDL_Quit();
		return 0;
	}