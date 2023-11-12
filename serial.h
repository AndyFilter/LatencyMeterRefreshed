#pragma once

#include <vector>
#include <thread>

#include <io.h>
#include <fcntl.h>

const int SERIAL_NO_RESPONSE_DELAY = 2000000;
const uint64_t SERIAL_UPDATE_TIME = 1000; // Runs serial read code ever x micro seconds (max 1000)

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

	inline HANDLE hPort;
	inline int fd = -1;

	inline std::thread ioThread;
}