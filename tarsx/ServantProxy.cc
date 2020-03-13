#include <sys/time.h>

#include "ServantProxy.h"
#include "CoroutineScheduler.h"
#include "ObjectProxy.h"

using namespace tarsx;
ServantProxy::ServantProxy(Communicator* communicator, std::vector<std::shared_ptr<ObjectProxy>>& objectProxy, size_t clientThreadNum) {
	communicator_ = communicator;
	objectProxyNum_ = clientThreadNum;
	objectProxy_.swap(objectProxy);

	//for (auto i = 0; i < objectProxyNum_; ++i) {
	//	objectProxy_[i]->set_servantProxy(this);
	//}
}

auto ServantProxy::tars_invoke(const std::string& func_name, const std::string& request, std::string& resp) -> void {
	auto msg = new ReqMessage;
	msg->init(ReqMessage::SYNC_CALL);
	msg->request = func_name + ":" + request;
	invoke(msg);
	resp = std::move(msg->response);
	delete msg;
	msg = nullptr;
}

auto ServantProxy::tars_invoke_async(const std::string& func_name, const std::string& request,const asyncCallback& cb) -> void {
	auto msg = new ReqMessage();
	msg->init(ReqMessage::ASYNC_CALL);
	msg->callback = cb;
	msg->request = func_name + ":" + request;
	invoke(msg);
}

auto ServantProxy::invoke(ReqMessage* msg, bool coroAsync) -> void {
	if(sptd == nullptr) {
		sptd = ServantProxyThreadData::get_data();
	}

	std::shared_ptr<ObjectProxy> objectproxy = nullptr;
	ReqInfoQueue* reqInfoQueue = nullptr;

	selectNetThreadInfo(sptd, objectproxy, reqInfoQueue);

	timeval tv;
	::gettimeofday(&tv, NULL);
	msg->beginTime = tv.tv_sec * static_cast<int64_t>(1000) + tv.tv_usec / 1000;
	msg->objectProxy = objectproxy;
	if (msg->type == ReqMessage::SYNC_CALL) {
		msg->monitorFin = false;
		msg->monitor.reset(new ReqMonitor);
	}

	auto empty = false;
	auto sync = (msg->type == ReqMessage::SYNC_CALL);

	if (!reqInfoQueue->push_back(msg, empty)) {
		delete msg;
		msg = nullptr;

		objectproxy->communicatorEpoll()->notify(sptd->netSeq_, *reqInfoQueue);
		printf("client queue full \n");
	}
	objectproxy->communicatorEpoll()->notify(sptd->netSeq_, *reqInfoQueue);
	if (sync) {
		if (!msg->monitorFin) {
			Lock lock(*(msg->monitor));

			if (!msg->monitorFin) {
				msg->monitor->wait();
			}
		}
	}
}

auto ServantProxy::selectNetThreadInfo(ServantProxyThreadData* sptd, std::shared_ptr<ObjectProxy>& objectProxy, ReqInfoQueue*& reqQueue) -> void {
	if (!sptd->queueInit_) {
		for (auto i = 0; i < objectProxyNum_; ++i) {
			sptd->reqQueue_[i] = new ReqInfoQueue(queueSize_);
		}
		sptd->objectProxyNum_ = objectProxyNum_;
		sptd->objectProxy_ = &objectProxy_;
		sptd->queueInit_ = true;
	}

	if (objectProxyNum_ == 1) {
		objectProxy = objectProxy_[0];
		reqQueue = sptd->reqQueue_[0];
	}
	else {
		if (sptd->netThreadSeq_ >= 0) {
			assert(sptd->netThreadSeq_ < static_cast<int>(objectProxyNum_));
			objectProxy = objectProxy_[sptd->netThreadSeq_];
			reqQueue = sptd->reqQueue_[sptd->netThreadSeq_];
		}
		else {
			objectProxy = objectProxy_[sptd->netSeq_];
			reqQueue = sptd->reqQueue_[sptd->netSeq_];
			++(sptd->netSeq_);
			printf("sptd->netSeq_ is %d \n", sptd->netSeq_);

			if (sptd->netSeq_ == objectProxyNum_) {
				sptd->netSeq_ = 0;
			}
		}
	}
}


auto ServantProxyThreadData::destructor(void* p) -> void {
	auto sptd = reinterpret_cast<ServantProxyThreadData*>(p);
	if (sptd) {
		delete sptd;
		sptd = nullptr;
	}
}

auto ServantProxyThreadData::get_data() -> ServantProxyThreadData* {
	if (key_ == 0) {
		Lock lock(mutex_);
		if (key_ == 0) {
			auto ret = ::pthread_key_create(&key_, ServantProxyThreadData::destructor);

			if (ret != 0) {
				return nullptr;
			}
		}
	}

	auto sptd = reinterpret_cast<ServantProxyThreadData*>(::pthread_getspecific(key_));

	if (!sptd) {
		Lock lock(mutex_);
		sptd = new ServantProxyThreadData;
		//sptd->reqQNo_ = seq_->get();

		auto ret = ::pthread_setspecific(key_, reinterpret_cast<void*>(sptd));

		assert(ret == 0);
	}

	return sptd;
}