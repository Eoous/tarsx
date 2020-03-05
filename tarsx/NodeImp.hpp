#include <algorithm>
#include <string>
#include <vector>

#include "ServantProxy.h"

auto startServer = [](const std::string& req, std::vector<char>& buffer) {
	//auto pos = req.find(":");
	//auto requestId = req.substr(0, pos);

	//auto ret = -1;

	//std::string s;
	//auto result = std::string("have notify to startServer");
	//std::shared_ptr<ServerObject> serobj = std::make_shared<ServerObject>(req);
	//serobj->set_startScript("./NodeServer-start.sh");

	//if (serobj) {
	//	auto byNode = true;
	//	CommandStart command(serobj, byNode);
	//	ret = command.doProcess(s);
	//	if (ret == 0) {
	//		result = "server is activating";
	//	}
	//}
	//result = requestId + ":" + result;
	//buffer.assign(result.begin(), result.end());
	//return ret;
};

auto NodeImp = [](const std::string& request, std::vector<char>& buffer) {
	static std::string __tars__Node_all[] = { "startServer","stopServer" };
	auto [a, b, c, method] = tarsx::parseRequest(request);
	auto r = std::equal_range(__tars__Node_all, __tars__Node_all + 2, method);
	if (r.first == r.second) {
		printf("Node do not find right method %s \n", method.data());
		return -3;
	}
	switch ((r.first - __tars__Node_all)) {
	case 0: {
		//auto ret = startServer(request, buffer);
		//return ret;
	}
	}
	return 0;
};
