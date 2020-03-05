#pragma once

#include "Transceiver.h"
#include "TimeoutQueueNew.hpp"

namespace tarsx {
	class ObjectProxy {
	public:
		ObjectProxy(CommunicatorEpoll* communicatorEpoll,const std::string& host,const uint16_t& port);
		~ObjectProxy() = default;

		auto invoke(ReqMessage* msg) -> void;
		auto doInvoke() -> void;
		auto finishInvoke(const std::string& response) -> void;
		auto finishInvoke(ReqMessage* msg) -> void;
		auto communicatorEpoll() -> CommunicatorEpoll* {
			return communicatorEpoll_;
		}
		auto generateId() -> uint32_t {
			++id_;
			if(id_==0) {
				++id_;
			}
			return id_;
		}

		//auto set_servantProxy(ServantProxy* servantProxy) {
		//	servantProxy_ = servantProxy;
		//}
		
	private:
		int fd_;
		CommunicatorEpoll* communicatorEpoll_;

		std::unique_ptr<Transceiver> trans_;
		std::unique_ptr<TimeoutQueueNew<ReqMessage*>> timeoutQueue_;
		uint32_t id_ = 0;
		//ServantProxy* servantProxy_;
	};
}