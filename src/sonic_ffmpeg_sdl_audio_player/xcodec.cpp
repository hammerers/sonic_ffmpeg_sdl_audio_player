#include "xcodec.h"
#include<iostream>
using namespace std;
void PrintErr(int err) {
    char buf[1024];
    av_strerror(err, buf, sizeof(buf) - 1);
    cerr << buf << endl;
}
AVCodecContext* XCodec::Create(int codec_id,bool is_encode)
{
    AVCodec* codec = nullptr;
    if(is_encode)codec= avcodec_find_encoder((AVCodecID)codec_id);
    else codec = avcodec_find_decoder((AVCodecID)codec_id);
    if (!codec) {
        cerr << "avcodec_find_encoder failed " << codec_id << endl;
        return nullptr;
    }
    auto c = avcodec_alloc_context3(codec);
    if (!c) {
        cerr << "avcodec_alloc_context3 failed " << codec_id << endl;
        return nullptr;
    }
    //设置参数默认值
    c->time_base = { 1,25 };
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    c->thread_count = 16;

    return c;
}

void XCodec::set_c(AVCodecContext* c)
{
    unique_lock<mutex>lock(mux_);
    if (c_) {
        avcodec_free_context(&c_);

    }
    this->c_ = c;
}

bool XCodec::SetOpt(const char* key, const char* val)
{
    unique_lock<mutex>lock(mux_);
    if (!c_)return false;
    auto re = av_opt_set(c_->priv_data, key, val, 0);
    if (re != 0) {
        cerr << "av_opt_set failed" << endl;
        PrintErr(re);
        return false;
    }
    return true;
}

bool XCodec::SetOpt(const char* key, int val)
{
    unique_lock<mutex>lock(mux_);
    if (!c_)return false;
    auto re = av_opt_set_int(c_->priv_data, key, val, 0);
    if (re != 0) {
        cerr << "av_opt_set failed" << endl;
        PrintErr(re);
        return false;
    }
    return true;
}

bool XCodec::Open()
{
    if (!c_)return false;
    auto re = avcodec_open2(c_, NULL, NULL);
    if (re != 0) {
        cerr << "avcodec_open2 failed" << endl;
        PrintErr(re);
        return false;
    }
    return true;
}

AVFrame* XCodec::CreateFrame()
{
    unique_lock<mutex>lock(mux_);
    if (!c_)return nullptr;
    auto frame = av_frame_alloc();
    frame->width = c_->width;
    frame->height = c_->height;
    frame->format = c_->pix_fmt;
    int re = av_frame_get_buffer(frame, 0);
    if (re != 0) {
        av_frame_free(&frame);
        PrintErr(re);
    }
    return frame;
}

AVFrame* XCodec::CreateAudioFrame(int freq, AVSampleFormat format, int channels)
{
    unique_lock<mutex>lock(mux_);
    if (!c_)return nullptr;
    AVFrame* frame = av_frame_alloc();
    frame->sample_rate = freq;
    frame->format = format;
    frame->channels = channels;
    frame->channel_layout = c_->channel_layout;
    int re = av_frame_get_buffer(frame, 0);
    if (re != 0) {
        av_frame_free(&frame);
        PrintErr(re);
    }
    return frame;
}
