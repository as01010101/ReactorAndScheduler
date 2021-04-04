#pragma once
#include <thread>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <atomic>



	class Semaphore {
	public:
		Semaphore(uint32_t count = 0);
		~Semaphore();

		void wait();
		void notify();
	private:
		Semaphore(const Semaphore&) = delete;
		Semaphore(const Semaphore&&) = delete;
		Semaphore& operator=(const Semaphore&) = delete;
	private:
		sem_t m_semaphore;
	};


	class Thread {
	public:
		typedef std::shared_ptr<Thread> ptr;
		Thread(std::function<void()> cb, const std::string& name);
		~Thread();


		const std::string& getName() const { return m_name; }

		void join();
		pthread_t getId() { return m_thread; }
		static Thread* GetThis();
		static const std::string& GetName();
		static void SetName(const std::string& name);
	private:
		Thread(const Thread&) = delete;
		Thread(const Thread&&) = delete;
		Thread& operator=(const Thread&) = delete;

		static void* run(void* arg);
	private:
	
		pthread_t m_thread = 0;
		std::function<void()> m_cb;
		std::string m_name;

		Semaphore m_semaphore;
	};



