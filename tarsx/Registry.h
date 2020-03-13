#pragma once
#include <unordered_map>
#include <set>
#include <string>

#include "Singleton.h"

namespace tarsx {
	class Registry :public Singleton<Registry>{
	public:
		Registry() = default;
		~Registry() = default;

		auto get_allAddress(const std::string& name) -> void;
		
		auto addServantAddress(const std::string& name, const std::string& ip, const int32_t& port) -> void;
		auto delServantAddress(const std::string& name, const std::string& ip, const int32_t& port) -> void;
	private:
		// name -> set(ip,port)
		std::unordered_map<std::string, std::set<std::pair<std::string, int32_t>>> allServants_;
	};
}
