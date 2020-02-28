#pragma once
#include <list>
#include <deque>
#include <map>
#include <cstddef>
#include <functional>
#include <vector>


#include "Monitor.hpp"
#include "ThreadDeque.hpp"
#include "tc_fcontext.h"

namespace tarsx {
	class ServantHandle;

	// 栈内容信息
	struct stack_context {
		size_t size = 0;
		void* sp = 0;
	};

	// 栈内存分配器
	class standard_stack_allocator {
	public:
		static auto is_stack_unbound() -> bool;

		static auto default_stacksize()->size_t;
		static auto minimum_stacksize()->size_t;
		static auto maximum_stacksize()->size_t;

		auto allocate(stack_context& stackContext, size_t x) -> int;
		auto deallocate(stack_context& stackContext) -> void;
	};

	// 状态信息
	enum CORO_STATUS {
		CORO_FREE = 0,
		CORO_ACTIVE,
		CORO_AVAIL,
		CORO_INACTIVE,
		CORO_TIMEOUT
	};

	// 协程内部函数
	typedef void(*Func)(void*);

	struct CoroutineFunc {
		Func coroFunc;
		void* args;
	};

	class CoroutineScheduler;

	// 协程信息类
	class CoroutineInfo {
	public:
		// 链表初始化
		static auto coroutineHeadInit(CoroutineInfo* coro) {
			coro->next_ = coro;
			coro->prev_ = coro;
		}

		// 链表是否为空
		static auto coroutineHeadEmpty(CoroutineInfo* coro_head) {
			return coro_head->next_ == coro_head;
		}

		// 链表插入
		static auto __coroutineAdd(CoroutineInfo* coro, CoroutineInfo* prev, CoroutineInfo* next) {
			next->prev_ = coro;
			coro->next_ = next;
			coro->prev_ = prev;
			prev->next_ = coro;
		}

		// 插入头部
		static auto coroutineAdd(CoroutineInfo* new_coro, CoroutineInfo* coro_head) {
			__coroutineAdd(new_coro, coro_head, coro_head->next_);
		}

		// 插入尾部
		static auto coroutineAddTail(CoroutineInfo* new_coro, CoroutineInfo* coro_head) {
			__coroutineAdd(new_coro, coro_head->prev_, coro_head);
		}

		// 删除
		static auto __coroutineDel(CoroutineInfo* prev, CoroutineInfo* next) {
			next->prev_ = prev;
			prev->next_ = next;
		}

		static auto coroutineDel(CoroutineInfo* coro) {
			__coroutineDel(coro->prev_, coro->next_);
			coro->next_ = nullptr;
			coro->prev_ = nullptr;
		}

		// 从一个链表移动到另一个链表头部
		static auto coroutineMove(CoroutineInfo* coro, CoroutineInfo* coro_head) {
			coroutineDel(coro);
			coroutineAdd(coro, coro_head);
		}

		// 从一个链表移动到另一个链表尾部
		static auto coroutineMoveTail(CoroutineInfo* coro, CoroutineInfo* coro_head) {
			coroutineDel(coro);
			coroutineAddTail(coro, coro_head);
		}

	protected:
		// 协程的入口函数
		static auto corotineEntry(intptr_t q) -> void;

		// 协程里执行实际逻辑的入口函数
		static auto corotineProc(void* args) -> void;

	public:
		CoroutineInfo() = default;
		CoroutineInfo(CoroutineScheduler* scheduler);
		CoroutineInfo(CoroutineScheduler* scheduler, uint32_t uid, stack_context stack_ctx);
		~CoroutineInfo() = default;

		auto registerFunc(const std::function<void()>& callback) -> void;
		auto set_stackContext(stack_context stack_ctx) -> void;
		auto& get_stackContext() {
			return stack_ctx_;
		}
		auto get_scheduler() {
			return scheduler_;
		}
		auto set_uid(uint32_t uid) {
			uid_ = uid;
		}
		auto get_uid() {
			return uid_;
		}
		auto set_status(CORO_STATUS status) {
			status_ = status;
		}
		auto get_status() {
			return status_;
		}
		auto get_ctx() {
			return ((!main_) ? ctx_to_ : &ctx_from_);
		}
	public:
		// 双向链表指针
		CoroutineInfo* prev_ = nullptr;
		CoroutineInfo* next_ = nullptr;
	private:
		// 是否是主协程
		bool main_ = true;
		//协程所属调度器
		CoroutineScheduler* scheduler_ = nullptr;
		// 协程的标识
		uint32_t uid_ = 0;
		// 协程的状态
		CORO_STATUS status_ = CORO_FREE;
		// 协程的内存空间
		stack_context stack_ctx_;
		// 创建协程后，协程所在的上下文
		fcontext_t* ctx_to_ = nullptr;
		// 创建携程前的上下文
		fcontext_t ctx_from_;
		// 协程初始化函数入口函数
		CoroutineFunc init_func_;
		// 协程具体执行函数
		std::function<void()> callback_;
	};

	class CoroutineScheduler {
	public:
		CoroutineScheduler();
		~CoroutineScheduler() = default;

		auto init(uint32_t poolSize, size_t stackSize) -> void;
		auto createCoroutine(const std::function<void()>& callback)->uint32_t;

		// 在用户自己起的线程中使用协程时，用到的协程调度
		auto run() -> void;

		// tars业务线程使用协程时，用的协程调度
		auto tars_run() -> void;

