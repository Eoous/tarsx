#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <assert.h>

#include "CoroutineScheduler.h"
#include "ServantHandle.h"
//#include "ServantProxy.h"

using namespace tarsx;

namespace details {
	auto pagesize() -> size_t {
		static size_t size = ::sysconf(_SC_PAGESIZE);
		return size;
	}

	auto stacksize_limit_() -> rlimit {
		rlimit limit;
		const auto result = ::getrlimit(RLIMIT_STACK, &limit);
		assert(0 == result);

		return limit;
	}

	auto stacksize_limit() -> rlimit {
		static rlimit limit = stacksize_limit_();
		return limit;
	}

	auto page_count(size_t stacksize) -> size_t {
		return static_cast<size_t>(ceil(static_cast<float>(stacksize) / pagesize()));
	}
}

auto standard_stack_allocator::is_stack_unbound() -> bool {
	return details::stacksize_limit().rlim_max == RLIM_INFINITY;
}

auto standard_stack_allocator::default_stacksize() -> size_t {
	auto size = 8 * minimum_stacksize();
	if (is_stack_unbound()) {
		return size;
	}
	assert(maximum_stacksize() >= minimum_stacksize());
	return maximum_stacksize() == size ? size : std::min(size, maximum_stacksize());
}

auto standard_stack_allocator::minimum_stacksize() -> size_t {
	return 8 * 1024 + sizeof(fcontext_t) + 15;
}

auto standard_stack_allocator::maximum_stacksize() -> size_t {
	assert(!is_stack_unbound());
	return static_cast<size_t>(details::stacksize_limit().rlim_max);
}

