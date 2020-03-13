#include <sys/epoll.h>
#include <unistd.h>
#include <thread>
#include <arpa/inet.h>

#include "NetThread.h"
#include "ClientSocket.h"
#include "Connection.h"
#include "BindAdapter.h"

using namespace tarsx;

#define H64(x) ((static_cast<uint64_t>(x))<<32)

NetThread::NetThread(EpollServer* epoll_server)
	:epollServer_(epoll_server){
	shutdownSock_.createSocket();
	notifySocket_.createSocket();
}

auto NetThread::createEpoll(uint32_t number) -> void {
	auto total = 2000;
	epoller_.create(number);

	epoller_.add(shutdownSock_.fd(), H64(ET_CLOSE), EPOLLIN);
	epoller_.add(notifySocket_.fd(), H64(ET_NOTIFY), EPOLLIN);
	epoller_.add(listenSocket_.fd(), H64(ET_LISTEN) | listenSocket_.fd(), EPOLLIN);

	for(auto&[fd,adapter]:listeners_) {
		epoller_.add(fd, H64(ET_LISTEN) | fd, EPOLLIN);
	}

	for(uint32_t i=1;i<=total;++i) {
		free_.push_back(i);
		++freeSize_;
	}
	LOG_TRACE << "NetThread::createEpoll() successful";
}

auto NetThread::bind(const EndPoint& end_point, Socket& socket) -> void {
	socket.createSocket(SOCK_STREAM, AF_INET);
	socket.bind(end_point.get_host(),end_point.get_port());

	socket.listen(100);
	socket.set_keepAlive();
	socket.set_tcpNoDelay();
	socket.set_noCloseWait();
	socket.set_block(false);
}

auto NetThread::bind(std::shared_ptr<BindAdapter> bind_adapter) -> int {
	const auto& end_point = bind_adapter->get_endpoint();
	auto& socket = bind_adapter->get_socket();
	bind(end_point, socket);

	listeners_[socket.fd()] = bind_adapter;
	return socket.fd();
}

auto NetThread::accept(int listenfd) -> bool {
	sockaddr_in sock_addr;
	socklen_t sock_len = sizeof(sockaddr_in);
	Socket client_socket;
	client_socket.set_owner(false);
	
	Socket receive_socket;
	receive_socket.init(listenfd, false, AF_INET);
	auto ret = receive_socket.accept(client_socket, reinterpret_cast<sockaddr*>(&sock_addr), sock_len);

	if (ret > 0) {
		char addr[INET_ADDRSTRLEN] = "\0";
		auto p = &sock_addr;
		inet_ntop(AF_INET, &p->sin_addr, addr, sizeof addr);

		std::string ip = addr;
		uint16_t port = ::ntohs(p->sin_port);

		printf("accept ip is %s , port is %d \n", ip.data(), port);
		client_socket.set_block(false);
		client_socket.set_keepAlive();
		client_socket.set_tcpNoDelay();
		client_socket.set_closeWaitDefault();

		std::shared_ptr<Connection> connection = std::make_shared<Connection>(listeners_[listenfd], listenfd, 2, client_socket.fd(), ip, port);
		//epollServer_->addConnection(connection, client_socket.fd(), 0);
		addTConnection(connection);
		return true;
	}
	else {
		if (errno == EAGAIN) {
			return false;
		}
		return true;
	}
}

auto NetThread::processNet(const epoll_event& ev) -> void {
	auto uid = ev.data.u32;
	auto connection = connectionUids_[uid];
	if (ev.events & EPOLLERR || ev.events & EPOLLHUP) {
		printf("should delete connection \n");
		delConnection(connection, true, EM_SERVER_CLOSE);
		return;
	}
	if (ev.events & EPOLLIN) {
		std::deque<utagRecvData> recv_deque;
		auto ret = conRecv(connection, recv_deque);
		if (ret < 0) {
			delConnection(connection, true, EM_CLIENT_CLOSE);
		}
		if (!recv_deque.empty()) {
			connection->get_bindAdapter()->insertRecvDeque(recv_deque, true);
		}
	}

	if (ev.events & EPOLLOUT) {
		auto ret = conSend(connection);
		if (ret < 0) {
			printf("need delConnection \n");
		}
	}
}

