#pragma once
#include <array>

#include "Epoller.h"
#include "Socket.h"
#include "Message.h"

namespace tarsx {

	class Communicator;
	class Transceiver;
	class AsyncProcThread;
	class ObjectProxyFactory;

	class CommunicatorEpoll {
	public:
		CommunicatorEpoll(size_t netThreadSeq);
		CommunicatorEpoll(Communicator* communicator, size_t netThreadSeq);
		~CommunicatorEpoll() = default;
		auto run() -> void ;

		auto get_objectProxy(const std::string& ip, const uint16_t& port)->std::shared_ptr<ObjectProxy>;

		// 关于fd的操作
		auto add(int fd, FDInfo* info, uint32_t events) -> void;
		auto del(int fd, FDInfo* info, uint32_t events) -> void;
		auto notify(size_t seq, ReqInfoQueue& reqQueue) -> void;
		auto notifyDel(size_t seq) -> void;

		auto pushAsyncThreadQueue(ReqMessage* msg) -> void;

		auto terminate() -> void;
		auto start() -> void;

	private:
		auto handle(FDInfo* info, uint32_t events) -> void;
		auto handleInputImp(Transceiver* transceiver) -> void;
		auto handleOutputImp(Transceiver* transceiver) -> void;

	private:
		std::array<NotifyInfo, 2048> notify_;
		Socket shutdown_;
		Epoller epoller_;

		size_t asyncThreadNum_ = 3;
		std::vector<std::unique_ptr<AsyncProcThread>> asyncThread_;
		size_t asyncSeq_ = 0;

		Communicator* communicator_;
		bool terminate_ = false;
		size_t netThreadSeq_;
		std::unique_ptr<ObjectProxyFactory> objectProxyFactory_;
	};
}