#pragma once
#include <string>

namespace tarsx {
	class EndPoint {
	public:
		EndPoint() = default;
		EndPoint(const std::string& host, int port);
		EndPoint(const EndPoint& end_point);
		~EndPoint() = default;

		EndPoint& operator=(const EndPoint& end_point);

		auto set_host(const std::string& host) { host_ = host; }
		auto get_host()const { return host_; }
		auto set_port(int port) { port_ = port; }
		auto get_port()const { return port_; }
	private:
		std::string host_;
		int port_ = 0;
	};
}