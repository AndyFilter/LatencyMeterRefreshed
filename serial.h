#pragma once

#include <vector>

namespace Serial
{
	bool Setup(char COM_Number, void (*OnCharReceivedFunc)(char c));
	void HandleInput();
	void Close();

	inline std::vector<std::string> availablePorts;
}