#include <muduo/base/Logging.h>

#include "Communicator.h"
#include "ServantProxy.h"

using namespace tarsx;
auto func2 = [](auto&& aa,auto&& b) {
	std::string a = "func2 response";
	b.assign(a.begin(), a.end());
};
auto HelloImp = [](const std::string& request, std::vector<char>& buffer) -> int {

	func2(request, buffer);
};

int main() {
	Communicator comm;
	ServantProxy* prx = nullptr;

	prx=comm.stringToProxy("127.0.0.1", 9000);
	std::string response = "";
	if(prx) {
		{
			prx->tars_invoke("test", "hello", response);
			LOG_INFO <<"response:"<< response;
		}
	}
	return 0;
}