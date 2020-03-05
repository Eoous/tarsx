#pragma once
#include <list>
#include <string>

#include "Buffer.h"
#include "CommunicatorEpoll.h"

namespace tarsx {
	class ObjectProxy;
	class Transceiver {
	public:
		enum ReturnStatus {
			retError=-1,
			retOk,
			retFull
		};

		Transceiver(ObjectProxy* objectProxy, const std::string& ip, const uint16_t& port);
		virtual ~Transceiver();

		auto valid() const -> bool {
			return (fd_ != -1);
		}

		virtual auto close() -> void;
		auto connect() -> void;
		ObjectProxy* objectProxy() {
			return objectProxy_;
		}

		virtual auto fd() const -> int {
			return fd_;
		}

		virtual auto doRequest() -> int;
		virtual auto doResponse(std::list<std::string>& done) -> int;

		virtual auto send(const void* buf, uint32_t len, uint32_t flag) -> int;

		auto sendRequest(const char* data, size_t size, bool forceSend = false) -> int;
		auto readv(const iovec*, int32_t count) -> int;

	protected:
		int fd_ = -1;
		std::string ip_;
		uint16_t port_;

		ObjectProxy* objectProxy_;
		Buffer sendBuffer_;
		Buffer receiveBuffer_;

		FDInfo fdInfo_{ 0,-1,FDInfo::ET_C_NET,(void*)this };
	};
}