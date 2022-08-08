#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <string.h>
#include <tchar.h>

#include "serial.h"
#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_internal.h"
#include "structs.h"
#include "helperJson.h"
#include "constants.h"
#include "gui.h"

HWND hwnd;

ImVec2 detectionRectSize{ 0, 200 };

unsigned int lastFrameGui = 0;
unsigned int lastFrame = 0;

// Frametime in microseconds
float averageFrametime = 0;
const unsigned int AVERAGE_FRAME_AMOUNT = 10000;
unsigned int frames[AVERAGE_FRAME_AMOUNT]{ 0 };
unsigned long long totalFrames = 0;

// ---- Styling ---- (Don't ask me why I didn't create structures for these things earlier)

const int styleColorNum = 2;
const unsigned int colorSize = 16;

ImFont* boldFont;

//float accentColor[4];
//float accentBrightness = 0.1f;
//
//float fontColor[4];
//float fontBrightness = 0.1f;
//
//int selectedFont = 0;
//float fontSize = 1.f;


//float internalPlotColor[4] {0.1f, 0.8f, 0.2f, 1.f};
//float externalPlotColor[4] { 0.3f, 0.3f, 0.9f, 1.f };
//float inputPlotColor[4] { 0.8f, 0.1f, 0.2f, 1.f };
//
//
//float accentColorBak[4];
//float accentBrightnessBak = 0.1f;
//
//float fontColorBak[4];
//float fontBrightnessBak = 0.1f;
//
//int selectedFontBak = 0;
//float fontSizeBak = 1.f;
//float internalPlotColorBak[4]{ 0.1f, 0.8f, 0.2f, 1.f };
//float externalPlotColorBak[4]{ 0.3f, 0.3f, 0.9f, 1.f };
//float inputPlotColorBak[4]{ 0.8f, 0.1f, 0.2f, 1.f };


const char* fonts[4]{ "Courier Prime", "Source Sans Pro", "Franklin Gothic", "Lucida Console" };
const int fontIndex[4]{ 0, 2, 4, 5 };

// --------


// ---- Performance ----

int guiLockedFps;
bool lockGuiFps = true;

bool showPlots = true;


int guiLockedFpsBak;
bool lockGuiFpsBak = true;

bool showPlotsBak;

// --------


// ---- User Data ----

UserData currentUserData {};

UserData backupUserData {};

// --------


// ---- Functionality ----

SerialStatus serialStatus = Status_Idle;

int selectedPort = 0;
//std::vector<LatencyReading> latencyTests;
//
//LatencyStats latencyStats;
//
//char note[1000];


std::vector<TabInfo> tabsInfo{};
// --------


//bool isSaved = true;
//char savePath[MAX_PATH]{ 0 };

bool isExiting = false;

static ImGuiTableSortSpecs* sortSpec;

bool isFullscreen = false;
bool fullscreenModeOpenPopup = false;
bool fullscreenModeClosePopup = false;

bool isGameMode = false;
bool wasLMB_Pressed = false;
bool wasMouseClickSent = false;

int selectedTab = 0;

// Forward declaration
void GotSerialChar(char c);
bool SaveAsMeasurements();
void ClearData();


/*
TODO:

+ Add multiple fonts to chose from.
- Add a way to change background color (?)
+ Add the functionality...
+ Saving measurement records to a json file.
+ Open a window to select which save file you want to open
+ Better fullscreen implementation (ESCAPE to exit etc.)
+ Max Gui fps slider in settings
+ Load the font in earlier selected size (?)
- Movable black Square for color detection (?) Check for any additional latency this might involve
+ Clear / Reset button
+ Game integration. Please don't get me banned again (needs testing) Update: (testing done)
+ Fix overwriting saves
+ Unsaved work notification
- Custom Fonts (?)
- Multiple tabs for measurements
- Save Tabs
*/

bool isSettingOpen = false;
static LARGE_INTEGER StartingTime{ 0 };
uint64_t micros()
{
	//auto timeNow = std::chrono::high_resolution_clock::now();
	//uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(timeNow.time_since_epoch()).count();
	//static const uint64_t start{ us };
	//return us - start;

	//FILETIME ft;
	//GetSystemTimeAsFileTime(&ft);
	//ULARGE_INTEGER li;
	//li.LowPart = ft.dwLowDateTime;
	//li.HighPart = ft.dwHighDateTime;
	//unsigned long long valueAsHns = li.QuadPart;
	//unsigned long long valueAsUs = valueAsHns / 10;

	//return valueAsUs;


	// QueryPerformance method is about 15-25% faster than std::chrono implementation of the same code

	LARGE_INTEGER EndingTime, ElapsedMicroseconds;
	LARGE_INTEGER Frequency;

	QueryPerformanceFrequency(&Frequency);

	// Activity to be timed

	QueryPerformanceCounter(&EndingTime);
	ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;


	//
	// We now have the elapsed number of ticks, along with the
	// number of ticks-per-second. We use these values
	// to convert to the number of elapsed microseconds.
	// To guard against loss-of-precision, we convert
	// to microseconds *before* dividing by ticks-per-second.
	//

	ElapsedMicroseconds.QuadPart *= 1000000;
	ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

	return ElapsedMicroseconds.QuadPart;
}

void ApplyStyle(float colors[styleColorNum][4], float brightnesses[styleColorNum])
{
	auto& style = ImGui::GetStyle();
	for (int i = 0; i < styleColorNum; i++)
	{
		float brightness = brightnesses[i];

		auto baseColor = ImVec4(*(ImVec4*)colors[i]);
		auto darkerBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness);
		auto darkestBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness * 2);

		// Alpha has to be set separatly, because it also gets multiplied times brightness.
		auto alphaBase = baseColor.w;
		baseColor.w = alphaBase;
		darkerBase.w = alphaBase;
		darkestBase.w = alphaBase;

		switch (i)
		{
		case 0:
			style.Colors[ImGuiCol_Header] = baseColor;
			style.Colors[ImGuiCol_HeaderHovered] = darkerBase;
			style.Colors[ImGuiCol_HeaderActive] = darkestBase;

			style.Colors[ImGuiCol_TabHovered] = baseColor;
			style.Colors[ImGuiCol_TabActive] = darkerBase;
			style.Colors[ImGuiCol_Tab] = darkestBase;

			style.Colors[ImGuiCol_CheckMark] = baseColor;

			style.Colors[ImGuiCol_PlotLines] = style.Colors[ImGuiCol_PlotHistogram] = darkerBase;
			style.Colors[ImGuiCol_PlotLinesHovered] = style.Colors[ImGuiCol_PlotHistogramHovered] = baseColor;
			break;
		case 1:
			style.Colors[ImGuiCol_Text] = baseColor;
			style.Colors[ImGuiCol_TextDisabled] = darkerBase;
			break;
		}
	}
}

void RevertConfig()
{
	currentUserData = backupUserData;

	//std::copy(accentColorBak, accentColorBak + 4, accentColor);
	//std::copy(fontColorBak, fontColorBak + 4, fontColor);

	//accentBrightness = accentBrightnessBak;
	//fontBrightness = fontBrightnessBak;

	//fontSize = fontSizeBak;
	//selectedFont = selectedFontBak;
	//boldFont = boldFontBak;

	//lockGuiFps = lockGuiFpsBak;
	//guiLockedFps = guiLockedFpsBak;

	//showPlots = showPlotsBak;

	ImGuiIO& io = ImGui::GetIO();

	for (int i = 0; i < io.Fonts->Fonts.Size; i++)
	{
		io.Fonts->Fonts[i]->Scale = currentUserData.style.fontSize;
	}
	io.FontDefault = io.Fonts->Fonts[fontIndex[currentUserData.style.selectedFont]];
}

int LatencyCompare(const void* a, const void* b)
{
	LatencyReading* _a = (LatencyReading*)a;
	LatencyReading* _b = (LatencyReading*)b;

	int delta = 0;

	switch (sortSpec->Specs->ColumnUserID)
	{
	case 0: delta = _a->index - _b->index;					break;
	case 1: delta = _a->timeExternal - _b->timeExternal;	break;
	case 2: delta = _a->timeInternal - _b->timeInternal;	break;
	case 3: delta = _a->timePing - _b->timePing;			break;
	case 4: delta = 0;										break;
		//case 0:
		//	if (sortSpec->Specs->SortDirection == ImGuiSortDirection_Ascending)
		//		return 1;
		//	else
		//		return -1;
		//	break;
		//case 1:
		//	if (_a->timeExternal < _b->timeExternal)
		//		return -1;
		//	else if (_a->timeExternal == _b->timeExternal)
		//		return 0;
		//	else
		//		return 1;
		//	break;
		//case 2:
		//	if (_a->timeInternal < _b->timeInternal)
		//		return -1;
		//	else if (_a->timeInternal == _b->timeInternal)
		//		return 0;
		//	else
		//		return 1;
		//	break;
		//case 3:
		//	if (_a->timePing < _b->timePing)
		//		return -1;
		//	else if (_a->timePing == _b->timePing)
		//		return 0;
		//	else
		//		return 1;
		//	break;
	}

	if (sortSpec->Specs->SortDirection == ImGuiSortDirection_Ascending)
	{
		return delta > 0 ? +1 : -1;
	}
	else
	{
		return delta > 0 ? -1 : +1;
	}
}

// deprecated, moved to json based files.
//bool SaveStyleConfig(float colors[styleColorNum][4], float brightnesses[styleColorNum], int fontIndex, float fontSize, size_t size = styleColorNum)
//{
//	//This function is deprecated, please use SaveCurrentUserConfig
//	assert(false);
//	char buffer[MAX_PATH];
//	::GetCurrentDirectory(MAX_PATH, buffer);
//
//	auto filePath = std::string(buffer).find("Debug") != std::string::npos ? "Style.cfg" : "Debug/Style.cfg";
//
//	std::ofstream configFile(filePath);
//	for (size_t i = 0; i < size; i++)
//	{
//		auto color = colors[i];
//		auto packedColor = ImGui::ColorConvertFloat4ToU32(*(ImVec4*)(color));
//		configFile << i << packedColor << "," << std::to_string(brightnesses[i]) << std::endl;
//
//		switch (i)
//		{
//		case 0:
//			std::copy(color, color + 4, accentColorBak);
//			accentBrightnessBak = brightnesses[i];
//			break;
//		case 1:
//			std::copy(color, color + 4, fontColorBak);
//			fontBrightnessBak = brightnesses[i];
//			break;
//		}
//	}
//	selectedFontBak = currentUserData.style.selectedFont;
//	fontSizeBak = fontSize;
//	boldFontBak = boldFont;
//	configFile << "F" << fontIndex << std::to_string(fontSize) << std::endl;
//	configFile.close();
//
//	std::cout << "File Closed\n";
//
//	return true;
//}

