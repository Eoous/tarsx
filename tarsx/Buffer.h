#pragma once
namespace tarsx {
	class Buffer {
	public:
		Buffer() = default;
		~Buffer() {
			delete[] buffer_;
		}

		auto produce(size_t bytes) {
			writePos_ += bytes;
		}
		auto pushData(const void* data, size_t size)->size_t;
		auto popData(void* buf, size_t size)->size_t;
		auto peekData(void*& buf, size_t& size) -> void;
		auto consume(size_t bytes) -> void;

		auto readAddr() {
			return &buffer_[readPos_];
		}
		auto writeAddr() {
			return &buffer_[writePos_];
		}
		auto empty() const {
			return readableSize() == 0;
		}

		auto readableSize() const -> size_t {
			return writePos_ - readPos_;
		}

		auto writableSize() const {
			return capacity_ - writePos_;
		}
		auto capacity() const {
			return capacity_;
		}

		auto shrink() -> void;
		auto clear() -> void;
		auto swap(Buffer& buf) -> void;
		auto assureSpace(size_t size) -> void;
		auto set_highWaterPercent(size_t percent) -> void;

		static const size_t MaxBufferSize;
		static const size_t DefaultSize;
	private:
		auto resetBuffer(void* ptr = nullptr) -> void;

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;
	private:
		size_t readPos_ = 0;
		size_t writePos_ = 0;
		size_t capacity_ = 0;
		char* buffer_ = nullptr;
		size_t highWaterPercent_ = 50;
	};
}