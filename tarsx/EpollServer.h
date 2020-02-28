#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "Message.h"
namespace tarsx {
	class NetThread;
	class Connection;
	class BindAdapter;
	class EpollServer {
	public:
		EpollServer(uint32_t net_thread_num);
		~EpollServer();

		auto get_netThreadNum() { return netThreadNum_; }
		auto& get_netThreads() { return netThreads_; }
		
		auto send(uint32_t uid, const std::string& msg, const std::string& ip, uint16_t port, int fd) -> void;
		auto bind(std::shared_ptr<BindAdapter> bindadapter) -> int;
		auto addConnection(std::shared_ptr<Connection> connection, int fd, int type) -> void;

		auto close(unsigned int uid, int fd) -> void;
		auto& getNetThreadByfd(int fd) {
			return netThreads_[fd % netThreads_.size()];
		}

		auto startHandle() -> void;
		auto createEpoll() -> void;
		auto isTerminate() const -> bool { return terminate_; }
		auto terminate() -> void;
		auto stopThread() -> void;

		auto set_handleGroup(const std::string& groupName, int32_t handleNum, std::shared_ptr<BindAdapter>& adapter) -> void;
	private:
		std::vector<std::unique_ptr<NetThread>> netThreads_;
		bool terminate_ = false;
		uint32_t netThreadNum_;
		bool handleStarted_;
		std::map<std::string, std::shared_ptr<HandleGroup>> handleGroups_;
	};
}
