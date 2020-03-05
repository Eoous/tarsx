#pragma once

#include "Message.h"

namespace tarsx {
	class Communicator;
	class CoroutineScheduler;
	class ServantProxyThreadData {
	public:
		inline static Monitor mutex_;
		inline static pthread_key_t key_ = 0;
		//static SeqManager* seq_;

		ServantProxyThreadData() = default;
		~ServantProxyThreadData() = default;

		static auto destructor(void* p) -> void;
		static auto get_data()->ServantProxyThreadData*;

	public:
		std::array< ReqInfoQueue*, 64> reqQueue_;
		bool queueInit_ = false;
		size_t reqQNo_ = 0;
		size_t netSeq_ = 0;
		int netThreadSeq_ = -1;
		size_t objectProxyNum_ = 0;
		std::vector<std::shared_ptr<ObjectProxy>>* objectProxy_ = nullptr;
		CoroutineScheduler* scheduler_ = nullptr;
	};

	class ServantProxy {
	public:
		ServantProxy(Communicator* communicator, std::vector<std::shared_ptr<ObjectProxy>>& objectProxy, size_t clientThreadNum);
		~ServantProxy() = default;

		auto tars_invoke(const std::string& func_name, const std::string& request, std::string& resp) -> void;
		auto tars_invoke_async(const std::string& func_name, const std::string& request,const asyncCallback& cb = nullptr) -> void;
		auto ters_communicator() { return communicator_; }
	private:
		auto invoke(ReqMessage* msg, bool coroAsync = false) -> void;
		auto selectNetThreadInfo(ServantProxyThreadData* sptd, std::shared_ptr<ObjectProxy>& objectProxy, ReqInfoQueue*& reqQueue) -> void;
	private:
		Communicator* communicator_;
		std::vector<std::shared_ptr<ObjectProxy>> objectProxy_;
		size_t objectProxyNum_;
		int queueSize_ = 1000;
	};

	inline auto parseRequest(const std::string& request) {
		auto pos = request.find(":");
		auto requestBody = request.substr(pos + 1);
		auto posMethod = requestBody.find(":");
		auto method = requestBody.substr(0, posMethod);
		return std::tuple{ pos,requestBody,posMethod,method };
	}
}
