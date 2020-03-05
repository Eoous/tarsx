#pragma once
#include <map>
#include <list>
#include <unordered_map>
#include <cassert>
#include <functional>
#include <sys/time.h>

#include "Lock.hpp"

namespace tarsx {
	template<typename T>
	class TimeoutQueueNew {
	public:
		struct PtrInfo;
		struct NodeInfo;
		struct SendInfo;

		using DataType = std::unordered_map<uint32_t, PtrInfo>;
		using TimeType = std::multimap<int64_t, NodeInfo>;
		using SendType = std::list<SendInfo>;

		using DataFunction = std::function<void(T&)>;

		struct PtrInfo {
			T ptr;
			bool hasSend;
			typename TimeType::iterator timeIter;
			typename SendType::iterator sendIter;
		};

		struct NodeInfo {
			typename DataType::iterator dataIter;
		};

		struct SendInfo {
			typename DataType::iterator dataIter;
		};

		TimeoutQueueNew(int timeout=5*1000,size_t size=100) {
			data_.reserve(size);
		}

		auto generateId() -> uint32_t;

		auto sendListEmpty() -> bool {
			return send_.empty();
		}

		auto getSend(T& t) -> bool;
		auto popSend(bool del = false) -> void;

		auto getSendListSize() -> size_t {
			return send_.size();
		}

		auto get(uint32_t uniqId, T& t, bool erase = true) -> bool;
		auto erase(uint32_t uniqId, T& t) -> bool;
		auto push(T& ptr, uint32_t uniqId, int64_t timeout, bool hasSend = true) -> bool;

		auto timeout() -> void;
		auto timeout(T& t) -> bool;
		auto timeout(const DataFunction& func) -> void;
		
		auto size() const -> size_t {
			return data_.size();
		}
	protected:
		uint32_t uniqueId_;
		std::unordered_map<uint32_t, PtrInfo> data_;
		std::multimap<int64_t, NodeInfo> time_;
		std::list<SendInfo> send_;
	};

	template <typename T>
	auto TimeoutQueueNew<T>::getSend(T& t) -> bool {
		if(send_.empty()) {
			return false;
		}
		SendInfo& sendinfo = send_.back();
		assert(!sendinfo.dataIter->second.ptr);
		return true;
	}

	template <typename T>
	auto TimeoutQueueNew<T>::popSend(bool del) -> void {
		assert(!send_.empty());
		SendInfo sendinfo;
		sendinfo = send_.back();
		sendinfo.dataIter->second.hasSend = true;
		if(del) {
			time_.erase(sendinfo.dataIter->second.timeIter);
			data_.erase(sendinfo.dataIter);
		}
		send_.pop_back();
	}

	template <typename T>
	auto TimeoutQueueNew<T>::get(uint32_t uniqId, T& t, bool erase) -> bool {
		auto it = data_.find(uniqId);
		if(it==data_.end()) {
			return false;
		}
		t = it->second.ptr;
		if(erase) {
			time_.erase(it->second.timeIter);
			if(!it->second.hasSend) {
				send_.erase(it->second.sendIter);
			}
			data_.erase(it);
		}
		return true;
	}

	template <typename T>
	auto TimeoutQueueNew<T>::generateId() -> uint32_t {
		Lock lock(*this);
		while (++uniqueId_ == 0);
		return uniqueId_;
	}

	template <typename T>
	auto TimeoutQueueNew<T>::push(T& ptr, uint32_t uniqId, int64_t timeout, bool hasSend) -> bool {
		PtrInfo pi{ ptr,hasSend };

		std::pair<typename DataType::iterator, bool> result;
		result = data_.insert(std::make_pair(uniqId, pi));
		if (result.second == false)return false;

		NodeInfo nodeinfo{ result.first };
		result.first->second.timeIter = time_.insert(std::make_pair(timeout, nodeinfo));

		if(!hasSend) {
			SendInfo sendinfo;
			sendinfo.dataIter = result.first;
			send_.push_front(sendinfo);
			result.first->second.sendIter = send_.begin();
		}

		return true;
	}

	template <typename T>
	auto TimeoutQueueNew<T>::timeout() -> void {
		timeval tv;
		::gettimeofday(&tv, NULL);

		auto now = tv.tv_sec * static_cast<int64_t>(1000) + tv.tv_usec / 1000;
		while (true) {
			auto it = time_.begin();
			if (it == time_.end() || it->first > now) {
				break;
			}

			if(!it->second.dataIter->second.hasSend) {
				send_.erase(it->second.dataIter->second.sendIter);
			}
			data_.erase(it->second.dataIter);
			time_.erase(it);
		}
	}

	template <typename T>
	auto TimeoutQueueNew<T>::timeout(T& t) -> bool {
		timeval tv;
		::gettimeofday(&tv, nullptr);

		auto now = tv.tv_sec * static_cast<int64_t>(1000) + tv.tv_usec / 1000;

		if(time_.empty()) {
			return false;
		}
		
		auto it = time_.begin();
		if(it->first > now) {
			return false;
		}
		t = it->second.dataIter->second.ptr;
		if(!it->second.dataIter->second.hasSend) {
			send_.erase(it->second.dataIter->second.sendIter);
		}
		
		data_.erase(it->second.dataIter);
		time_.erase(it);
		return true;
	}

	template <typename T>
	auto TimeoutQueueNew<T>::timeout(const DataFunction& func) -> void {
		while(true) {
			timeval tv;
			::gettimeofday(&tv, nullptr);

			auto now = tv.tv_sec * static_cast<int64_t>(1000) + tv.tv_usec / 1000;

			T ptr;
			auto it = time_.begin();
			if(it == time_.end() || it->first > now) {
				break;
			}
			ptr = it->second.dataIter->second.ptr;
			if(!it->second.dataIter->second.hasSend) {
				send_.erase(it->second.dataIter->second.sendIter);
			}
			data_.erase(it->second.dataIter);
			time_.erase(it);

			try {
				func(ptr);
			}
			catch (...) {
				
			}
		}
	}

	template <typename T>
	auto TimeoutQueueNew<T>::erase(uint32_t uniqId, T& t) -> bool {
		auto it = data_.find(uniqId);

		if(it == data_.end()) {
			return false;
		}

		t = it->second.ptr;
		if(!it->second.hasSend) {
			send_.erase(it->second.sendIter);
		}

		time_.erase(it->second.timeIter);
		data_.erase(it);

		return true;
	}
}
