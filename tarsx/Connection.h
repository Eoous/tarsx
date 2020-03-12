#pragma once
#include <memory>
#include <vector>

#include "Buffer.h"
#include "Socket.h"
#include "BufferPoll.h"
#include "Message.h"
#include "ThreadDeque.hpp"

namespace tarsx {
	class BindAdapter;
	class Connection {
	public:
		Connection(std::shared_ptr<BindAdapter> bind_adapter, int listenfd, int timeout,
				   int fd, const std::string& ip, uint16_t port);
		~Connection() {
			LOG_INFO << "~Connection()";
			if(listenfd_ != -1) {
				assert(!socket_.valid());
			}
		}

		auto init(uint32_t uid) -> void { uid_ = uid; }

		auto get_bindAdapter() { return bindAdapter_; }
		auto get_uid() const { return uid_; }
		auto get_listenfd()const { return listenfd_; }
		auto fd() const { return socket_.fd(); }
		auto get_ip() const { return ip_; }
		auto get_port() const { return port_; }
		auto set_close() { close_ = true; return sendBuffer_.empty(); }

		auto recv(std::deque<utagRecvData>& queue) -> int;
		auto send() -> int;
		auto send(const std::string& buffer, const std::string& ip, uint16_t port, bool byEpollout = false) -> int;
		auto send(const std::vector<Slice>& slices) -> int;
		
		auto close() -> void;
	private:
		auto tcpSend(const void* data, size_t len) -> int;
		auto tcpWriteV(const std::vector<iovec>& buffers) -> int;
		auto clearSlices(std::vector<Slice>& slices) -> void;
		auto adjustSlices(std::vector<Slice>& slices, size_t to_skipped_bytes) -> void;
	private:
		std::shared_ptr<BindAdapter> bindAdapter_;
		Socket socket_;
		volatile uint32_t uid_ = 0;
		int listenfd_;
		std::string ip_;
		uint16_t port_;
		bool close_;
		std::string receiveBuffer_;
		Buffer recvBuffer_;
		std::vector<Slice> sendBuffer_;
	};

	inline auto Connection::close() -> void {
		if(listenfd_ != -1) {
			if(socket_.valid()) {
				socket_.close();
			}
		}
	}

}
