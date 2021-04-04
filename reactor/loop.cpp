#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include<unistd.h>
#include <stdio.h>
#include <sys/epoll.h>
#include<sys/time.h>
#include <vector>
#include <memory>
#include <functional>
#include "timer.h"
#include <thread>
#include <sys/eventfd.h>
using namespace std;

class Channel
{
public:

	Channel(int _fd, int _event)
	{
		fd = _fd;
		event = _event;

	}

	void handleEvent()
	{
		if (event&(EPOLLIN | EPOLLPRI | EPOLLRDHUP))
		{
			printf("read fd from :%d\n", fd);



		}
		else if (event&EPOLLOUT)
		{
			printf("write\n");
		}
		else
		{
			printf("otherevent\n");
		}
	}
	int fd;

	bool islistenfd = true;
	int event;



};

class Poller
{
public:
	Poller()
	{
		_epollfd = epoll_create(256);
	}
	void   epoll_add_(shared_ptr<Channel> para)
	{
		struct  epoll_event ev;
		ev.data.fd = para->fd;
		ev.events = para->event;

		sp_all_channel[para->fd] = para;


		if (epoll_ctl(_epollfd, EPOLL_CTL_ADD, para->fd, &ev) < 0)
		{
			printf("error\n");
		}
	}



	std::vector<shared_ptr<Channel>>  pollrun()
	{
		vector<shared_ptr<Channel>>  vec;
		struct  epoll_event events[128];

		struct timeval now;
		gettimeofday(&now, NULL);
		unsigned long   temp = now.tv_sec * 1000 + now.tv_usec / 1000;
		unsigned long  next_timeout = TimeManager->timerNodeQueue.top()->getExpTime()-temp;
		int MAX_TIMEOUT = 3000;
		if (next_timeout >0) {   //usigned long long 
			next_timeout = next_timeout > MAX_TIMEOUT
				? MAX_TIMEOUT : next_timeout;
			}
		else {
			next_timeout = MAX_TIMEOUT;
		}
		int calledfd = epoll_wait(_epollfd, events, 20, next_timeout);
		for (int i = 0; i < calledfd; i++)
		{
			printf("eventhappen\n");
			int fd = events[i].data.fd;
			shared_ptr<Channel> currChannel = sp_all_channel[fd];
			currChannel->event = events[i].events;
			vec.push_back(currChannel);
		}
		return vec;
	}

	int _epollfd;
	static const int MAXFDSMAXFDS = 100000;
	shared_ptr<Channel>  sp_all_channel[MAXFDS];
	shared_ptr< TimerManager<function<void()> >   >TimeManager;

};

class Loop

{
public:
	Loop() : _poll(new Poller)
	{
		epfd2 = eventfd(1000, 0);
		TimeManager = make_shared<TimerManager<function<void()> >>();  ///initial the timemanager   notice the initial !!!
		_poll->TimeManager = TimeManager;
		shared_ptr<Channel>  ch = make_shared<Channel>(epfd2, EPOLLIN | EPOLLET);
		ch->islistenfd = false;
		AddChannel(ch);
	}
	void AddChannel(shared_ptr<Channel> para)
	{
		(*_poll).epoll_add_(para);
	}
	void myloop()
	{
		printf("beginloop\n");
		while (true)
		{
			vector<shared_ptr<Channel>>  rlt = _poll->pollrun(); //thin not use auto
			for (auto ele : rlt)
			{


				if (ele->islistenfd)
				{
					struct sockaddr_in cliaddr;
					socklen_t clilen = sizeof(struct sockaddr_in);
					int connfd = accept(ele->fd, (sockaddr*)&cliaddr, (socklen_t*)&clilen);
					char buff[] = "12345";
					write(connfd, buff, 6);
					shared_ptr<Channel>  ch = make_shared<Channel>(connfd, EPOLLIN | EPOLLET); //fd event
					ch->islistenfd = false;
					AddChannel(ch);
				}
				if (ele->fd == epfd2)
				{
					uint64_t count;
					read(epfd2, &count, sizeof(count));
					printf("read count: %zu\n", count);
				}

				ele->handleEvent();

			}

			//  deal the  expired time  cb
			struct timeval now;
			gettimeofday(&now, NULL);
			unsigned long   temp = now.tv_sec * 1000 + now.tv_usec / 1000;
			printf("curr:%ld\n",temp);
			while (!TimeManager->timerNodeQueue.empty() && (TimeManager->timerNodeQueue.top()->getExpTime() < temp))
			{
				printf("%ld\n",TimeManager->timerNodeQueue.top()->getExpTime());
				TimeManager->timerNodeQueue.top()->cb();
				TimeManager->timerNodeQueue.pop();
			}
		}

	}
	void Addcb(function<void()>cb, int time)  ///need mutex
	{
		unsigned long  curradd = TimeManager->addTimer(cb, time);
		printf("addtimer:%ld\n",curradd);
		unsigned long  top = TimeManager->timerNodeQueue.top()->getExpTime();
		if (curradd == top)
		{
			printf("change epoll time \n");
			tick();
		}
	}
	void tick()
	{
		write(epfd2, "T", 1);
	}
	shared_ptr<Poller> _poll;
	int epfd2; //have new cb    ---  notify  epoll
	shared_ptr< TimerManager<function<void()> >   >TimeManager; ///template have  severe type dependence     ,here  instance a Instance  to  avoid  dependence

};



int CreateListenFd(int port)
{

	printf("createfd\n");
	int on = 1;
	int    _listenfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in servaddr;
	fcntl(_listenfd, F_SETFL, O_NONBLOCK); //no-block io 
	setsockopt(_listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	bind(_listenfd, (const struct sockaddr*)&servaddr, sizeof(servaddr));

	listen(_listenfd, 10);

	return _listenfd;
}

void testfun()
{
	printf("cb callled\n");
}
int main()

{
	Loop myrunloop;
	for (int i = 0; i < 5; i++)
	{
		int  fd = CreateListenFd(i + 9000);
		shared_ptr<Channel>  ch = make_shared<Channel>(fd, EPOLLIN | EPOLLET); //fd event

		myrunloop.AddChannel(ch);
	}
	thread  loopthread(&Loop::myloop, myrunloop); /// notice !!!
	loopthread.detach();
	myrunloop.Addcb(testfun,5000);
	myrunloop.Addcb(testfun,7000);
	myrunloop.Addcb(testfun,8000);
	myrunloop.Addcb(testfun,9000);

	while (true)
	{
		sleep(100);

	}
	return 0;
}