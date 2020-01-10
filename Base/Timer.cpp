#include "timer.h"

#include <sys/timeb.h>

bool Timer::isTimeout(long long milliseconds)
{
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	long long gone = 
		std::chrono::duration_cast<std::chrono::milliseconds>(now - mStartTime).count();

	return gone >= milliseconds;
}

void Timer::start()
{
	mStartTime = std::chrono::system_clock::now();
}
