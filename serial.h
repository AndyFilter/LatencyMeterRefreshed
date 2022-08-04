#pragma once

#include <vector>
#include <thread>

namespace Serial
{
	bool Setup(const char* szPortName, void (*OnCharReceivedFunc)(char c) = nullptr);
	void HandleInput();
	void Close();
	void FindAvailableSerialPorts();
	bool Write(const char* c, size_t size);

	inline std::vector<std::string> availablePorts;
	inline bool isConnected{false};
	inline HANDLE hPort;
	inline FILE* hFile;

	inline std::thread ioThread;
}