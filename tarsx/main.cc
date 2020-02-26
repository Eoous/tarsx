#include <memory>
#include "NetThread.h"
#include "BindAdapter.h"
using namespace tarsx;
int main() {
	NetThread a;
	std::shared_ptr<BindAdapter> adapter(new BindAdapter());
	adapter->set_endpoint("", 9000);
	a.bind(adapter);
	a.createEpoll(100);
	a.run();
}