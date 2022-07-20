#include <fstream>

#include "helperJson.h"
#include "External/nlohmann/json.hpp"

using namespace nlohmann;

const char* configFileName{ "UserData.json" };

void ToJson(json& j, StyleData& styleData)
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

void ToJson(json& j, UserData& userData)
{
	json styleJson;
	ToJson(styleJson, userData.style);

	j = json
	{
		{ "style", styleJson }
	};
}

void FromJson(json& j, UserData& userData)
{
	auto &style = j.at("style");
	style.at("mainColor").get_to(userData.style.mainColor);
	style.at("fontColor").get_to(userData.style.fontColor);
	style.at("mainColorBrightness").get_to(userData.style.mainColorBrightness);
	style.at("fontColorBrightness").get_to(userData.style.fontColorBrightness);
	style.at("fontSize").get_to(userData.style.fontSize);
	style.at("selectedFont").get_to(userData.style.selectedFont);
}

void HelperJson::SaveUserData(UserData& userData)
{
	std::ofstream configFile(configFileName);

	json j;

	ToJson(j, userData);

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
	FromJson(data, userData);
	//userData.style.mainColor = data["mainColor"];
	configFile.close();
	return true;
}

