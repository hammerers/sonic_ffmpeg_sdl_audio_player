#include "xdecode.h"
#include<iostream>
using namespace std;
bool XDecode::Send(const AVPacket* pkt)
{
	unique_lock<mutex>lock(mux_);
	if (!c_)return false;
	auto re = avcodec_send_packet(c_,pkt);
	if (re != 0)return false;
	return true;
}
bool XDecode::Recv(AVFrame* frame)
{
	unique_lock<mutex>lock(mux_);
	if (!c_)return false;
    auto f = frame;
    if (c_->hw_device_ctx) {
        f = av_frame_alloc();
    } 
	auto re = avcodec_receive_frame(c_, f);

	if (re == 0) {
        if (c_->hw_device_ctx) {
            re=av_hwframe_transfer_data(frame,f, 0); 
            av_frame_free(&f);
            if (re != 0) {
                PrintErr(re);
                return false;
            }
            
        }
        return true;
	}
    if (c_->hw_device_ctx) {
        av_frame_free(&f);
    }
        return false;
}

std::vector<AVFrame*> XDecode::End()
{
    std::vector<AVFrame*>res;
    if (!c_)return res;
    auto re = avcodec_send_packet(c_, nullptr);
    if (re != 0)return res;
    while (re >= 0) {
        auto frame = av_frame_alloc();
        re = avcodec_receive_frame(c_, frame);
        if (re != 0) {
            av_frame_free(&frame);
            break;
        }
        res.push_back(frame);
    }
    return res;
}

bool XDecode::InitHW(int type=4)
{
    unique_lock<mutex>lock(mux_);
    if (!c_)return false; 
    AVBufferRef* hw_ctx = nullptr;
    auto re=av_hwdevice_ctx_create(&hw_ctx, (AVHWDeviceType)type, NULL, NULL, 0);
    if (re != 0) {
        PrintErr(re);
        return false;
    }
    c_->hw_device_ctx = hw_ctx;
    return true;
}
