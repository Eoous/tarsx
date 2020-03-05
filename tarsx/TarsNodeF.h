#pragma once
#include <memory>
#include <set>

#include "Mutex.h"
#include "NodeF.h"
#include "Singleton.h"

namespace tarsx {
	class ServantProxy;
	class Communicator;
	class TarsNodeFHelper : public Singleton<TarsNodeFHelper> {
	public:
		auto set_nodeInfo(const std::shared_ptr<Communicator>& comm,
			const std::string& obj, const std::string& app, const std::string& server) -> void;

		auto keepAlive(const std::string& adapter = "") -> void;
	protected:
		std::shared_ptr<Communicator> communicator_;

		std::unique_ptr<ServantProxy> nodeProxy_;
		ServerInfo serverInfo_;
		std::set<std::string> adapterSet_;
	private:
		Mutex mutex_;
	};
}