auto standard_stack_allocator::allocate(stack_context& stackContext, size_t size) -> int {
	assert(minimum_stacksize() <= size);
	assert(is_stack_unbound() || maximum_stacksize() >= size);

	const auto pages = details::page_count(size) + 1;
	const auto size_ = pages * details::pagesize();
	assert(size > 0 && size_ > 0);

	// mmap() 返回被映射区域指针
	void* limit = ::mmap(0, size_, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (limit == (void*)-1) {
		printf("standard_stack_allocator::allocate memory failed \n");
		return -1;
	}
	std::memset(limit, '\0', size_);

	// mprotect()用来修改一段指定内存区域
	const auto result = ::mprotect(limit, details::pagesize(), PROT_NONE);
	assert(result == 0);

	// 协程帧大小
	stackContext.size = size_;
	// 协程帧的位置 内存中栈底在高位
	stackContext.sp = static_cast<char*>(limit) + stackContext.size;

	return 0;
}

auto standard_stack_allocator::deallocate(stack_context& stackContext) -> void {
	assert(stackContext.sp);
	assert(minimum_stacksize() <= stackContext.size);
	assert(is_stack_unbound() || maximum_stacksize() >= stackContext.size);

	void* limit = static_cast<char*>(stackContext.sp) - stackContext.size;
	::munmap(limit, stackContext.size);
}

CoroutineInfo::CoroutineInfo(CoroutineScheduler* scheduler)
	:main_(false), scheduler_(scheduler) {

}

CoroutineInfo::CoroutineInfo(CoroutineScheduler* scheduler, uint32_t uid, stack_context stack_ctx)
	: main_(false), scheduler_(scheduler), uid_(uid), stack_ctx_(stack_ctx) {

}

auto CoroutineInfo::registerFunc(const std::function<void()>& callback) -> void {
	callback_ = callback;

	init_func_.coroFunc = CoroutineInfo::corotineProc;
	init_func_.args = this;

	// 保存了当前栈信息，但是没有寄存器信息
	// 就算是用过了的stack_ctx_ 之前的寄存器信息也只是会被覆盖而已
	ctx_to_ = make_fcontext(stack_ctx_.sp, stack_ctx_.size, CoroutineInfo::corotineEntry);
	// 先将当前各种寄存器(+rsp+返回值(PC值))保存到ctx_from_（第一个参数保存在rdi）
	// 再从ctx_to_ 恢复寄存器(+rsp+返回值(rip,PC值，函数入口))
	// 跳转到rcx(corotineEntry)
	jump_fcontext(&ctx_from_, ctx_to_, reinterpret_cast<intptr_t>(this), false);

}

auto CoroutineInfo::set_stackContext(stack_context stack_ctx) -> void {
	stack_ctx_ = stack_ctx;
}

auto CoroutineInfo::corotineEntry(intptr_t q) -> void {
	auto coro = reinterpret_cast<CoroutineInfo*>(q);

	auto func = coro->init_func_.coroFunc;
	void* args = coro->init_func_.args;

	// 先将各种寄存器(+rsp+返回值)保存到ctx_to_
	// 再从ctx_form_ 恢复寄存器(+rsp+返回值) ==> registerFunc()中jump_fcontext()之后程序
	jump_fcontext(coro->ctx_to_, &(coro->ctx_from_), 0, false);

	try {
		func(args);
	}
	catch (std::exception & ex) {
		//TLOGERROR("[TARS][CoroutineInfo::corotineEntry exception:" << ex.what() << endl);
	}
	catch (...) {
		//TLOGERROR("[TARS][CoroutineInfo::corotineEntry unknown exception." << endl);
	}
}

auto CoroutineInfo::corotineProc(void* args) -> void {
	auto coro = reinterpret_cast<CoroutineInfo*>(args);

	try {
		auto callback = coro->callback_;
		callback();
	}
	catch (std::exception & ex) {
		//TLOGERROR("[TARS][CoroutineInfo::corotineProc exception:" << ex.what() << endl);
	}
	catch (...) {
		//TLOGERROR("[TARS][CoroutineInfo::corotineProc unknown exception." << endl);
	}

	// 运行完之后放入freeQueue_
	auto scheduler = coro->get_scheduler();
	scheduler->decUsedSize();
	scheduler->moveToFreeList(coro);

	// 切换到主协程
	scheduler->switchCoro(coro, &scheduler->get_mainCoroutine());
	//TLOGERROR("[TARS][CoroutineInfo::corotineProc no come." << endl);
}

CoroutineScheduler::CoroutineScheduler() {
	//allCoro_ = new CoroutineInfo* [poolSize_];
	allCoro_.resize(poolSize_);
	for (auto& coro : allCoro_) {
		coro = nullptr;
	}

	CoroutineInfo::coroutineHeadInit(&active_);
	CoroutineInfo::coroutineHeadInit(&avail_);
	CoroutineInfo::coroutineHeadInit(&inactive_);
	CoroutineInfo::coroutineHeadInit(&timeout_);
	CoroutineInfo::coroutineHeadInit(&free_);
}

auto CoroutineScheduler::init(uint32_t poolSize, size_t stackSize) -> void {
	if (poolSize <= 0) {
		//TLOGERROR("[TARS][[CoroutineScheduler::init iPoolSize <= 0." << endl);
		return;
	}

	terminal_ = false;
	poolSize_ = poolSize;
	stackSize_ = stackSize;

	currentSize_ = (poolSize_ <= 100) ? poolSize_ : 100;

	if (!allCoro_.empty()) {
		allCoro_.clear();
		allCoro_.resize(poolSize_ + 1);
		for (auto& coro : allCoro_) {
			coro = nullptr;
		}
	}

	usedSize_ = 0;
	uniqId_ = 0;

	auto succ = 0;
	for (size_t i = 0; i < currentSize_; ++i) {
		auto coro = new CoroutineInfo(this);

		stack_context s_ctx;
		// 分配321页，每页4096B=4KB=4K
		auto ret = alloc_.allocate(s_ctx, stackSize);
		if (ret != 0) {
			//TLOGERROR("[TARS][CoroutineScheduler::init iPoolSize:" << iPoolSize << "|iStackSize:" << iStackSize << "|i:" << i << endl);
			delete coro;
			coro = nullptr;
			break;
		}
		coro->set_stackContext(s_ctx);

		auto id = generateId();
		coro->set_uid(id);
		coro->set_status(CORO_FREE);

		allCoro_[id] = coro;
		CoroutineInfo::coroutineAddTail(coro, &free_);
		++succ;
	}

	currentSize_ = succ;
	mainCoro_.set_uid(0);
	mainCoro_.set_status(CORO_FREE);

	currentCoro_ = &mainCoro_;
	//TLOGDEBUG("[TARS][CoroutineScheduler::init iPoolSize:" << _poolSize << "|iCurrentSize:" << _currentSize << "|iStackSize:" << _stackSize << endl);
}

auto CoroutineScheduler::increaseCoroPoolSize() -> int {
	auto inc = ((poolSize_ - currentSize_) > 100) ? 100 : (poolSize_ - currentSize_);

	if (inc <= 0) {
		//TLOGERROR("[TARS][CoroutineScheduler::increaseCoroPoolSize full iPoolSize:" << _poolSize << "|iCurrentSize:" << _currentSize << endl);
		return -1;
	}

	auto succ = 0;
	for (int i = 0; i < inc; ++i) {
		auto coro = new CoroutineInfo(this);
		auto id = generateId();
		coro->set_uid(id);
		coro->set_status(CORO_FREE);

		stack_context s_ctx;
		auto ret = alloc_.allocate(s_ctx, stackSize_);
		if (ret != 0) {
			//TLOGERROR("[TARS][CoroutineScheduler::increaseCoroPoolSize iPoolSize:" << _poolSize << "|iStackSize:" << _stackSize << "|i:" << i << endl);
			delete coro;
			coro = nullptr;
			break;
		}

		coro->set_stackContext(s_ctx);
		allCoro_[id] = coro;
		CoroutineInfo::coroutineAddTail(coro, &free_);
		++succ;
	}

	if (succ == 0) {
		//TLOGERROR("[TARS][CoroutineScheduler::increaseCoroPoolSize cannot create iInc:" << iInc << "|iPoolSize:" << _poolSize << endl);
		return -1;
	}

	currentSize_ += succ;
	return 0;
}

auto CoroutineScheduler::createCoroutine(const std::function<void()>& callback) -> uint32_t {
	if (usedSize_ >= currentSize_ || CoroutineInfo::coroutineHeadEmpty(&free_)) {
		auto ret = increaseCoroPoolSize();

		if (ret != 0) {
			return 0;
		}
	}

	auto coro = free_.next_;
	assert(coro != nullptr);

	CoroutineInfo::coroutineDel(coro);
	++usedSize_;

	coro->set_status(CORO_AVAIL);
	// 新创建的协程放入avail_里面
	CoroutineInfo::coroutineAddTail(coro, &avail_);
	// register coro function and stack context;
	coro->registerFunc(callback);
	return coro->get_uid();
}

auto CoroutineScheduler::run() -> void {
	while (!terminal_) {
		// 如果两个列表为空
		if (CoroutineInfo::coroutineHeadEmpty(&avail_) && CoroutineInfo::coroutineHeadEmpty(&active_)) {
			Lock lock(monitor_);
			if (activeCoroQueue_.size() <= 0) {
				monitor_.timedWait(1000);
			}
		}

		wakeupbytimeout();
		wakeupbyself();
		wakeup();

		// 活跃协程链表非空
		if (!CoroutineInfo::coroutineHeadEmpty(&active_)) {
			auto loop = 100;
			while (loop > 0 && !CoroutineInfo::coroutineHeadEmpty(&active_)) {
				auto coro = active_.next_;
				assert(coro != nullptr);
				// 切换协程
				switchCoro(&mainCoro_, coro);
				--loop;
			}
		}

		if (!CoroutineInfo::coroutineHeadEmpty(&avail_)) {
			auto coro = avail_.next_;
			assert(coro != nullptr);
			switchCoro(&mainCoro_, coro);
		}

		if (usedSize_ == 0) {
			break;
		}
	}
	destroy();
}

auto CoroutineScheduler::tars_run() -> void {
	if (!terminal_) {
		wakeupbytimeout();
		wakeupbyself();
		wakeup();
		if (!CoroutineInfo::coroutineHeadEmpty(&active_)) {
			auto loop = 100;
			while (loop > 0 && !CoroutineInfo::coroutineHeadEmpty(&active_)) {
				auto coro = active_.next_;
				assert(coro != nullptr);
				switchCoro(&mainCoro_, coro);
				--loop;
			}
		}

		if (!CoroutineInfo::coroutineHeadEmpty(&avail_)) {
			auto loop = 100;
			while (loop > 0 && !CoroutineInfo::coroutineHeadEmpty(&avail_)) {
				auto coro = avail_.next_;
				assert(coro != nullptr);
				switchCoro(&mainCoro_, coro);
				--loop;
			}
		}
	}
}

auto CoroutineScheduler::yield(bool flag) -> void {
	if (flag) {
		// 加入needActiveQueue_
		putbyself(currentCoro_->get_uid());
	}
	moveToInactive(currentCoro_);
	switchCoro(currentCoro_, &mainCoro_);
}

auto CoroutineScheduler::sleep(int sleeptime) -> void {
	struct timeval tv;
	::gettimeofday(&tv, nullptr);

	auto now = tv.tv_sec * static_cast<int64_t>(1000) + tv.tv_usec / 1000;
	auto timeout = now + ((sleeptime >= 0) ? sleeptime : -sleeptime);

	timeoutCoroId_.insert(std::make_pair(timeout, currentCoro_->get_uid()));

	moveToTimeout(currentCoro_);
	switchCoro(currentCoro_, &mainCoro_);
}

auto CoroutineScheduler::putbyself(uint32_t coroId) -> void {
	if (!terminal_) {
		needActiveCoroId_.push_back(coroId);
	}
}

auto CoroutineScheduler::wakeupbyself() -> void {
	if (!terminal_) {
		if (needActiveCoroId_.size() > 0) {
			for (auto& it : needActiveCoroId_) {
				auto coro = allCoro_[it];
				assert(coro != nullptr);
				moveToAvail(coro);
			}
			needActiveCoroId_.clear();
		}
	}
}

auto CoroutineScheduler::put(uint32_t coroId) -> void {
	if (!terminal_) {
		activeCoroQueue_.push_back(coroId);
		if (handle_) {
			//handle_->notifyFilter();
		}
		else {
			Lock lock(monitor_);
			monitor_.notifyAll();
		}
	}
}

auto CoroutineScheduler::wakeup() -> void {
	if (!terminal_) {
		if (activeCoroQueue_.size() > 0) {
			std::deque<uint32_t> coroIds;
			activeCoroQueue_.swap(coroIds);

			for (auto& it : coroIds) {
				auto coro = allCoro_[it];
				assert(coro != nullptr);
				moveToActive(coro);
			}
		}
	}
}

auto CoroutineScheduler::wakeupbytimeout() -> void {
	if (!terminal_) {
		if (timeoutCoroId_.size() > 0) {
			timeval tv;
			::gettimeofday(&tv, nullptr);

			auto now = tv.tv_sec * static_cast<int64_t>(1000) + tv.tv_usec / 1000;
			while (true) {
				auto it = timeoutCoroId_.begin();
				if (it == timeoutCoroId_.end() || it->first > now) {
					break;
				}
				auto coro = allCoro_[it->second];
				assert(coro != nullptr);
				moveToActive(coro);
				timeoutCoroId_.erase(it);
			}
		}
	}
}

auto CoroutineScheduler::terminate() -> void {
	terminal_ = true;
	if (handle_) {
		//handle_->notifyFilter();
	}
	else {
		Lock lock(monitor_);
		monitor_.notifyAll();
	}
}

auto CoroutineScheduler::generateId() -> uint32_t {
	while (++uniqId_ < 1);
	assert(uniqId_ <= poolSize_);
	return uniqId_;
}

auto CoroutineScheduler::switchCoro(CoroutineInfo* from, CoroutineInfo* to) -> void {
	currentCoro_ = to;
	jump_fcontext(from->get_ctx(), to->get_ctx(), 0, false);
}

auto CoroutineScheduler::moveToActive(CoroutineInfo* coro, bool flag) -> void {
	if (coro->get_status() == CORO_INACTIVE) {
		CoroutineInfo::coroutineDel(coro);
		coro->set_status(CORO_ACTIVE);
		CoroutineInfo::coroutineAddTail(coro, &active_);
	}
	else if (coro->get_status() == CORO_TIMEOUT) {
		CoroutineInfo::coroutineDel(coro);
		coro->set_status(CORO_ACTIVE);
		CoroutineInfo::coroutineAddTail(coro, &active_);
	}
	else {
		//TLOGERROR("[TARS][CoroutineScheduler::moveToActive ERROR|iCoroId:" << coro->getUid() << "|tyep:" << coro->getStatus() << endl);
	}
}

auto CoroutineScheduler::moveToAvail(CoroutineInfo* coro) -> void {
	if (coro->get_status() == CORO_INACTIVE) {
		CoroutineInfo::coroutineDel(coro);
		coro->set_status(CORO_AVAIL);
		CoroutineInfo::coroutineAddTail(coro, &avail_);
	}
	else {
		//TLOGERROR("[TARS][CoroutineScheduler::moveToAvail ERROR:|iCoroId:" << coro->getUid() << "|tyep:" << coro->getStatus() << endl);
	}
}

auto CoroutineScheduler::moveToInactive(CoroutineInfo* coro) -> void {
	if (coro->get_status() == CORO_ACTIVE) {
		CoroutineInfo::coroutineDel(coro);
		coro->set_status(CORO_INACTIVE);
		CoroutineInfo::coroutineAddTail(coro, &inactive_);
	}
	else if (coro->get_status() == CORO_AVAIL) {
		CoroutineInfo::coroutineDel(coro);
		coro->set_status(CORO_INACTIVE);
		CoroutineInfo::coroutineAddTail(coro, &inactive_);
	}
	else {
		//TLOGERROR("[TARS][CoroutineScheduler::moveToInactive ERROR|iCoroId:" << coro->getUid() << "|tyep:" << coro->getStatus() << endl);
	}
}

auto CoroutineScheduler::moveToTimeout(CoroutineInfo* coro) -> void {
	if (coro->get_status() == CORO_ACTIVE) {
		CoroutineInfo::coroutineDel(coro);
		coro->set_status(CORO_TIMEOUT);
		CoroutineInfo::coroutineAddTail(coro, &timeout_);
	}
	else if (coro->get_status() == CORO_AVAIL) {
		CoroutineInfo::coroutineDel(coro);
		coro->set_status(CORO_TIMEOUT);
		CoroutineInfo::coroutineAddTail(coro, &timeout_);
	}
	else {
		//TLOGERROR("[TARS][CoroutineScheduler::moveToTimeout ERROR|iCoroId:" << coro->getUid() << "|tyep:" << coro->getStatus() << endl);
	}
}

auto moveTo(CoroutineInfo* coro, CoroutineInfo* list, CORO_STATUS status) {
	CoroutineInfo::coroutineDel(coro);
	coro->set_status(status);
	CoroutineInfo::coroutineAddTail(coro, list);
}

auto CoroutineScheduler::moveToFreeList(CoroutineInfo* coro) -> void {
	if (coro->get_status() == CORO_ACTIVE) {
		moveTo(coro, &free_, CORO_FREE);
	}
	else if (coro->get_status() == CORO_AVAIL) {
		moveTo(coro, &free_, CORO_FREE);
	}
	else if (coro->get_status() == CORO_INACTIVE) {
		moveTo(coro, &free_, CORO_FREE);
	}
	else if (coro->get_status() == CORO_TIMEOUT) {
		moveTo(coro, &free_, CORO_FREE);
	}
	else {
		//TLOGERROR("[TARS][CoroutineScheduler::moveToFreeList ERROR: already free|iCoroId:" << coro->getUid() << "|tyep:" << coro->getStatus() << endl);
	}
}

auto CoroutineScheduler::destroy() -> void {
	if (!allCoro_.empty()) {
		for (size_t i = 1; i <= poolSize_; ++i) {
			if (allCoro_[i]) {
				alloc_.deallocate(allCoro_[i]->get_stackContext());
			}
		}
		// allCoro_是vector 会自动收回
	}
}
//
//Coroutine::~Coroutine() {
//	if (alive()) {
//		terminate();
//		getThreadControl().join();
//	}
//}
//
//auto Coroutine::set_coroInfo(uint32_t num, uint32_t maxNum, size_t stackSize) -> void {
//	maxNum_ = ((maxNum > 0) ? maxNum : 1);
//	auto temp = (num <= maxNum_) ? num : maxNum_;
//	num_ = ((num > 0) ? temp : 1);
//	stackSize_ = ((stackSize >= details::pagesize()) ? stackSize : details::pagesize());
//}
//
//auto Coroutine::run() -> void {
//	initialize();
//	handleCoro();
//	destroy();
//}
//
//auto Coroutine::terminate() -> void {
//	if (coroSched_) {
//		coroSched_->terminate();
//	}
//}
//
//auto Coroutine::handleCoro() -> void {
//	coroSched_ = new CoroutineScheduler();
//	coroSched_->init(maxNum_, stackSize_);
//
//	ServantProxyThreadData* thread = ServantProxyThreadData::get_data();
//	assert(thread != nullptr);
//
//	thread->scheduler_ = coroSched_;
//	for (auto i = 0; i < num_; ++i) {
//		coroSched_->createCoroutine(std::bind(&Coroutine::coroEntry, this));
//	}
//
//	coroSched_->run();
//	delete coroSched_;
//	coroSched_ = nullptr;
//}
//
//auto Coroutine::coroEntry(Coroutine* coro) -> void {
//	try {
//		coro->handle();
//	}
//	catch (std::exception & e) {
//		//TLOGERROR("[TARS][[Coroutine::coroEntry exception:" << ex.what() << "]" << endl);
//	}
//	catch (...) {
//		//TLOGERROR("[TARS][[Coroutine::coroEntry unknown exception]" << endl);
//	}
//}
//
//auto Coroutine::createCoroutine(const std::function<void()>& corofunc) -> uint32_t {
//	if (coroSched_) {
//		return coroSched_->createCoroutine(corofunc);
//	}
//	else {
//		//TLOGERROR("[TARS][[Coroutine::createCoroutine coro sched no init]" << endl);
//	}
//	return -1;
//}
//
//auto Coroutine::yield() -> void {
//	if (coroSched_) {
//		coroSched_->yield();
//	}
//	else {
//		throw CoroutineException("[tarsl][Coroutine::yield coro sched no init]");
//	}
//}
//
//auto Coroutine::sleep(int sleeptime) -> void {
//	if (coroSched_) {
//		coroSched_->sleep(sleeptime);
//	}
//	else {
//		throw CoroutineException("[tarsl][Coroutine::yield coro sched no init]");
//	}
//}
