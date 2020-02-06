#include "StopWatch.h"

#include <Windows.h>

StopWatch::StopWatch(): lastTime(0) {
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	frequency = freq.QuadPart;
}

void StopWatch::start() {
	LARGE_INTEGER timer;
	QueryPerformanceCounter(&timer);
	lastTime = timer.QuadPart;
}

int StopWatch::time() {
	LARGE_INTEGER timer;
	QueryPerformanceCounter(&timer);
	auto delta = timer.QuadPart - lastTime;

	return (delta * 1000) / frequency;
}
