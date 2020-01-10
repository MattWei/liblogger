#ifndef TIMER_H
#define TIMER_H

#include <sstream>

#include <stdint.h>
#include <time.h>

#include <chrono>

class Timer {
private:
	std::chrono::system_clock::time_point mStartTime;

public:
	bool isTimeout(long long milliseconds);
	void start();
};

#endif // TIMER_H
