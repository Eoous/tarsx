#pragma once

#include "ServantManager.h"
#include "ServantHandle.h"
#include "TarsNodeF.h"

namespace tarsx {

	struct ServerConfig {
		static std::string application; // 应用名称
		static std::string serverName;  // 服务名称
		static std::string node;		// 本机Node地址

		static bool openCoroutine;
		static size_t coroutineMemSize;
		static uint32_t coroutineStackSize; // 默认128k

		static std::string adapterName;
		static std::string servantName;
		static std::string adapterIp;
		static int adapterPort;

		static std::string adapterName1;
		static std::string servantName1;
		static std::string adapterIp1;
		static int adapterPort1;
	};

	class Application {
	public:
		Application() = default;
		~Application();

		auto extra_main(int argc, char* argv[]) -> void;
		auto waitForShutdown() -> void;
		auto waitForQuit() -> void;
		auto destroyApp() -> void;

		static auto& get_EpollServer() {
			return epollServer_;
		}
		static auto terminate() -> void;
		static auto get_communicator() {
			return communicator_;
		}
	protected:
		auto initializeClient() -> void;
		auto initializeServer() -> void;
		auto bindAdapter(std::vector<std::shared_ptr<BindAdapter>>& adapters) -> void;

		auto initialize() -> void;
		auto set_handle(std::shared_ptr<BindAdapter>& adapter) -> void;

		auto addServant(const std::string& name, dispatcFunc func) {
			ServantHelperManager::instance().addServant(name, func, true);
		}
	protected:
		static std::shared_ptr<EpollServer> epollServer_;
		static std::shared_ptr<Communicator> communicator_;
	};
}