#include <cassert>
#include <vector>
#include <thread>
#include <sys/time.h>
#include <muduo/base/Logging.h>

#include "ServantHandle.h"
#include "CoroutineScheduler.h"
#include "BindAdapter.h"
#include "EpollServer.h"
#include "ServantManager.h"
#include "TarsNodeF.h"

using namespace tarsx;

size_t coroutineMemSize = 1073741824;
uint32_t coroutineStackSize = 1310272;

auto ServantHandle::init() -> void {
	auto handle_group = handleGroup_.lock();
	assert(handle_group);
	
	for(auto&[name,adapter]:handle_group->adapters) {
		auto servant_func= ServantHelperManager::instance().get_funcByName(name);
		servants_[name] = servant_func;
	}
	
}

auto ServantHandle::start() -> void {
	LOG_INFO << "ServantHandle::start()";
	std::thread running([this]() {
		run();
		});
	running.detach();
}

auto ServantHandle::run() -> void {
	init();

	LOG_INFO << "open coroutine";
	auto coroutineNum = (coroutineMemSize > coroutineStackSize) ? (coroutineMemSize / (coroutineStackSize * 4)) : 1;

	if(coroutineNum < 1) {
		coroutineNum = 1;
	}
	corouScheduler_ = new CoroutineScheduler;
	corouScheduler_->init(coroutineNum, coroutineStackSize);
	corouScheduler_->set_handle(this);
	auto ret = corouScheduler_->createCoroutine(std::bind(&ServantHandle::handleRequest, this));

	while(!epollServer_->isTerminate()) {
		corouScheduler_->tars_run();
	}
	corouScheduler_->terminate();
	corouScheduler_->destroy();
	stopHandle();
}

auto ServantHandle::handleRequest() -> void {
	auto yield = false;
	//while (!getEpollServer()->isTerminate()) {
	while(1){
		auto serverReqEmpty = false;
		auto handle_group = handleGroup_.lock();
		{
			Lock lock(handle_group->monitor);
			{
				if (corouScheduler_->get_responseCoroSize() > 0) {
					serverReqEmpty = true;
				}
				else {
					handle_group->monitor.timedWait(3);
				}
			}
		}
		heartbeat();
		// 如果有正在运行的协程，进入if
		if (serverReqEmpty) {
			corouScheduler_->yield();
			continue;
		}
		yield = false;
		auto& adapters = handle_group->adapters;
		for (auto& [name,adapter] : adapters) {
			auto flag = true;
			auto loop = 100;

			while (flag && loop > 0) {
				--loop;
				auto recv = adapter->waitForRecvQueue(0);
				if (recv) {
					yield = true;
					recv->adapter = adapter;
					if (recv->closed) {
						LOG_TRACE <<"give into to real business to close";
						recv.reset();
					}
					else {
						tagRecvData* temp=recv.release();
						auto ret = corouScheduler_->createCoroutine([temp, this]() {
							LOG_TRACE << "enter ServantHandle::handleRecvData";
							utagRecvData recv(temp);
							handle(recv);
							});

						// handle(recv);
					}
				}
				else {
					flag = false;
					yield = false;
				}
				if(recv) {
					LOG_INFO << "recv delete";
				}
			}
			if (loop == 0) {
				yield = false;
			}
		}
		if (!yield) {
			corouScheduler_->yield();
		}
	}
}

auto ServantHandle::handle(utagRecvData& recv_data) -> void {
	/* first->name second->dispatchFunc */
	auto func = servants_.find(recv_data->adapter->get_name());
	assert(func != servants_.end());
	std::vector<char> buffer;

	auto funcc = func->second;
	auto recv_data_d = recv_data->buffer;
	funcc(recv_data_d, buffer);
	std::string response;
	response.resize(buffer.size());
	response.assign(buffer.begin(), buffer.end());

	sendToEpollServer(recv_data->uid, response, recv_data->ip, recv_data->port, recv_data->fd);
}

auto ServantHandle::heartbeat() -> void {
	timeval tv;
	::gettimeofday(&tv, nullptr);
	time_t cur = tv.tv_sec;
	auto handle_group = handleGroup_.lock();
	auto& adapters = handle_group->adapters;
	for (auto& [name, adapter] : adapters) {
		if (abs(cur - adapter->get_heartBeatTime()) > HEART_BEAT_INTERVAL) {
			adapter->set_heartBeatTime(cur);
			TarsNodeFHelper::instance().keepAlive(adapter->get_name());
		}
	}
}

auto ServantHandle::sendToEpollServer(uint32_t uid, const std::string& msg, 
									  const std::string& ip, int port,int fd) -> void {
	epollServer_->send(uid, msg, ip, port, fd);
}

auto ServantHandle::set_epollServer(EpollServer* epoll_server) -> void {
	Lock lock(monitor_);
	epollServer_ = epoll_server;
}

auto ServantHandle::set_handleGroup(std::shared_ptr<HandleGroup>& handle_group) -> void {
	Lock lock(monitor_);
	handleGroup_ = handle_group;
}
