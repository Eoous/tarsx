#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <net/if_arp.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <muduo/base/Logging.h>

#include "NetworkUtil.h"

using namespace tarsx;

auto NetworkUtil::createSocket(bool udp, bool local) -> int {
	auto fd = ::socket((local ? PF_LOCAL : PF_INET), SOCK_STREAM, IPPROTO_TCP);

	if(fd == INVALID_SOCKET) {
		LOG_SYSFATAL << "NetworkUtil::createSocket"<<errno;
	}
	if(!udp) {
		set_tcpNoDealy(fd);
		set_keepAlive(fd);
	}
	return fd;
}

auto NetworkUtil::closeSocketNoThrow(int fd) -> void {
	auto error = errno;
	::close(fd);
	errno = error;
}

auto NetworkUtil::set_block(int fd, bool block) -> void {
	auto flags = ::fcntl(fd, F_GETFL);
	if(block) {
		flags &= ~O_NONBLOCK;
	}
	else {
		flags |= O_NONBLOCK;
	}
	if (::fcntl(fd, F_SETFL, flags) == SOCKET_ERROR) {
		closeSocketNoThrow(fd);
		LOG_SYSFATAL << "set_block() no block errno:" << errno;
	}
}

auto NetworkUtil::set_tcpNoDealy(int fd) -> void {
	auto flag = 1;
	if(::setsockopt(fd,IPPROTO_TCP,TCP_NODELAY,
					reinterpret_cast<char*>(&flag),sizeof(int)) == SOCKET_ERROR) {
		closeSocketNoThrow(fd);
		LOG_SYSFATAL << "set_tcpNoDelay() errno:" << errno;
	}
}

auto NetworkUtil::set_keepAlive(int fd) -> void {
	auto flag = 1;
	if(::setsockopt(fd,SOL_SOCKET,SO_KEEPALIVE,
					reinterpret_cast<char*>(&flag),sizeof(int)) == SOCKET_ERROR) {
		closeSocketNoThrow(fd);
		LOG_SYSFATAL << "set_keepAlive() errno:" << errno;
	}
}

auto NetworkUtil::bind(int fd, sockaddr_in& addr) -> void {
	if(::bind(fd,reinterpret_cast<sockaddr*>(&addr),sizeof(addr)) == SOCKET_ERROR) {
		closeSocketNoThrow(fd);
		LOG_SYSFATAL << "bind() errno:"<<errno;
	}
	auto len = static_cast<socklen_t>(sizeof(addr));
	::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len);
}

auto NetworkUtil::connect(int fd, sockaddr_in& addr) -> bool {
	auto connected = false;
	auto ret = ::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	if(ret == 0) {
		connected = true;
	}
	else if(ret == -1 && errno!=EINPROGRESS) {
		::close(fd);
		LOG_SYSFATAL << "connect() ret == -1 && errno!=EINPROGRESS errno:" << errno;
	}
	return connected;
}

auto NetworkUtil::getAddress(const std::string& host, int port, sockaddr_in& addr) -> void {
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host.data());

	if(addr.sin_addr.s_addr == INADDR_NONE) {
		addrinfo* info = 0;
		auto retry = 5;
		
		addrinfo hints;
		hints.ai_family = PF_INET;

		auto rs = 0;
		do {
			rs = ::getaddrinfo(host.data(), 0, &hints, &info);
		} while (info == 0 && rs == EAI_AGAIN && --retry >= 0);

		if(rs!=0) {
			if(info != nullptr) {
				::freeaddrinfo(info);
			}
			LOG_SYSFATAL << "getAddress rs!=0 errno:" << errno;
		}
		assert(info != nullptr);
		assert(info->ai_family == PF_INET);
		auto sin = reinterpret_cast<sockaddr_in*>(info->ai_addr);
		addr.sin_addr.s_addr = sin->sin_addr.s_addr;
		::freeaddrinfo(info);
	}
}