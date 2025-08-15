#pragma once
#include<vector>
#include<mutex>
extern "C" {
#include<libavcodec/avcodec.h>
#include <libavutil/opt.h>
}
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avutil.lib")
struct AVCodecContext;
struct AVPacket;
struct AVFrame;
void PrintErr(int err);
class XCodec
{
public:
	static AVCodecContext* Create(int codec_id,bool is_encode);
	void set_c(AVCodecContext* c);
	bool SetOpt(const char* key, const char* val);
	bool SetOpt(const char* key, int val);
	bool Open();
	AVFrame* CreateFrame();
	AVFrame* CreateAudioFrame(int,AVSampleFormat,int);

protected:
	AVCodecContext* c_ = nullptr;
	std::mutex mux_;
};

