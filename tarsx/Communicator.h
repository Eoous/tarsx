#pragma once
#include <array>
#include <cassert>
#include <string>
#include <vector>

namespace tarsx {
	class ServantProxy;
	class CommunicatorEpoll;
	class Communicator {
	public:
		Communicator();
		~Communicator();

		auto stringToProxy(const std::string& ip, const std::uint16_t& port)->ServantProxy*;
		auto get_clientThreadNum() {
			return clientThreadNum_;
		}
		auto get_communicatorEpoll(size_t index) {
			assert(index < clientThreadNum_);
			return communicatorEpoll_[index];
		}

		auto terminate() -> void;
		auto get_servantProxy(const std::string& ip, const uint16_t& port)->ServantProxy*;
	private:
		auto initialize() -> void;
		auto isTerminating() -> bool;
	private:
		std::array<CommunicatorEpoll*, 64> communicatorEpoll_;
		size_t clientThreadNum_ = 1;
		bool initialized_ = false;
		bool terminating_ = false;
		bool registed_ = false;
	};
}
