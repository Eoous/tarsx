#include "Mutex.h"

#include <cassert>
#include <cstring>
#include <cerrno>
#include <muduo/base/Logging.h>

using namespace tarsx;

Mutex::Mutex() {
	pthread_mutexattr_t attr;
	auto rc = pthread_mutexattr_init(&attr);
	assert(rc == 0);

	rc = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
	assert(rc == 0);

	rc = pthread_mutex_init(&mutex_, &attr);
	assert(rc == 0);

	rc = pthread_mutexattr_destroy(&attr);
	assert(rc == 0);

	if (rc != 0) {
		LOG_SYSFATAL << "pthread_mutexattr_init error";
	}
}

Mutex::~Mutex() {
	auto rc = 0;
	rc = pthread_mutex_destroy(&mutex_);
	if (rc != 0) {
		LOG_SYSFATAL << "pthread_mutex_destroy error:" << ::strerror(rc);
	}
}

auto Mutex::lock() const -> void {
	auto rc = pthread_mutex_lock(&mutex_);
	if (rc != 0) {
		if (rc == EDEADLK) {
			LOG_SYSFATAL << "mutex dead lock error:" << rc;
		}
		else {
			LOG_SYSFATAL << "mutex lock error:" << rc;
		}
	}
}

auto Mutex::tryLock() const -> bool {
	auto rc = pthread_mutex_trylock(&mutex_);
	if (rc != 0 && rc != EBUSY) {
		if (rc == EDEADLK) {
			LOG_SYSFATAL << "trylock dead lock error:" << rc;
		}
		else {
			LOG_SYSFATAL << "trylock error:" << rc;
		}
	}
	return rc == 0;
}

auto Mutex::unlock() const -> void {
	auto rc = pthread_mutex_unlock(&mutex_);
	if (rc != 0) {
		LOG_SYSFATAL << "unlock error" << rc;
	}
}

auto Mutex::count() const -> int {
	return 0;
}

auto Mutex::count(int c) const -> void {

}
