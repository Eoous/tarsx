#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <regex>

#include "Registry.h"
using namespace tarsx;

inline auto registerServant = [](auto&& request,auto&& buffer) {
	//printf("%s \n", request.data());
	
	Registry::instance().addServantAddress(request[2], request[3], atoi(request[4].data()));
	
	std::string response = request[0]+":registed!";
	buffer.assign(response.begin(), response.end());
	return -1;
};

inline auto deleteServant = [](auto&& request, auto&& buffer) {

	return -1;
};

auto RegisterImp = [](const std::string& request,std::vector<char>& buffer) {
	static std::string __TestAll__Register_all[] = { "registerServant","deleteServant" };
	std::regex ws_re(":");
	std::vector<std::string> v(std::sregex_token_iterator(request.begin(), request.end(), ws_re, -1), std::sregex_token_iterator());
	auto& method = v[1];
	
	/*auto r = std::equal_range(__TestAll__Register_all, __TestAll__Register_all + 2, method);
	if (r.first == r.second) {
		printf("error method: %s\n", method.data());
		return -3;
	}*/

	/*switch (method) {
	case __TestAll__Register_all[0]: {
		auto ret = registerServant(v, buffer);
		return ret;
	}
	case __TestAll__Register_all[1]: {
		auto ret = deleteServant(request, buffer);
		return ret;
	}
	}*/
	if(method == __TestAll__Register_all[0]) {
		auto ret = registerServant(v, buffer);
		return ret;
	}
	else if(method == __TestAll__Register_all[1]) {
		auto ret = deleteServant(request, buffer);
		return ret;
	}
	else {
		printf("error method: %s\n", method.data());
		return -3;
	}
	return 0;
};