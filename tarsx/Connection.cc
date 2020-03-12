#include <limits>

#include "Connection.h"
#include "BindAdapter.h"

using namespace tarsx;

Connection::Connection(std::shared_ptr<BindAdapter> bind_adapter, int listenfd, int timeout, int fd, const std::string& ip, uint16_t port)
	:bindAdapter_(bind_adapter),listenfd_(listenfd),
	 ip_(ip),port_(port){
	assert(fd != -1);
	socket_.init(fd, true, AF_INET);
}

auto Connection::recv(std::deque<utagRecvData>& queue) -> int {
	queue.clear();

	while(true) {
		char buffer[32 * 1024];

		int bytes_received = ::read(socket_.fd(), static_cast<void*>(buffer), sizeof(buffer));
		if(bytes_received < 0) {
			if(errno == EAGAIN) {
				// 没有数据了
				break;
			}
			else {
				// client closed
				return -1;
			}
		}
		else if( bytes_received == 0 ) {
			// client closed
			return -1;
		}
		recvBuffer_.pushData(buffer, bytes_received);
		if(static_cast<size_t>(bytes_received) < sizeof(buffer)) {
			break;
		}
	}

	if(listenfd_ != -1) {
		// 4 is message length
		while(recvBuffer_.readableSize() >= kHeaderSize-1) {
			const void* data = recvBuffer_.peek();
			int32_t be32 = *static_cast<const int32_t*>(data);
			const int32_t len = ::ntohl(be32);
			
			if(recvBuffer_.readableSize() >= len + kHeaderSize) {
				recvBuffer_.consume(kHeaderSize);
				std::string req(recvBuffer_.peek(), len);
				std::unique_ptr<tagRecvData> recv(new tagRecvData{
					uid_,std::move(req),
					ip_,port_,0,false,false,
					fd(),bindAdapter_
				});
				recvBuffer_.consume(len);
				queue.push_back(std::move(recv));
			}
			else {
				break;
			}
		}
	}
	return queue.size();
}

auto Connection::send() -> int {
	if(sendBuffer_.empty()) {
		return 0;
	}
	return send("", ip_, port_, true);
}

auto Connection::send(const std::string& buffer, const std::string& ip, uint16_t port, bool byEpollout) -> int {
	if(byEpollout) {
		auto bytes = send(sendBuffer_);
		if(bytes == -1) {
			return -1;
		}
		adjustSlices(sendBuffer_, bytes);
	}
	else {
		auto bytes = tcpSend(buffer.data(), buffer.size());

		if(bytes == -1) {
			LOG_INFO << "Connection::send() bytes == -1";
			return -1;
		}
		// TO BE DONE
	}

	size_t to_send_bytes = 0;
	for(const auto& slice:sendBuffer_) {
		to_send_bytes += slice.dataLen;
	}

	if(to_send_bytes >= 8*1024) {
		LOG_INFO << "buffer too long to close";
		clearSlices(sendBuffer_);
		return -2;
	}

	if(close_ && sendBuffer_.empty()) {
		LOG_INFO << "close connection by user";
		return -2;
	}
	return 0;
}

auto Connection::send(const std::vector<Slice>& slices) -> int {
	const auto maxIOVcount = std::max<int>(sysconf(_SC_IOV_MAX), 16);

	size_t alreadySentVecs = 0;
	size_t alreadySentBytes = 0;
	while (alreadySentVecs < slices.size()) {
		const size_t vc = std::min<int>(slices.size() - alreadySentVecs, maxIOVcount);

		std::vector<iovec> vecs;
		size_t expectSent = 0;
		for (size_t i = alreadySentVecs; i < alreadySentVecs + vc; ++i) {
			assert(slices[i].dataLen > 0);

			iovec ivc{ slices[i].data,slices[i].dataLen };
			expectSent += slices[i].dataLen;
			vecs.push_back(ivc);
		}

		auto bytes = tcpWriteV(vecs);
		if (bytes == -1) {
			return -1;
		}
		else if (bytes == 0) {
			return alreadySentBytes;
		}
		else if (bytes == static_cast<int>(expectSent)) {
			alreadySentBytes += bytes;
			alreadySentVecs += vc;
		}
		else {
			assert(bytes > 0);
			alreadySentBytes += bytes;
			return alreadySentBytes;
		}
	}

	return alreadySentBytes;
}

auto Connection::tcpSend(const void* data, size_t len) -> int {
	if (len == 0) {
		return 0;
	}
	auto bytes = ::send(socket_.fd(), data, len, 0);
	if (bytes == -1) {
		if (errno == EAGAIN) {
			bytes = 0;
		}
		if (errno == EINTR) {
			bytes = 0;
		}
	}
	return bytes;
}

auto Connection::tcpWriteV(const std::vector<iovec>& buffers) -> int {
	const auto maxIOVcount = std::max<int>(sysconf(_SC_IOV_MAX), 16);
	const auto cnt = static_cast<int>(buffers.size());

	assert(cnt <= maxIOVcount);
	const auto sock = socket_.fd();
	auto bytes = static_cast<int>(::writev(sock, buffers.data(), cnt));
	if (bytes == -1) {
		assert(errno != EINVAL);
		if (errno == EAGAIN) {
			return 0;
		}
		return -1;
	}
	else {
		return bytes;
	}
}

auto Connection::clearSlices(std::vector<Slice>& slices) -> void {
	adjustSlices(slices, std::numeric_limits<std::size_t>::max());
}

auto Connection::adjustSlices(std::vector<Slice>& slices, size_t to_skipped_bytes) -> void {
	size_t skippedVecs = 0;
	for (auto slice : slices) {
		assert(slice.dataLen > 0);
		if (to_skipped_bytes >= slice.dataLen) {
			to_skipped_bytes -= slice.dataLen;
			++skippedVecs;
		}
		else {
			if (to_skipped_bytes != 0) {
				const auto src = static_cast<const char*>(slice.data) + to_skipped_bytes;
				memmove(slice.data, src, slice.dataLen - to_skipped_bytes);
				slice.dataLen -= to_skipped_bytes;
			}
			break;
		}
	}
	// TO BE DONE
	// free to pool
	slices.erase(slices.begin(), slices.begin() + skippedVecs);
}
