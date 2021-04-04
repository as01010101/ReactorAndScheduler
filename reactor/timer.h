
#pragma once
#include <unistd.h>
#include <deque>
#include <memory>
#include <queue>
#include <functional>
#include <sys/time.h>
using namespace std;
template <class T>
class TimerNode {
public:
	TimerNode(T  requestData, unsigned long timeout);
	~TimerNode();
	size_t getExpTime() const { return expiredTime_; }
	function<void()> cb;
	bool deleted_;
	unsigned long expiredTime_;
	
};

template <class T>
struct TimerCmp {
	bool operator()(std::shared_ptr<TimerNode<T>> &a,
		std::shared_ptr<TimerNode<T>> &b) const {
		return a->getExpTime() > b->getExpTime();
	}
};
template <class T1> // T1  --time called woud do the thing  , eg:  cb()       T2 -- the cmp  for TimerNode      
class TimerManager {
public:
	TimerManager();
	~TimerManager();
	size_t addTimer(T1  SPHttpData, unsigned long timeout);

	void handleExpiredEvent();
//private:
	typedef std::shared_ptr<TimerNode<T1>> SPTimerNode;
	std::priority_queue<SPTimerNode, std::deque<SPTimerNode>, TimerCmp<T1>>
		timerNodeQueue;

};

template<class T>  ///notice herer
TimerNode<T>::TimerNode(T requestData, unsigned long timeout) ///not  TimerNode<class T> !!!
	: deleted_(false),cb(requestData) {
	struct timeval now;
	gettimeofday(&now, NULL);
	unsigned long   temp = now.tv_sec * 1000 + now.tv_usec / 1000;
	// mseconds
	//expiredTime_ = now.tv_sec * 1000 + now.tv_usec / 1000;
	expiredTime_=temp+timeout;

	//	(((now.tv_sec % 10000) * 1000) + (now.tv_usec / 1000)) + timeout;
}

template<class T>
TimerNode<T>::~TimerNode() {
	
}

template<class T>
TimerNode< T>::TimerNode(TimerNode &tn)
	: expiredTime_(5000) {}

template<class T>
bool TimerNode< T>::isValid() {
	struct timeval now;
	gettimeofday(&now, NULL);
	unsigned  temp = now.tv_sec * 1000ul + now.tv_usec / 1000;
	if (temp < expiredTime_)
		return true;
	else {
		this->setDeleted();
		return false;
	}
}

template<class T>
void TimerNode< T>::clearReq() {

	this->setDeleted();
}

template <class T1>
TimerManager< T1>::TimerManager() {}

template <class T1>
TimerManager< T1>::~TimerManager() {}

template <class T1>
size_t  TimerManager<T1>::addTimer(T1  SPHttpData, unsigned long timeout) {
	SPTimerNode new_node(new TimerNode<T1>(SPHttpData, timeout));
	timerNodeQueue.push(new_node);
	return new_node->expiredTime_;
	
}

template <class T1>
void TimerManager<T1>::handleExpiredEvent() {
	// MutexLockGuard locker(lock);
	while (!timerNodeQueue.empty()) {
		SPTimerNode ptimer_now = timerNodeQueue.top();
		if (ptimer_now->isDeleted())
			timerNodeQueue.pop();
		else if (ptimer_now->isValid() == false)
			timerNodeQueue.pop();
		else
			break;
	}
}
//// usage :
//#include <functional>
//using namespace std;
//
//void test()
//{
//	
//}
//
//int main()
//{
//	
//
//	TimerManager<function<void() >, TimerCmp<function<void() >> > ss;
//	function<void() > cb =test ;
//	ss.addTimer(cb,1000);
//  
//
//}