#pragma once
#include <pthread.h>

namespace tarsx {

	class Mutex;
	class Condition {
	public:
		Condition();
		~Condition();

		auto signal() -> void;
		auto broadcast() ->void;

		timespec abstime(int millsecond) const;

		void wait(const Mutex& mutex) const;

		bool timedWait(const Mutex& mutex, int millsecond) const;

	private:
		Condition(const Condition&);
		Condition& operator=(const Condition&);
	private:
		mutable pthread_cond_t cond_;
	};
}

