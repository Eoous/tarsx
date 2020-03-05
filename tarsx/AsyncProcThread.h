#pragma once
#include "Message.h"

namespace tarsx {
	class AsyncProcThread {
	public:
		AsyncProcThread(size_t queueCap = 1000);
		virtual ~AsyncProcThread();

		auto terminate() -> void;
		auto push_back(ReqMessage* msg) -> void;
		auto run() -> void;
		auto start() -> void;
		auto size() -> size_t {
			return msgQueue_->size();
		}
	public:
		ThreadLock monitor_;
		std::unique_ptr<ReqInfoQueue> msgQueue_;
		bool exit = false;
		bool terminate_ = false;
	};
}