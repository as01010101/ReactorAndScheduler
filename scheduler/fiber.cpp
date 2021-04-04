#include "fiber.h"

#include "scheduler.h"
#include <atomic>



	

	static std::atomic<uint64_t> s_fiber_id{ 0 };
	static std::atomic<uint64_t> s_fiber_count{ 0 };

	static thread_local Fiber* t_fiber = nullptr;
	static thread_local Fiber::ptr t_threadFiber = nullptr;

	class MallocStackAllocator {
	public:
		static void* Alloc(size_t size) {
			return malloc(size);
		}

		static void Dealloc(void* vp, size_t size) {
			return free(vp);
		}
	};


	using StackAllocator = MallocStackAllocator;

	uint64_t Fiber::GetFiberId() {
		if (t_fiber) {
			return t_fiber->getId();
		}
		return 0;
	}

	Fiber::Fiber() {
		printf("function name :%s\n", __func__);
		m_state = EXEC;
		SetThis(this);

		if (getcontext(&m_ctx)) {
			printf("getcontext  error\n");
		}
		++s_fiber_count;

		
	}

	Fiber::Fiber(std::function<void()> cb, size_t stacksize, bool use_caller)
		:m_id(++s_fiber_id)
		, m_cb(cb) {
		printf("function name :%s  whith para\n ", __func__);;
		++s_fiber_count;
		m_stacksize = stacksize ? stacksize : 1024 * 1024;

		m_stack = StackAllocator::Alloc(m_stacksize);
		if (getcontext(&m_ctx)) {
			printf( "getcontext  error\n");
		}
		m_ctx.uc_link = nullptr;
		m_ctx.uc_stack.ss_sp = m_stack;
		m_ctx.uc_stack.ss_size = m_stacksize;

		if (!use_caller) {
			makecontext(&m_ctx, &Fiber::MainFunc, 0);
		}
		else {
			makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
		}
		printf("Fiber::Fiber id=%d", m_id);

	
	}

	Fiber::~Fiber() {
		printf("function name :%s\n", __func__);
		--s_fiber_count;
		if (m_stack) {
	

			StackAllocator::Dealloc(m_stack, m_stacksize);
		}
		else {
			

			Fiber* cur = t_fiber;
			if (cur == this) {
				SetThis(nullptr);
			}
		}
		printf( "Fiber::~Fiber id=%d" ,m_id);
	}

	//重置协程函数，并重置状态
	//INIT，TERM
	void Fiber::reset(std::function<void()> cb) {
		printf("function name :%s\n", __func__);
		m_cb = cb;
		if (getcontext(&m_ctx)) {
			printf("getcontext  error\n");
		}

		m_ctx.uc_link = nullptr;
		m_ctx.uc_stack.ss_sp = m_stack;
		m_ctx.uc_stack.ss_size = m_stacksize;

		makecontext(&m_ctx, &Fiber::MainFunc, 0);
		m_state = INIT;
	}

	void Fiber::call() {
		printf("function name :%s\n", __func__);
		SetThis(this);
		m_state = EXEC;
		printf("call id :%d\n", getId());

		if (swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
			printf("swapcontext  error\n");
		}
	}

	void Fiber::back() {
		printf("function name :%s\n", __func__);
		SetThis(t_threadFiber.get());
		if (swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
			printf("swapcontext  error\n");
		}
	}

	//切换到当前协程执行
	void Fiber::swapIn() {

		SetThis(this);
		
		m_state = EXEC;
		if (swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
			printf("swapcontext  error\n");
		}
	}

	//切换到后台执行
	void Fiber::swapOut() {

		SetThis(Scheduler::GetMainFiber());
		if (swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
			printf("swapcontext  error\n");
		}
	}

	//设置当前协程
	void Fiber::SetThis(Fiber* f) {
		t_fiber = f;
	}

	//返回当前协程
	Fiber::ptr Fiber::GetThis() {
		if (t_fiber) {
			return t_fiber->shared_from_this();
		}
		Fiber::ptr main_fiber(new Fiber);
		
		t_threadFiber = main_fiber;
		return t_fiber->shared_from_this();
	}

	//协程切换到后台，并且设置为Ready状态
	void Fiber::YieldToReady() {
		Fiber::ptr cur = GetThis();
		cur->m_state = READY;
		cur->swapOut();
	}

	//协程切换到后台，并且设置为Hold状态
	void Fiber::YieldToHold() {
		Fiber::ptr cur = GetThis();
		cur->m_state = HOLD;
		cur->swapOut();
	}

	//总协程数
	uint64_t Fiber::TotalFibers() {
		return s_fiber_count;
	}

	void Fiber::MainFunc() {
		printf("function name :%s\n", __func__);
		Fiber::ptr cur = GetThis();
			cur->m_cb();
			cur->m_cb = nullptr;
			cur->m_state = TERM;

		auto raw_ptr = cur.get();
		cur.reset();
		raw_ptr->swapOut();

	}

	void Fiber::CallerMainFunc() {
		printf("function name :%s\n", __func__);
		Fiber::ptr cur = GetThis();

			cur->m_cb();
			cur->m_cb = nullptr;
			cur->m_state = TERM;

		auto raw_ptr = cur.get();
		cur.reset();
		raw_ptr->back();
		printf("never reach \n");

	}


