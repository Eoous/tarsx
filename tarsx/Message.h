#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <map>
#include <vector>

#include "Monitor.hpp"
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
}
