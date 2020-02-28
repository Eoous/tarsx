#pragma once
#include <map>
#include <list>
#include <unordered_map>

#include "Message.h"
#include "Epoller.h"
#include "Socket.h"
#include "ThreadDeque.hpp"

struct epoll_event;
namespace tarsx {
	class Connection;
	class EpollServer;
	class EndPoint;
	class NetThread {
	public:
		NetThread(EpollServer* epoll_server);
		~NetThread() = default;

		auto createEpoll(uint32_t number = 0) -> void;
		auto bind(const EndPoint& end_point,Socket& socket) -> void;
		auto bind(std::shared_ptr<BindAdapter> bind_adapter) -> int;
		auto accept(int listenfd) -> bool;
		
		auto processNet(const epoll_event& ev) -> void;
		auto processPipe() -> void;

		auto conRecv(std::shared_ptr<Connection>& connection, std::deque<utagRecvData>& deque) -> int;
		auto conSend(std::shared_ptr<Connection>& connection, const std::string& buffer, const std::string& ip, uint16_t port) -> int;
		auto conSend(std::shared_ptr<Connection>& connection) -> int;

		auto recvNeedToSendFromServer(uint32_t uid, const std::string& msg, const std::string& ip, uint16_t port) -> void;
		auto addTConnection(std::shared_ptr<Connection> connection) -> void;
		auto delConnection(std::shared_ptr<Connection> connection, bool eraseList = true, EM_CLOSE_T closeType = EM_CLIENT_CLOSE) -> void;

		auto start() -> void;
		auto run() -> void;
		auto terminate() -> void;
		auto close(uint32_t uid) -> void;
		
		enum {
			ET_LISTEN = 1,
			ET_CLOSE=2,
			ET_NOTIFY = 3,
			ET_NET = 0
		};
	private:
		EpollServer* epollServer_;
		Epoller epoller_;
		Socket shutdownSock_;
		Socket notifySocket_;
		Socket listenSocket_;

		std::map<int, std::shared_ptr<Connection>> connectionUids_;
		std::list<uint32_t> free_;
		volatile size_t freeSize_;

		ThreadDeque<utagSendData> sendBuffer_;
		std::unordered_map<int, std::shared_ptr<BindAdapter>> listeners_;
		bool terminate_ = false;
		ThreadLock monitor_;
	};
}