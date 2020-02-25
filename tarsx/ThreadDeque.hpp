#pragma once
#include <deque>
#include <tuple>
#include <muduo/base/Logging.h>

#include "Lock.hpp"

namespace tarsx {
	template<typename T,typename D=std::deque<T>>
	class ThreadDeque {
	public:
		ThreadDeque() = default;
		~ThreadDeque() = default;

		auto pop_front(size_t millsecond = 0) -> T;
		auto test() {
			deque_.push_back(std::make_unique<typename T::element_type>(5));
			++size_;
		}
	private:
		ThreadLock monitor_;
		std::deque<T> deque_;
		size_t size_ = 0;
	};
	template <typename T, typename D>
	auto ThreadDeque<T, D>::pop_front(size_t millsecond) -> T {
		Lock lock(monitor_);
		if(deque_.empty()) {
			if(millsecond == 0) {
				return nullptr;
			}
			if(millsecond == static_cast<size_t>(-1)) {
				LOG_INFO << "pop_front() start waiting";
				monitor_.wait();
				LOG_INFO << "pop_front() over waiting";
			}
			else {
				LOG_INFO << "pop_front() start timedWait(" << millsecond << ")";
				if(!monitor_.timedWait(millsecond)) {
					return nullptr;
				}
			}
			return nullptr;
		}

		auto front = std::move(deque_.front());
		deque_.pop_front();
		assert(size_ > 0);
		--size_;

		return front;
	}

}