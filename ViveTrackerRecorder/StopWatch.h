#pragma once
class StopWatch {
private:
	long long frequency;
	long long lastTime;
public:
	StopWatch();
	void start();
	int time();
};

