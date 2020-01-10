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
	virtual void run(zsock_t * pipe) = 0;
	bool isExit(zsock_t * socket);

private:
	std::unique_ptr<zactor_t, ZactorDeleter> mZactor;
	static void zactor_fn(zsock_t *pipe, void *args);
};

#endif