// deprecated, moved to json based files.
//bool ReadStyleConfig(float(&colors)[styleColorNum][4], float(&brightnesses)[styleColorNum], int& fontIndex, float& fontSize)
//{
//	//This function is deprecated, please use SaveCurrentUserConfig
//	assert(false);
//	char buffer[MAX_PATH];
//	::GetCurrentDirectory(MAX_PATH, buffer);
//
//	auto filePath = std::string(buffer).find("Debug") != std::string::npos ? "Style.cfg" : "Debug/Style.cfg";
//
//	std::ifstream configFile(filePath);
//
//	if (!configFile.good())
//		return false;
//
//	std::string line, colorPart, brightnessPart;
//	while (std::getline(configFile, line))
//	{
//		if (line[0] == 'F')
//		{
//			fontIndex = line[1] - '0';
//			fontSize = std::stof(line.substr(2, line.find("\n")));
//			continue;
//		}
//
//		// First char of the line is the index (just to be sure)
//		int index = (int)(line[0] - '0');
//
//		// Color is stored in front of the comma and the brightness after it
//		int delimiterPos = line.find(",");
//		colorPart = line.substr(1, delimiterPos - 1);
//		brightnessPart = line.substr(delimiterPos + 1, line.find("\n"));
//
//		// Extract the color from the string
//		auto subInt = stoll(colorPart);
//		auto color = ImGui::ColorConvertU32ToFloat4(subInt);
//		auto offset = (index * (colorSize));
//
//		// Same with brightness
//		float brightness = std::stof(brightnessPart);
//
//		// Set both of the values
//		memcpy(&(colors[index][0]), &color, colorSize);
//		brightnesses[index] = brightness;
//	}
//
//	// Remember to close the handle
//	configFile.close();
//
//	std::cout << "File Closed\n";
//
//	return true;
//}

// Does the calcualtions and copying for you
void SaveCurrentUserConfig()
{
	//UserData* userData = new UserData();

	//StyleData* style = &userData->style;

	//std::copy(accentColor, accentColor + 4, style->mainColor);
	//std::copy(fontColor, fontColor + 4, style->fontColor);
	//style->mainColorBrightness = accentBrightness;
	//style->fontColorBrightness = fontBrightness;
	//style->fontSize = fontSize;
	//style->selectedFont = selectedFont;

	//std::copy(accentColor, accentColor + 4, accentColorBak);
	//accentBrightnessBak = accentBrightness;

	//std::copy(fontColor, fontColor + 4, fontColorBak);
	//fontBrightnessBak = fontBrightness;

	//selectedFontBak = selectedFont;
	//fontSizeBak = fontSize;
	//boldFontBak = boldFont;

	//PerformanceData* performance = &userData->performance;

	//performance->guiLockedFps = guiLockedFps;
	//performance->lockGuiFps = lockGuiFps;

	//performance->showPlots = showPlots;

	//guiLockedFpsBak = guiLockedFps;
	//lockGuiFpsBak = lockGuiFps;

	//showPlotsBak = showPlots;

	backupUserData = currentUserData;

	HelperJson::SaveUserData(currentUserData);
	//delete userData;
}

bool LoadCurrentUserConfig()
{
	//UserData userData;
	if (!HelperJson::GetUserData(currentUserData))
		return false;

	//auto style = userData.style;
	//std::copy(style.mainColor, style.mainColor + 4, accentColor);
	//std::copy(style.fontColor, style.fontColor + 4, fontColor);
	//accentBrightness = style.mainColorBrightness;
	//fontBrightness = style.fontColorBrightness;
	//fontSize = style.fontSize;
	//selectedFont = style.selectedFont;

	//lockGuiFps = userData.performance.lockGuiFps;
	//guiLockedFps = userData.performance.guiLockedFps;

	//showPlots = userData.performance.showPlots;

	return true;
}

void ApplyCurrentStyle()
{
	auto& style = ImGui::GetStyle();

	float brightnesses[styleColorNum]{ currentUserData.style.mainColorBrightness, currentUserData.style.fontColorBrightness };
	float* colors[styleColorNum]{ currentUserData.style.mainColor, currentUserData.style.fontColor };

	for (int i = 0; i < styleColorNum; i++)
	{
		float brightness = brightnesses[i];

		auto baseColor = ImVec4(*(ImVec4*)colors[i]);
		auto darkerBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness);
		auto darkestBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness * 2);

		// Alpha has to be set separatly, because it also gets multiplied times brightness.
		auto alphaBase = baseColor.w;
		baseColor.w = alphaBase;
		darkerBase.w = alphaBase;
		darkestBase.w = alphaBase;

		switch (i)
		{
		case 0:
			style.Colors[ImGuiCol_Header] = baseColor;
			style.Colors[ImGuiCol_HeaderHovered] = darkerBase;
			style.Colors[ImGuiCol_HeaderActive] = darkestBase;

			style.Colors[ImGuiCol_TabHovered] = baseColor;
			style.Colors[ImGuiCol_TabActive] = darkerBase;
			style.Colors[ImGuiCol_Tab] = darkestBase;

			style.Colors[ImGuiCol_CheckMark] = baseColor;

			style.Colors[ImGuiCol_PlotLines] = style.Colors[ImGuiCol_PlotHistogram] = darkerBase;
			style.Colors[ImGuiCol_PlotLinesHovered] = style.Colors[ImGuiCol_PlotHistogramHovered] = baseColor;
			break;
		case 1:
			style.Colors[ImGuiCol_Text] = baseColor;
			style.Colors[ImGuiCol_TextDisabled] = darkerBase;
			break;
		}
	}
}

void SetPlotLinesColor(ImVec4 color)
{
	auto dark = color * (1 - currentUserData.style.mainColorBrightness);

	auto& style = ImGui::GetStyle();

	ImGui::PushStyleColor(ImGuiCol_PlotLines, dark);
	ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, color);
}

ImFont* GetFontBold(int baseFontIndex)
{
	ImGuiIO& io = ImGui::GetIO();
	std::string fontDebugName = (std::string)io.Fonts->Fonts[fontIndex[baseFontIndex]]->GetDebugName();
	if (fontDebugName.find('-') == std::string::npos || fontDebugName.find(',') == std::string::npos)
		return nullptr;
	auto fontName = fontDebugName.substr(0, fontDebugName.find('-'));
	auto _boldFont = (io.Fonts->Fonts[fontIndex[baseFontIndex] + 1]);
	auto boldName = ((std::string)(_boldFont->GetDebugName())).substr(0, fontDebugName.find('-'));
	if (fontName == boldName)
	{
		printf("found bold font %s\n", _boldFont->GetDebugName());
		return _boldFont;
	}
	return nullptr;
}

// This could also be done just using a single global variable like "hasStyleChanged", because ImGui elements like FloatSlider, ColorEdit4 return a value (true) if the value has changed.
// But this method would not work as expected when user reverts back the changes or sets the variable to it's original value
bool HasConfigChanged()
{
	bool brightnesses = currentUserData.style.mainColorBrightness == backupUserData.style.mainColorBrightness && currentUserData.style.fontColorBrightness == backupUserData.style.fontColorBrightness;
	bool font = currentUserData.style.selectedFont == backupUserData.style.selectedFont && currentUserData.style.fontSize == backupUserData.style.fontSize;
	bool performance = currentUserData.performance.guiLockedFps == backupUserData.performance.guiLockedFps &&
		currentUserData.performance.lockGuiFps == backupUserData.performance.lockGuiFps &&
		currentUserData.performance.showPlots == backupUserData.performance.showPlots &&
		currentUserData.performance.VSync == backupUserData.performance.VSync;

	bool colors{ false };
	// Compare arrays of colors column by column, because otherwise we would just compare pointers to these values which would always yield a false positive result.
	// (Pointers point to different addresses even tho the values at these addresses are the same)
	for (int column = 0; column < 4; column++)
	{
		if (currentUserData.style.mainColor[column] != backupUserData.style.mainColor[column])
		{
			colors = false;
			break;
		}

		if (currentUserData.style.fontColor[column] != backupUserData.style.fontColor[column])
		{
			colors = false;
			break;
		}

		if (currentUserData.style.internalPlotColor[column] != backupUserData.style.internalPlotColor[column])
		{
			colors = false;
			break;
		}

		if (currentUserData.style.externalPlotColor[column] != backupUserData.style.externalPlotColor[column])
		{
			colors = false;
			break;
		}

		if (currentUserData.style.inputPlotColor[column] != backupUserData.style.inputPlotColor[column])
		{
			colors = false;
			break;
		}

		colors = true;
	}

	if (!colors || !brightnesses || !font || !performance)
		return true;
	else
		return false;
}

void RecalculateStats(bool recalculate_Average = false)
{
	size_t size = tabsInfo[selectedTab].latencyData.measurements.size();
	if (size <= 0)
		return;
	if (recalculate_Average)
	{
		tabsInfo[selectedTab].latencyStats.externalLatency.highest = 0;
		tabsInfo[selectedTab].latencyStats.internalLatency.highest = 0;
		tabsInfo[selectedTab].latencyStats.inputLatency.highest = 0;

		unsigned int extSum = 0, intSum = 0, inpSum = 0;

		for (size_t i = 0; i < size; i++)
		{
			auto& test = tabsInfo[selectedTab].latencyData.measurements[i];

			if (i == 0)
			{
				tabsInfo[selectedTab].latencyStats.externalLatency.lowest = test.timeExternal;
				tabsInfo[selectedTab].latencyStats.internalLatency.lowest = test.timeInternal;
				tabsInfo[selectedTab].latencyStats.inputLatency.lowest = test.timePing;
			}

			extSum += test.timeExternal;
			intSum += test.timeInternal;
			inpSum += test.timePing;

			if (test.timeExternal > tabsInfo[selectedTab].latencyStats.externalLatency.highest)
			{
				tabsInfo[selectedTab].latencyStats.externalLatency.highest = test.timeExternal;
			}
			else if (test.timeExternal < tabsInfo[selectedTab].latencyStats.externalLatency.lowest)
			{
				tabsInfo[selectedTab].latencyStats.externalLatency.lowest = test.timeExternal;
			}

			if (test.timeInternal > tabsInfo[selectedTab].latencyStats.internalLatency.highest)
			{
				tabsInfo[selectedTab].latencyStats.internalLatency.highest = test.timeInternal;
			}
			else if (test.timeInternal < tabsInfo[selectedTab].latencyStats.internalLatency.lowest)
			{
				tabsInfo[selectedTab].latencyStats.internalLatency.lowest = test.timeInternal;
			}

			if (test.timePing > tabsInfo[selectedTab].latencyStats.inputLatency.highest)
			{
				tabsInfo[selectedTab].latencyStats.inputLatency.highest = test.timePing;
			}
			else if (test.timePing < tabsInfo[selectedTab].latencyStats.inputLatency.lowest)
			{
				tabsInfo[selectedTab].latencyStats.inputLatency.lowest = test.timePing;
			}
		}

		tabsInfo[selectedTab].latencyStats.externalLatency.average = static_cast<float>(extSum) / size;
		tabsInfo[selectedTab].latencyStats.internalLatency.average = static_cast<float>(intSum) / size;
		tabsInfo[selectedTab].latencyStats.inputLatency.average = static_cast<float>(inpSum) / size;
	}
	else
	{
		for (size_t i = 0; i < size; i++)
		{
			auto& test = tabsInfo[selectedTab].latencyData.measurements[i];

			if (i == 0)
			{
				tabsInfo[selectedTab].latencyStats.externalLatency.lowest = test.timeExternal;
				tabsInfo[selectedTab].latencyStats.internalLatency.lowest = test.timeInternal;
				tabsInfo[selectedTab].latencyStats.inputLatency.lowest = test.timePing;
			}

			if (test.timeExternal > tabsInfo[selectedTab].latencyStats.externalLatency.highest)
			{
				tabsInfo[selectedTab].latencyStats.externalLatency.highest = test.timeExternal;
			}
			else if (test.timeExternal < tabsInfo[selectedTab].latencyStats.externalLatency.lowest)
			{
				tabsInfo[selectedTab].latencyStats.externalLatency.lowest = test.timeExternal;
			}

			if (test.timeInternal > tabsInfo[selectedTab].latencyStats.internalLatency.highest)
			{
				tabsInfo[selectedTab].latencyStats.internalLatency.highest = test.timeInternal;
			}
			else if (test.timeInternal < tabsInfo[selectedTab].latencyStats.internalLatency.lowest)
			{
				tabsInfo[selectedTab].latencyStats.internalLatency.lowest = test.timeInternal;
			}

			if (test.timePing > tabsInfo[selectedTab].latencyStats.inputLatency.highest)
			{
				tabsInfo[selectedTab].latencyStats.inputLatency.highest = test.timePing;
			}
			else if (test.timePing < tabsInfo[selectedTab].latencyStats.inputLatency.lowest)
			{
				tabsInfo[selectedTab].latencyStats.inputLatency.lowest = test.timePing;
			}
		}
	}
}

