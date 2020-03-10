#include "Application.h"
#include "HelloImp.hpp"
using namespace tarsx;

Application g_app;

auto Application::initialize() -> void {
	addServant(ServerConfig::adapterName,HelloImp);
}

int main(int argc, char* argv[]) {
	ServerConfig::application = "TestApp";
	ServerConfig::serverName = "HelloServer";
	ServerConfig::node = "127.0.0.1:19386";
	ServerConfig::openCoroutine = true;

	ServerConfig::adapterName = "TestApp.HelloServer.HelloObjAdapter";
	ServerConfig::servantName = "TestApp.HelloServer.HelloObj";
	ServerConfig::adapterIp = "127.0.0.1";
	ServerConfig::adapterPort = 9877;
	g_app.extra_main(argc, argv);
	g_app.waitForShutdown();
	return 0;
}