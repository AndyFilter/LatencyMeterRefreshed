#include <fstream>
#include <filesystem>

#include "helperJson.h"
#include "External/nlohmann/json.hpp"

#include <windows.h>
#include <initguid.h>
#include <KnownFolders.h>
#include <ShlObj.h>
#include <wchar.h>

using json = nlohmann::json;

const wchar_t* configFileName{ L"UserData.json" };
const char* savesPath{ "Saves/" };
const wchar_t* appDataFolderName{ L"\\LatencyMeterRefreshed\\" };

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
		//{"maxSerialReadFrequency", performanceData.maxSerialReadFrequency},
	};
}

void to_json(json& j, const MiscData& miscData)
{
	j = json
	{

		{"localUserData", miscData.localUserData},
	};
}

void to_json(json& j, const UserData& userData)
{
	//json styleJson = userData.style;

	j = json
	{
		{ "style", userData.style },
		{ "performance", userData.performance },
		{ "misc", userData.misc },
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

void to_json(json& j, TabInfo data)
{
	j = json
	{
		{"measurements", data.latencyData.measurements },
		{"note", data.latencyData.note },
		{"name", data.name },
	};
}

//void to_json(json& j, std::vector<TabInfo> data)
//{
//	//for (auto& info : data)
//	//{
//	//	json _j = info;
//	//	j += _j;
//	//	//j += json
//	//	//{
//	//	//	{"measurements", info.latencyData.measurements },
//	//	//	{"note", info.latencyData.note },
//	//	//	{"name", info.name },
//	//	//};
//	//}
//}


void from_json(const json& j, PerformanceData& performanceData)
{
	j.at("guiLockedFps").get_to(performanceData.guiLockedFps);
	j.at("lockGuiFps").get_to(performanceData.lockGuiFps);
	j.at("showPlots").get_to(performanceData.showPlots);
	j.at("VSync").get_to(performanceData.VSync);
	//j.at("maxSerialReadFrequency").get_to(performanceData.maxSerialReadFrequency);
}

void from_json(const json& j, MiscData& miscData)
{
	j.at("localUserData").get_to(miscData.localUserData);
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
	j.at("misc").get_to(userData.misc);
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

void from_json(const json& j, TabInfo& reading)
{
	j.at("measurements").get_to(reading.latencyData.measurements);
	strcpy_s(reading.latencyData.note, j.value("note", "").c_str());
	strcpy_s(reading.name, j.value("name", "").c_str());
}

void from_json(const json& j, std::vector<TabInfo> &data)
{
	for (auto& elem : j)
	{
		data.push_back(elem.get<TabInfo>());
	}
}

std::wstring HelperJson::GetAppDataUserConfingPath()
{
	PWSTR appDataPath = NULL;
	if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath) == S_OK)
	{
		std::wstring filePath = std::wstring(appDataPath) + std::wstring(appDataFolderName) + std::wstring(configFileName);
		filePath = filePath.substr(0, filePath.find_last_of(L"\\/") + 1) + std::wstring(configFileName);

		std::ifstream configFile(filePath);

		configFile.close();
		return filePath;
	}

	return L"";
}

std::wstring HelperJson::GetLocalUserConfingPath()
{
	wchar_t localPath[_MAX_PATH];
	GetModuleFileNameW(NULL, localPath, _MAX_PATH);

	std::wstring filePath = std::wstring(localPath) + std::wstring(configFileName);
	filePath = filePath.substr(0, filePath.find_last_of(L"\\/") + 1) + std::wstring(configFileName);

	std::ifstream configFile(filePath);

	configFile.close();
	return filePath;
}

std::wstring GetUserDataPath(bool islocal)
{
	std::ofstream configFile;
	std::wstring filePath;
	if (islocal)
	{
		wchar_t localPath[_MAX_PATH];
		GetModuleFileNameW(NULL, localPath, _MAX_PATH);

		filePath = std::wstring(localPath) + std::wstring(configFileName);
		filePath = filePath.substr(0, filePath.find_last_of(L"\\/") + 1) + std::wstring(configFileName);

		configFile = std::ofstream(filePath);
	}

	else
	{
		PWSTR appDataPath = NULL;
		if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath) == S_OK)
		{
			filePath = std::wstring(appDataPath) + std::wstring(appDataFolderName) + std::wstring(configFileName);
			filePath = filePath.substr(0, filePath.find_last_of(L"\\/") + 1) + std::wstring(configFileName);

			std::filesystem::create_directories(std::wstring(appDataPath) + std::wstring(appDataFolderName));

			configFile = std::ofstream(filePath);
		}
	}

	if (!configFile.good())
	{
		configFile.close();
		return L"";
	}

	configFile.close();

	return filePath;
}

