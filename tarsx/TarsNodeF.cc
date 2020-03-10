#include <unistd.h>

#include "TarsNodeF.h"
#include "Communicator.h"
#include "ServantProxy.h"
#include "Lock.hpp"

using namespace tarsx;

auto TarsNodeFHelper::set_nodeInfo(const std::shared_ptr<Communicator>& comm, const std::string& node, const std::string& app, const std::string& server) -> void {
	communicator_ = comm;
	if (!node.empty()) {
		auto pos = node.find(":");
		auto host = node.substr(0, pos);
		auto portstr = node.substr(pos + 1);
		uint16_t port = atoi(portstr.data());
		printf("set_nodeInfo host is %s  \n", host.data());
		nodeProxy_.reset(communicator_->stringToProxy(host, port));
	}
	serverInfo_.application = app;
	serverInfo_.serverName = server;
	serverInfo_.pid = getpid();
}

auto TarsNodeFHelper::keepAlive(const std::string& adapter) -> void {
	try {
		if (nodeProxy_) {
			decltype(adapterSet_) s;
			{
				Lock lock(ServantProxyThreadData::mutex_);
				adapterSet_.insert(adapter);
				if (adapter != "AdminAdapter") {
					return;
				}
				s.swap(adapterSet_);
			}
			ServerInfo si = serverInfo_;
			for (auto& name : s) {
				si.adapter = name;
				async_keepAlive2(si, nodeProxy_);
			}
		}
	}
	catch (std::exception & e) {
		LOG_SYSFATAL<<"TarsNodeFHelper::keepAlive error:" << e.what();
	}
	catch (...) {
		LOG_SYSFATAL<<"TarsNodeFHelper::keepAlive unknown error";
	}
}

