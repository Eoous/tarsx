#include <cassert>

#include "ObjectProxyFactory.h"
#include "ObjectProxy.h"

using namespace tarsx;

ObjectProxyFactory::ObjectProxyFactory(CommunicatorEpoll* communicator)
	:communicatorEpoll_(communicator){
}

auto ObjectProxyFactory::get_objectProxy(const std::string& ip, const uint16_t& port) -> std::shared_ptr<ObjectProxy> {
	std::string tmpObjName = ip + ":" + std::to_string(port);

	auto result = objectProxys_.find(tmpObjName);
	if(result!=objectProxys_.end()) {
		return result->second;
	}

	std::shared_ptr<ObjectProxy> object(std::make_shared<ObjectProxy>(communicatorEpoll_, ip, port));

	objectProxys_[tmpObjName] = object;
	objectproxys_.push_back(object);
	++objectNum_;
	return object;
}

auto ObjectProxyFactory::get_objectProxy(size_t index) -> std::shared_ptr<ObjectProxy> {
		assert(index < objectNum_);
		return objectproxys_[index];
}
