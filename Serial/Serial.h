#pragma once

#include <thread>
#include <vector>
#include "../App/Config.h"
#include <filesystem>

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#else
#include <iostream>
#endif
#include <fcntl.h>

#define BufferedSerialComm

namespace Serial {
	bool Setup(const char *szPortName, void (*OnCharReceivedFunc)(char c) = nullptr);
	// void HandleInput();
	void Close();
	void FindAvailableSerialPorts();
	bool Write(const char *c, size_t size);

	inline std::vector<std::string> g_availablePorts;
	inline bool g_isConnected{false};

	constexpr int SERIAL_NO_RESPONSE_DELAY = 2000000;
#ifdef _WIN32
	inline HANDLE g_hPort;
	inline int g_fd = -1;
	constexpr std::uint64_t SERIAL_UPDATE_TIME = 1000; // Runs serial read code ever x micro seconds (max 1000)
#else
	inline int g_hPort;
	constexpr std::uint64_t SERIAL_UPDATE_TIME = 1;
#endif

	inline std::thread g_ioThread;
} // namespace Serial
