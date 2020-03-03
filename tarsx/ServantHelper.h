#pragma once
#include <map>
#include <functional>
#include <vector>


#include "Mutex.h"
#include "Singleton.h"

namespace tarsx {

	class ServantHelperManager :public Singleton<ServantHelperManager> {
	public:
		ServantHelperManager() = default;

		template<typename T>
		auto addServant(const std::string& id, bool check = false) -> void {
			if (check && servantAdapter_.end() == servantAdapter_.find(id)) {
				std::cerr << "[TARS]ServantHelperManager::addServant " << id << " not find adapter.(maybe not conf in the web \n)";
				throw std::runtime_error("[TARS]ServantHelperManager::addServant " + id + " not find adapter.(maybe not conf in the web) \n");
			}

			std::shared_ptr<ServantCreation<T>> servantCreation = std::make_shared<ServantCreation<T>>();
			servantCreator_[id] = servantCreation;
		}

		//auto create(const std::string& adapter)->std::shared_ptr<Servant>;
		auto set_adapter_servant(const std::string& adapter, const std::string& servant) -> void;

		auto getAdapterServant(const std::string& adapter) -> std::string {
			auto result = adapterServant_.find(adapter);
			if (result != adapterServant_.end()) {
				return result->second;
			}
			return "(NO TARS PROTOCOL)";
		}

		auto getServantAdapter(const std::string& servant) -> std::string {
			auto result = servantAdapter_.find(servant);
			if (result != servantAdapter_.end()) {
				return result->second;
			}
			return "";
		}

		auto adapterServant() ->std::map<std::string, std::string>& {
			return adapterServant_;
		}

	private:
		std::map<std::string, std::function<int(const std::string&, std::vector<char>&)>> servantMap_;
		std::map<std::string, std::string> adapterServant_;
		std::map<std::string, std::string> servantAdapter_;
		Mutex mutex_;
	};
}