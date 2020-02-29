#pragma once
#include "ClientSocket.h"
#include "Message.h"
#include "Socket.h"
#include "ThreadDeque.hpp"

namespace tarsx {
	class EpollServer;
	class BindAdapter {
	public:
		BindAdapter() = default;
		//BindAdapter(EpollServer* epoll_server);
		~BindAdapter();

		auto set_name(const std::string& name) { Lock lock(monitor_); name_ = name; }
		auto get_name() const { return name_; }
		auto set_endpoint(const std::string& host, int port) {
			Lock lock(monitor_);
			endpoint_.set_host(host);
			endpoint_.set_port(port);
		}
		auto& get_endpoint()const { return endpoint_; }
		auto& get_socket() { return socket_; }
		
		auto get_handleGroupName() const { return handleGroupName_; }
		auto set_handleGroupName(const std::string& handleGroupName) { handleGroupName_ = handleGroupName; }

		auto set_heartBeatTime(time_t t) { Lock lock(monitor_); heartBeatTime_ = t; }
		auto get_heartBeatTime() const { return heartBeatTime_; }

		auto insertRecvDeque(std::deque<utagRecvData>& deque, bool pushback = true) -> void;
		auto waitForRecvQueue(uint32_t wait_time) -> utagRecvData;

		auto set_handleGroup(std::shared_ptr<HandleGroup>& handle_group) {
			handleGroup_ = handle_group;
		}
	private:
		EpollServer* epollServer_;
		Socket socket_;
		EndPoint endpoint_;
		std::string name_;
		ThreadDeque<utagRecvData> receiveBuffer_;

		std::weak_ptr<HandleGroup> handleGroup_;
		std::string handleGroupName_;
		std::size_t handleNum_ = 0;

		volatile time_t heartBeatTime_ = 0;
		ThreadLock monitor_;
	};
}
