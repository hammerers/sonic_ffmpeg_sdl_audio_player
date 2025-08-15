#include "xformat.h"

bool XFormat::CopyPara(int stream_index, AVCodecParameters* dst)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
	if (stream_index < 0 || stream_index >= c_->nb_streams) { return false; }

	auto re = avcodec_parameters_copy(dst,c_->streams[stream_index]->codecpar);
	if (re < 0) {
		return false;
	}

	return true;
}

bool XFormat::CopyPara(int stream_index, AVCodecContext*dst)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
	if (stream_index < 0 || stream_index >= c_->nb_streams) { return false; }

	auto re = avcodec_parameters_to_context(dst, c_->streams[stream_index]->codecpar);
	if (re < 0) {
		return false;
	}

	return true;
}

void XFormat::set_c(AVFormatContext* c)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (c_) {
		if (c_->oformat) {
			if (c_->pb) {
				avio_closep(&c_->pb);
			}
			avformat_free_context(c_);
		}
		else if (c_->iformat) {
			avformat_close_input(&c_);
		}
		else {
			avformat_free_context(c_);
		}
	}
	c_ = nullptr;
	c_ = c;
	if (!c_)return;
	for (int i = 0; i < c_->nb_streams; i++) {
		if (c_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_codec_id_ = c_->streams[i]->codecpar->codec_id;
			audio_index_ = i;
			audio_time_base_.num = c_->streams[i]->time_base.num;
			audio_time_base_.den = c_->streams[i]->time_base.den;
		}
		else if (c_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			video_codec_id_ = c_->streams[i]->codecpar->codec_id;
			video_index_ = i;
			video_time_base_.num = c_->streams[i]->time_base.num;
			video_time_base_.den = c_->streams[i]->time_base.den;
		}
	}
}

bool XFormat::RescaleTime(AVPacket* pkt, long long offset_pts, XRational time_base)
{
	std::unique_lock<std::mutex>lock(mux_);
	if (!c_)return false;
	auto out_stream = c_->streams[pkt->stream_index];
	AVRational in_time_base;
	in_time_base.den = time_base.den;
	in_time_base.num = time_base.num;
	pkt->pts = av_rescale_q_rnd(pkt->pts-offset_pts,in_time_base,out_stream->time_base,(AVRounding)(AV_ROUND_NEAR_INF| AV_ROUND_PASS_MINMAX));
	pkt->dts = av_rescale_q_rnd(pkt->dts-offset_pts , in_time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
	pkt->duration = av_rescale_q(pkt->duration, in_time_base, out_stream->time_base);
	pkt->pos = -1;
	return true;
}

void XFormat::PrintInfo()
{
	std::cout << "audio_index_:" << audio_index_ << std::endl;
	std::cout << "video_index_:" << video_index_ << std::endl;
	std::cout << "video_codec_id_:" << video_codec_id_ << std::endl;
	std::cout << "audio_codec_id_:" << audio_codec_id_ << std::endl;
}
