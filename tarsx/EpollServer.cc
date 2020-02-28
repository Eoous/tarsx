

#include "EpollServer.h"
#include "NetThread.h"
#include "BindAdapter.h"
#include "Connection.h"
#include "ServantHandle.h"

using namespace tarsx;

auto EpollServer::send(uint32_t uid, const std::string& msg, const std::string& ip, uint16_t port, int fd) -> void {
	auto& net_thread = getNetThreadByfd(fd);
	net_thread->recvNeedToSendFromServer(uid, msg, ip, port);
}

auto EpollServer::bind(std::shared_ptr<BindAdapter> bind_adapter) -> int {
	auto ret = netThreads_[0]->bind(bind_adapter);
	return ret;
}

auto EpollServer::addConnection(std::shared_ptr<Connection> connection, int fd, int type) -> void {
	auto& net_thread = getNetThreadByfd(fd);
	/* type留下来为udp做扩展 */
	net_thread->addTConnection(connection);
}


auto EpollServer::set_handleGroup(const std::string& groupName, int32_t handleNum, std::shared_ptr<BindAdapter>& adapter) -> void {
	auto res = handleGroups_.find(groupName);
	if(res == handleGroups_.end()) {
		std::shared_ptr<HandleGroup> handle_group = std::make_shared<HandleGroup>();
		handle_group->name = groupName;
		adapter->set_handleGroup(handle_group);

		for (int i = 0; i < handleNum; ++i) {
			std::shared_ptr<ServantHandle> handle = std::make_shared<ServantHandle>();
			handle->set_epollServer(this);
			handle->set_handleGroup(handle_group);
			handle_group->handles.push_back(handle);
		}
		handleGroups_[groupName] = handle_group;
		res = handleGroups_.find(groupName);
	}
	res->second->adapters[adapter->get_name()] = adapter;
	adapter->set_handleGroup(res->second);
}