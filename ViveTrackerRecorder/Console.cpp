#include "Console.h"

#include <windows.h>

Console::Console() {
	handle = GetStdHandle(STD_OUTPUT_HANDLE);
}

void Console::moveCursor(short dy) {
	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	GetConsoleScreenBufferInfo(handle, &consoleInfo);
	SetConsoleCursorPosition(handle, { 0, consoleInfo.dwCursorPosition.Y + dy });
}