// This code is very much unoptimized, but it has low optimization priority just because of how rarely this function is called
void RemoveMeasurement(size_t index)
{
	auto result = tabsInfo[selectedTab].latencyData.measurements[index];
	size_t size = tabsInfo[selectedTab].latencyData.measurements.size();
	bool findMax = false, findMin = false;

	// Fix stats
	if (size > 1)
	{
		auto _latencyStats = tabsInfo[selectedTab].latencyStats;
		_latencyStats.externalLatency.average = ((tabsInfo[selectedTab].latencyStats.externalLatency.average * size) - result.timeExternal) / (size - 1);
		_latencyStats.internalLatency.average = ((tabsInfo[selectedTab].latencyStats.internalLatency.average * size) - result.timeInternal) / (size - 1);
		_latencyStats.inputLatency.average = ((tabsInfo[selectedTab].latencyStats.inputLatency.average * size) - result.timePing) / (size - 1);

		tabsInfo[selectedTab].latencyStats = LatencyStats();
		ZeroMemory(&tabsInfo[selectedTab].latencyStats, sizeof(tabsInfo[selectedTab].latencyStats));
		tabsInfo[selectedTab].latencyStats.externalLatency.average = _latencyStats.externalLatency.average;
		tabsInfo[selectedTab].latencyStats.internalLatency.average = _latencyStats.internalLatency.average;
		tabsInfo[selectedTab].latencyStats.inputLatency.average = _latencyStats.inputLatency.average;

		// In most cases a measurement won't be an edge case so It's better to check if it is and only then try to find a new edge value
		//if(result.timeExternal == tabsInfo[selectedTab].latencyStats.externalLatency.highest || result.timeInternal == tabsInfo[selectedTab].latencyStats.internalLatency.highest || result.timePing == tabsInfo[selectedTab].latencyStats.inputLatency.highest ||
		//	result.timeExternal == tabsInfo[selectedTab].latencyStats.externalLatency.lowest || result.timeInternal == tabsInfo[selectedTab].latencyStats.internalLatency.lowest || result.timePing == tabsInfo[selectedTab].latencyStats.inputLatency.lowest)
		for (size_t i = 0; i < size; i++)
		{
			if (index == i)
				continue;

			auto& test = tabsInfo[selectedTab].latencyData.measurements[i];

			if (i == 0 || (index == 0 && i == 1))
			{
				tabsInfo[selectedTab].latencyStats.externalLatency.lowest = test.timeExternal;
				tabsInfo[selectedTab].latencyStats.internalLatency.lowest = test.timeInternal;
				tabsInfo[selectedTab].latencyStats.inputLatency.lowest = test.timePing;
			}

			//if (test.timeExternal > result.timeExternal)
			//{
			//	tabsInfo[selectedTab].latencyStats.externalLatency.highest = test.timeExternal;
			//}
			//else if (test.timeExternal < result.timeExternal)
			//{
			//	tabsInfo[selectedTab].latencyStats.externalLatency.lowest = test.timeExternal;
			//}

			//if (test.timeInternal > result.timeInternal)
			//{
			//	tabsInfo[selectedTab].latencyStats.internalLatency.highest = test.timeInternal;
			//}
			//else if (test.timeInternal < result.timeInternal)
			//{
			//	tabsInfo[selectedTab].latencyStats.internalLatency.lowest = test.timeInternal;
			//}

			//if (test.timePing > result.timePing)
			//{
			//	tabsInfo[selectedTab].latencyStats.inputLatency.highest = test.timePing;
			//}
			//else if (test.timeExternal < result.timeExternal)
			//{
			//	tabsInfo[selectedTab].latencyStats.inputLatency.lowest = test.timePing;
			//}

			//if (test.timeExternal > tabsInfo[selectedTab].latencyStats.externalLatency.highest)
			//{
			//	tabsInfo[selectedTab].latencyStats.externalLatency.highest = test.timeExternal;
			//}
			//else if (test.timeExternal < tabsInfo[selectedTab].latencyStats.externalLatency.lowest)
			//{
			//	tabsInfo[selectedTab].latencyStats.externalLatency.lowest = test.timeExternal;
			//}

			//if (test.timeInternal > tabsInfo[selectedTab].latencyStats.internalLatency.highest)
			//{
			//	tabsInfo[selectedTab].latencyStats.internalLatency.highest = test.timeInternal;
			//}
			//else if (test.timeInternal < tabsInfo[selectedTab].latencyStats.internalLatency.lowest)
			//{
			//	tabsInfo[selectedTab].latencyStats.internalLatency.lowest = test.timeInternal;
			//}

			//if (test.timePing > tabsInfo[selectedTab].latencyStats.inputLatency.highest)
			//{
			//	tabsInfo[selectedTab].latencyStats.inputLatency.highest = test.timePing;
			//}
			//else if (test.timePing < tabsInfo[selectedTab].latencyStats.inputLatency.lowest)
			//{
			//	tabsInfo[selectedTab].latencyStats.inputLatency.lowest = test.timePing;
			//}
		}

		tabsInfo[selectedTab].latencyData.measurements.erase(tabsInfo[selectedTab].latencyData.measurements.begin() + index);
		RecalculateStats();
	}
	else
	{
		tabsInfo[selectedTab].latencyStats = LatencyStats();
		tabsInfo[selectedTab].latencyData.measurements.clear();
		tabsInfo[selectedTab].isSaved = true;
	}
}

bool SaveMeasurements()
{
	if (tabsInfo[selectedTab].latencyData.measurements.size() < 1)
		return false;

	if (std::strlen(tabsInfo[selectedTab].savePath) == 0)
		return SaveAsMeasurements();

	LatencyData latencyData{};
	ZeroMemory(&latencyData, sizeof(LatencyData));

	latencyData.measurements = tabsInfo[selectedTab].latencyData.measurements;
	strcpy_s(latencyData.note, tabsInfo[selectedTab].latencyData.note);

	HelperJson::SaveLatencyTests(latencyData, tabsInfo[selectedTab].savePath);
	tabsInfo[selectedTab].isSaved = true;

	return tabsInfo[selectedTab].isSaved;
}

bool SaveAsMeasurements()
{
	if (tabsInfo[selectedTab].latencyData.measurements.size() < 1)
		return false;

	bool wasFullscreen = isFullscreen;
	if (isFullscreen)
	{
		GUI::g_pSwapChain->SetFullscreenState(false, nullptr);
		isFullscreen = false;
		fullscreenModeOpenPopup = false;
		fullscreenModeClosePopup = false;
	}

	char filename[MAX_PATH]{ "name" };
	const char szExt[] = "Json\0*.json\0\0";

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;

	if (GetSaveFileName(&ofn))
	{
		LatencyData latencyData{};
		ZeroMemory(&latencyData, sizeof(LatencyData));

		latencyData.measurements = tabsInfo[selectedTab].latencyData.measurements;
		strcpy_s(latencyData.note, tabsInfo[selectedTab].latencyData.note);

		HelperJson::SaveLatencyTests(latencyData, filename);
		strncpy_s(tabsInfo[selectedTab].savePath, filename, MAX_PATH);
		tabsInfo[selectedTab].isSaved = true;
	}
	else
		tabsInfo[selectedTab].isSaved = false;

	if (wasFullscreen)
	{
		GUI::g_pSwapChain->SetFullscreenState(true, nullptr);
		isFullscreen = true;
	}

	return tabsInfo[selectedTab].isSaved;
}

void OpenMeasurements()
{
	bool wasFullscreen = isFullscreen;
	if (isFullscreen)
	{
		GUI::g_pSwapChain->SetFullscreenState(false, nullptr);
		isFullscreen = false;
		fullscreenModeOpenPopup = false;
		fullscreenModeClosePopup = false;
	}

	char filename[MAX_PATH];
	const char szExt[] = "json\0";

	ZeroMemory(filename, sizeof(filename));

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
	ofn.Flags = OFN_DONTADDTORECENT;

	if (GetOpenFileName(&ofn))
	{
		LatencyData latencyData{};
		ZeroMemory(&latencyData, sizeof(LatencyData));

		ClearData();
		HelperJson::GetLatencyTests(latencyData, filename);

		tabsInfo[selectedTab].latencyData.measurements = latencyData.measurements;
		strcpy_s(tabsInfo[selectedTab].latencyData.note, latencyData.note);

		if (tabsInfo[selectedTab].latencyData.measurements.empty())
			return;

		RecalculateStats(true);
		auto x = 0;
	}

	// Add this to preferences or smth
	if (wasFullscreen)
	{
		GUI::g_pSwapChain->SetFullscreenState(true, nullptr);
		isFullscreen = true;
	}
}

static int FilterValidPath(ImGuiInputTextCallbackData* data)
{
	if (std::find(std::begin(invalidPathCharacters), std::end(invalidPathCharacters), data->EventChar) != std::end(invalidPathCharacters))  return 1;
	return 0;
}

