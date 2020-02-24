#pragma once

#include "Mutex.h"
#include "Condition.h"

namespace tarsx {

	class Monitor {
	public:
		Monitor() = default;
		~Monitor() = default;

		auto lock() const -> void {
			mutex_.lock();
			notify_ = 0;
		}

		auto unlock() const -> void {
			notifyImpl(notify_);
			mutex_.unlock();
		}

		auto tryLock() const -> bool {
			auto result = mutex_.tryLock();
			if (result) {
				notify_ = 0;
			}
			return result;
		}

		// 条件等待 释放锁
		// 关于条件完成 属于notifyImpl()函数管理
		// 一般在析构的时候达成条件 notifyImpl()来通知可以用了
		auto wait() const -> void {
			notifyImpl(notify_);
			try {
				cond_.wait(mutex_);
			}
			catch (...) {
				notify_ = 0;
				throw;
			}
			notify_ = 0;
		}

		auto timedWait(int millsecond) const -> bool {
			notifyImpl(notify_);
			bool rc;
			try {
				rc = cond_.timedWait(mutex_, millsecond);
			}
			catch (...) {
				notify_ = 0;
				throw;
			}

			notify_ = 0;
			return rc;
		}

		auto notify() -> void {
			if (notify_ != -1) {
				++notify_;
			}
		}

		auto notifyAll() -> void {
			notify_ = -1;
		}


		Monitor(const Monitor&) = delete;
		void operator=(const Monitor&) = delete;
	private:
		auto notifyImpl(int notify) const -> void {
			if (notify != 0) {
				if (notify == -1) {
					cond_.broadcast();
					return;
				}
				else {
					while (notify > 0) {
						cond_.signal();
						--notify;
					}
				}
			}
		}
	private:
		mutable int notify_ = 0;
		mutable Condition cond_;
		Mutex mutex_;
	};
	using ThreadLock = Monitor;
}