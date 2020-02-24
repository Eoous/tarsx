#pragma once
#include <muduo/base/Logging.h>

#include "Monitor.hpp"

namespace tarsx {

	class Lock {
	public:
		Lock(const Monitor& mutex) :mutex_(mutex) {
			mutex_.lock();
			acquired_ = true;
		}

		~Lock() {
			if (acquired_) {
				mutex_.unlock();
			}
		}

		auto acquire() const -> void {
			if (acquired_) {
				LOG_FATAL << "thread has locked!";
			}
			mutex_.lock();
			acquired_ = true;
		}

		auto tryAcquire() const -> bool {
			acquired_ = mutex_.tryLock();
			return acquired_;
		}

		auto release() const -> void {
			if (!acquired_) {
				LOG_FATAL << "thread hasn't been locked!";
			}
			mutex_.unlock();
			acquired_ = false;
		}

		auto acquired() const -> bool {
			return acquired_;
		}

		Lock(const Lock&) = delete;
		Lock& operator=(const Lock&) = delete;
	private:
		Lock(const Monitor& mutex, bool) :mutex_(mutex) {
			acquired_ = mutex_.tryLock();
		}
	private:
		const Monitor& mutex_;
		mutable bool acquired_;
	};
#define Lock(x) "The lock is a xvalue!"
}