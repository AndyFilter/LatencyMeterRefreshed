#include <fstream>
#include <filesystem>

#include "helperJson.h"
#include "External/nlohmann/json.hpp"

using json = nlohmann::json;

const char* configFileName{ "UserData.json" };
const char* savesPath{ "Saves/" };

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

void from_json(json& j, LatencyReading reading)
{
	j.at("timeExternal").get_to(reading.timeExternal);
	j.at("timeInternal").get_to(reading.timeInternal);
	j.at("timePing").get_to(reading.timePing);
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

void HelperJson::SaveLatencyTests(std::vector<LatencyReading> tests, char name[MEASUREMENTS_FILE_NAME_LENGTH])
{
	std::string fileName = savesPath + std::string(name) + std::string(".json");
	if(!std::filesystem::is_directory(savesPath))
		std::filesystem::create_directory(savesPath);
	std::ofstream saveFile(fileName);

	if (!saveFile.good())
		return;

	json j = tests;

	saveFile << j.dump();

	saveFile.close();
}

void HelperJson::GetLatencyTests(std::vector<LatencyReading>& tests)
{
	std::string fileName = savesPath + std::string(std::to_string(clock())) + std::string(".json");
	std::ifstream saveFile(configFileName);
}
