#pragma once
#include "xformat.h"
class XDemux :public XFormat
{
public:
	static AVFormatContext* Open(const char* url);
	bool Read(AVPacket* pkt);
	bool Seek(long long pts, int stream_index);
private:

};