void ClearData()
{
	tabsInfo[selectedTab].latencyData.measurements.clear();
	tabsInfo[selectedTab].latencyStats = LatencyStats();
	tabsInfo[selectedTab].isSaved = true;
	ZeroMemory(tabsInfo[selectedTab].latencyData.note, 1000);
}

// Not the best GUI solution, but it's simple, fast and gets the job done.
int OnGui()
{
	ImGuiStyle& style = ImGui::GetStyle();

	if (ImGui::BeginMenuBar())
	{
		//static bool saveMeasurementsPopup = false;
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open", "Ctrl+O")) { OpenMeasurements(); }
			if (ImGui::MenuItem("Save", "Ctrl+S")) { SaveMeasurements(); }
			if (ImGui::MenuItem("Save as", "")) { SaveAsMeasurements(); }
			if (ImGui::MenuItem("Settings", "")) { isSettingOpen = !isSettingOpen; }
			if (ImGui::MenuItem("Close", "")) { return 1; }
			ImGui::EndMenu();
		}

		//if (saveMeasurementsPopup)
		//	ImGui::OpenPopup("Save measurements");

		//if (ImGui::BeginPopupModal("Save measurements", &saveMeasurementsPopup, ImGuiWindowFlags_NoResize))
		//{
		//	auto modalAvail = ImGui::GetContentRegionAvail();
		//	static char inputName[MEASUREMENTS_FILE_NAME_LENGTH]{"Name"};
		//	for (size_t i = 0; i < MEASUREMENTS_FILE_NAME_LENGTH; i++)
		//	{
		//		char c = inputName[i];
		//		if (std::find(invalidPathCharacters, invalidPathCharacters + 9, c))
		//			inputName[i] = 0;
		//	}
		//	ImGui::InputText("Name", inputName, MEASUREMENTS_FILE_NAME_LENGTH, ImGuiInputTextFlags_CallbackCharFilter, FilterValidPath);
		//	if (ImGui::Button("Save", { modalAvail.x , ImGui::GetFrameHeightWithSpacing() }))
		//	{
		//		SaveMeasurements(inputName);
		//		ImGui::CloseCurrentPopup();
		//		saveMeasurementsPopup = false;
		//	}
		//	ImGui::EndPopup();
		//}
		// Draw FPS
		{
			auto menuBarAvail = ImGui::GetContentRegionAvail();

			const float width = ImGui::CalcTextSize("FPS12345FPS    FPS123456789FPS").x;

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (menuBarAvail.x - width));
			ImGui::BeginGroup();

			ImGui::Text("FPS:");
			ImGui::SameLine();
			ImGui::Text("%.1f GUI", ImGui::GetIO().Framerate);
			ImGui::SameLine();
			// Right click total frames to reset it
			ImGui::Text("%.1f CPU", 1000000.f / averageFrametime);
			static uint64_t totalFramerateStartTime{ micros() };
			// The framerate is sometimes just too high to be properly displayed by the moving average and it's "small" buffer. So this should show average framerate over the entire program lifespan
			TOOLTIP("Avg. framerate: %.1f", ((float)totalFrames / (micros() - totalFramerateStartTime)) * 1000000.f);
			if (ImGui::IsItemClicked(1))
			{
				totalFramerateStartTime = micros();
				totalFrames = 0;
			}

			ImGui::EndGroup();
		}
		ImGui::EndMenuBar();
	}

	const auto windowAvail = ImGui::GetContentRegionAvail();
	ImGuiIO& io = ImGui::GetIO();

	// Handle Shortcuts
	ImGuiKey pressedKeys[ImGuiKey_COUNT]{0};
	size_t addedKeys = 0;

	//ZeroMemory(pressedKeys, ImGuiKey_COUNT);

	for (ImGuiKey key = 0; key < ImGuiKey_COUNT; key++)
	{
		bool isPressed = ImGui::IsKeyPressed(key);

		// Shift, ctrl and alt
		if ((key >= 641 && key <= 643) || (key >= 527 && key <= 533))
			continue;

		if (ImGui::IsLegacyKey(key))
			continue;

		if (isPressed) {
			auto name = ImGui::GetKeyName(key);
			auto s = io.KeyMap;
			pressedKeys[addedKeys++] = key;
			//if (keyName == 's' && io.KeyCtrl && !io.KeyShift && !io.KeyAlt)
			//	SaveMeasurements();
		}
	}

	static bool wasEscapeUp{ true };

	if (addedKeys == 1)
	{
		const char* name = ImGui::GetKeyName(pressedKeys[0]);
		if (io.KeyCtrl)
		{
			io.ClearInputKeys();
			if (name[0] == 'S')
			{
				SaveMeasurements();
			}
			else if (name[0] == 'O')
			{
				OpenMeasurements();
			}
		}
		else if (io.KeyAlt)
		{
			io.ClearInputKeys();

			// Enter
			if (pressedKeys[0] == 615 || pressedKeys[0] == 525)
			{
				BOOL isFS;
				GUI::g_pSwapChain->GetFullscreenState(&isFS, nullptr);
				isFullscreen = isFS;
				if (!isFullscreen && !fullscreenModeOpenPopup)
				{
					ImGui::OpenPopup("Enter Fullscreen mode?");
					fullscreenModeOpenPopup = true;
				}
				else if(!fullscreenModeClosePopup && !fullscreenModeOpenPopup)
				{
					ImGui::OpenPopup("Exit Fullscreen mode?");
					fullscreenModeClosePopup = true;
				}
			}
		}
		else if (!io.KeyShift)
		{
			BOOL isFS;
			GUI::g_pSwapChain->GetFullscreenState(&isFS, nullptr);
			isFullscreen = isFS;
			if (pressedKeys[0] == ImGuiKey_Escape && isFullscreen && !fullscreenModeClosePopup && wasEscapeUp)
			{
				wasEscapeUp = false;
				ImGui::OpenPopup("Exit Fullscreen mode?");
				fullscreenModeClosePopup = true;
				wasEscapeUp = false;
			}

		}
	}

	if (ImGui::BeginPopupModal("Enter Fullscreen mode?", &fullscreenModeOpenPopup, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to enter Fullscreen mode?");
		TOOLTIP("Press Escape to Exit");
		ImGui::SeparatorSpace(0, { 0, 10 });
		if (ImGui::Button("Yes") || IS_ONLY_ENTER_PRESSED)
		{
			// I'm pretty sure I should also resize the swapchain buffer to use the new resolution, but maybe later
			if (GUI::g_pSwapChain->SetFullscreenState(true, nullptr) == S_OK)
				isFullscreen = true;
			else
				isFullscreen = false;

			ImGui::CloseCurrentPopup();
			fullscreenModeOpenPopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("No") || IS_ONLY_ESCAPE_PRESSED)
		{
			ImGui::CloseCurrentPopup();
			fullscreenModeOpenPopup = false;
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Exit Fullscreen mode?", &fullscreenModeClosePopup, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to exit Fullscreen mode?");
		ImGui::SeparatorSpace(0, { 0, 10 });

		// ImGui doesn't yet support focusing a button

		//ImGui::PushAllowKeyboardFocus(true);
		//ImGui::PushID("ExitFSYes");
		//ImGui::SetKeyboardFocusHere();
		//bool isYesDown = ImGui::Button("Yes");
		//ImGui::PopID();
		////ImGui::SetKeyboardFocusHere(-1);
		//auto curWindow = ImGui::GetCurrentWindow();
		//ImGui::PopAllowKeyboardFocus();
		//ImGui::SetFocusID(curWindow->GetID("ExitFSYes"), curWindow);

		if (ImGui::Button("Yes") || IS_ONLY_ENTER_PRESSED)
		{
			if (GUI::g_pSwapChain->SetFullscreenState(false, nullptr) == S_OK)
				isFullscreen = false;
			else
				isFullscreen = true;

			ImGui::CloseCurrentPopup();
			fullscreenModeClosePopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("No") || (IS_ONLY_ESCAPE_PRESSED && wasEscapeUp))
		{
			wasEscapeUp = false;
			ImGui::CloseCurrentPopup();
			fullscreenModeClosePopup = false;
		}
		ImGui::EndPopup();
	}

	if (ImGui::IsKeyReleased(ImGuiKey_Escape))
		wasEscapeUp = true;

	// following way might be better, but it requires a use of some weird values like 0xF for CTRL+S
	//if (io.InputQueueCharacters.size() == 1)
	//{
	//	ImWchar c = io.InputQueueCharacters[0];
	//	if (((c > ' ' && c <= 255) ? (char)c : '?' == 's') && io.KeyCtrl)
	//	{
	//		SaveMeasurements();
	//	}

	//}

	if (isSettingOpen)
	{
		ImGui::SetNextWindowSize({ 480.0f, 480.0f });

		bool wasLastSettingsOpen = isSettingOpen;
		if (ImGui::Begin("Settings", &isSettingOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
		{
			// On Settings Window Closed:
			if (wasLastSettingsOpen && !isSettingOpen && HasConfigChanged())
			{
				// Call a popup by name
				ImGui::OpenPopup("Save Style?");
			}

			// Define the messagebox popup
			if (ImGui::BeginPopupModal("Save Style?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				isSettingOpen = true;
				ImGui::PushFont(boldFont);
				ImGui::Text("Do you want to save before exiting?");
				ImGui::PopFont();


				ImGui::Separator();
				ImGui::Dummy({ 0, ImGui::GetTextLineHeight() });

				// These buttons don't really fit the window perfectly with some fonts, I might have to look into that.
				ImGui::BeginGroup();

				if (ImGui::Button("Save"))
				{
					SaveCurrentUserConfig();
					isSettingOpen = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Discard"))
				{
					RevertConfig();
					isSettingOpen = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndGroup();

				ImGui::EndPopup();
			}


			static int selectedSettings = 0;
			const auto avail = ImGui::GetContentRegionAvail();

			const char* items[2]{ "Style", "Performance" };

			// Makes list take ({listBoxSize}*100)% width of the parent
			float listBoxSize = 0.3f;

			if (ImGui::BeginListBox("##Setting", { avail.x * listBoxSize, avail.y }))
			{
				for (int i = 0; i < sizeof(items) / sizeof(items[0]); i++)
				{
					//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (style.ItemSpacing.x / 2) - 2);
					ImVec2 label_size = ImGui::CalcTextSize(items[i], NULL, true);
					bool isSelected = (selectedSettings == i);
					//if (ImGui::Selectable(items[i], isSelected, 0, { (avail.x * listBoxSize - (style.ItemSpacing.x) - (style.FramePadding.x * 2)) + 4, label_size.y }, style.FrameRounding))
					if (ImGui::Selectable(items[i], isSelected, 0, { 0, 0 }, style.FrameRounding))
					{
						selectedSettings = i;
					}
				}
				ImGui::EndListBox();
			}

			ImGui::SameLine();

			ImGui::BeginGroup();

			switch (selectedSettings)
			{
			case 0: // Style
			{
				ImGui::BeginChild("Style", { ((1 - listBoxSize) * avail.x) - style.FramePadding.x * 2, avail.y - ImGui::GetFrameHeightWithSpacing() }, true);

				ImGui::ColorEdit4("Main Color", currentUserData.style.mainColor);

				ImGui::PushID(02);
				ImGui::SliderFloat("Brightness", &currentUserData.style.mainColorBrightness, -0.5f, 0.5f);
				REVERT(&currentUserData.style.mainColorBrightness, backupUserData.style.mainColorBrightness);
				ImGui::PopID();

				auto _accentColor = ImVec4(*(ImVec4*)currentUserData.style.mainColor);
				auto darkerAccent = ImVec4(*(ImVec4*)&_accentColor) * (1 - currentUserData.style.mainColorBrightness);
				auto darkestAccent = ImVec4(*(ImVec4*)&_accentColor) * (1 - currentUserData.style.mainColorBrightness * 2);

				// Alpha has to be set separatly, because it also gets multiplied times brightness.
				auto alphaAccent = ImVec4(*(ImVec4*)currentUserData.style.mainColor).w;
				_accentColor.w = alphaAccent;
				darkerAccent.w = alphaAccent;
				darkestAccent.w = alphaAccent;

				ImGui::Dummy({ 0, ImGui::GetFrameHeight() / 2 });

				ImVec2 colorsAvail = ImGui::GetContentRegionAvail();

				ImGui::ColorButton("Accent Color", (ImVec4)_accentColor, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
				ImGui::ColorButton("Accent Color Dark", darkerAccent, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
				ImGui::ColorButton("Accent Color Darkest", darkestAccent, 0, { colorsAvail.x, ImGui::GetFrameHeight() });


				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


				ImGui::ColorEdit4("Font Color", currentUserData.style.fontColor);

				ImGui::PushID(12);
				ImGui::SliderFloat("Brightness", &currentUserData.style.fontColorBrightness, -1.0f, 1.0f, "%.3f");
				REVERT(&currentUserData.style.fontColorBrightness, backupUserData.style.fontColorBrightness);
				ImGui::PopID();

				auto _fontColor = ImVec4(*(ImVec4*)currentUserData.style.fontColor);
				auto darkerFont = ImVec4(*(ImVec4*)&_fontColor) * (1 - currentUserData.style.fontColorBrightness);

				// Alpha has to be set separatly, because it also gets multiplied times brightness.
				auto alphaFont = ImVec4(*(ImVec4*)currentUserData.style.fontColor).w;
				_fontColor.w = alphaFont;
				darkerFont.w = alphaFont;

				ImGui::Dummy({ 0, ImGui::GetFrameHeight() / 2 });

				ImGui::ColorButton("Font Color", _fontColor, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
				ImGui::ColorButton("Font Color Dark", darkerFont, 0, { colorsAvail.x, ImGui::GetFrameHeight() });

				//ImGui::SetCursorPosY(avail.y - ImGui::GetFrameHeight() - style.WindowPadding.y);

				//float colors[2][4];
				//float brightnesses[2]{ accentBrightness, fontBrightness };
				//memcpy(&colors, &accentColor, colorSize);
				//memcpy(&colors[1], &fontColor, colorSize);
				//ApplyStyle(colors, brightnesses);

				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


				ImGui::ColorEdit4("Int. Plot", currentUserData.style.internalPlotColor);
				ImGui::ColorEdit4("Ext. Plot", currentUserData.style.externalPlotColor);
				ImGui::ColorEdit4("Input Plot", currentUserData.style.inputPlotColor);

				ApplyCurrentStyle();


				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


				//ImGui::PushFont(io.Fonts->Fonts[fontIndex[selectedFont]]);
				// This has to be done manually (instead of ImGui::Combo()) to be able to change the fonts of each selectable to a corresponding one.
				if (ImGui::BeginCombo("Font", fonts[currentUserData.style.selectedFont]))
				{
					for (int i = 0; i < (io.Fonts->Fonts.Size / 2) + 1; i++)
					{
						auto selectableSpace = ImGui::GetContentRegionAvail();
						//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2);

						ImGui::PushFont(io.Fonts->Fonts[fontIndex[i]]);

						bool isSelected = (currentUserData.style.selectedFont == i);
						if (ImGui::Selectable(fonts[i], isSelected, 0, { 0, 0 }, style.FrameRounding))
						{
							if (currentUserData.style.selectedFont != i) {
								io.FontDefault = io.Fonts->Fonts[fontIndex[i]];
								if (auto _boldFont = GetFontBold(i); _boldFont != nullptr)
									boldFont = _boldFont;
								else
									boldFont = io.Fonts->Fonts[fontIndex[i]];
								currentUserData.style.selectedFont = i;
							}
						}
						ImGui::PopFont();
					}
					ImGui::EndCombo();
				}
				//ImGui::PopFont();

				bool hasFontSizeChanged = ImGui::SliderFloat("Font Size", &currentUserData.style.fontSize, 0.5f, 2, "%.2f");
				REVERT(&currentUserData.style.fontSize, backupUserData.style.fontSize);
				if (hasFontSizeChanged || ImGui::IsItemClicked(1))
				{
					for (int i = 0; i < io.Fonts->Fonts.Size; i++)
					{
						io.Fonts->Fonts[i]->Scale = currentUserData.style.fontSize;
					}
				}


				ImGui::EndChild();

				break;
			}
			// Performance
			case 1:
				if (ImGui::BeginChild("Performance", { ((1 - listBoxSize) * avail.x) - style.FramePadding.x * 2, avail.y - ImGui::GetFrameHeightWithSpacing() }, true))
				{
					auto performanceAvail = ImGui::GetContentRegionAvail();
					ImGui::Checkbox("###LockGuiFpsCB", &currentUserData.performance.lockGuiFps);
					TOOLTIP("It's strongly recommended to keep GUI FPS locked for the best performance");

					ImGui::BeginDisabled(!currentUserData.performance.lockGuiFps);
					ImGui::SameLine();
					ImGui::PushItemWidth(performanceAvail.x - ImGui::GetItemRectSize().x - style.FramePadding.x * 3 - ImGui::CalcTextSize("GUI Refresh Rate").x);
					//ImGui::SliderInt("GUI FPS", &guiLockedFps, 30, 360, "%.1f");
					ImGui::DragInt("GUI Refresh Rate", &currentUserData.performance.guiLockedFps, .5f, 30, 480);
					REVERT(&currentUserData.performance.guiLockedFps, backupUserData.performance.guiLockedFps);
					ImGui::PopItemWidth();
					ImGui::EndDisabled();

					ImGui::Spacing();

					ImGui::Checkbox("Show Plots", &currentUserData.performance.showPlots);
					TOOLTIP("Plots can have a small impact on the performance");

					//ImGui::Checkbox("VSync", &currentUserData.performance.VSync);
					ImGui::SliderInt("VSync", &(int&)currentUserData.performance.VSync, 0, 4, "%d", ImGuiSliderFlags_AlwaysClamp);
					TOOLTIP("This setting synchronizes your monitor with the program to avoid screen tearing.\n(Adds a significant amount of latency)");

					ImGui::EndChild();

					break;
				}
			}
			ImGui::BeginGroup();

			auto buttonsAvail = ImGui::GetContentRegionAvail();

			if (ImGui::Button("Revert", { (buttonsAvail.x / 2 - style.FramePadding.x), ImGui::GetFrameHeight() }))
			{
				RevertConfig();
			}

			ImGui::SameLine();

			if (ImGui::Button("Save", { (buttonsAvail.x / 2 - style.FramePadding.x), ImGui::GetFrameHeight() }))
			{
				SaveCurrentUserConfig();
				//SaveStyleConfig(colors, brightnesses, selectedFont, fontSize);
			}

			ImGui::EndGroup();

			ImGui::EndGroup();
		}
		ImGui::End();
	}

	if (ImGui::BeginTabBar("TestsTab", ImGuiTabBarFlags_AutoSelectNewTabs))
	{
		for (int tabN = 0; tabN < tabsInfo.size(); tabN++)
		{
			//char idText[24];
			//snprintf(idText, 24, "Tab Item %i", tabN);
			//ImGui::PushID(idText);

			char tabNameLabel[48];
			snprintf(tabNameLabel, 48, "%s###Tab%i", tabsInfo[tabN].name, tabN);

			//ImGui::PushItemWidth(ImGui::CalcTextSize(tabNameLabel).x + 100);
			if(!tabsInfo[tabN].isSaved)
				ImGui::SetNextItemWidth(ImGui::CalcTextSize(tabsInfo[tabN].name, NULL).x + (style.FramePadding.x * 2) + 15);
			if (ImGui::BeginTabItem(tabNameLabel, NULL, tabsInfo[tabN].isSaved ? ImGuiTabItemFlags_None : ImGuiTabItemFlags_UnsavedDocument))
			{
				if(selectedTab != tabN)
					selectedTab = tabN; 
				ImGui::EndTabItem(); 
			};
			//ImGui::PopItemWidth();
			//ImGui::PopID();

				char popupName[32]{ 0 };
				snprintf(popupName, 32, "Change tab %i name", tabN);

				if (ImGui::IsItemClicked(1) || (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)))
				{
#ifdef _DEBUG
					printf("Open tab %i tooltip\n", tabN);
#endif

					ImGui::OpenPopup(popupName);
				}

				if (ImGui::BeginPopup(popupName))
				{
					// Check if this name already exists
					if (ImGui::InputText("Tab name", tabsInfo[tabN].name, 32)) { }

					if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape))
						ImGui::CloseCurrentPopup();

					ImGui::EndPopup();
				}
			

			//if (ImGui::TabItemButton((std::string("Tab ") + std::to_string(tabN)).c_str()))
			//{
			//	if (tabN != 0)
			//		auto x = 0;
			//	selectedTab = tabN;
			//	//ImGui::EndTabItem(); 
			//};
		}

		if (ImGui::TabItemButton(" + ", ImGuiTabItemFlags_Trailing))
		{
			auto newTab = TabInfo();
			strcat_s<32>(newTab.name, std::to_string(tabsInfo.size()).c_str());
			tabsInfo.push_back(newTab);
			selectedTab = tabsInfo.size() - 1;

			//ImGui::EndTabItem(); 
		}

		//ImGui::Text("Selected Item %i", selectedTab);

		ImGui::EndTabBar();
	}

	const auto avail = ImGui::GetContentRegionAvail();

	if (ImGui::BeginChild("LeftChild", { avail.x / 2 - style.WindowPadding.x, avail.y - detectionRectSize.y - 5 }, true))
	{

#ifdef _DEBUG 

		//ImGui::Text("Time is: %ims", clock());
		ImGui::Text("Time is: %lu\xC2\xB5s", micros());

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (GUI ONLY)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (CPU ONLY)", (micros() - lastFrame) / 1000.f, 1000000.0f / (micros() - lastFrame));

		ImGui::Text("Application average %.3f \xC2\xB5s/frame (%.1f FPS) (CPU ONLY)", averageFrametime, 1000000.f / averageFrametime);

		// The buffer contains just too many frames to display them on the screen (and displaying every 10th or so frame makes no sense). Also smoothing out the data set would hurt the performance too much
		//ImGui::PlotLines("FPS Chart", [](void* data, int idx) { return sinf(float(frames[idx%10]) / 1000); }, frames, AVERAGE_FRAME_AMOUNT);

		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { avail.x, ImGui::GetFrameHeight() });

#endif // DEBUG

		//ImGui::Combo("Select Port", &selectedPort, Serial::availablePorts.data());

		static ImVec2 restItemsSize;

		ImGui::BeginGroup();

		ImGui::PushItemWidth(80);
		if (ImGui::BeginCombo("Port", Serial::availablePorts[selectedPort].c_str()))
		{
			for (int i = 0; i < Serial::availablePorts.size(); i++)
			{
				bool isSelected = (selectedPort == i);
				if (ImGui::Selectable(Serial::availablePorts[i].c_str(), isSelected, 0, { 0,0 }, style.FrameRounding))
				{
					if (selectedPort != i)
						Serial::Close();
					selectedPort = i;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();

		if (ImGui::Button("Refresh"))
		{
			Serial::FindAvailableSerialPorts();
		}

		ImGui::SameLine();
		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });

		ImGui::BeginDisabled(Serial::isConnected);
		if (ImGui::ButtonEx("Connect"))
		{
			Serial::Setup(Serial::availablePorts[selectedPort].c_str(), GotSerialChar);
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		ImGui::BeginDisabled(!Serial::isConnected);
		if (ImGui::Button("Disconnect"))
		{
			Serial::Close();
		}
		ImGui::EndDisabled();

		ImGui::EndGroup();

		auto portItemsSize = ImGui::GetItemRectSize();

		if (portItemsSize.x + restItemsSize.x + 30 < ImGui::GetContentRegionAvail().x)
		{
			ImGui::SameLine();
			ImGui::Dummy({ -10, 0 });
			ImGui::SameLine();
			//ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			//ImGui::SameLine();

			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });
		}
		else
			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 5 });

		ImGui::BeginGroup();

		ImGui::Checkbox("Game Mode", &isGameMode);
		TOOLTIP("This mode instead of lighting up the rectangle at the bottom will simulate pressing the left mouse button (fire in game).\nTo register the delay between input and shot in game.")

		ImGui::SameLine();
		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });

		if (ImGui::Button("Clear"))
		{
			ImGui::OpenPopup("Clear?");
		}

		if (ImGui::BeginPopupModal("Clear?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to clear EVERYTHING?");

			ImGui::SeparatorSpace(0, { 0, 10 });

			if (ImGui::Button("Yes") || IS_ONLY_ENTER_PRESSED)
			{
				ClearData();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("No") || IS_ONLY_ESCAPE_PRESSED)
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::EndGroup();

		restItemsSize = ImGui::GetItemRectSize();

		ImGui::Spacing();

		ImGui::PushFont(boldFont);

		if (ImGui::BeginChild("MeasurementStats", { 0, ImGui::GetTextLineHeightWithSpacing() * 4 + style.WindowPadding.y * 2 - style.ItemSpacing.y + style.FramePadding.y * 2 }, true))
		{
			/*
			//ImGui::BeginGroup();
			//ImGui::Text("");
			//ImGui::Text("Highest");
			//ImGui::Text("Average");
			//ImGui::Text("Minimum");
			//ImGui::EndGroup();

			//ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();

			//ImGui::BeginGroup();
			//ImGui::Text("Internal");
			//ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000);
			//ImGui::Text("%.2f", tabsInfo[selectedTab].latencyStats.internalLatency.average / 1000.f);
			//ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.internalLatency.lowest / 1000);
			//ImGui::EndGroup();

			//ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();

			//ImGui::BeginGroup();
			//ImGui::Text("External");
			//ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.externalLatency.highest);
			//ImGui::Text("%.2f", tabsInfo[selectedTab].latencyStats.externalLatency.average);
			//ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.externalLatency.lowest);
			//ImGui::EndGroup();

			//ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();

			//ImGui::BeginGroup();
			//ImGui::Text("Input");
			//ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.inputLatency.highest / 1000);
			//ImGui::Text("%.2f", tabsInfo[selectedTab].latencyStats.inputLatency.average / 1000.f);
			//ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000);
			//ImGui::EndGroup();
			*/

			if (ImGui::BeginTable("Latency Stats Table", 4, ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoPadOuterX))
			{
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("Internal");
				TOOLTIPFONT("Time measured by the computer from the moment it got the start signal (button press) to the signal that the light intensity change was spotted");
				ImGui::TableNextColumn();
				ImGui::Text("External");
				TOOLTIPFONT("Time measured by the microcontroller from the button press to the change of light intensity");
				ImGui::TableNextColumn();
				ImGui::Text("Input");
				TOOLTIPFONT("Time between computer sending a message to the microcontroller and receiving it back (Ping)");

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Highest");
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.externalLatency.highest);
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.inputLatency.highest / 1000);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Average");
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", tabsInfo[selectedTab].latencyStats.internalLatency.average / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", tabsInfo[selectedTab].latencyStats.externalLatency.average);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", tabsInfo[selectedTab].latencyStats.inputLatency.average / 1000);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Lowest");
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.internalLatency.lowest / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.externalLatency.lowest);
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000);

				ImGui::EndTable();
			}

			ImGui::EndChild();
		}

		ImGui::PopFont();
	}

	if (currentUserData.performance.showPlots)
	{
		auto plotsAvail = ImGui::GetContentRegionAvail();
		auto plotHeight = min(max((plotsAvail.y - (4 * style.FramePadding.y)) / 4, 40), 100);
		// Separate Plots
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.internalPlotColor);
		ImGui::PlotLines("Internal Latency", [](void* data, int idx) { return tabsInfo[selectedTab].latencyData.measurements[idx].timeInternal / 1000.f; }, tabsInfo[selectedTab].latencyData.measurements.data(), tabsInfo[selectedTab].latencyData.measurements.size(), 0, NULL, FLT_MAX, FLT_MAX, { 0,plotHeight });
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.externalPlotColor);
		ImGui::PlotLines("External Latency", [](void* data, int idx) { return (float)tabsInfo[selectedTab].latencyData.measurements[idx].timeExternal; }, tabsInfo[selectedTab].latencyData.measurements.data(), tabsInfo[selectedTab].latencyData.measurements.size(), 0, NULL, FLT_MAX, FLT_MAX, { 0,plotHeight });
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.inputPlotColor);
		ImGui::PlotLines("Input Latency", [](void* data, int idx) { return tabsInfo[selectedTab].latencyData.measurements[idx].timePing / 1000.f; }, tabsInfo[selectedTab].latencyData.measurements.data(), tabsInfo[selectedTab].latencyData.measurements.size(), 0, NULL, FLT_MAX, FLT_MAX, { 0,plotHeight });
		ImGui::PopStyleColor(6);

		// Combined Plots
		auto startCursorPos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(startCursorPos);
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.internalPlotColor);
		ImGui::PlotLines("Combined Plots", [](void* data, int idx) { return tabsInfo[selectedTab].latencyData.measurements[idx].timeInternal / 1000.f; }, tabsInfo[selectedTab].latencyData.measurements.data(), tabsInfo[selectedTab].latencyData.measurements.size(), 0, NULL, tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000, tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000 + 1, { 0,plotHeight });

		ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });
		ImGui::PushStyleColor(ImGuiCol_Border, { 0,0,0,0 });
		ImGui::SetCursorPos(startCursorPos);
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.externalPlotColor);
		ImGui::PlotLines("###ExternalPlot", [](void* data, int idx) { return (float)tabsInfo[selectedTab].latencyData.measurements[idx].timeExternal; }, tabsInfo[selectedTab].latencyData.measurements.data(), tabsInfo[selectedTab].latencyData.measurements.size(), 0, NULL, tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000, tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000 + 1, { 0,plotHeight });

		ImGui::SetCursorPos(startCursorPos);
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.inputPlotColor);
		ImGui::PlotLines("###InputPlot", [](void* data, int idx) { return tabsInfo[selectedTab].latencyData.measurements[idx].timePing / 1000.f; }, tabsInfo[selectedTab].latencyData.measurements.data(), tabsInfo[selectedTab].latencyData.measurements.size(), 0, NULL, tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000, tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000 + 1, { 0,plotHeight });
		ImGui::PopStyleColor(8);
	}

	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginGroup();

	auto tableAvail = ImGui::GetContentRegionAvail();

	if (ImGui::BeginTable("measurementsTable", 5, ImGuiTableFlags_Reorderable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, { avail.x / 2, 200 }))
	{
		ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
		ImGui::TableSetupColumn("External Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 1);
		ImGui::TableSetupColumn("Internal Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 2);
		ImGui::TableSetupColumn("Input Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 3);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.0f, 4);
		ImGui::TableHeadersRow();

		// Copy of the original vector has to be used because there is a possibility that some of the items will be removed from it. In which case changes will be updated on the next frame
		std::vector<LatencyReading> _latencyTestsCopy = tabsInfo[selectedTab].latencyData.measurements;

		if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
			if (sorts_specs->SpecsDirty)
			{
				sortSpec = sorts_specs;
				if (tabsInfo[selectedTab].latencyData.measurements.size() > 1)
					qsort(&tabsInfo[selectedTab].latencyData.measurements[0], (size_t)tabsInfo[selectedTab].latencyData.measurements.size(), sizeof(tabsInfo[selectedTab].latencyData.measurements[0]), LatencyCompare);
				sortSpec = NULL;
				sorts_specs->SpecsDirty = false;
			}

		ImGuiListClipper clipper;
		clipper.Begin(_latencyTestsCopy.size());
		while (clipper.Step())
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				auto& reading = _latencyTestsCopy[i];

				ImGui::PushID(i + 100);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.index);
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.timeExternal);
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.timeInternal / 1000);
				// \xC2\xB5 is the microseconds character (looks like u (not you, just "u"))
				TOOLTIP("%i\xC2\xB5s", reading.timeInternal);

				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.timePing / 1000);
				TOOLTIP("%i\xC2\xB5s", reading.timePing);

				ImGui::TableNextColumn();

				if (ImGui::SmallButton("X"))
				{
					RemoveMeasurement(i);
				}

				ImGui::PopID();
			}

		ImGui::EndTable();
	}

	//ImGui::SameLine();

	// Notes area
	ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 10 });
	ImGui::Text("Measurement notes");
	auto notesAvail = ImGui::GetContentRegionAvail();
	ImGui::InputTextMultiline("Measurements Notes", tabsInfo[selectedTab].latencyData.note, 1000, { tableAvail.x, min(notesAvail.y - detectionRectSize.y - 5, 600)});
	//ImGui::InputTextMultiline("Measurements Notes", tabsInfo[selectedTab].latencyData.note, 1000, { tableAvail.x, min(tableAvail.y - 200 - detectionRectSize.y - style.WindowPadding.y - style.ItemSpacing.y * 5, 600) - ImGui::GetItemRectSize().y - 8});

	ImGui::EndGroup();

	// Color change detection rectangle.
	ImVec2 rectSize{ 0, detectionRectSize.y };
	rectSize.x = detectionRectSize.x == 0 ? windowAvail.x + style.WindowPadding.x + style.FramePadding.x : detectionRectSize.x;
	ImVec2 pos = { 0, windowAvail.y - rectSize.y + style.WindowPadding.y * 2 + style.FramePadding.y * 2 + 10 };
	ImRect bb{ pos, pos + rectSize };
	ImGui::RenderFrame(
		bb.Min,
		bb.Max,
		// Change the color to white to be detected by the light sensor
		serialStatus == Status_WaitingForResult && !isGameMode ? ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1)) : ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 0.f, 0.f, 1.f)),
		false
	);
	
	static bool isExitingWindowOpen = false;

	if (isExiting)
	{
		if (!tabsInfo[selectedTab].isSaved)
		{
			isExitingWindowOpen = true;
			ImGui::OpenPopup("Exit?");
		}
	}

	if (ImGui::BeginPopupModal("Exit?", &isExitingWindowOpen, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to exit before saving?");

		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 5 });

		if (ImGui::Button("Save and exit"))
		{
			if (SaveMeasurements())
			{
				ImGui::CloseCurrentPopup();
				isExitingWindowOpen = false;
				return 2;
			}
			else
				printf("Could not save the file");
		}
		ImGui::SameLine();
		if (ImGui::Button("Exit"))
		{
			ImGui::CloseCurrentPopup();
			isExitingWindowOpen = false;
			ImGui::EndPopup();
			return 2;
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
			isExitingWindowOpen = false;
			isExiting = false;
		}
		ImGui::EndPopup();
	}

	lastFrameGui = micros();

	return 0;
}

