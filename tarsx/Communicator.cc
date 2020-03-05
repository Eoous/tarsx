#include "Communicator.h"
#include "ServantProxy.h"
#include "ObjectProxyFactory.h"
#include "AsyncProcThread.h"
#include "CommunicatorEpoll.h"
using namespace tarsx;

Communicator::Communicator() {
	memset(communicatorEpoll_.data(), 0, sizeof(int*) * communicatorEpoll_.size());
}

Communicator::~Communicator() {
	terminate();
}

auto Communicator::initialize() -> void {
	if (initialized_) {
		return;
	}
	initialized_ = true;

	clientThreadNum_ = 1;
	for (size_t i = 0; i < clientThreadNum_; ++i) {
		communicatorEpoll_[i] = new CommunicatorEpoll(this, i);
		communicatorEpoll_[i]->start();
	}
}

auto Communicator::terminate() -> void {
	{
		terminating_ = true;
	}
	if (initialized_) {
		for (auto i = 0; i < clientThreadNum_; ++i) {
			communicatorEpoll_[i]->terminate();
			delete communicatorEpoll_[i];
		}
	}
}

auto Communicator::get_servantProxy(const std::string& ip, const uint16_t& port) -> ServantProxy* {
	initialize();

	std::vector<std::shared_ptr<ObjectProxy>> object;
	object.reserve(clientThreadNum_);
	for (auto i = 0; i < clientThreadNum_; ++i) {
		object.push_back(communicatorEpoll_[i]->get_objectProxy(ip, port));
	}
	auto servant_proxy = new ServantProxy(this, object, clientThreadNum_);
	return servant_proxy;
}

auto Communicator::stringToProxy(const std::string& ip, const std::uint16_t& port) -> ServantProxy* {
	return get_servantProxy(ip, port);
}
