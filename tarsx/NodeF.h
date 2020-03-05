#include <string>

#include "ServantProxy.h"

namespace tarsx {
	struct ServerInfo {
	public:
		static auto className() {
			return std::string("tars.ServerInfo");
		}
		static auto MD5() {
			return std::string("34368b1d85d52126f8efa6e27af8f151");
		}

		auto resetDefault() {
			application = "";
			serverName = "";
			adapter = "";
			pid = 0;
		}
	public:
		std::string application;
		std::string serverName;
		std::string adapter;
		int pid = 0;
	};

	inline auto async_keepAlive = [](auto&& request,auto&& servant_proxy) {
		servant_proxy->tars_invoke_async("keepAlive", request);
	};

	inline auto async_keepAlive2 = [](auto&& server_info, auto&& servant_proxy) {
		auto req = server_info.application + ":" + server_info.serverName + ":" + std::to_string(server_info.pid) + ":" + server_info.adapter;
		servant_proxy->tars_invoke_async("keepAlive", req);
	};
	
}