std::chrono::steady_clock::time_point lastTimeGotChar;

void GotSerialChar(char c)
{
#ifdef _DEBUG
	printf("Got: %c\n", c);
#endif

	// 5 numbers should be enough. I doubt the latency can be bigger than 10 seconds (anything greater than 100ms should be discarded)
	static BYTE resultBuffer[5]{0};
	static BYTE resultNum = 0;

	static std::chrono::steady_clock::time_point internalStartTime;
	static std::chrono::steady_clock::time_point internalEndTime;

	static std::chrono::steady_clock::time_point pingStartTime;

	switch (serialStatus)
	{
	case Status_Idle:
		// Input (button press) detected (l for light)
		if (c == 'l')
		{
			internalStartTime = std::chrono::high_resolution_clock::now();
#ifdef _DEBUG
			printf("waiting for results\n");
#endif
			serialStatus = Status_WaitingForResult;


			if (isGameMode)
			{
				INPUT input;
				ZeroMemory(&input, sizeof(input));

				input.type = INPUT_MOUSE;
				input.mi.dx = 0;
				input.mi.dy = 0;
				input.mi.mouseData = 0;
				input.mi.dwExtraInfo = NULL;
				input.mi.time = 0;
				input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_LEFTUP;

				if (SendInput(1, &input, sizeof(input)))
				{
					wasMouseClickSent = false;
					wasLMB_Pressed = false;
				}
			}
		}
		break;
	case Status_WaitingForResult:
		internalEndTime = std::chrono::high_resolution_clock::now();
		// e for end (end of the numbers)
		// All of the code below will have to be moved to a sepearate function in the future when saving/loading from a file will be added.
		if (c == 'e')
		{
			// Because we are subtracting 2 similar unsigned longs longs, we dont need another unsigned long long, we just need and int
			unsigned int internalTime = std::chrono::duration_cast<std::chrono::microseconds>(internalEndTime - internalStartTime).count();
			unsigned int externalTime = 0;
			// Convert the byte array to int
			for (int i = 0; i < resultNum; i++)
			{
				externalTime += resultBuffer[i] * pow<int>(10, (resultNum - i - 1));
			}

			wasMouseClickSent = false;
			wasLMB_Pressed = false;
#ifdef _DEBUG
			printf("Final result: %i\n", externalTime);
#endif
			LatencyReading reading{ externalTime, internalTime };
			size_t size = tabsInfo[selectedTab].latencyData.measurements.size();

			// Update the stats
			tabsInfo[selectedTab].latencyStats.internalLatency.average = (tabsInfo[selectedTab].latencyStats.internalLatency.average * size) / (size + 1.f) + (internalTime / (size + 1.f));
			tabsInfo[selectedTab].latencyStats.externalLatency.average = (tabsInfo[selectedTab].latencyStats.externalLatency.average * size) / (size + 1.f) + (externalTime / (size + 1.f));

			tabsInfo[selectedTab].latencyStats.internalLatency.highest = internalTime > tabsInfo[selectedTab].latencyStats.internalLatency.highest ? internalTime : tabsInfo[selectedTab].latencyStats.internalLatency.highest;
			tabsInfo[selectedTab].latencyStats.externalLatency.highest = externalTime > tabsInfo[selectedTab].latencyStats.externalLatency.highest ? externalTime : tabsInfo[selectedTab].latencyStats.externalLatency.highest;

			tabsInfo[selectedTab].latencyStats.internalLatency.lowest = internalTime < tabsInfo[selectedTab].latencyStats.internalLatency.lowest ? internalTime : tabsInfo[selectedTab].latencyStats.internalLatency.lowest;
			tabsInfo[selectedTab].latencyStats.externalLatency.lowest = externalTime < tabsInfo[selectedTab].latencyStats.externalLatency.lowest ? externalTime : tabsInfo[selectedTab].latencyStats.externalLatency.lowest;

			// If there are not measurements done yet set the minimum value to the current one
			if (size == 0)
			{
				tabsInfo[selectedTab].latencyStats.internalLatency.lowest = internalTime;
				tabsInfo[selectedTab].latencyStats.externalLatency.lowest = externalTime;
			}

			reading.index = size;
			tabsInfo[selectedTab].latencyData.measurements.push_back(reading);
			resultNum = 0;
			std::fill_n(resultBuffer, 5, 0);
#ifdef BufferedSerialComm
			_write(Serial::fd, "p", 1);
#else
			Serial::Write("p", 1);
#endif
			pingStartTime = std::chrono::high_resolution_clock::now();
			//fwrite(&ch, sizeof(char), 1, Serial::hFile);
			//fflush(Serial::hFile);
			serialStatus = Status_WaitingForPing;
		}
		else
		{
			int digit = c - '0';
			resultBuffer[resultNum] = digit;
			resultNum += 1;
		}
		break;
	case Status_WaitingForPing:
		if (c == 'b')
		{
			unsigned int pingTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - pingStartTime).count();
			tabsInfo[selectedTab].latencyData.measurements[tabsInfo[selectedTab].latencyData.measurements.size() - 1].timePing = pingTime;

			size_t size = tabsInfo[selectedTab].latencyData.measurements.size() - 1;

			tabsInfo[selectedTab].latencyStats.inputLatency.average = (tabsInfo[selectedTab].latencyStats.inputLatency.average * size) / (size + 1.f) + (pingTime / (size + 1.f));
			tabsInfo[selectedTab].latencyStats.inputLatency.highest = pingTime > tabsInfo[selectedTab].latencyStats.inputLatency.highest ? pingTime : tabsInfo[selectedTab].latencyStats.inputLatency.highest;
			tabsInfo[selectedTab].latencyStats.inputLatency.lowest = pingTime < tabsInfo[selectedTab].latencyStats.inputLatency.lowest ? pingTime : tabsInfo[selectedTab].latencyStats.inputLatency.lowest;

			if (size == 0)
			{
				tabsInfo[selectedTab].latencyStats.inputLatency.lowest = pingTime;
			}

			serialStatus = Status_Idle;
			tabsInfo[selectedTab].isSaved = false;
		}
		break;
	default:
		break;
	}

