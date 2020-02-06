#pragma once

class Console {
	void* handle;
public:
	Console();
	void moveCursor(short dy);
};

