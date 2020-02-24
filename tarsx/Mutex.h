#pragma once
#include <pthread.h>

namespace tarsx {

	class Condition;

	class Mutex {
	public:
		Mutex();
		~Mutex();

		auto lock() const -> void;
		auto tryLock() const -> bool;
		auto unlock() const -> void;
		auto willUnlock() const -> bool { return true; }

		Mutex(const Mutex&) = delete;
		void operator=(const Mutex&) = delete;
	private:
		friend class Condition;
		
		auto count() const -> int;
		auto count(int c) const -> void;
		mutable pthread_mutex_t mutex_;
	};

}