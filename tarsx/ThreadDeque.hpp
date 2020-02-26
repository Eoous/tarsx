#pragma once
#include <deque>
#include <muduo/base/Logging.h>

#include "Lock.hpp"

namespace tarsx {
	template<typename T,typename D=std::deque<T>>
	class ThreadDeque {
	public:
		ThreadDeque() = default;
		~ThreadDeque() = default;

		auto pop_front(size_t millsecond = 0) -> T;
		auto notifyT() -> void;
		auto push_back(T& element) -> void;
		auto push_back(D& deque) -> void;
		auto push_front(T& element) -> void;
		auto push_front(D& deque) -> void;
		auto swap(D& deque, size_t millsecond = 0) -> bool;
		auto size() const -> size_t {
			Lock lock(monitor_);
			return size_;
		}
		auto clear() -> void;
		auto empty() const -> bool;
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

	template <typename T, typename D>
	auto ThreadDeque<T, D>::notifyT() -> void {
		Lock lock(monitor_);
		monitor_.notifyAll();
	}

	template <typename T, typename D>
	auto ThreadDeque<T, D>::push_back(T& element) -> void {
		Lock lock(monitor_);
		monitor_.notify();
		deque_.emplace_back(std::move(element));
		++size_;
	}

	template <typename T, typename D>
	auto ThreadDeque<T, D>::push_back(D& deque) -> void {
		Lock lock(monitor_);
		for(auto& element:deque) {
			deque_.push_back(std::move(element));
			++size_;
			monitor_.notify();
		}
	}

	template <typename T, typename D>
	auto ThreadDeque<T, D>::push_front(T& element) -> void {
		Lock lock(monitor_);
		monitor_.notify();
		deque_.push_front(std::move(element));
		++size_;
	}

	template <typename T, typename D>
	auto ThreadDeque<T, D>::push_front(D& deque) -> void {
		Lock lock(monitor_);
		for(auto& element : deque) {
			deque_.push_front(std::move(element));
			++size_;
			monitor_.notify();
		}
	}

	template <typename T, typename D>
	auto ThreadDeque<T, D>::swap(D& deque, size_t millsecond) -> bool {
		Lock lock(monitor_);
		if(deque_.empty()) {
			if(millsecond == 0) {
				return false;
			}
			if(millsecond == static_cast<size_t>(-1)) {
				monitor_.wait();
			}
			else {
				LOG_INFO << "ThreadDeque::swap()";
				if(!monitor_.timedWait(millsecond)) {
					return false;
				}
			}
			return false;
		}

		deque_.swap(deque);
		size_ = deque_.size();

		return true;
	}

	template <typename T, typename D>
	auto ThreadDeque<T, D>::clear() -> void {
		Lock lock(monitor_);
		deque_.clear();
		size_ = 0;
	}

	template <typename T, typename D>
	auto ThreadDeque<T, D>::empty() const -> bool {
		Lock lock(monitor_);
		return deque_.empty();
	}
}