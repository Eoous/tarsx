#pragma once
#include <map>
#include <forward_list>
#include "Epoller.h"
#include "Socket.h"
namespace tarsx {
	class Connection;
	class EpollServer;
	class NetThread {
	public:
		NetThread() = default;
		//NetThread(EpollServer* epoll_server);
		~NetThread() = default;

	private:
		//EpollServer* epollServer_;
		Epoller epoller_;
		Socket shutdownSock_;
		Socket notifySocket_;
		Socket listenSocket_;

		std::map<int, std::__shrink_to_fit_aux<Connection>> connectionUids_;
		std::forward_list<uint32_t> free_;
		volatile size_t freeSize_;
	};
}