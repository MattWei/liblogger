#include "ZactorThread.h"

#include "czmq_defines.h"

void ZactorThread::zactor_fn(zsock_t * pipe, void * args)
{
	ZactorThread *thread = (ZactorThread*)args;
	thread->run(pipe);
}

ZactorThread::ZactorThread()
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
	zmsg_t *msg = zmsg_recv(socket);
	if (!msg) {
		return true;
	}

	std::string command = zmsg_pop_stdstring(msg);
	zmsg_destroy(&msg);

	return command == ZACTOR_EXIT_CMD;
}