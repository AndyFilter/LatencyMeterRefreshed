#include <fstream>
#include <filesystem>

#include "helperJson.h"
#include "External/nlohmann/json.hpp"

using json = nlohmann::json;

const char* configFileName{ "UserData.json" };
const char* savesPath{ "Saves/" };

int LatencyIndexSort(const void* a, const void* b)
{
	LatencyReading* _a = (LatencyReading*)a;
	LatencyReading* _b = (LatencyReading*)b;

	return (int)_a->index - (int)_b->index > 0 ? +1 : -1;
}

void to_json(json& j, const StyleData& styleData)
{
	j = json
	{
		{"mainColor", styleData.mainColor},
		{"fontColor", styleData.fontColor},
		{"mainColorBrightness", styleData.mainColorBrightness},
		{"fontColorBrightness", styleData.fontColorBrightness},
		{"fontSize", styleData.fontSize},
		{"selectedFont", styleData.selectedFont},
	};
}

void to_json(json& j, const UserData& userData)
{
	json styleJson = userData.style;

	j = json
	{
		{ "style", styleJson }
	};
}

void to_json(json& j, LatencyReading reading)
{
	j = json
	{
		{"timeExternal", reading.timeExternal },
		{"timeInternal", reading.timeInternal },
		{"timePing", reading.timePing },
		{"index", reading.index },
	};
}

void from_json(json& j, UserData& userData)
{
	auto &style = j.at("style");
	style.at("mainColor").get_to(userData.style.mainColor);
	style.at("fontColor").get_to(userData.style.fontColor);
	style.at("mainColorBrightness").get_to(userData.style.mainColorBrightness);
	style.at("fontColorBrightness").get_to(userData.style.fontColorBrightness);
	style.at("fontSize").get_to(userData.style.fontSize);
	style.at("selectedFont").get_to(userData.style.selectedFont);
}

void from_json(const json& j, LatencyReading& reading)
{
	j.at("timeExternal").get_to(reading.timeExternal);
	j.at("timeInternal").get_to(reading.timeInternal);
	j.at("timePing").get_to(reading.timePing);
	j.at("index").get_to(reading.index);
}

void HelperJson::SaveUserData(UserData& userData)
{
	std::ofstream configFile(configFileName);

	json j = userData;

	configFile << j.dump();

	configFile.close();
}

void HelperJson::SaveUserStyle(StyleData& styleData)
{
	UserData userData{};
	GetUserData(userData);

	userData.style = styleData;

	SaveUserData(userData);
}

bool HelperJson::GetUserData(UserData& userData)
{
	std::ifstream configFile(configFileName);

	if (!configFile.good())
	{
		configFile.close();
		return false;
	}

	json data = json::parse(configFile);
	from_json(data, userData);
	configFile.close();
	return true;
}

void HelperJson::SaveLatencyTests(std::vector<LatencyReading> tests, char path[_MAX_PATH])
{
	//std::string fileName = savesPath + std::string(name) + std::string(".json");

	// Sort array by index before saving
	qsort(&tests[0], (size_t)tests.size(), sizeof(tests[0]), LatencyIndexSort);

	std::ofstream saveFile(path);

	if (!saveFile.good())
		return;

	json j = tests;

	saveFile << j.dump();

	saveFile.close();
}

void HelperJson::GetLatencyTests(std::vector<LatencyReading>& tests, char path[_MAX_PATH])
{
	//std::string fileName = savesPath + std::string(name) + std::string(".json");
	using s = std::vector<LatencyReading>;
	std::ifstream saveFile(path);

	if (!saveFile.good())
		return;

	json j = json::parse(saveFile);
	j.get_to(tests);

	saveFile.close();
}