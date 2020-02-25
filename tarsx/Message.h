#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <map>

#include "Monitor.hpp"
namespace tarsx {
	
	class BindAdapter;
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
		//std::vector<std::shared_ptr<Handle>> handles;
		std::map<std::string, std::shared_ptr<BindAdapter>> adapters;
	};
}
