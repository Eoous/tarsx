#pragma once
#include <netinet/in.h>
#include <string>

namespace tarsx {
	namespace NetworkUtil {
		static constexpr int INVALID_SOCKET = -1;
		static constexpr int SOCKET_ERROR = -1;

		auto createSocket(bool udp, bool local = false) -> int;
		auto closeSocketNoThrow(int fd) -> void;

		auto set_block(int fd, bool block) -> void;
		auto set_tcpNoDealy(int fd) -> void;
		auto set_keepAlive(int fd) -> void;

		auto bind(int fd, sockaddr_in& addr) -> void;
		auto connect(int fd, sockaddr_in& addr) -> bool;

		auto getAddress(const std::string& host, int port, sockaddr_in& addr) -> void;
	}
}