#ifdef _DEBUG
	printf("char receive delay: %lld\n", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - lastTimeGotChar).count());
#endif
	lastTimeGotChar = std::chrono::high_resolution_clock::now();
}

// Will be removed in some later push
#ifdef LocalSerial
bool SetupSerialPort(char COM_Number)
{
	std::string serialCom = "\\\\.\\COM";
	serialCom += COM_Number;
	hPort = CreateFile(
		serialCom.c_str(),
		GENERIC_WRITE | GENERIC_READ,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, // Make reading async
		NULL
	);

	DCB serialParams;
	serialParams.ByteSize = sizeof(serialParams);

	if (!GetCommState(hPort, &serialParams))
		return false;

	serialParams.BaudRate = 19200; // Can be changed, but this value is fast enought not to introduce any significant latency and pretty reliable
	serialParams.Parity = NOPARITY;
	serialParams.ByteSize = 8;
	serialParams.StopBits = ONESTOPBIT;

	if (!SetCommState(hPort, &serialParams))
		return false;

	COMMTIMEOUTS timeout = { 0 };
	timeout.ReadIntervalTimeout = MAXDWORD;
	timeout.ReadTotalTimeoutConstant = 0;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 0;
	timeout.WriteTotalTimeoutMultiplier = 0;

	if (SetCommTimeouts(hPort, &timeout))
		return false;

	//SetCommMask(hPort, EV_RXCHAR | EV_ERR); //receive character event

	return true;
}

