#include <algorithm>
#include <limits>
#include <cassert>
#include <string>
#include <cstring>

#include "Buffer.h"

using namespace tarsx;

namespace detail {
	auto roundUp2Power(size_t size) {
		if (size == 0) {
			return (size_t)0;
		}
		size_t roundUp = 1;
		while (roundUp < size) {
			roundUp <<= 1;
		}
		return roundUp;
	}
}

const size_t Buffer::MaxBufferSize = std::numeric_limits<size_t>::max() / 2;
const size_t Buffer::DefaultSize = 128;

auto Buffer::pushData(const void* data, size_t size) -> size_t {
	if (!data || size == 0) {
		return 0;
	}
	if (readableSize() + size >= MaxBufferSize) {
		return 0;
	}
	assureSpace(size);
	memcpy(&buffer_[writePos_], data, size);
	produce(size);
	return size;
}

auto Buffer::popData(void* buf, size_t size) -> size_t {
	const size_t dataSize = readableSize();
	if (!buf || size == 0 || dataSize == 0) {
		return 0;
	}
	if (size > dataSize) {
		size = dataSize;
	}

	memcpy(buf, &buffer_[readPos_], size);
	consume(size);
	return size;
}

auto Buffer::peekData(void*& buf, size_t& size) -> void {
	buf = readAddr();
	size = readableSize();
}

auto Buffer::consume(size_t bytes) -> void {
	assert(readPos_ + bytes <= writePos_);

	readPos_ += bytes;
	if (empty()) {
		clear();
	}
}

auto Buffer::shrink() -> void {
	if (empty()) {
		clear();
		capacity_ = 0;
		resetBuffer();
		return;
	}

	if (capacity_ <= DefaultSize) {
		return;
	}

	size_t oldcap = capacity_;
	size_t dataSize = readableSize();
	if (dataSize * 100 > oldcap* highWaterPercent_) {
		return;
	}

	size_t newcap = detail::roundUp2Power(dataSize);
	char* tmp(new char[newcap]);
	memcpy(&tmp[0], &buffer_[readPos_], dataSize);
	resetBuffer(tmp);
	capacity_ = newcap;

	readPos_ = 0;
	writePos_ = dataSize;
}

auto Buffer::clear() -> void {
	readPos_ = writePos_ = 0;
}

auto Buffer::swap(Buffer& buf) -> void {
	std::swap(readPos_, buf.readPos_);
	std::swap(writePos_, buf.writePos_);
	std::swap(capacity_, buf.capacity_);
	std::swap(buffer_, buf.buffer_);
}

auto Buffer::resetBuffer(void* ptr) -> void {
	delete[] buffer_;
	buffer_ = reinterpret_cast<char*>(ptr);
}

auto Buffer::assureSpace(size_t needsize) -> void {
	if (writableSize() >= needsize) {
		return;
	}

	const size_t dataSize = readableSize();
	const size_t oldcap = capacity_;

	while (writableSize() + readPos_ < needsize) {
		if (capacity_ < DefaultSize) {
			capacity_ = DefaultSize;
		}
		else if (capacity_ <= MaxBufferSize) {
			const size_t newCapacity = detail::roundUp2Power(capacity_);
			if (capacity_ < newCapacity) {
				capacity_ = newCapacity;
			}
			else {
				capacity_ = newCapacity << 1;
			}
		}
		else {
			assert(false);
		}
	}

	if (oldcap < capacity_) {
		char* tmp(new char[capacity_]);

		if (dataSize != 0) {
			memcpy(&tmp[0], &buffer_[readPos_], dataSize);
		}
		resetBuffer(tmp);
	}
	else {
		assert(readPos_ > 0);
		memmove(&buffer_[0], &buffer_[readPos_], dataSize);
	}

	readPos_ = 0;
	writePos_ = dataSize;

	assert(needsize <= writableSize());
}