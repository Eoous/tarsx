#include <sys/epoll.h>

#include "ObjectProxy.h"
#include "Communicator.h"
#include "CommunicatorEpoll.h"

#include <thread>

#include "AsyncProcThread.h"
#include "ObjectProxyFactory.h"
#include "ServantProxy.h"

using namespace tarsx;

CommunicatorEpoll::CommunicatorEpoll(size_t netThreadSeq)
	:netThreadSeq_(netThreadSeq),
	objectProxyFactory_(new ObjectProxyFactory(this)) {

	epoller_.create(1024);
	shutdown_.createSocket();
	toserver_.createSocket();
	epoller_.add(shutdown_.fd(), 0, EPOLLIN);
	epoller_.add(toserver_.fd(), 0, EPOLLIN);
	
	asyncThreadNum_ = 1;
	for (size_t i = 0; i < asyncThreadNum_; ++i) {
		asyncThread_.emplace_back(std::make_unique<AsyncProcThread>(100));
		asyncThread_[i]->start();
	}

	for (auto& notify : notify_) {
		notify.valid = false;
	}
}

CommunicatorEpoll::CommunicatorEpoll(Communicator* communicator, size_t netThreadSeq)
	:communicator_(communicator), netThreadSeq_(netThreadSeq),
	objectProxyFactory_(new ObjectProxyFactory(this)) {

	epoller_.create(1024);
	shutdown_.createSocket();
	epoller_.add(shutdown_.fd(), 0, EPOLLIN);

	if (asyncThreadNum_ == 0) {
		asyncThreadNum_ = 3;
	}
	asyncThreadNum_ = 1;
	for (size_t i = 0; i < asyncThreadNum_; ++i) {
		asyncThread_.emplace_back(std::make_unique<AsyncProcThread>(100));
		asyncThread_[i]->start();
	}
	for (auto& notify : notify_) {
		notify.valid = false;
	}
}

auto CommunicatorEpoll::get_objectProxy(const std::string& ip, const uint16_t& port) -> std::shared_ptr<ObjectProxy> {
	return objectProxyFactory_->get_objectProxy(ip, port);
}


auto CommunicatorEpoll::run() -> void {
	auto sptd = ServantProxyThreadData::get_data();
	assert(sptd != nullptr);
	sptd->netThreadSeq_ = static_cast<int>(netThreadSeq_);

	while (!terminate_) {
		try {
			auto num = epoller_.wait(100);
			for (auto i = 0; i < num; ++i) {
				const auto& ev = epoller_.get(i);
				auto data = ev.data.u64;

				if (data == 0) {
					continue; // data非指针
				}
				handle(reinterpret_cast<FDInfo*>(data), ev.events);
			}
		}
		catch (std::exception & e) {

		}
		catch (...) {

		}
	}
}

auto CommunicatorEpoll::terminate() -> void {
	terminate_ = true;
	//delete objectProxyFactory_.release();
	for (auto& it : asyncThread_) {
		it->terminate();
	}
	epoller_.mod(shutdown_.fd(), 0, EPOLLOUT);
}

auto CommunicatorEpoll::start() -> void {
	std::thread running([this]() {
		run();
		LOG_INFO << "CommunicatorEpoll::start() 退出";
		});
	running.detach();
}

auto CommunicatorEpoll::add(int fd, FDInfo* info, uint32_t events) -> void {
	epoller_.add(fd, reinterpret_cast<long long>(info), events);
}

auto CommunicatorEpoll::del(int fd, FDInfo* info, uint32_t events) -> void {
	epoller_.del(fd, reinterpret_cast<long long>(info), events);
}

// notify函数用来把ReqInfoQueue的地址保存在p
// handle的时候直接取出来就行了
auto CommunicatorEpoll::notify(size_t seq, ReqInfoQueue& reqQueue) -> void {
	if (notify_[seq].valid) {
		notify_[seq].fd_info.p = (void*)&reqQueue;
		epoller_.mod(notify_[seq].notify.fd(), (long long)&notify_[seq].fd_info, EPOLLIN);
		assert(notify_[seq].fd_info.p == (void*)&reqQueue);
	}
	else {
		notify_[seq].fd_info.type = FDInfo::ET_C_NOTIFY;
		notify_[seq].fd_info.p = (void*)&reqQueue;
		notify_[seq].fd_info.fd = notify_[seq].eventfd;
		notify_[seq].fd_info.seq = seq;
		notify_[seq].notify.createSocket();
		notify_[seq].valid = true;

		epoller_.add(notify_[seq].notify.fd(), (long long)&notify_[seq].fd_info, EPOLLIN);
	}
}

auto CommunicatorEpoll::notifyDel(size_t seq) -> void {
	if (notify_[seq].valid &&
		notify_[seq].fd_info.p != nullptr) {
		epoller_.mod(notify_[seq].notify.fd(), reinterpret_cast<int64_t>(&notify_[seq].fd_info), EPOLLIN);
	}
}

auto CommunicatorEpoll::handle(FDInfo* info, uint32_t events) -> void {
	try {
		assert(info != nullptr);

		// notify()中赋值的
		// 其他都是NET
		if (FDInfo::ET_C_NOTIFY == info->type) {
			auto infoQueue = reinterpret_cast<ReqInfoQueue*>(info->p);
			ReqMessage* msg = nullptr;

			while (infoQueue->pop_front(msg)) {
				if (ReqMessage::THREAD_EXIT == msg->type) {
					assert(infoQueue->empty());

					delete msg;
					msg = nullptr;

					epoller_.del(notify_[info->seq].notify.fd(), (long long) & (notify_[info->seq].fd_info), EPOLLIN);
					delete infoQueue;
					infoQueue = nullptr;

					notify_[info->seq].fd_info.p = nullptr;
					notify_[info->seq].notify.close();
					notify_[info->seq].valid = false;

					return;
				}
				msg->objectProxy->sendReqMessage(msg);
			}
		}
		else {
			auto* transceiver = reinterpret_cast<Transceiver*>(info->p);
			if (events & EPOLLIN) {
				handleInputImp(transceiver);
			}

			if (events & EPOLLOUT) {
				handleOutputImp(transceiver);
			}

			// 连接出错 直接关闭连接
			if (events & EPOLLERR) {

			}
		}
	}
	catch (std::exception & e) {

	}
}

auto CommunicatorEpoll::handleInputImp(Transceiver* transceiver) -> void {
	std::list<std::string> done;
	if (transceiver->doResponse(done) > 0) {
		for (auto& it : done) {
			std::string response = it;
			transceiver->objectProxy()->finishInvoke(response);
		}
	}
}

auto CommunicatorEpoll::handleOutputImp(Transceiver* transceiver) -> void {
	transceiver->doRequest();
}

auto CommunicatorEpoll::registe2Center() -> void {
	
}

auto CommunicatorEpoll::pushAsyncThreadQueue(ReqMessage* msg) -> void {
	// 先不考虑每个线程队列数目不一样的情况
	asyncThread_[asyncSeq_]->push_back(msg);
	++asyncSeq_;

	if (asyncSeq_ == asyncThreadNum_) {
		asyncSeq_ = 0;
	}
}