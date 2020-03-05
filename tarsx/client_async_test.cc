#include "Communicator.h"
#include "ServantProxy.h"
#include <muduo/base/Logging.h>

using namespace tarsx;

int main() {
	Communicator comm;
	ServantProxy* prx = nullptr;
	auto callback = [](ReqMessage* req) {
		LOG_INFO<<req->response;
		return 0;
	};
	prx = comm.stringToProxy("127.0.0.1", 9000);

	prx->tars_invoke_async("test", "hello world",callback);

	getchar();
	return 0;
}