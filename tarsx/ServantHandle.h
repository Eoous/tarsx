#pragma once
#include <functional>
#include <vector>

#include "Message.h"

namespace tarsx {
	class EpollServer;
	class CoroutineScheduler;
	using dispatcFunc = std::function<int(const std::string&, std::vector<char>&)>;
	class ServantHandle {
		enum {
			HEART_BEAT_INTERVAL=10
		};
	public:
		ServantHandle() = default;
		~ServantHandle() = default;

		auto start() -> void;
		auto run() -> void;
		auto handle(utagRecvData& recv_data) -> void;

		auto set_epollServer(EpollServer* epoll_server) -> void;
		auto set_handleGroup(std::shared_ptr<HandleGroup>& handle_group) -> void;
	private:
		auto init() -> void;
		auto handleRequest() -> void;
		auto stopHandle() {
			
		}
		auto heartbeat() -> void;
		auto sendToEpollServer(uint32_t uid,const std::string& msg,const std::string& ip,int port,int fd) -> void;
	private:
		EpollServer* epollServer_;
		std::map<std::string, dispatcFunc> servants_;
		std::weak_ptr<HandleGroup> handleGroup_;
		CoroutineScheduler* corouScheduler_;
		ThreadLock monitor_;
	};
	
}