void HelperJson::SaveUserData(UserData& userData)
{
	std::wstring filePath = GetUserDataPath(userData.misc.localUserData);

	if (filePath.empty())
		return;

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
	std::wstring filePath;

	wchar_t localPath[_MAX_PATH];
	GetModuleFileNameW(NULL, localPath, _MAX_PATH);

	filePath = std::wstring(localPath) + std::wstring(configFileName);
	filePath = filePath.substr(0, filePath.find_last_of(L"\\/") + 1) + std::wstring(configFileName);

	std::ifstream configFile(filePath);

	if (!configFile.good())
	{
		configFile.close();

		PWSTR appDataPath = NULL;
		if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath) == S_OK)
		{

			filePath = std::wstring(appDataPath) + std::wstring(appDataFolderName) + std::wstring(configFileName);
			filePath = filePath.substr(0, filePath.find_last_of(L"\\/") + 1) + std::wstring(configFileName);

			configFile = std::ifstream(filePath);

			if (!configFile.good())
			{
				configFile.close();
				return false;
			}
		}
		else
			return false;
	}

	json data = json::parse(configFile);
	from_json(data, userData);
	configFile.close();
	return true;
}

void HelperJson::SaveLatencyTests(TabInfo tests, char path[_MAX_PATH])
{
	//std::string fileName = savesPath + std::string(name) + std::string(".json");

	// Sort array by index before saving
	if(tests.latencyData.measurements.size() > 0)
		qsort(&tests.latencyData.measurements[0], (size_t)tests.latencyData.measurements.size(), sizeof(tests.latencyData.measurements[0]), LatencyIndexSort);

	std::ofstream saveFile(path);

	if (!saveFile.good())
		return;

	json j = tests;

	saveFile << j.dump();

	saveFile.close();
}

size_t HelperJson::GetLatencyTests(std::vector<TabInfo> &tests, const char path[_MAX_PATH])
{
	//std::string fileName = savesPath + std::string(name) + std::string(".json");
	std::ifstream saveFile(path);

	if (!saveFile.good())
		return 0;

	size_t elements = 0;

	json j = json::parse(saveFile);
	if (j.is_array()) {
		from_json(j, tests);
		elements = j.size();
	}
	else {
		TabInfo info;
		from_json(j, info);
		tests.push_back(info);
		elements = 1;
	}

	saveFile.close();

	return elements;
}

void HelperJson::SaveLatencyTestsPack(std::vector<TabInfo> tabs, char path[_MAX_PATH])
{
	for (auto& tab : tabs) {
		if (tab.latencyData.measurements.size() > 0)
			qsort(&tab.latencyData.measurements[0], (size_t)tab.latencyData.measurements.size(), sizeof(tab.latencyData.measurements[0]), LatencyIndexSort);
	}

	std::ofstream saveFile(path);

	if (!saveFile.good())
		return;

	json j = tabs;

	saveFile << j.dump();

	saveFile.close();
}

void HelperJson::UserConfigLocationChanged(bool isNewLocal)
{
	std::wstring appDataPath = GetAppDataUserConfingPath();
	std::wstring localPath = GetLocalUserConfingPath();

	bool appDataPathExists = false;
	bool localPathExists = false;

	if (!appDataPath.empty())
	{
		std::ifstream testFile(appDataPath);
		appDataPathExists = testFile.good();
		testFile.close();
	}

	if (!localPath.empty())
	{
		std::ifstream testFile(localPath);
		localPathExists = testFile.good();
		testFile.close();
	}

	if ((isNewLocal && !appDataPathExists) || (!isNewLocal && !localPathExists))
		return;

	if (isNewLocal)
	{
		std::filesystem::copy(appDataPath, localPath, std::filesystem::copy_options::overwrite_existing);
		std::filesystem::remove(appDataPath);
	}
	else
	{
		std::filesystem::copy(localPath, appDataPath, std::filesystem::copy_options::overwrite_existing);
		std::filesystem::remove(localPath);
	}
}