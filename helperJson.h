#pragma once
#include <vector>

#include "structs.h"
#include "constants.h"

namespace HelperJson
{
	void SaveUserStyle(StyleData &styleData);
	void SaveUserData(UserData& userData);

	bool GetUserData(UserData &userData);

	void SaveLatencyTests(std::vector<LatencyReading> tests, char path[_MAX_PATH]);

	void GetLatencyTests(std::vector<LatencyReading>& tests, char path[_MAX_PATH]);
}