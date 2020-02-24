#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <vector>

#include <muduo/base/Logging.h>

#include "NetworkUtil.h"

using namespace tarsx;

auto NetworkUtil::createSocket(bool udp, bool local) -> int {
	auto fd = ::socket((local ? PF_LOCAL : PF_INET), SOCK_STREAM, IPPROTO_TCP);

	if(fd==INVALID_SOCKET) {
		LOG_SYSFATAL << "NetworkUtil::createSocket"<<errorToString(errno);
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
}
