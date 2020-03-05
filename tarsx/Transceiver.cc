#include <sys/epoll.h>
#include <sys/uio.h>

#include "Transceiver.h"
#include "NetworkUtil.h"
#include "ObjectProxy.h"

using namespace tarsx;

Transceiver::Transceiver(ObjectProxy* objectProxy, 
						 const std::string& ip, 
						 const uint16_t& port)
	:ip_(ip),port_(port),objectProxy_(objectProxy){
	
}

Transceiver::~Transceiver() {
	close();
}

auto Transceiver::close() -> void {
	if(!valid()) {
		return;
	}
	objectProxy()->communicatorEpoll()->del(fd_, &fdInfo_, EPOLLIN | EPOLLOUT);
	NetworkUtil::closeSocketNoThrow(fd_);
	fd_ = -1;

	sendBuffer_.clear();
	receiveBuffer_.clear();
}

auto Transceiver::connect() -> void {
	connected_ = true;
	auto fd = -1;
	fd = NetworkUtil::createSocket(false);
	NetworkUtil::set_block(fd, false);

	sockaddr_in addr;
	NetworkUtil::getAddress(ip_, port_, addr);

	auto connected = NetworkUtil::connect(fd, addr);
	if(connected) {
		printf("connect successful \n");
	}
	else {
		printf("connecting \n");
	}

	fd_ = fd;

	//int qos;
	//::setsockopt(fd, SOL_IP, IP_TOS, &qos, sizeof(qos));
	objectProxy()->communicatorEpoll()->add(fd, &fdInfo_, EPOLLIN);
}

auto Transceiver::doRequest() -> int {
	auto ret = 0;

	if(!sendBuffer_.empty()) {
		size_t length = 0;
		void* data = nullptr;
		sendBuffer_.peekData(data, length);

		ret = this->send(data, length, 0);

		if(ret<0) {
			return ret;
		}
		else if(ret > 0) {
			sendBuffer_.consume(ret);
			if(sendBuffer_.empty()) {
				sendBuffer_.shrink();
			}
			else {
				return 0;
			}
		}
	}

	objectProxy()->doInvoke();

	return 0;
}

auto Transceiver::sendRequest(const char* data, size_t size, bool forceSend) -> int {
	// 保证发送顺序，数据不会丢失
	// 而是放在了ObjectProxy里的timeoutQueue_里面
	if(!sendBuffer_.empty()) {
		return retError;
	}
	auto ret = this->send(data, size, 0);

	if(ret<0) {
		return ReturnStatus::retError;
	}

	// 发不完再放入sendBuffer_
	if(ret < static_cast<int>(size)) {
		sendBuffer_.pushData(data + ret, size - ret);
		return retFull;
	}
	return ret;
}

auto Transceiver::doResponse(std::list<std::string>& done) -> int {
	auto ret = 0;
	done.clear();

	do {
		receiveBuffer_.assureSpace(8 * 1024);
		char stackBuffer[64 * 1024];

		iovec vecs[2];
		vecs[0].iov_base = receiveBuffer_.writeAddr();
		vecs[0].iov_len = receiveBuffer_.writableSize();
		vecs[1].iov_base = stackBuffer;
		vecs[1].iov_len = sizeof(stackBuffer);

		ret = this->readv(vecs, 2);
		if(ret > 0) {
			if(static_cast<size_t>(ret) <= vecs[0].iov_len) {
				receiveBuffer_.produce(ret);
			}
			else {
				receiveBuffer_.produce(vecs[0].iov_len);
				auto stackBytes = static_cast<size_t>(ret) - vecs[0].iov_len;
				receiveBuffer_.pushData(stackBuffer, stackBytes);
			}
		}
	}
	while (ret>0);

	if(!receiveBuffer_.empty()) {
		const auto data = receiveBuffer_.readAddr();
		auto len = receiveBuffer_.readableSize();
		done.push_back(std::string(data, len));

		if(len>0) {
			receiveBuffer_.consume(len);
			if(receiveBuffer_.capacity()>8*1024*1024) {
				receiveBuffer_.shrink();
			}
		}
	}

	return done.empty() ? 0 : 1;
}

auto Transceiver::send(const void* buf, uint32_t len, uint32_t flag) -> int {
	auto ret = ::send(fd_, buf, len, flag);
	if(ret<0 && errno !=EAGAIN) {
		LOG_SYSFATAL << "Transceiver::send fail";
		return ret;
	}
	if(ret<0 && errno == EAGAIN) {
		ret = 0;
	}
	return ret;
}

auto Transceiver::readv(const iovec* vec, int32_t count) -> int {
	auto ret = ::readv(fd_, vec, count);

	if(ret==0 || (ret<0 && errno != EAGAIN)) {
		close();
		return 0;
	}
	return ret;
}
