#include "Application.h"
#include "ServerImp.hpp"
#include "NodeImp.hpp"
using namespace tarsx;

Application g_app;

auto Application::initialize() -> void {
	auto serverObj = tarsx::ServerConfig::adapterName;
	addServant(serverObj,ServerImp);
	//auto serverObj1 = ServerConfig::servantName1;
	//addServant(serverObj1,NodeImp);
}

int main(int argc, char* argv[]) {
	ServerConfig::application = "tars";
	ServerConfig::serverName = "tarsnode";
	//ServerConfig::Node         = "127.0.0.1:19386";
	ServerConfig::openCoroutine = true;

	ServerConfig::adapterName = "tars.tarsnode.ServerAdapter";
	ServerConfig::servantName = "tars.tarsnode.ServerObj";
	ServerConfig::adapterIp = "127.0.0.1";
	ServerConfig::adapterPort = 19386;

	ServerConfig::adapterName1 = "tars.tarsnode.NodeAdapter";
	ServerConfig::servantName1 = "tars.tarsnode.NodeObj";
	ServerConfig::adapterIp1 = "127.0.0.1";
	ServerConfig::adapterPort1 = 19385;

	g_app.extra_main(argc, argv);
	g_app.waitForShutdown();
	return -1;
}
