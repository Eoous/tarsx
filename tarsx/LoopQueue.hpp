#pragma once
#include <assert.h>
#include <vector>
#include <cstdint>
#include <stdlib.h>
#include <string.h>

namespace tarsx {
	template<typename T>
	class LoopQueue {
	public:
		using QueueType = std::vector<T>;
		LoopQueue(uint32_t size = 5) {
			assert(size < 10000);
			capacity_ = size + 1;
			capacitySub_ = size;
			p_ = (T*)malloc(capacity_ * sizeof(T));
		}
		~LoopQueue() {
			free(p_);
		}

		auto push_back(const T& t, bool& empty, uint32_t& begin, uint32_t& end) -> bool {
			empty = false;
			end = end_;
			begin = begin_;
			if ((end > begin_&& end - begin_ < 2) ||
				(begin_ > end&& begin_ - end > (capacity_ - 2))) {
				return false;
			}
			else {
				memcpy(p_ + begin_, &t, sizeof(T));

				if (end_ == begin_) {
					empty = true;
				}

				if (begin_ == capacitySub_) {
					begin_ = 0;
				}
				else {
					++begin_;
				}

				if (!empty && size() == 1) {
					empty == true;
				}

				return true;
			}
		}

		auto push_back(const T& t, bool& empty) -> bool {
			empty = false;
			uint32_t end = end_;
			if ((end > begin_&& end - begin_ < 2) ||
				(begin_ > end&& begin_ - end > (capacity_ - 2))) {
				return false;
			}
			else {
				memcpy(p_ + begin_, &t, sizeof(T));

				if (end_ == begin_) {
					empty = true;
				}

				if (begin_ == capacitySub_) {
					begin_ = 0;
				}
				else {
					++begin_;
				}

				if (!empty && size() == 1) {
					empty = true;
				}
#if 0
				if (size() == 1) {
					empty = true;
				}
#endif
				return true;
			}
		}

		auto push_back(const T& t) -> bool {
			bool empty;
			return push_back(t, empty);
		}

		auto push_back(const std::vector<T>& queue) -> bool {
			uint32_t end = end_;
			if (queue.size() > (capacity_ - 1) ||
				(end > begin_ && (end - begin_) < queue.size() + 1) ||
				(begin_ > end && (begin_ - end) > (capacity_ - queue.size() - 1))) {
				return false;
			}
			else {
				for (uint32_t i = 0; i < queue.size(); ++i) {
					memcpy(p_ + begin_, &queue[i], sizeof(T));
					if (begin_ == capacitySub_) {
						begin_ = 0;
					}
					else {
						++begin_;
					}
				}
				return true;
			}
		}

		auto pop_front(T& t) -> bool {
			if (end_ == begin_) {
				return false;
			}
			memcpy(&t, p_ + end_, sizeof(T));
			if (end_ == capacitySub_) {
				end_ = 0;
			}
			else {
				++end_;
			}
			return true;
		}

		auto pop_front() -> bool {
			if (end_ == begin_) {
				return false;
			}
			if (end_ == capacitySub_) {
				end_ = 0;
			}
			else {
				++end_;
			}
			return true;
		}

		auto get_front(T& t) -> bool {
			if (end_ == begin_) {
				return false;
			}
			memcpy(&t, p_ + end_, sizeof(T));
			return true;
		}

		auto empty() const -> bool {
			if (end_ == begin_) {
				return true;
			}
			return false;
		}

		auto size() const -> uint32_t {
			uint32_t begin = begin_;
			uint32_t end = end_;
			if (begin < end) {
				return begin + capacity_ - end;
			}
			return begin - end;
		}

		auto capacity() const -> uint32_t {
			return capacity_;
		}

	private:
		T* p_;
		uint32_t capacity_;
		uint32_t capacitySub_;
		uint32_t begin_ = 0;
		uint32_t end_ = 0;
	};
}