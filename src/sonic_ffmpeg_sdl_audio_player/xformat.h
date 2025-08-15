#pragma once
#include<mutex>
#include<thread>
#include<iostream>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include<libavformat/avformat.h> 
}
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"avformat.lib")
struct XRational {
	int num; ///< Numerator
	int den; ///< Denominator
};
class XFormat
{
public:
	bool CopyPara(int stream_index, AVCodecParameters* dst);
	bool CopyPara(int stream_index, AVCodecContext* dst);
	void set_c(AVFormatContext* c);
	int audio_index() { return audio_index_; }
	int video_index() { return video_index_; }
	XRational video_time_base() { return video_time_base_; }
	XRational audio_time_base() { return audio_time_base_; }

	bool RescaleTime(AVPacket* pkt,long long offset_pts,XRational time_base);
	int video_codec_id() { return video_codec_id_; }
	int audio_codec_id() { return audio_codec_id_; }
	void PrintInfo();
protected:
	std::mutex mux_;
	AVFormatContext* c_ = nullptr;
	int audio_index_ = -1;
	int video_index_ = -1;
	XRational video_time_base_ = { 1,25 };
	XRational audio_time_base_ = { 1,9000 };
	int video_codec_id_ = 0;
	int audio_codec_id_ = 0;
};

