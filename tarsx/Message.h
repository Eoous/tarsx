#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <memory>
#include <map>
#include <vector>

#include "Monitor.hpp"
#include "LoopQueue.hpp"
#include "Socket.h"

namespace tarsx {
	class ServantHandle;
	class BindAdapter;
	enum EM_CLOSE_T {
		EM_CLIENT_CLOSE = 0,
		EM_SERVER_CLOSE= 1,
		EM_SERVER_TIMEOUT_CLOSE = 2
	};
	
	struct tagRecvData {
		uint32_t uid;
		std::string buffer;
		std::string ip;
		uint16_t port;
		int64_t recvTimestamp;
		bool overload;
		bool closed;
		int fd;
		std::shared_ptr<BindAdapter> adapter;
		int closeType;
	};

	struct tagSendData {
		char cmd;
		uint32_t uid;
		std::string buffer;
		std::string ip;
		uint16_t port;
	};

	using utagRecvData = std::unique_ptr<tagRecvData>;
	using utagSendData = std::unique_ptr<tagSendData>;

	struct HandleGroup {
		std::string name;
		ThreadLock monitor;
		std::vector<std::shared_ptr<ServantHandle>> handles;
		std::map<std::string, std::shared_ptr<BindAdapter>> adapters;
	};

	struct FDInfo {
		enum {
			ET_C_NOTIFY = 1,
			ET_C_NET
		};

		size_t seq = 0;
		int fd = -1;
		int type = ET_C_NOTIFY;
		void* p = nullptr;
	};
	struct ReqMessage;
	using asyncCallback = std::function<int(ReqMessage*)>;
	
	struct ReqMonitor :public ThreadLock {

	};

	class ObjectProxy;
	struct ReqMessage {
		enum CallType {
			SYNC_CALL = 1,
			ASYNC_CALL,
			ONE_WAY,
			THREAD_EXIT
		};

		enum ReqStatus {
			REQ_REQ = 0,
			REQ_RSP,
			REQ_TIME,
			REQ_BUSY,
			REQ_EXC
		};

		auto init(CallType calltype) -> void {
			status = REQ_REQ;
			type = calltype;

			//callback = nullptr;
			reqData.clear();
			monitor.release();
			monitorFin = false;
			push = false;

			beginTime = 0;
			requestId = 1;
		}

		ReqStatus status = REQ_REQ;
		CallType type = SYNC_CALL;
		asyncCallback callback;
		//std::shared_ptr<ServantProxyCallback> callback;
		std::shared_ptr<ObjectProxy> objectProxy;
		std::string request;
		std::string response;
		std::string reqData;
		std::unique_ptr<ReqMonitor> monitor;
		bool monitorFin = false;
		bool push = false;

		int64_t beginTime = 0;
		int requestId = 1;
	};

	using ReqInfoQueue = LoopQueue<ReqMessage*>;

	struct NotifyInfo {
		FDInfo fd_info;
		Socket notify;
		int eventfd = -1;
		bool valid = false;
	};

}
