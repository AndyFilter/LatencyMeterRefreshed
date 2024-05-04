#pragma once

#include <vector>
#include <thread>
#include "constants.h"

#ifdef _WIN32
#include <io.h>
#include <Windows.h>
#else
#include <iostream>
#include <filesystem>
#endif
#include <fcntl.h>

const int SERIAL_NO_RESPONSE_DELAY = 2000000;
#ifdef _WIN32
const uint64_t SERIAL_UPDATE_TIME = 1000; // Runs serial read code ever x micro seconds (_max 1000)
#else
const uint64_t SERIAL_UPDATE_TIME = 1;
#endif

#define BufferedSerialComm

namespace Serial
{
	bool Setup(const char* szPortName, void (*OnCharReceivedFunc)(char c) = nullptr);
	//void HandleInput();
	void Close();
	void FindAvailableSerialPorts();
	bool Write(const char* c, size_t size);

	inline std::vector<std::string> availablePorts;
	inline bool isConnected{false};

#ifdef _WIN32
	inline HANDLE hPort;
	inline int fd = -1;
#else
    inline int hPort;
#endif

	inline std::thread ioThread;
}