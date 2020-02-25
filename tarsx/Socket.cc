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

Socket::~Socket() {
	if(owner_) {
		close();
	}
}

auto Socket::init(int fd, bool owner, int domain) -> void {
	if(owner_) {
		close();
	}
	fd_ = fd;
	owner_ = owner;
	domain_ = domain;
}

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

auto Socket::connect(const std::string& ser_addr, uint16_t port) -> void {
	assert(domain_ == AF_INET);

	if(ser_addr == "") {
		LOG_DEBUG << "Server address is empty";
	}
	sockaddr_in host;
	bzero(&host, sizeof(host));
	host.sin_family = domain_;
	parseAddr(ser_addr, host.sin_addr);
	host.sin_port = ::htons(port);
	NetworkUtil::connect(fd_, host);
}

auto Socket::accept(Socket& sock, sockaddr* sock_addr, socklen_t& sock_len) -> int {
	assert(sock.fd_ == NetworkUtil::INVALID_SOCKET);
	int fd;
	while ((fd = ::accept(fd_, sock_addr, &sock_len)) < 0 && errno == EINTR);
	sock.fd_ = fd;
	sock.domain_ = domain_;
	return fd;
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

auto Socket::set_keepAlive() -> void {
	NetworkUtil::set_keepAlive(fd_);
}

auto Socket::set_tcpNoDelay() -> void {
	NetworkUtil::set_tcpNoDealy(fd_);
}

auto Socket::set_noCloseWait() -> void {
	linger lin{ 1,0 };
	if(::setsockopt(fd_,SOL_SOCKET,SO_LINGER,reinterpret_cast<const void*>(&lin),sizeof(lin)) == -1) {
		LOG_SYSFATAL << "set_noCloseWait() error";
	}
}

auto Socket::set_closeWaitDefault() -> void {
	linger lin{ 0,0 };
	if (::setsockopt(fd_, SOL_SOCKET, SO_LINGER, reinterpret_cast<const void*>(&lin), sizeof(lin)) == -1) {
		LOG_SYSFATAL << "set_noCloseWaitDefault() error";
	}
}

auto Socket::recvfrom(void* buf, size_t len, std::string& from_addr, uint16_t& from_port, int flags) -> int {
	sockaddr fromaddr;
	socklen_t fromlen = sizeof(sockaddr);
	auto p = reinterpret_cast<sockaddr_in*>(&fromaddr);
	bzero(&fromaddr, sizeof(sockaddr));
	
	auto bytes = ::recvfrom(fd_,buf,len,flags,&fromaddr,&fromlen);
	if(bytes >= 0) {
		char addr[INET_ADDRSTRLEN] = "\0";
		::inet_ntop(domain_, &p->sin_addr, addr, sizeof(addr));
		from_addr = addr;
		from_port = ::ntohs(p->sin_port);
	}
	return bytes;
}
