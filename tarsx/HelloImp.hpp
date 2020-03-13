#pragma once
#include <algorithm>
#include <string>
#include <vector>

inline auto test = [](auto&& req, auto&& b) {
	auto pos = req.find(":");
	auto requestId = req.substr(0, pos);
	auto response = requestId + ":" + "test response";
	b.assign(req.begin(), req.end());
	return 0;
};

inline auto testHello = [](auto&& req, auto&& b) {
	auto pos = req.find(":");
	auto requestId = req.substr(0, pos);
	auto response = requestId + ":" + "testHello response";
	b.assign(req.begin(), req.end());
	return 0;
};

auto HelloImp = [](const std::string& request, std::vector<char>& buffer) -> int {
	static std::string __TestAll__Hello_all[] = { "test","testHello" };
	size_t pos = request.find(":");
	auto requestBody = request.substr(pos + 1);
	auto posMethod = requestBody.find(":");
	auto method = requestBody.substr(0, posMethod);

	auto r = std::equal_range(__TestAll__Hello_all, __TestAll__Hello_all + 2, method);
	if (r.first == r.second) {
		printf("do not find right method: %s\n", method.data());
		return -3;
	}

	switch (r.first - __TestAll__Hello_all) {
	case 0: {
		auto ret = test(request, buffer);
		return ret;
	}
	case 1: {
		auto ret = testHello(request, buffer);
		return ret;
	}
	}
	return 0;
};