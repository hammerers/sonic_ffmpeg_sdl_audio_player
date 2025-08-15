#pragma once
#include<mutex>
#include<queue>
#include <condition_variable>
template<typename T>
class AVQueue
{
public:
	void SetMaxSize(int size) {
		std::unique_lock<std::mutex>lock(mux_);
		max_size_ = size;
	}
	void Push(const T& item) {
		std::unique_lock<std::mutex>lock(mux_);
		push_cond_.wait(lock, [this]() {return queue_.size() != max_size_||stop_; });
		queue_.push(std::move(item));
		pop_cond_.notify_one();
	}
	bool Pop(T& item) {
		std::unique_lock<std::mutex> lock(mux_);
		pop_cond_.wait(lock, [this] { return !queue_.empty() || stop_; });
		if (stop_ && queue_.empty()) return false;
		item = std::move(queue_.front());
		queue_.pop();
		push_cond_.notify_one();
		return true;
	}
	void Start() {
		std::unique_lock<std::mutex>lock(mux_);
		stop_ = false;
		push_cond_.notify_all();
		pop_cond_.notify_all();
	}
	void Stop() {
		std::unique_lock<std::mutex>lock(mux_);
		stop_ = true;
		push_cond_.notify_all();
		pop_cond_.notify_all();
	}
	bool Empty() {
		
		return queue_.empty();
	}
	void Clear() {
		std::unique_lock<std::mutex>lock(mux_);
		while (!Empty()) {
			auto item = queue_.front();
			queue_.pop();
			AVClean(item);
		}
		stop_ = false;
	}
	int Size() {
		std::unique_lock<std::mutex>lock(mux_);
		return queue_.size();
	}

private:
	void AVClean(AVFrame* item) {
		if (item) {
			av_frame_unref(item);
			av_frame_free(&item);
		}
	}
	void AVClean(AVPacket* item) {
		if (item) {
			av_packet_unref(item);
			av_packet_free(&item);
		}
	}
	std::mutex mux_;
	std::condition_variable push_cond_;
	std::condition_variable pop_cond_;
	std::queue<T>queue_;
	int max_size_ = 1000;
	bool stop_ = false;
};


