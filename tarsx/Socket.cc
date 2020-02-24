#include <cassert>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <strings.h>
#include <muduo/base/Logging.h>

#include "Socket.h"
#include "NetworkUtil.h"

using namespace tarsx;

auto Socket::createSocket(int socketType, int domain) -> void {
	assert(socketType == SOCK_STREAM || socketType == SOCK_DGRAM);
	close();
	domain_ = domain;
	fd_ = NetworkUtil::createSocket(false, domain_ == PF_LOCAL);
}

auto Socket::bind(const std::string& seraddr, int port) -> void {
	assert(domain_ == AF_INET);

	sockaddr_in bindAddr;
	::bzero(&bindAddr, sizeof(bindAddr));
	bindAddr.sin_family = domain_;
	bindAddr.sin_port = htons(port);
	if(seraddr == "") {
		bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else {
		parseAddr(seraddr, bindAddr.sin_addr);
	}
	auto reuseAddr = 1;
	::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&reuseAddr), sizeof(int));
	NetworkUtil::bind(fd_, bindAddr);
}

auto Socket::listen(int backlog) -> void {
	if(::listen(fd_,backlog) < 0) {
		LOG_SYSERR << "listen() error";
	}
}

auto Socket::close() -> void {
	if(fd_ != NetworkUtil::INVALID_SOCKET) {
		::close(fd_);
		fd_ = NetworkUtil::INVALID_SOCKET;
	}
}

auto Socket::parseAddr(const std::string& addr, in_addr& sinaddr) -> void {
	auto ret = inet_pton(AF_INET, addr.data(), &sinaddr);
	if (ret < 0) {
		printf("inet_pton error \n");
	}
	else if (ret == 0) {
		hostent host1, * host2;
		char buf[2048] = "\0";
		int error;

		::gethostbyname_r(addr.data(), &host1, buf, sizeof(buf), &host2, &error);

		if (host2 == nullptr) {
			printf("gethostbyname_r error \n");
		}
		else {
			sinaddr = *reinterpret_cast<in_addr*>(host2->h_addr);
		}
	}
}

auto Socket::set_block(bool block) -> void {
	NetworkUtil::set_block(fd_, block);
}