		// 当前协程放弃执行
		auto yield(bool flag = true) -> void;
		// 当前协程休眠sleeptime(毫秒),然后会被唤醒继续执行
		auto sleep(int sleeptime) -> void;

		auto put(uint32_t coroId) -> void;
		auto switchCoro(CoroutineInfo* from, CoroutineInfo* to) -> void;
		auto terminate() -> void;
		auto destroy() -> void;

		auto get_poolSize() { return poolSize_; }
		auto get_currentSize() { return currentSize_; }
		auto get_responseCoroSize() { return activeCoroQueue_.size(); }
		auto set_handle(ServantHandle* handle) { handle_ = handle; }
		auto get_handle() { return handle_; }
		auto get_freeSize() { return poolSize_ - usedSize_; }
		// 减少正在使用的协程数目
		auto decUsedSize() { --usedSize_; }
		auto incUsedSize() { ++usedSize_; }

		// 调度器中的主协程
		auto& get_mainCoroutine() { return mainCoro_; }
		// 当前协程的标识id
		auto get_coroutineId() { return currentCoro_->get_uid(); }

		friend class CoroutineInfo;
	private:
		// 生成协程id
		auto generateId()->uint32_t;
		auto increaseCoroPoolSize() -> int;
		// 唤醒需要运行的协程
		auto wakeup() -> void;
		// 唤醒自己放弃运行的协程
		auto wakeupbyself() -> void;
		// 自己放弃运行时会用到
		auto putbyself(uint32_t coroId) -> void;
		// 唤醒休眠的协程
		auto wakeupbytimeout() -> void;
		// 放到active的协程链表中
		auto moveToActive(CoroutineInfo* coro, bool flag = false) -> void;
		auto moveToAvail(CoroutineInfo* coro) -> void;
		auto moveToInactive(CoroutineInfo* coro) -> void;
		auto moveToTimeout(CoroutineInfo* coro) -> void;
		auto moveToFreeList(CoroutineInfo* coro) -> void;
	private:
		bool terminal_ = false;
		uint32_t poolSize_ = 1000;
		size_t stackSize_ = 128 * 1024;
		// 当前创建的协程数
		uint32_t currentSize_ = 0;
		uint32_t usedSize_ = 0;
		uint32_t uniqId_ = 0;
		ServantHandle* handle_ = nullptr;
		CoroutineInfo mainCoro_;
		CoroutineInfo* currentCoro_ = nullptr;
		//CoroutineInfo** allCoro_ = nullptr;
		std::vector<CoroutineInfo*> allCoro_;
		// 活跃的协程链表
		CoroutineInfo active_;
		// 可用的协程链表
		CoroutineInfo avail_;
		// 不活跃的协程链表
		CoroutineInfo inactive_;
		// 超时的协程链表
		CoroutineInfo timeout_;
		// 空闲的协程链表
		CoroutineInfo free_;

		// 协程栈空间的内存分配器
		standard_stack_allocator alloc_;

		// 需要激活的协程队列，其他线程使用，用来激活等待结果的协程
		ThreadDeque<uint32_t, std::deque<uint32_t>> activeCoroQueue_;
		// 锁通知
		ThreadLock monitor_;
		// 需要激活的协程队列，本线程使用
		std::list<uint32_t> needActiveCoroId_;
		// 存放超时的协程 second是allCoro_的下标
		std::multimap<int64_t, uint32_t> timeoutCoroId_;
	};

	// 对线程进行包装的协程类，主要用于在自己起的线程中使用此协程，业务可以继承这个类
	class Coroutine {
	public:
		Coroutine() = default;
		virtual ~Coroutine();

		// num:表示要运行多个协程，即会有这个类多少个coroRun运行
		// maxNum:表示这个线程最多包含的协程个数
		// stackSize:协程栈大小
		auto set_coroInfo(uint32_t num, uint32_t maxNum, size_t stackSize) -> void;

		// 创建协程，在已经创建的协程中使用
		// 返回协程id, >0 = 成功 ， <=0 = 失败
		auto createCoroutine(const std::function<void()>& corofunc)->uint32_t;

		// 当前协程自己放弃执行，会自动被调度器唤醒
		// 在已经创建的协程中使用
		auto yield() -> void;

		// 当前协程休眠sleeptime(毫秒) 时间到了，会自动被调度器唤醒
		// 在已经创建的协程中使用
		auto sleep(int sleeptime) -> void;

		auto get_coroSched() { return coroSched_; }
		auto get_maxCoroNum() { return maxNum_; }
		auto get_coroNum() { return num_; }
		auto get_coroStackSize() { return stackSize_; }

		auto terminate() -> void;
	protected:
		// 线程处理方法
		auto run() -> void;
		// 静态函数，协程入口
		static auto coroEntry(Coroutine* coro) -> void;
		// 协程运行的函数，根据num_的数目，会启动num_个这个函数
		auto handle() -> void;

		// 线程已经启动，进入协程处理前调用
		auto initialize() -> void {}
		// 所有协程停止运行之后，线程退出之前时调用
		auto destroy() -> void {}
		// 具体的处理逻辑
		auto handleCoro() -> void;
	private:
		CoroutineScheduler* coroSched_ = nullptr;
		uint32_t num_ = 1;
		uint32_t maxNum_ = 128;
		uint32_t stackSize_ = 128 * 1024;
	};
}