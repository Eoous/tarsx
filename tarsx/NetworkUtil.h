#pragma once
#include <netinet/in.h>
#include <string>

namespace tarsx {
	namespace NetworkUtil {
		static constexpr int INVALID_SOCKET = -1;
		static constexpr int SOCKET_ERROR = -1;

		static auto createSocket(bool udp, bool local = false) -> int;
		static auto closeSocketNoThrow(int fd) -> void;

		static auto set_block(int fd, bool block) -> void;
		static auto set_tcpNoDealy(int fd) -> void;
		static auto set_keepAlive(int fd) -> void;

		static auto bind(int fd, sockaddr_in& addr) -> void;
		static auto connect(int fd, sockaddr_in& addr) -> bool;

		static auto getAddress(const std::string& host, int port, sockaddr_in& addr) -> void;
	}
}