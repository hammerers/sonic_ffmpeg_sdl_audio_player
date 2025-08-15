#include "xdemux.h"
#define BERR(re)if(re!=0){PrintErr(re);return 0;}
static void PrintErr(int re) {
    char buf[1024] = { 0 };
    av_strerror(re, buf, sizeof(buf) - 1);
    std::cerr << buf << std::endl;
}
AVFormatContext* XDemux::Open(const char* url)
{
    AVFormatContext* c = nullptr;
    auto re=avformat_open_input(&c, url, NULL, NULL);
    
    BERR(re);
    re = avformat_find_stream_info(c, nullptr);
    BERR(re);
    av_dump_format(c, 0, url, 0);
    return c;
}

bool XDemux::Read(AVPacket* pkt)
{
    std::unique_lock<std::mutex>lock(mux_);
    if (!c_)return false;
    auto re=av_read_frame(c_, pkt);
    BERR(re)

    return true;
}

bool XDemux::Seek(long long pts, int stream_index)
{
    std::unique_lock<std::mutex>lock(mux_);
    if (!c_)return false;
    int re=av_seek_frame(c_, stream_index,pts , AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
    if (re <0) {
        return false;
    }
    return true;
}
