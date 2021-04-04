#include "thread.h"
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>

	static thread_local Thread* t_thread = nullptr;
	static thread_local std::string t_thread_name = "UNKNOW";

	Semaphore::Semaphore(uint32_t count) {
		if (sem_init(&m_semaphore, 0, count)) {
			printf("sem_init error\n");
		}
	}

	Semaphore::~Semaphore() {
		sem_destroy(&m_semaphore);
	}

	void Semaphore::wait() {
		if (sem_wait(&m_semaphore)) {
			printf("sem_wait error");
		}
	}

	void Semaphore::notify() {
		if (sem_post(&m_semaphore)) {
			printf("sem_post error");
		}
	}

	Thread* Thread::GetThis() {
		return t_thread;
	}

	const std::string& Thread::GetName() {
		return t_thread_name;
	}

	void Thread::SetName(const std::string& name) {
		if (t_thread) {
			t_thread->m_name = name;
		}
		t_thread_name = name;
	}

	Thread::Thread(std::function<void()> cb, const std::string& name)
		:m_cb(cb)
		, m_name(name) {
		if (name.empty()) {
			m_name = "UNKNOW";
		}
		int rt = pthread_create(&m_thread, nullptr, &Thread::run, this);
		if (rt) {

			printf("pthread_create error");
		}
		m_semaphore.wait();
	}

	Thread::~Thread() {
		if (m_thread) {
			pthread_detach(m_thread);
		}
	}

//	pid_t Thread::GetThreadId() {
//		return syscall(SYS_gettid);
//	}
	void Thread::join() {
		if (m_thread) {
			int rt = pthread_join(m_thread, nullptr);
			if (rt) {

				printf("pthread_join error");
			}
			m_thread = 0;
		}
	}

	void* Thread::run(void* arg) {
		Thread* thread = (Thread*)arg;
		t_thread = thread;
		
		std::function<void()> cb;
		cb.swap(thread->m_cb);
		thread->m_semaphore.notify();
		cb();
		return 0;
	}

