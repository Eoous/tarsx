#pragma once
#include <vector>
#include <map>
#include <memory>

namespace tarsx {
	class ObjectProxy;
	class CommunicatorEpoll;
	
	class ObjectProxyFactory {
	public:
		ObjectProxyFactory(CommunicatorEpoll* communicator);

		~ObjectProxyFactory() = default;

		auto get_objectProxy(const std::string& ip, const uint16_t& port) -> std::shared_ptr<ObjectProxy>;
		
		auto get_objNum() {
			return objectNum_;
		}

		auto get_objectProxy(size_t index) ->std::shared_ptr<ObjectProxy>;
	private:
		CommunicatorEpoll* communicatorEpoll_;
		std::map<std::string, std::shared_ptr<ObjectProxy>> objectProxys_;
		std::vector<std::shared_ptr<ObjectProxy>> objectproxys_;
		size_t objectNum_ = 0;
	};
}