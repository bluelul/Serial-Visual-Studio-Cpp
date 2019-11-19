// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "targetver.h"
#include <stdio.h>
#include <tchar.h>

// TODO: mencionar aquí los encabezados adicionales que el programa necesita
#include <conio.h>
#include <string.h>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
using namespace std;

void error_system(char *name) {
	WCHAR ptr[1024];
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), \
		0, ptr, 1024, NULL);
	wcout << endl << "Error " << name << ": " << ptr << endl;
	LocalFree(ptr);
}


int _tmain(int argc, _TCHAR* argv[])
{
	int ch;
	char buffer[1];
	HANDLE file;
	COMMTIMEOUTS timeouts;
	DWORD read, written;
	DCB port;
	HANDLE keyboard = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE screen = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode;
	LPCWSTR port_name = L"\\\\.\\COM10";
	char init[] = ""; // v.gr., "ATZ" resetea un modem por completo.

	if (argc > 2)
		swprintf_s((wchar_t *)&port_name, 128, L"\\\\.\\COM%c", argv[1][0]);

	// Open port
	file = CreateFile(port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if (file == INVALID_HANDLE_VALUE) {
		error_system("Open port");
		return 1;
	}

	// Config UART setting
	memset(&port, 0, sizeof(port));
	port.DCBlength = sizeof(port);
	if (!GetCommState(file, &port))
		error_system("get port status");
	if (!BuildCommDCB(L"baud=9600 parity=n data=8 stop=1", &port))
		error_system("create Communications DCB block contain UART setting");
	if (!SetCommState(file, &port))
		error_system("config port setting");

	// Set timeout
	timeouts.ReadIntervalTimeout = 1;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.ReadTotalTimeoutConstant = 1;
	timeouts.WriteTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 1;
	if (!SetCommTimeouts(file, &timeouts))
		error_system("config timeout of port");

	// Configure the keyboard for raw reading
	if (!GetConsoleMode(keyboard, &mode))
		error_system("get keyboard mode");
	mode &= ~ENABLE_PROCESSED_INPUT;
	if (!SetConsoleMode(keyboard, mode))
		error_system("set keyboard mode");

	if (!EscapeCommFunction(file, CLRDTR))
		error_system("turn off DTR");
	Sleep(200);
	if (!EscapeCommFunction(file, SETDTR))
		error_system("turn on DTR");

	// Check writing port
	if (!WriteFile(file, init, sizeof(init), &written, NULL))
		error_system("write init data to port");

	if (written != sizeof(init))
		error_system("lost init written data");

	// basic terminal
	do {
		// check for data in the port and display them on the screen
		ReadFile(file, buffer, sizeof(buffer), &read, NULL);
		if (read)
			WriteFile(screen, buffer, read, &written, NULL);

		// check pressing key, and send it to the port.
		if (_kbhit()) {
			ch = _getch();
			WriteFile(file, &ch, 1, &written, NULL);
		}
	} while (ch != 127); // until the user presses ctrl-backspace.

	// close everything
	CloseHandle(keyboard);
	CloseHandle(file);
	return 0;
}
