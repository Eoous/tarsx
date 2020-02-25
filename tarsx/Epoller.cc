#include <assert.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "Epoller.h"

using namespace tarsx;

Epoller::Epoller(bool et) :et_(et) {
}

Epoller::~Epoller() {
	if (epollfd_ > 0) {
		::close(epollfd_);
	}
}

auto Epoller::ctrl(int op,int fd, long long data, uint32_t event) -> void {
	epoll_event ev;
	ev.data.u64 = data;
	if (et_) {
		ev.events = event | EPOLLET;
	}
	else {
		ev.events = event;
	}
	::epoll_ctl(epollfd_, op, fd, &ev);
}

auto Epoller::create(int maxConnections) -> void {
	maxConnections_ = maxConnections;
	epollfd_ = ::epoll_create(maxConnections_ + 1);
	events_.resize(maxConnections_ + 1);
}

auto Epoller::add(int fd, long long data, uint32_t event) -> void {
	ctrl(EPOLL_CTL_ADD, fd, data, event);
}

auto Epoller::mod(int fd, long long data, uint32_t event) -> void {
	ctrl(EPOLL_CTL_MOD, fd, data, event);
}

auto Epoller::del(int fd, long long data, uint32_t event) -> void {
	ctrl(EPOLL_CTL_DEL, fd, data, event);
}

auto Epoller::wait(int millsecond) -> int {
	return ::epoll_wait(epollfd_, events_.data(), maxConnections_ + 1, millsecond);
}

auto Epoller::get(int i) -> epoll_event& {
	assert(!events_.empty());
	return events_[i];
}
