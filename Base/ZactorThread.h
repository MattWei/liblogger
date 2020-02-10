#ifndef __ZACTOR_THREAD_H__
#define __ZACTOR_THREAD_H__

#include <czmq.h>
#include <memory>

struct ZactorDeleter
{
public:
	void operator()(zactor_t *p)
	{
		zactor_destroy(&p);
	}
};

class ZactorThread
{
public:
	ZactorThread();
	virtual ~ZactorThread();

	bool start();
	bool stop();
	
protected:
	virtual void run() = 0;
	bool isExit(zsock_t * socket);

	void init(zsock_t *pipe);
	virtual bool onStarted();
	virtual void onExit();

	void addPollerSock(zsock_t *sock);
	zsock_t *pollerWait(int timeout);

	void sendSignal(int signal);
	void sendMsg(zmsg_t **msg);
	//zpoller_t *getPoller();

private:
	static void zactor_fn(zsock_t *pipe, void *args);

	zsock_t *mPipe;
	zpoller_t *mPoller;
	std::unique_ptr<zactor_t, ZactorDeleter> mZactor;
};

/*
inline zpoller_t *ZactorThread::getPoller()
{
	return mPoller;
}
*/
#endif