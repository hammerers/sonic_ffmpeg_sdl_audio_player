#pragma once
#include"xcodec.h"
class XDecode:public XCodec
{
public:
	bool Send(const AVPacket* pkt);
	bool Recv(AVFrame* frame);
	std::vector<AVFrame*>End();
	bool InitHW(int type);

};