auto NetThread::processPipe() -> void {
	std::deque<utagSendData> send_deque;
	sendBuffer_.swap(send_deque);

	for(auto& send_msg:send_deque) {
		switch(send_msg->cmd) {
		case 's': {
			uint32_t uid = send_msg->uid;
			auto connection = connectionUids_[uid];
			auto ret = conSend(connection, send_msg->buffer, send_msg->ip, send_msg->port);
			if (ret < 0) {
				printf("need to delete Connection \n");
			}
			break;
		}
		case 'c': {
			auto connection = connectionUids_[send_msg->uid];
			if (connection) {
				if (connection->set_close()) {
					delConnection(connection, true, EM_SERVER_CLOSE);
				}
			}
			break;
		}
		default: {
			assert(false);
		}
		}
	}
}

auto NetThread::conRecv(std::shared_ptr<Connection>& connection, std::deque<utagRecvData>& deque) -> int {
	return connection->recv(deque);
}

auto NetThread::conSend(std::shared_ptr<Connection>& connection,
						   const std::string& buffer, const std::string& ip,
						   uint16_t port) -> int {
	return connection->send(buffer,ip,port);
}

auto NetThread::conSend(std::shared_ptr<Connection>& connection) -> int {
	return connection->send();
}

auto NetThread::recvNeedToSendFromServer(uint32_t uid, const std::string& msg,
									     const std::string& ip,uint16_t port) -> void {
	if(terminate_) {
		return;
	}
	std::unique_ptr<tagSendData> send(new tagSendData{ 's',uid,msg,ip,port });
	sendBuffer_.push_back(send);

	epoller_.mod(notifySocket_.fd(), H64(ET_NOTIFY), EPOLLOUT);
}

auto NetThread::addTConnection(std::shared_ptr<Connection> connection) -> void {
	uint32_t uid = free_.front();
	connection->init(uid);
	free_.pop_front();
	--freeSize_;
	connectionUids_[uid] = connection;
	// EPOLLIN和EPOLLOUT最好不要同时监听
	// 除非确实需要写入文件
	epoller_.add(connection->fd(), connection->get_uid(), EPOLLIN);
}

auto NetThread::delConnection(std::shared_ptr<Connection> connection, bool eraseList, EM_CLOSE_T closeType) -> void {
	if (connection->get_listenfd() != -1) {
		uint32_t uid = connection->get_uid();

		std::unique_ptr<tagRecvData> recv_msg(
			new tagRecvData{
				uid,"",
				connection->get_ip(),connection->get_port(),0,
				false,true,
				connection->fd(),nullptr,closeType });

		std::deque<utagRecvData> recv_deque;
		recv_deque.push_back(std::move(recv_msg));
		connection->get_bindAdapter()->insertRecvDeque(recv_deque);

		epoller_.del(connection->fd(), uid, 0);
		connection->close();
		connectionUids_.erase(uid);
	}
}

auto NetThread::start() -> void {
	LOG_INFO << "NetThread::start()";
	std::thread epoll_loop(&NetThread::run,this);
	epoll_loop.detach();
}

auto NetThread::run() -> void {
	while(!terminate_) {
		auto ev_num = epoller_.wait(2000);
		for (auto i = 0; i < ev_num; ++i) {
			const auto& ev = epoller_.get(i);
			uint32_t h = ev.data.u64 >> 32;
			switch (h)
			{
			case ET_LISTEN:
				printf("ET_LISTEN \n");
				if (ev.events & EPOLLIN) {
					bool ret;
					do {
						// ev.data.u32就是listenfd
						// createEpoll()中第三个add
						ret = accept(ev.data.u32);
					} while (ret);
				}
				break;
			case ET_CLOSE:
				printf("ET_CLOSE \n");
				break;
			case ET_NOTIFY:
				printf("ET_NOTIFY \n");
				processPipe();
				break;
			case ET_NET:
				printf("ET_NET \n");
				processNet(ev);
				break;
			//case ET_TIME:
			//	printf("ET_TIME \n");
			//	timerQueue_->handleRead();
			//	break;
			default:
				assert(true);
			}
		}
	}
}

auto NetThread::terminate() -> void {
	terminate_ = true;

	sendBuffer_.notifyT();
	epoller_.mod(shutdownSock_.fd(), H64(ET_CLOSE), EPOLLOUT);

	for(auto& [fd,adapter]:listeners_) {
		epoller_.mod(fd, H64(ET_CLOSE), EPOLLOUT);
	}
}

auto NetThread::close(uint32_t uid) -> void {
	std::unique_ptr<tagSendData> msg(new tagSendData{ 'c',uid });
	sendBuffer_.push_back(msg);
	epoller_.mod(notifySocket_.fd(), H64(ET_NOTIFY), EPOLLOUT);
}