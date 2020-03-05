#include <thread>

#include "AsyncProcThread.h"
#include "Lock.hpp"

using namespace tarsx;

AsyncProcThread::AsyncProcThread(size_t queueCap)
	:msgQueue_(new ReqInfoQueue(queueCap)) {

}

AsyncProcThread::~AsyncProcThread() {
	while (!exit);
}

auto AsyncProcThread::terminate() -> void {
	Lock lock(monitor_);
	terminate_ = true;
	monitor_.notifyAll();
}

auto AsyncProcThread::push_back(ReqMessage* msg) -> void {
	auto flag = msgQueue_->push_back(msg);
	{
		Lock lock(monitor_);
		monitor_.notify();
	}

	if (!flag) {
		LOG_DEBUG << "AsyncProcThread::push_back() async_queue full";
		delete msg;
		msg = nullptr;
	}
}
int a = 0;
auto AsyncProcThread::run() -> void {
	while (!terminate_) {
		ReqMessage* msg = nullptr;

		if (msgQueue_->empty()) {
			Lock lock(monitor_);
			monitor_.timedWait(1000);
		}

		if (msgQueue_->pop_front(msg)) {
			msg->callback(msg);
			delete msg;
		}
	}
	exit = true;
}

auto AsyncProcThread::start() -> void {
	std::thread running([this]() {
		run();
		LOG_INFO << "AsyncProcThread::start() 退出";
		});
	running.detach();
}
