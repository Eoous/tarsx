#pragma once
#include <vector>
#include <stdint.h>

struct epoll_event;
namespace tarsx {
	class Epoller {
	public:
		Epoller(bool et = true);
		~Epoller();

		auto create(int maxConnections) -> void;
		auto add(int fd, long long data, uint32_t event) -> void;
		auto mod(int fd, long long data, uint32_t event) -> void;
		auto del(int fd, long long data, uint32_t event) -> void;
		auto wait(int millsecond) -> int;
		auto get(int i)->epoll_event&;
	private:
		auto ctrl(int op, int fd, long long data, uint32_t event) -> void;

	private:
		int epollfd_ = -1;
		int maxConnections_ = 1024;
		std::vector<epoll_event> events_;
		bool et_;
	};
}