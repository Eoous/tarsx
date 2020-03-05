#include <sys/epoll.h>

#include "ObjectProxy.h"
#include "Lock.hpp"

using namespace tarsx;

ObjectProxy::ObjectProxy(CommunicatorEpoll* communicatorEpoll, const std::string& host, const uint16_t& port)
	:communicatorEpoll_(communicatorEpoll),
	 timeoutQueue_(new TimeoutQueueNew<ReqMessage*>()),
	 trans_(new Transceiver(this,host,port)){
}

auto ObjectProxy::invoke(ReqMessage* msg) -> void {
	if(!trans_->connected_) {
		trans_->connect();
	}
	msg->requestId=generateId();

	// 序列化msg中的内容，传递给服务端
	msg->reqData = std::to_string(msg->requestId) + ":" + msg->request;

	if(timeoutQueue_->sendListEmpty() && trans_->sendRequest(msg->reqData.data(),msg->reqData.size())!=Transceiver::retError) {
		auto flag = timeoutQueue_->push(msg, msg->requestId, 1000 + msg->beginTime);
		if(!flag) {
			printf("timeoutQueue_->push fail \n");
			msg->status = ReqMessage::REQ_EXC;
			finishInvoke(msg);
		}
	}
	else {
		auto flag = timeoutQueue_->push(msg, msg->requestId, 1000 + msg->beginTime, false);
		if(!flag) {
			msg->status = ReqMessage::REQ_EXC;
			finishInvoke(msg);
		}
	}
}

auto ObjectProxy::doInvoke() -> void {
	if(timeoutQueue_->sendListEmpty()) {
		return;
	}

	while(!timeoutQueue_->sendListEmpty()) {
		ReqMessage* msg = nullptr;
		timeoutQueue_->getSend(msg);

		auto ret = trans_->sendRequest(msg->reqData.data(), msg->reqData.size());
		if(ret == Transceiver::retError) {
			printf("ObjectProxy::doInvoke send fail \n");
			return;
		}
		else if(ret == Transceiver::retFull) {
			return;
		}
		
	}
	
}

auto ObjectProxy::finishInvoke(const std::string& response) -> void {
	printf("finish invoke \n");

	ReqMessage* msg = nullptr;
	size_t pos = response.find(":");

	auto requestId = response.substr(0, pos);
	// erase也是取出之前存放的msg
	auto retErase = timeoutQueue_->erase(atoi(requestId.data()), msg);

	if(!retErase) {
		printf("get message from timeoutQueue fail \n");
		return;
	}
	msg->response = response;
	finishInvoke(msg);
}

auto ObjectProxy::finishInvoke(ReqMessage* msg) -> void {
	if(msg->type==ReqMessage::SYNC_CALL) {
		Lock sync(*msg->monitor.get());
		msg->monitor->notify();
		msg->monitorFin = true;
	}
	else if(msg->type==ReqMessage::ASYNC_CALL) {
		communicatorEpoll()->pushAsyncThreadQueue(msg);
	}
}
