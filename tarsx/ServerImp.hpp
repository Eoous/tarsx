#include <algorithm>
#include <string>
#include <vector>

#include "ServantProxy.h"

auto keepAlive = [](const std::string& req, std::vector<char>& buffer) {
	printf("Node Server have recieve heartbeat from %s \n", req.data());
	auto pos = req.find(":");
	auto requestId = req.substr(0, pos);
	auto response = requestId + ":" + std::string("I have received");
	buffer.assign(response.begin(), response.end());
	return -1;
};

auto ServerImp = [](const std::string& request, std::vector<char>& buffer) {
	static std::string __tars__ServerF_all[] = { "keepAlive","reportVersion" };
	auto [a, b, c, method] = tarsx::parseRequest(request);
	auto r = std::equal_range(__tars__ServerF_all, __tars__ServerF_all + 2, method);
	if (r.first == r.second) {
		printf("do not fine right method %s \n", method.data());
		return -3;
	}
	switch (r.first - __tars__ServerF_all) {
	case 0: {
		auto ret = keepAlive(request, buffer);
		return ret;
	}
	case 1: {
		printf("do not have reportVersion yet \n");
		return -3;
	}
	}
	return 0;
};
