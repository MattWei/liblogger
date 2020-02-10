#include "ZactorThread.h"

#include "czmq_defines.h"

void ZactorThread::zactor_fn(zsock_t * pipe, void * args)
{
	ZactorThread *thread = (ZactorThread*)args;
	thread->init(pipe);
	if (thread->onStarted()) {
		thread->run();
	}
	thread->onExit();
}

void ZactorThread::onExit()
{
	if (mPoller) {
		zpoller_destroy(&mPoller);
		mPoller = NULL;
	}
}

ZactorThread::ZactorThread()
	: mPipe(NULL)
	, mPoller(NULL)
{
}

ZactorThread::~ZactorThread()
{
	stop();
}

bool ZactorThread::start()
{
	if (!mZactor) {
		mZactor = std::unique_ptr<zactor_t, ZactorDeleter>(zactor_new(zactor_fn, this));
		if (mZactor) {
			return false;
		}
	}

	return true;
}

bool ZactorThread::stop()
{
	mZactor.reset();

	return true;
}

bool ZactorThread::isExit(zsock_t *socket)
{
	if (socket != mPipe) {
		return false;
	}

	zmsg_t *msg = zmsg_recv(mPipe);
	if (!msg) {
		return true;
	}

	std::string command = zmsg_pop_stdstring(msg);
	zmsg_destroy(&msg);

	return command == ZACTOR_EXIT_CMD;
}

void ZactorThread::init(zsock_t * pipe)
{
	mPipe = pipe;
	mPoller = zpoller_new(pipe, NULL);
}

bool ZactorThread::onStarted()
{
	sendSignal(0);

	return true;
}

void ZactorThread::sendSignal(int signal)
{
	assert(zsock_signal(mPipe, signal) == 0);
}

void ZactorThread::sendMsg(zmsg_t ** msg)
{
	assert(zmsg_send(msg, mPipe) == 0);
}

void ZactorThread::addPollerSock(zsock_t * sock)
{
	zpoller_add(mPoller, sock);
}

zsock_t * ZactorThread::pollerWait(int timeout)
{
	return static_cast<zsock_t *>(zpoller_wait(mPoller, timeout));
}


