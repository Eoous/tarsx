#include <memory>
#include "BindAdapter.h"
#include "NetThread.h"
#include "EpollServer.h"
#include "ServantManager.h"

using namespace tarsx;
auto func2 = [](auto&& aa,auto&& b) {
	std::string a = "func2 response";
	b.assign(a.begin(), a.end());
};
auto func = [](const std::string& request, std::vector<char>& buffer) -> int {

	func2(request, buffer);
};

int main() {
	ServantHelperManager::instance().addServant("test",func);
	HandleGroup handle_group;
	std::shared_ptr<BindAdapter> adapter(new BindAdapter());
	adapter->set_name("test");
	adapter->set_endpoint("", 9000);
	adapter->set_handleGroupName("test");
	
	EpollServer a(2);
	a.set_handleGroup("test", 2, adapter);
	a.bind(adapter);
	a.createEpoll();

	for(auto& it : a.get_netThreads()) {
		it->start();
	}
	a.startHandle();
	a.waitForWait();

	return 0;
}