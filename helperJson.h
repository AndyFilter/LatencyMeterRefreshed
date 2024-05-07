#pragma once
#include <vector>

#include "structs.h"
#include "constants.h"

#ifndef _MAX_PATH
#define _MAX_PATH MAX_PATH
#endif

namespace HelperJson
{
	void SaveUserStyle(StyleData &styleData);

	void SaveUserData(UserData& userData);

	bool GetUserData(UserData &userData);

	void SaveLatencyTests(TabInfo& tests, char path[_MAX_PATH]);

	size_t GetLatencyTests(std::vector<TabInfo> &tests, const char path[_MAX_PATH]);

	void SaveLatencyTestsPack(std::vector<TabInfo> &tabs, char path[_MAX_PATH]);

	void UserConfigLocationChanged(bool isNewLocal);

    OS_SPEC_PATH_STR GetAppDataUserConfigPath();
    OS_SPEC_PATH_STR GetLocalUserConfigPath();
}