#include "Application.h"

#include "AdminF.h"
#include "Communicator.h"
#include "TarsNodeF.h"
#include "EpollServer.h"
#include "NetThread.h"
#include "BindAdapter.h"

using namespace tarsx;

std::shared_ptr<EpollServer> Application::epollServer_ = nullptr;
std::shared_ptr<Communicator> Application::communicator_ = nullptr;

std::string ServerConfig::application;
std::string ServerConfig::serverName;
std::string ServerConfig::node;

std::string ServerConfig::adapterName;
std::string ServerConfig::servantName;
std::string ServerConfig::adapterIp;
int ServerConfig::adapterPort;

std::string ServerConfig::adapterName1;
std::string ServerConfig::servantName1;
std::string ServerConfig::adapterIp1;
int ServerConfig::adapterPort1;

bool ServerConfig::openCoroutine;
size_t ServerConfig::coroutineMemSize;
uint32_t ServerConfig::coroutineStackSize;

Application::~Application() {
	terminate();
}

auto Application::extra_main(int argc, char* argv[]) -> void {
	initializeClient();
	initializeServer();
	std::vector<std::shared_ptr<BindAdapter>> adapters;
	bindAdapter(adapters);
	initialize();

	for (auto i = 0; i < adapters.size(); ++i) {
		set_handle(adapters[i]);
	}
	epollServer_->startHandle();
	epollServer_->createEpoll();
}

auto Application::waitForShutdown() -> void {
	waitForQuit();

	//destroyApp();
}

auto Application::waitForQuit() -> void {
	auto netThreadNum = epollServer_->get_netThreadNum();
	auto& netThread = epollServer_->get_netThreads();

	for (auto& thread : netThread) {
		thread->start();
	}
	while (!epollServer_->isTerminate()) {
		{
			Lock sync(epollServer_->monitor_);
			epollServer_->monitor_.timedWait(5000);
		}
	}
	if (epollServer_->isTerminate()) {
		for (auto& thread : netThread) {
			thread->terminate();
		}
		//epollServer_->stopThread();
	}
}

auto Application::destroyApp() -> void {
}

auto Application::terminate() -> void {
	if (epollServer_) {
		epollServer_->terminate();
	}
}

auto Application::initializeClient() -> void {
	communicator_ = std::make_shared<Communicator>();
	communicator_->stringToProxy("127.0.0.1", 19386);
}


auto Application::initializeServer() -> void {
	//ServerConfig::openCoroutine = true;
	ServerConfig::coroutineMemSize = 1073741824;
	ServerConfig::coroutineStackSize = 1310272;

	uint32_t netThreadNum = 1;
	epollServer_ = std::make_shared<EpollServer>(netThreadNum);
	TarsNodeFHelper::instance().set_nodeInfo(communicator_, ServerConfig::node, ServerConfig::application, ServerConfig::servantName);
	ServantHelperManager::instance().addServant("AdminObj",AdminServant);
	ServantHelperManager::instance().set_adapter_servant("AdminAdapter", "AdminObj");
	auto adapter = std::make_shared<BindAdapter>(epollServer_.get());
	adapter->set_name("AdminAdapter");
	adapter->set_endpoint("127.0.0.1", 20002);
	adapter->set_handleGroupName("AdminAdapter");
	adapter->set_handleNum(1);
	epollServer_->set_handleGroup(adapter->get_handleGroupName(), adapter->get_handleNum(), adapter);
	epollServer_->bind(adapter);
}

auto Application::bindAdapter(std::vector<std::shared_ptr<BindAdapter>>& adapters) -> void {
	auto handleNum = 1;

	ServantHelperManager::instance().set_adapter_servant(ServerConfig::adapterName, ServerConfig::servantName);

	std::shared_ptr<BindAdapter> bindAdapter = std::make_shared<BindAdapter>(epollServer_.get());
	bindAdapter->set_name(ServerConfig::adapterName);
	bindAdapter->set_endpoint(ServerConfig::adapterIp, ServerConfig::adapterPort);
	bindAdapter->set_handleGroupName(ServerConfig::adapterName);
	bindAdapter->set_handleNum(handleNum);

	epollServer_->bind(bindAdapter);
	adapters.push_back(bindAdapter);

	if (ServerConfig::servantName1 != "") {
		ServantHelperManager::instance().set_adapter_servant(ServerConfig::adapterName1, ServerConfig::servantName1);
		std::shared_ptr<BindAdapter> bindAdapter1 = std::make_shared<BindAdapter>(epollServer_.get());
		bindAdapter1->set_name(ServerConfig::adapterName1);
		bindAdapter1->set_endpoint(ServerConfig::adapterIp1, ServerConfig::adapterPort1);
		bindAdapter1->set_handleGroupName(ServerConfig::adapterName1);
		bindAdapter1->set_handleNum(handleNum);

		epollServer_->bind(bindAdapter1);
		adapters.push_back(bindAdapter1);
	}
}

auto Application::set_handle(std::shared_ptr<BindAdapter>& adapter) -> void {
	epollServer_->set_handleGroup(adapter->get_handleGroupName(), adapter->get_handleNum(), adapter);
}
