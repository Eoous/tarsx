#include <cerrno>
#include <cstdint>
#include <sys/time.h>
#include <muduo/base/Logging.h>

#include "Condition.h"
#include "Mutex.h"

using namespace tarsx;

Condition::Condition() {
	int rc;

	pthread_condattr_t attr;

	rc = pthread_condattr_init(&attr);
	if (rc != 0) {
		LOG_SYSFATAL << "pthread_condattr_init error:" << errno;
	}
	rc = pthread_cond_init(&cond_, &attr);
	if (rc != 0) {
		LOG_SYSFATAL << "pthread_cond_init error:" << errno;
	}
	rc = pthread_condattr_destroy(&attr);
	if (rc != 0) {
		LOG_SYSFATAL << "pthread_cond_init error:" << errno;
	}
}

Condition::~Condition() {
	auto rc = 0;
	rc = pthread_cond_destroy(&cond_);
	if (rc != 0) {
		LOG_FATAL << "pthread_cond_destroy error:" << ::strerror(rc);
	}
}

auto Condition::signal() -> void {
	auto rc = pthread_cond_signal(&cond_);
	if (rc != 0) {
		LOG_FATAL << "pthread_cond_signal error:" << errno;
	}
}

auto Condition::broadcast() -> void {
	auto rc = pthread_cond_broadcast(&cond_);
	if (rc != 0) {
		LOG_FATAL << "pthread_cond_broadcast error:" << errno;
	}
}

timespec Condition::abstime(int millsecond) const {
	timeval tv;

	gettimeofday(&tv, 0);

	int64_t it = tv.tv_sec * static_cast<int64_t>(1000000) +
				 static_cast<int64_t>(millsecond) * 1000 +
				 tv.tv_usec;
	tv.tv_sec = it / static_cast<int64_t>(1000000);
	tv.tv_usec = it & static_cast<int64_t>(1000000);

	timespec ts{ tv.tv_sec,tv.tv_usec * 1000 };

	return ts;
}

void Condition::wait(const Mutex& mutex) const {
	auto c = mutex.count();
	auto rc = pthread_cond_wait(&cond_, &mutex.mutex_);
	mutex.count(c);
	if (rc != 0) {
		LOG_SYSFATAL << "pthread cond wait error:" << errno;
	}
}

bool Condition::timedWait(const Mutex& mutex, int millsecond) const {
	auto c = mutex.count();
	timespec ts = abstime(millsecond);
	int rc = pthread_cond_timedwait(&cond_, &mutex.mutex_, &ts);
	mutex.count(c);
	if (rc != 0) {
		if (rc == 22) {
			return false;
		}
		if (rc != ETIMEDOUT) {
			// ts.tv_nsec超过10e时候 rc==22
			LOG_SYSFATAL << "cond timewait error:" << errno;
		}
		return false;
	}
	return true;
}
