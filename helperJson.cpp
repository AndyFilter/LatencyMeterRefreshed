#include <fstream>
#include <filesystem>

#include "helperJson.h"
#include "External/nlohmann/json.hpp"

#include <Windows.h>

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
		{"internalPlotColor", styleData.internalPlotColor},
		{"externalPlotColor", styleData.externalPlotColor},
		{"inputPlotColor", styleData.inputPlotColor},
	};
}

void to_json(json& j, const PerformanceData& performanceData)
{
	j = json
	{
		{"lockGuiFps", performanceData.lockGuiFps},
		{"guiLockedFps", performanceData.guiLockedFps},
		{"showPlots", performanceData.showPlots},
		{"VSync", performanceData.VSync},
	};
}

void to_json(json& j, const UserData& userData)
{
	//json styleJson = userData.style;

	j = json
	{
		{ "style", userData.style },
		{ "performance", userData.performance },
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

void to_json(json& j, LatencyData data)
{
	j = json
	{
		{"measurements", data.measurements },
		{"note", data.note },
	};
}


void from_json(const json& j, PerformanceData& performanceData)
{
	j.at("guiLockedFps").get_to(performanceData.guiLockedFps);
	j.at("lockGuiFps").get_to(performanceData.lockGuiFps);
	j.at("showPlots").get_to(performanceData.showPlots);
	j.at("VSync").get_to(performanceData.VSync);
}

void from_json(const json& j, StyleData& styleData)
{
	j.at("mainColor").get_to(styleData.mainColor);
	j.at("fontColor").get_to(styleData.fontColor);
	j.at("mainColorBrightness").get_to(styleData.mainColorBrightness);
	j.at("fontColorBrightness").get_to(styleData.fontColorBrightness);
	j.at("fontSize").get_to(styleData.fontSize);
	j.at("selectedFont").get_to(styleData.selectedFont);
	j.at("internalPlotColor").get_to(styleData.internalPlotColor);
	j.at("externalPlotColor").get_to(styleData.externalPlotColor);
	j.at("inputPlotColor").get_to(styleData.inputPlotColor);
}

void from_json(const json& j, UserData& userData)
{
	j.at("style").get_to(userData.style);
	j.at("performance").get_to(userData.performance);
	//auto &style = j.at("style");
	//style.at("mainColor").get_to(userData.style.mainColor);
	//style.at("fontColor").get_to(userData.style.fontColor);
	//style.at("mainColorBrightness").get_to(userData.style.mainColorBrightness);
	//style.at("fontColorBrightness").get_to(userData.style.fontColorBrightness);
	//style.at("fontSize").get_to(userData.style.fontSize);
	//style.at("selectedFont").get_to(userData.style.selectedFont);
}

void from_json(const json& j, LatencyReading& reading)
{
	j.at("timeExternal").get_to(reading.timeExternal);
	j.at("timeInternal").get_to(reading.timeInternal);
	j.at("timePing").get_to(reading.timePing);
	j.at("index").get_to(reading.index);
}

void from_json(const json& j, LatencyData& reading)
{
	j.at("measurements").get_to(reading.measurements);
	strcpy_s(reading.note, j.value("note", "").c_str());
}

void HelperJson::SaveUserData(UserData& userData)
{
	char localPath[_MAX_PATH + 1];
	GetModuleFileName(NULL, localPath, _MAX_PATH);

	std::string filePath = std::string(localPath);
	filePath = filePath.substr(0, filePath.find_last_of("\\/") + 1) + std::string(configFileName);

	std::ofstream configFile(filePath);

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
	char localPath[_MAX_PATH + 1];
	GetModuleFileName(NULL, localPath, _MAX_PATH);

	std::string filePath = std::string(localPath) + std::string(configFileName);
	filePath = filePath.substr(0, filePath.find_last_of("\\/") + 1) + std::string(configFileName);

	std::ifstream configFile(filePath);

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

void HelperJson::SaveLatencyTests(LatencyData tests, char path[_MAX_PATH])
{
	//std::string fileName = savesPath + std::string(name) + std::string(".json");

	// Sort array by index before saving
	qsort(&tests.measurements[0], (size_t)tests.measurements.size(), sizeof(tests.measurements[0]), LatencyIndexSort);

	std::ofstream saveFile(path);

	if (!saveFile.good())
		return;

	json j = tests;

	saveFile << j.dump();

	saveFile.close();
}

void HelperJson::GetLatencyTests(LatencyData& tests, char path[_MAX_PATH])
{
	//std::string fileName = savesPath + std::string(name) + std::string(".json");
	std::ifstream saveFile(path);

	if (!saveFile.good())
		return;

	json j = json::parse(saveFile);
	from_json(j, tests);

	saveFile.close();
}