void HandleSerial()
{
	DWORD dwBytesTransferred;
	DWORD dwCommModemStatus{};
	BYTE byte = NULL;

	/*
	//if (ReadFile(hPort, &byte, 1, &dwBytesTransferred, NULL))
	//	printf("Got: %c\n", byte);

	//return;

	//SetCommMask(hPort, EV_RXCHAR | EV_ERR); //receive character event
	//WaitCommEvent(hPort, &dwCommModemStatus, 0); //wait for character

	//if (dwCommModemStatus & EV_RXCHAR)
	// Does not work.
	//ReadFile(hPort, &byte, 1, NULL, NULL); //read 1
	//else if (dwCommModemStatus & EV_ERR)
	//	return;

	//OVERLAPPED o = { 0 };
	//o.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	//SetCommMask(hPort, EV_RXCHAR);

	//if (!WaitCommEvent(hPort, &dwCommModemStatus, &o))
	//{
	//	DWORD er = GetLastError();
	//	if (er == ERROR_IO_PENDING)
	//	{
	//		DWORD dwRes = WaitForSingleObject(o.hEvent, 1);
	//		switch (dwRes)
	//		{
	//			case WAIT_OBJECT_0:
	//				if (ReadFile(hPort, &byte, 1, &dwRead, &o))
	//					printf("Got: %c\n", byte);
	//				break;
	//		default:
	//			printf("care off");
	//			break;
	//		}
	//	}
	//	// Check GetLastError for ERROR_IO_PENDING, if I/O is pending then
	//	// use WaitForSingleObject() to determine when `o` is signaled, then check
	//	// the result. If a character arrived then perform your ReadFile.
	//}

		*/

	DWORD dwRead;
	BOOL fWaitingOnRead = FALSE;

	if (osReader.hEvent == NULL)
	{
		printf("Creating a new reader Event\n");
		osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	if (!fWaitingOnRead)
	{
		// Issue read operation.
		if (!ReadFile(hPort, &byte, 1, &dwRead, &osReader))
		{
			if (GetLastError() != ERROR_IO_PENDING)
				printf("IO Error");
			else
				fWaitingOnRead = TRUE;
		}
		else
		{
			// read completed immediately
			if (dwRead)
				GotSerialChar(byte);
		}
	}

	const DWORD READ_TIMEOUT = 1;

	DWORD dwRes;

	if (fWaitingOnRead) {
		dwRes = WaitForSingleObject(osReader.hEvent, READ_TIMEOUT);
		switch (dwRes)
		{
			// Read completed.
		case WAIT_OBJECT_0:
			if (!GetOverlappedResult(hPort, &osReader, &dwRead, FALSE))
				printf("IO Error");
			// Error in communications; report it.
			else
				// Read completed successfully.
				GotSerialChar(byte);

			//  Reset flag so that another opertion can be issued.
			fWaitingOnRead = FALSE;
			break;

			//case WAIT_TIMEOUT:
			//	break;

		default:
			printf("Event Error");
			break;
		}
	}

	/*
		//if (dwCommModemStatus & EV_RXCHAR) {
		//	ReadFile(hPort, &byte, 1, &dwBytesTransferred, 0); //read 1
		//	printf("Got: %c\n", byte);
		//}

		//COMSTAT comStat;
		//DWORD   dwErrors;
		//int bytesToRead = ClearCommError(hPort, &dwErrors, &comStat);

		//if (bytesToRead)
		//{
		//	ReadFile(hPort, &byte, 1, &dwBytesTransferred, 0);
		//	printf("There are %i bytes to read", bytesToRead);
		//	printf("Got: %c\n", byte);
		//}
		*/

	if (byte == NULL)
		return;


			}

void CloseSerial()
{
	CloseHandle(hPort);
	CloseHandle(osReader.hEvent);
}
#endif

bool GetMonitorModes(DXGI_MODE_DESC* modes, UINT* size)
{
	IDXGIOutput* output;

	if (GUI::g_pSwapChain->GetContainingOutput(&output) != S_OK)
		return false;

	if (output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, NULL, size, modes) != S_OK)
	{
		return false;
		printf("Error getting the display modes\n");
	}
}

