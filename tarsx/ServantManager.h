#pragma once
#include <map>
#include <functional>
#include <vector>
#include <memory>

#include "Mutex.h"
#include "Singleton.h"

namespace tarsx {
	using dispatcFunc = std::function<int(const std::string&, std::vector<char>&)>;
	class ServantHelperManager :public Singleton<ServantHelperManager> {
	public:
		ServantHelperManager() {
			printf("asd");
		}
		auto addServant(const std::string& name, dispatcFunc func,bool check = false) -> void {
			if (check && servantAdapter_.find(name) == servantAdapter_.end()) {
				throw std::runtime_error("[TARSX]ServantHelperManager::addServant " + name + " not find adapter.(maybe not conf in the web) \n");
			}
			servantMap_[name] = func;
		}

		auto set_adapter_servant(const std::string& adapter, const std::string& servant) -> void {
			adapterServant_[adapter] = servant;
			servantAdapter_[servant] = adapter;
		}

		auto get_adapter_servant(const std::string& adapter) -> std::string {
			auto result = adapterServant_.find(adapter);
			if (result != adapterServant_.end()) {
				return result->second;
			}
			return "(NO TARSX PROTOCOL)";
		}

		auto get_servant_adapter(const std::string& servant) -> std::string {
			auto result = servantAdapter_.find(servant);
			if (result != servantAdapter_.end()) {
				return result->second;
			}
			return "";
		}

		auto& adapterServant(){ return adapterServant_; }

		auto get_funcByName(const std::string& name) {
			return servantMap_[name];
		}

	private:
		std::map<std::string, dispatcFunc> servantMap_;
		std::map<std::string, std::string> adapterServant_;
		std::map<std::string, std::string> servantAdapter_;
		Mutex mutex_;
	};
}