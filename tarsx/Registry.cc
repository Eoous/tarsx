#include <muduo/base/Logging.h>

#include "Registry.h"

using namespace tarsx;

auto Registry::addServantAddress(const std::string& name, const std::string& ip, const int32_t& port) -> void {
	auto servant_set = allServants_[name];
	auto x=servant_set.insert(std::pair<std::string,int32_t>{ip,port});
	LOG_INFO << "Add servant address,ip:" << ip << " port:" << port;
}

auto Registry::delServantAddress(const std::string& name, const std::string& ip, const int32_t& port) -> void {
	auto servant_set = allServants_[name];
	auto ele = servant_set.find(std::pair<std::string, int32_t>{ip, port});
	servant_set.erase(ele);
	LOG_INFO << "Delete servant address,ip:" << ip << " port:" << port;
}