// It turns out you can just send MOUSEEVENTF_LEFTUP and MOUSEEVENTF_LEFTDOWN at the same time, and it counts as a click...
// (Unused)
void HandleGameMode()
{
	assert(false);
	if (!isGameMode)
		return;

	if (serialStatus != Status_WaitingForResult || wasMouseClickSent)
		return;

	INPUT input;
	ZeroMemory(&input, sizeof(input));

	input.type = INPUT_MOUSE;
	input.mi.dx = 0;
	input.mi.dy = 0;
	//input.mi.mouseData = NULL;
	//input.mi.dwExtraInfo = 0;
	//input.mi.time = 0;
	input.mi.dwFlags = MOUSEEVENTF_LEFTUP | MOUSEEVENTF_LEFTDOWN;

	if (SendInput(1, &input, sizeof(input)))
	{
		wasMouseClickSent = true;
		wasLMB_Pressed = true;
	}

	//if (SendInput(1, &input, sizeof(input)))
	//{
	//	wasMouseClickSent = wasLMB_Pressed;
	//	wasLMB_Pressed = true;
	//}
	//input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
	//if (SendInput(1, &input, sizeof(input)))
	//{
	//	wasMouseClickSent = wasLMB_Pressed;
	//	wasLMB_Pressed = true;
	//}
}

// Returns true if should exit, false otherwise
bool OnExit()
{
	if (tabsInfo[selectedTab].isSaved)
	{
		// Gui closes itself from the wnd messages
		Serial::Close();

		exit(0);
	}
	isExiting = true;
	return tabsInfo[selectedTab].isSaved;
}

// Main code
int main(int args, char** argv)
{
	hwnd = GUI::Setup(OnGui);
	GUI::onExitFunc = OnExit;

	localPath = argv[0];

	QueryPerformanceCounter(&StartingTime);

#ifndef _DEBUG
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#else
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
#endif

	//::ShowWindow(::GetConsoleWindow(), SW_SHOW);

	static unsigned int frameIndex = 0;
	static unsigned int frameSum = 0;

	bool done = false;

	ImGuiIO& io = ImGui::GetIO();

	auto defaultTab = TabInfo();
	defaultTab.name[4] = '0';
	tabsInfo.push_back(defaultTab);

	UINT modesNum = 256;
	DXGI_MODE_DESC monitorModes[256];
	GetMonitorModes(monitorModes, &modesNum);

	GUI::MAX_SUPPORTED_FRAMERATE = monitorModes[modesNum - 1].RefreshRate.Numerator;
	GUI::VSyncFrame = &currentUserData.performance.VSync;

	//float colors[styleColorNum][4];
	//float brightnesses[styleColorNum] = { accentBrightness, fontBrightness };
	//if (ReadStyleConfig(colors, brightnesses, selectedFont, fontSize))
	if (LoadCurrentUserConfig())
	{
		ApplyCurrentStyle();
		//ApplyStyle(colors, brightnesses);

		//// Set the default colors to the varaibles with a type conversion (ImVec4 -> float[4]) (could be done with std::copy, but the performance advantage it gives is just unmeasurable, especially for a single time execution code)
		//memcpy(&accentColor, &(colors[0][0]), colorSize);
		//memcpy(&fontColor, &(colors[1][0]), colorSize);

		//accentBrightness = brightnesses[0];
		//fontBrightness = brightnesses[1];

		//memcpy(&accentColorBak, &accentColor, colorSize);
		//memcpy(&fontColorBak, &fontColor, colorSize);

		//accentBrightnessBak = accentBrightness;
		//fontBrightnessBak = fontBrightness;

		//selectedFontBak = selectedFont;
		//fontSizeBak = fontSize;

		//guiLockedFpsBak = guiLockedFps;
		//lockGuiFpsBak = lockGuiFps;

		//showPlotsBak = showPlots;

		backupUserData = currentUserData;

		GUI::LoadFonts(currentUserData.style.fontSize);

		for (int i = 0; i < io.Fonts->Fonts.Size; i++)
		{
			io.Fonts->Fonts[i]->Scale = currentUserData.style.fontSize;
		}

		io.FontDefault = io.Fonts->Fonts[fontIndex[currentUserData.style.selectedFont]];

		auto _boldFont = GetFontBold(currentUserData.style.selectedFont);

		if (_boldFont != nullptr)
			boldFont = _boldFont;
		else
			boldFont = io.Fonts->Fonts[fontIndex[currentUserData.style.selectedFont]];

		//boldFontBak = boldFont;
	}
	else
	{
		auto& styleColors = ImGui::GetStyle().Colors;
		memcpy(&currentUserData.style.mainColor, &(styleColors[ImGuiCol_Header]), colorSize);
		memcpy(&currentUserData.style.fontColor, &(styleColors[ImGuiCol_Text]), colorSize);

		memcpy(&backupUserData.style.mainColor, &(styleColors[ImGuiCol_Header]), colorSize);
		memcpy(&backupUserData.style.fontColor, &(styleColors[ImGuiCol_Text]), colorSize);

		GUI::LoadFonts(1);

		currentUserData.performance.lockGuiFps = backupUserData.performance.lockGuiFps = true;

		currentUserData.performance.showPlots = backupUserData.performance.showPlots = true;

		currentUserData.performance.VSync = backupUserData.performance.VSync = false;

		printf("found %u monitor modes\n", modesNum);
		currentUserData.performance.guiLockedFps = monitorModes[modesNum - 1].RefreshRate.Numerator * 2;
		backupUserData.performance.guiLockedFps = currentUserData.performance.guiLockedFps;

		currentUserData.style.mainColorBrightness = backupUserData.style.mainColorBrightness = .1f;
		currentUserData.style.fontColorBrightness = backupUserData.style.fontColorBrightness = .1f;
		currentUserData.style.fontSize = backupUserData.style.fontSize = 1.f;

		// Note: this style might be a little bit different prior to applying it. (different darker colors)
	}

	LoadCurrentUserConfig();

	Serial::FindAvailableSerialPorts();


	//BOOL fScreen;
	//IDXGIOutput* output;
	//GUI::g_pSwapChain->GetFullscreenState(&fScreen, &output);
	//isFullscreen = fScreen;


	//if (Serial::Setup("COM4", GotSerialChar))
	//	printf("Serial Port opened successfuly");
	//else
	//	printf("Error setting up the Serial Port");

	// used to cover the time of rendering a frame when calculating when a next frame should be displayed
	unsigned int lastFrameRenderTime = 0;

	int GUI_Return_Code = 0;

MainLoop:

	// Main Loop
	while (!done)
	{
		Serial::HandleInput();

		//if(isGameMode)
		//	HandleGameMode();

		// GUI Loop

		uint64_t curTime = micros();
		if ((curTime - lastFrameGui + (lastFrameRenderTime) >= 1000000 / (currentUserData.performance.VSync ? (GUI::MAX_SUPPORTED_FRAMERATE/currentUserData.performance.VSync)-1 : currentUserData.performance.guiLockedFps)) || !currentUserData.performance.lockGuiFps)
		{
			uint64_t frameStartRender = curTime;
			if (GUI_Return_Code = GUI::DrawGui())
				break;
			lastFrameRenderTime = lastFrameGui - frameStartRender;
			curTime += lastFrameRenderTime;
		}

		//uint64_t CPUTime = micros();

		unsigned int newFrame = curTime - lastFrame;

		// Average frametime calculation
		frameSum -= frames[frameIndex];
		frameSum += newFrame;
		frames[frameIndex] = newFrame;
		frameIndex = (frameIndex + 1) % AVERAGE_FRAME_AMOUNT;

		averageFrametime = ((float)frameSum) / AVERAGE_FRAME_AMOUNT;

		totalFrames++;
		lastFrame = curTime;

		// Limit FPS for eco-friendly purposes (Significantly affects the performance) (Windows does not support sub 1 millisecond sleep)
		//Sleep(1);
	}

	// Check if everything is saved before exiting the program.
	if (!tabsInfo[selectedTab].isSaved && GUI_Return_Code < 1)
	{
		goto MainLoop;
	}

	Serial::Close();
	GUI::Destroy();
}
