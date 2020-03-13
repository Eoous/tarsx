#include "Application.h"
#include "RegistryImp.hpp"
using namespace tarsx;

Application g_app;

std::vector<std::string> funcNames;

auto Application::initialize() -> void {
	addServant(ServerConfig::adapterName, RegisterImp);
}

int main(int argc, char* argv[]) {
	ServerConfig::application = "Registry";
	ServerConfig::serverName = "RegistryServer";
	//ServerConfig::node = "127.0.0.1:19386";
	ServerConfig::openCoroutine = true;

	ServerConfig::adapterName = "RegistryAdapter";
	ServerConfig::servantName = "Registry";
	ServerConfig::adapterIp = "127.0.0.1";
	ServerConfig::adapterPort = 19386;


	g_app.extra_main(argc, argv);
	g_app.waitForShutdown();
	return 0;
}