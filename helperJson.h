#pragma once

#include "structs.h"

namespace HelperJson
{
	void SaveUserStyle(StyleData &styleData);
	void SaveUserData(UserData& userData);

	bool GetUserData(UserData &userData);
}