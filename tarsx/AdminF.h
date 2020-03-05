#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "Application.h"
#include "ServantProxy.h"

using namespace tarsx;
auto shutdown2 = []() {
	Application::terminate();
};

auto notify = [](auto&& command) {
	printf("AdminServant::notify \n");
	return std::string("notify");
};

auto AdminServant = [](const std::string& request, std::vector<char>& buffer) -> int {
	static std::string __tars__AdminF_all[] = { "notify","shutdown" };
	auto [a, b, c, method] = tarsx::parseRequest(request);
	auto r = std::equal_range(__tars__AdminF_all, __tars__AdminF_all + 2, method);
	if (r.first == r.second) {
		printf("can not find AdminF method \n");
		return -3;
	}

	switch (r.first - __tars__AdminF_all) {
	case 0: {
		auto ret = notify(request);
		printf("ret is %s \n", ret.data());
		return 0;
	}
	case 1: {
		shutdown2();
		return 0;
	}
	}
	return 0;
};