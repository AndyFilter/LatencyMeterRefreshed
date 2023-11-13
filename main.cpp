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
#include "Audio/Sound_Helper.h"
#include "External/ImGui/imgui_extensions.h"
#include <algorithm>

// idk why I didn't just expose it in GUI.h...
HWND hwnd;

ImVec2 detectionRectSize{ 0, 200 };

uint64_t lastFrameGui = 0;
uint64_t lastFrame = 0;

// Frametime in microseconds
float averageFrametime = 0;
const unsigned int AVERAGE_FRAME_COUNT = 10000;
uint64_t frames[AVERAGE_FRAME_COUNT]{ 0 };
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

UserData currentUserData{};

UserData backupUserData{};

// --------


// ---- Functionality ----

SerialStatus serialStatus = Status_Idle;

int selectedPort = 0;
int lastSelectedPort = 0;

InputMode selectedMode = InputMode_Normal;
//std::vector<LatencyReading> latencyTests;
//
//LatencyStats latencyStats;
//
//char note[1000];

std::vector<cAudioDeviceInfo> availableAudioDevices;
Waveform_SoundHelper* curAudioDevice;
bool isAudioDevConnected = false;
const UINT AUDIO_SAMPLE_RATE = 10000;//44100U;
const UINT TEST_AUDIO_BUFFER_SIZE = AUDIO_SAMPLE_RATE / 10;
const UINT WARMUP_AUDIO_BUFFER_SIZE = AUDIO_SAMPLE_RATE / 2;
const UINT AUDIO_BUFFER_SIZE = WARMUP_AUDIO_BUFFER_SIZE + TEST_AUDIO_BUFFER_SIZE;
const UINT INTERNAL_TEST_DELAY = 1195387; // in microseconds
uint64_t lastInternalTest = 0;

WinAudio_Player audioPlayer;

std::chrono::steady_clock::time_point moddedMouseTimeClicked;
bool wasModdedMouseTimeUpdated = false;

std::vector<TabInfo> tabsInfo{};
// --------


//bool isSaved = true;
//char savePath[MAX_PATH]{ 0 };

bool isExiting = false;
std::vector<int> unsavedTabs;

static ImGuiTableSortSpecs* sortSpec;

bool isFullscreen = false;
bool fullscreenModeOpenPopup = false;
bool fullscreenModeClosePopup = false;

bool isGameMode = false;
bool isInternalMode = false;
bool isAudioMode = false;
bool wasLMB_Pressed = false;
bool wasMouseClickSent = false;

int selectedTab = 0;

bool wasItemAddedGUI = false;

bool shouldRunConfiguration = false;

float leftTabWidth = 400;

// Forward declaration
void GotSerialChar(char c);
bool SaveAsMeasurements();
bool SavePackAsMeasurements();
void ClearData();
void DiscoverAudioDevices();
void StartInternalTest();
void AddMeasurement(UINT internalTime, UINT externalTime, UINT inputTime);
bool CanDoInternalTest();
bool SetupAudioDevice();
void AttemptConnect();
void AttemptDisconnect();


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
+ Movable black Square for color detection (?) Check for any additional latency this might involve
+ Clear / Reset button
+ Game integration. Please don't get me banned again (needs testing) Update: (testing done)
+ Fix overwriting saves
+ Unsaved work notification
+ Unsaved tabs alert
- Custom Fonts (?)
+ Multiple tabs for measurements
+ Move IO to a separate thread (ONLY if it doens't introduce additional latency)
+ Fix measurement index when removing (or don't, I don' know)
+ Reset status on disconnect and clear
+ Auto scroll measurements on new added
+ Changing tab name sets the status to unsaved
+ Fix too many tabs obscuring tabs beneath them
+ Check if ANY tab is unsaved when closing
+ Pack Save (Save all the tabs into a single file) (?)
+ Configuration Screen
+ Pack fonts into a byte array and load them that way.
- Add option to display frametime (?)
- See what can be done about the inefficient Serial Comm code
- Inform user of all the available keyboard shortcuts
- Clean-up the audio code
- Handle unpluging audio devices mid test
- Arduino debug mode (print sensor values etc.)
- Overall better communication with Arduino
- A way to change the Audio Latency "beep" wave function / frequency

*/

bool isSettingOpen = false;

bool AnalyzeData()
{
	UINT iter_offset = 00;
	const UINT BUFFER_MAX_VALUE = (1 << (sizeof(short) * 8)) / 2 - 1;
	const int BUFFER_THRESHOLD = BUFFER_MAX_VALUE / 5;

	//serialStatus = Status_Idle;

	int baseAvg = 0;
	const short AvgCount = 10;
	//const int REMAINDER_COUNTDOWN_VALUE = 10;
	//int isBufferRemainder = 0; // Value at which last measurement ended gets carried to the current buffer, we have to deal with it.

	for (int i = iter_offset; i < TEST_AUDIO_BUFFER_SIZE; i++)
	{
		if (i - iter_offset < AvgCount) {
			baseAvg += curAudioDevice->audioBuffer[i];
			continue;
		}
		else if (i - iter_offset == AvgCount) {
			baseAvg /= AvgCount;
			baseAvg = abs(baseAvg);
			//if (baseAvg > BUFFER_THRESHOLD)
			//	isBufferRemainder = REMAINDER_COUNTDOWN_VALUE;
		}

		//if (isBufferRemainder && abs(curAudioDevice->audioBuffer[i]) < BUFFER_THRESHOLD) {
		//	isBufferRemainder--;

		//	if (!isBufferRemainder) {
		//		iter_offset = i - REMAINDER_COUNTDOWN_VALUE;
		//		baseAvg = 0;
		//	}
		//	continue;
		//}

		short sample = abs(curAudioDevice->audioBuffer[i] - baseAvg);

		if (sample >= BUFFER_THRESHOLD / (isAudioMode ? 4 : 1))
		{
			float millisElapsed = (1000000 * (i - iter_offset)) / AUDIO_SAMPLE_RATE;
#ifdef _DEBUG
			std::cout << "latency: " << millisElapsed << ", base val: " << baseAvg << std::endl;
#endif // _DEBUG
			if (baseAvg > BUFFER_MAX_VALUE * 0.9f || millisElapsed <= 1)
				return false;
			AddMeasurement(millisElapsed, 0, 0);
			return true;
		}
	}
}

// Callback function called by waveInOpen
//HWAVEIN, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR
void CALLBACK waveInCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case WIM_DATA:
	{
		////if(serialStatus != Status_Idle)
		////	curAudioDevice->SwapToNextBuffer();
		//serialStatus = Status_Idle;
		////((Waveform_SoundHelper*)dwInstance)->GetPlayTime();
		////short sample = curAudioDevice->GetPlayTime().u.sample;
		////detection_Color = ImColor(0.f, 0.f, 0.f, 1.f);
		////printf("new data\n");
		//AnalyzeData();

		
		static bool swapped_buffers = false;
		static bool reverted_buffers = false;
		if (serialStatus != Status_Idle && !swapped_buffers) {

			swapped_buffers = true;
			reverted_buffers = false;
			curAudioDevice->SwapToNextBuffer();
			if (isAudioMode)
				audioPlayer.Play();
			serialStatus = Status_WaitingForResult;
			break;
		}
		swapped_buffers = false;
		if (!reverted_buffers) {
			reverted_buffers = true;
			AnalyzeData();
			serialStatus = Status_Idle;
			curAudioDevice->SwapToNextBuffer();
		}
		//printf("new data\n");
	}
	break;
	default:
		break;
	}
}


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

// Does the calcualtions and copying for you
void SaveCurrentUserConfig()
{
	if (backupUserData.misc.localUserData != currentUserData.misc.localUserData)
		HelperJson::UserConfigLocationChanged(currentUserData.misc.localUserData);

	backupUserData = currentUserData;

	HelperJson::SaveUserData(currentUserData);
	//delete userData;
}

bool LoadCurrentUserConfig()
{
	//UserData userData;
	if (!HelperJson::GetUserData(currentUserData))
		return false;

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

void RecalculateStats(bool recalculate_Average = false, int tabIdx = selectedTab)
{
	size_t size = tabsInfo[tabIdx].latencyData.measurements.size();
	if (size <= 0)
		return;
	if (recalculate_Average)
	{
		tabsInfo[tabIdx].latencyStats.externalLatency.highest = 0;
		tabsInfo[tabIdx].latencyStats.internalLatency.highest = 0;
		tabsInfo[tabIdx].latencyStats.inputLatency.highest = 0;

		unsigned int extSum = 0, intSum = 0, inpSum = 0;

		for (size_t i = 0; i < size; i++)
		{
			auto& test = tabsInfo[tabIdx].latencyData.measurements[i];

			if (i == 0)
			{
				tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
				tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
				tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timePing;
			}

			extSum += test.timeExternal;
			intSum += test.timeInternal;
			inpSum += test.timePing;

			if (test.timeExternal > tabsInfo[tabIdx].latencyStats.externalLatency.highest)
			{
				tabsInfo[tabIdx].latencyStats.externalLatency.highest = test.timeExternal;
			}
			else if (test.timeExternal < tabsInfo[tabIdx].latencyStats.externalLatency.lowest)
			{
				tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
			}

			if (test.timeInternal > tabsInfo[tabIdx].latencyStats.internalLatency.highest)
			{
				tabsInfo[tabIdx].latencyStats.internalLatency.highest = test.timeInternal;
			}
			else if (test.timeInternal < tabsInfo[tabIdx].latencyStats.internalLatency.lowest)
			{
				tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
			}

			if (test.timePing > tabsInfo[tabIdx].latencyStats.inputLatency.highest)
			{
				tabsInfo[tabIdx].latencyStats.inputLatency.highest = test.timePing;
			}
			else if (test.timePing < tabsInfo[tabIdx].latencyStats.inputLatency.lowest)
			{
				tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timePing;
			}
		}

		tabsInfo[tabIdx].latencyStats.externalLatency.average = static_cast<float>(extSum) / size;
		tabsInfo[tabIdx].latencyStats.internalLatency.average = static_cast<float>(intSum) / size;
		tabsInfo[tabIdx].latencyStats.inputLatency.average = static_cast<float>(inpSum) / size;
	}
	else
	{
		for (size_t i = 0; i < size; i++)
		{
			auto& test = tabsInfo[tabIdx].latencyData.measurements[i];

			if (i == 0)
			{
				tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
				tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
				tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timePing;
			}

			if (test.timeExternal > tabsInfo[tabIdx].latencyStats.externalLatency.highest)
			{
				tabsInfo[tabIdx].latencyStats.externalLatency.highest = test.timeExternal;
			}
			else if (test.timeExternal < tabsInfo[tabIdx].latencyStats.externalLatency.lowest)
			{
				tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
			}

			if (test.timeInternal > tabsInfo[tabIdx].latencyStats.internalLatency.highest)
			{
				tabsInfo[tabIdx].latencyStats.internalLatency.highest = test.timeInternal;
			}
			else if (test.timeInternal < tabsInfo[tabIdx].latencyStats.internalLatency.lowest)
			{
				tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
			}

			if (test.timePing > tabsInfo[tabIdx].latencyStats.inputLatency.highest)
			{
				tabsInfo[tabIdx].latencyStats.inputLatency.highest = test.timePing;
			}
			else if (test.timePing < tabsInfo[tabIdx].latencyStats.inputLatency.lowest)
			{
				tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timePing;
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

			if (i > index)
				test.index -= 1;
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
	//if (tabsInfo[selectedTab].latencyData.measurements.size() < 1)
	//	return false;

	if (std::strlen(tabsInfo[selectedTab].savePath) == 0)
		return SaveAsMeasurements();

	HelperJson::SaveLatencyTests(tabsInfo[selectedTab], tabsInfo[selectedTab].savePath);
	tabsInfo[selectedTab].isSaved = true;

	return true;
}

bool SaveAsMeasurements()
{
	//if (tabsInfo[selectedTab].latencyData.measurements.size() < 1)
	//	return false;

	bool wasFullscreen = isFullscreen;
	if (isFullscreen)
	{
		GUI::g_pSwapChain->SetFullscreenState(false, nullptr);
		isFullscreen = false;
		fullscreenModeOpenPopup = false;
		fullscreenModeClosePopup = false;
	}

	char filename[MAX_PATH]{ "name" };
	memcpy(filename, tabsInfo[selectedTab].name, TAB_NAME_MAX_SIZE);
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
		strncpy_s(tabsInfo[selectedTab].savePath, filename, MAX_PATH);

		SaveMeasurements();
	}
	//else
	//	tabsInfo[selectedTab].isSaved = false;

	if (wasFullscreen)
	{
		GUI::g_pSwapChain->SetFullscreenState(true, nullptr);
		isFullscreen = true;
	}

	return tabsInfo[selectedTab].isSaved;
}

bool SavePackMeasurements()
{
	char* packSavePath = nullptr;
	for (auto& tab : tabsInfo) {
		if (packSavePath && strcmp(packSavePath, tab.savePathPack) != 0)
		{
			packSavePath = nullptr;
			break;
		}
		if(strlen(tab.savePathPack) != 0)
			packSavePath = tab.savePathPack;
	}

	if (packSavePath == nullptr)
		return SavePackAsMeasurements();

	HelperJson::SaveLatencyTestsPack(tabsInfo, packSavePath);
	for(auto& tab : tabsInfo)
		tab.isSaved = true;

	return true;
}

bool SavePackAsMeasurements()
{
	bool wasFullscreen = isFullscreen;
	if (isFullscreen)
	{
		GUI::g_pSwapChain->SetFullscreenState(false, nullptr);
		isFullscreen = false;
		fullscreenModeOpenPopup = false;
		fullscreenModeClosePopup = false;
	}

	char filename[MAX_PATH]{ "PackName" };
	const char szExt[] = "Json\0*.json\0\0";

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle = "Save Pack As";

	if (GetSaveFileName(&ofn))
	{
		for (auto& tab : tabsInfo) {
			strncpy_s(tab.savePathPack, filename, MAX_PATH);
		}

		SavePackMeasurements();
	}
	//else
	//	tabsInfo[selectedTab].isSaved = false;

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

	char filename[MAX_PATH*10];
	const char szExt[] = "json\0";

	ZeroMemory(filename, sizeof(filename));

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
	ofn.Flags = OFN_DONTADDTORECENT | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

	if (GetOpenFileName(&ofn))
	{
		//ClearData();
		size_t added_tabs = 0;
		bool isMultiSelect = !filename[ofn.nFileOffset-1];
		if (isMultiSelect) {
			std::string baseDir = filename;
			char* pFileName = filename;
			pFileName += baseDir.length() + 1;
			while (*pFileName) {
				size_t new_tabs = HelperJson::GetLatencyTests(tabsInfo, (baseDir + "\\" + pFileName).c_str());
				for (size_t i = tabsInfo.size() - new_tabs; i < tabsInfo.size(); i++)
				{
					if (*tabsInfo[i].name == 0)
					{
						std::string pureFileName = pFileName;
						pureFileName = pureFileName.substr(0, pureFileName.find_last_of(".json") - 4);
						strcpy_s<TAB_NAME_MAX_SIZE>(tabsInfo[i].name, pureFileName.c_str());
					}
				}
				added_tabs += new_tabs;
				pFileName += strlen(pFileName) + 1;
			}
		}
		else {
			added_tabs = HelperJson::GetLatencyTests(tabsInfo, filename);
			strcpy_s(tabsInfo[selectedTab].savePath, filename);
		}
		selectedTab += added_tabs;

		for (size_t i = tabsInfo.size() - added_tabs; i < tabsInfo.size(); i++)
		{
			if (!tabsInfo[i].latencyData.measurements.empty())
				RecalculateStats(true, i);

			if (*tabsInfo[i].name == 0 && !isMultiSelect)
			{
				std::string pureFileName = filename;
				size_t lastSlash = pureFileName.find_last_of("\\");
				pureFileName = pureFileName.substr(lastSlash + 1, pureFileName.find_last_of(".json") - lastSlash - 5);
				strcpy_s<TAB_NAME_MAX_SIZE>(tabsInfo[i].name, pureFileName.c_str());
			}
		}

		//if (!tabsInfo[selectedTab].latencyData.measurements.empty())
		//	RecalculateStats(true);
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
	serialStatus = Status_Idle;
	tabsInfo[selectedTab].latencyData.measurements.clear();
	tabsInfo[selectedTab].latencyStats = LatencyStats();
	tabsInfo[selectedTab].isSaved = true;
	ZeroMemory(tabsInfo[selectedTab].latencyData.note, 1000);
}

// Not the best GUI solution, but it's simple, fast and gets the job done.
int OnGui()
{
	ImGuiStyle& style = ImGui::GetStyle();

	if (shouldRunConfiguration)
	{
		ImGui::OpenPopup("Set Up");
		shouldRunConfiguration = false;
	}

	if (ImGui::BeginMenuBar())
	{
		short openFullscreenCommand = 0;
		//static bool saveMeasurementsPopup = false;
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open", "Ctrl+O")) { OpenMeasurements(); }
			if (ImGui::MenuItem("Save", "Ctrl+S")) { SaveMeasurements(); }
			if (ImGui::MenuItem("Save as", "Ctrl+Shift+S")) { SaveAsMeasurements(); }
			if (ImGui::MenuItem("Save Pack", "Alt+S")) { SavePackMeasurements(); }
			if (ImGui::MenuItem("Save Pack As", "Alt+Shift+S")) { SavePackAsMeasurements(); }
			if (ImGui::MenuItem("Fullscreen", "Alt+Enter")) { 
				GUI::g_pSwapChain->GetFullscreenState((BOOL*)&isFullscreen, nullptr);

				if (!isFullscreen && !fullscreenModeOpenPopup)
					openFullscreenCommand = 1;
				else if (!fullscreenModeClosePopup && !fullscreenModeOpenPopup)
					openFullscreenCommand = -1;
			}
			if (ImGui::MenuItem("Settings", "")) { isSettingOpen = !isSettingOpen; }
			if (ImGui::MenuItem("Close", "")) { return 1; }
			ImGui::EndMenu();
		}

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
			ImGui::Text(" %.1f CPU", 1000000.f / averageFrametime);
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

		if (openFullscreenCommand == 1) {
			ImGui::OpenPopup("Enter Fullscreen mode?");
			fullscreenModeOpenPopup = true;
		}
		else if (openFullscreenCommand == -1) {
			ImGui::OpenPopup("Exit Fullscreen mode?");
			fullscreenModeClosePopup = true;
		}
	}


	const auto windowAvail = ImGui::GetContentRegionAvail();
	ImGuiIO& io = ImGui::GetIO();

	if (serialStatus != Status_Idle && micros() > (lastInternalTest + SERIAL_NO_RESPONSE_DELAY))
	{
		serialStatus = Status_Idle;
	}


	// Handle Shortcuts
	ImGuiKey pressedKeys[ImGuiKey_COUNT]{ 0 };
	size_t addedKeys = 0;

	//ZeroMemory(pressedKeys, ImGuiKey_COUNT);

	for (ImGuiKey key = 0; key < ImGuiKey_COUNT; key++)
	{
		bool isPressed = ImGui::IsKeyPressed(key, false);

		// Shift, ctrl and alt
		if ((key >= 641 && key <= 643) || (key >= 527 && key <= 533))
			continue;

		if (ImGui::IsLegacyKey(key))
			continue;
		
		if (isPressed) {
			auto name = ImGui::GetKeyName(key);
			pressedKeys[addedKeys++] = key;
		}
	}

	static bool wasEscapeUp{ true };

	if (addedKeys == 1)
	{
		//const char* name = ImGui::GetKeyName(pressedKeys[0]);
		if (io.KeyCtrl)
		{
			// Moved to the Main function
			//io.ClearInputKeys();
			//if (pressedKeys[0] == ImGuiKey_S)
			//{
			//	printf("Save intent\n");
			//	SaveMeasurements();
			//}
			//if (pressedKeys[0] == ImGuiKey_O)
			//{
			//	OpenMeasurements();
			//}
			//else if (pressedKeys[0] == ImGuiKey_N)
			//{
			//	auto newTab = TabInfo();
			//	strcat_s<TAB_NAME_MAX_SIZE>(newTab.name, std::to_string(tabsInfo.size()).c_str());
			//	tabsInfo.push_back(newTab);
			//}
			// Doesn't work yet!
			if (pressedKeys[0] == ImGuiKey_W)
			{
				char tabClosePopupName[48];
				snprintf(tabClosePopupName, 48, "Save before Closing?###TabExit%i", selectedTab);

				ImGui::OpenPopup(tabClosePopupName);
			}
		}
		else if (io.KeyAlt)
		{
			//io.ClearInputKeys();

			// Going fullscreen popup currently with a small issue

			// Enter
			if (pressedKeys[0] == ImGuiKey_Enter || pressedKeys[0] == ImGuiKey_KeypadEnter)
			{
				GUI::g_pSwapChain->GetFullscreenState((BOOL*)&isFullscreen, nullptr);
				if (!isFullscreen && !fullscreenModeOpenPopup)
				{
					ImGui::OpenPopup("Enter Fullscreen mode?");
					fullscreenModeOpenPopup = true;
				}
				else if (!fullscreenModeClosePopup && !fullscreenModeOpenPopup)
				{
					ImGui::OpenPopup("Exit Fullscreen mode?");
					fullscreenModeClosePopup = true;
				}
			}
		}
		else if (!io.KeyShift && pressedKeys[0] == ImGuiKey_Escape)
		{
			BOOL isFS;
			GUI::g_pSwapChain->GetFullscreenState(&isFS, nullptr);
			isFullscreen = isFS;
			if (isFullscreen && !fullscreenModeClosePopup && wasEscapeUp)
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
		if (ImGui::Button("Yes") || (IS_ONLY_ENTER_PRESSED && !io.KeyAlt))
		{
			// I'm pretty sure I should also resize the swapchain buffer to use the new resolution, but maybe later. It looks kind of goofy on displays with odd resolution :shrug:
			//GUI::g_pSwapChain->ResizeBuffers(0, 1080, 1920, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
			//UINT modesNum = 256;
			//DXGI_MODE_DESC monitorModes[256];
			//GUI::vOutputs[1]->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &modesNum, monitorModes);
			//DXGI_MODE_DESC mode = monitorModes[modesNum - 1];
			//GUI::g_pSwapChain->ResizeTarget(&mode);
			//GUI::g_pSwapChain->ResizeBuffers(0, 1080, 1920, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
			if (GUI::g_pSwapChain->SetFullscreenState(true, NULL) == S_OK)
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

			const char* items[3]{ "Style", "Performance", "Misc" };

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
				TOOLTIP("Internal plot color");
				ImGui::ColorEdit4("Ext. Plot", currentUserData.style.externalPlotColor);
				TOOLTIP("External plot color");
				ImGui::ColorEdit4("Input Plot", currentUserData.style.inputPlotColor);
				TOOLTIP("Input plot color");

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

					ImGui::Spacing();

					//ImGui::Checkbox("VSync", &currentUserData.performance.VSync);
					ImGui::SliderInt("VSync", &(int&)currentUserData.performance.VSync, 0, 4, "%d", ImGuiSliderFlags_AlwaysClamp);
					TOOLTIP("This setting synchronizes your monitor with the program to avoid screen tearing.\n(Adds a significant amount of latency)");

					ImGui::EndChild();
				}

				break;

				// Misc
			case 2:
				if ((ImGui::BeginChild("Misc", { ((1 - listBoxSize) * avail.x) - style.FramePadding.x * 2, avail.y - ImGui::GetFrameHeightWithSpacing() }, true)))
				{
					ImGui::Checkbox("Save config locally", &currentUserData.misc.localUserData);
					TOOLTIP("Chose whether you want to save config file in the same directory as the program, or in the AppData folder");

					ImGui::EndChild();
				}

				break;
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


	// TABS

	int deletedTabIndex = -1;

	if (ImGui::BeginTabBar("TestsTab", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll))
	{

		for (size_t tabN = 0; tabN < tabsInfo.size(); tabN++)
		{
			//char idText[24];
			//snprintf(idText, 24, "Tab Item %i", tabN);
			//ImGui::PushID(idText);

			char tabNameLabel[48];
			snprintf(tabNameLabel, 48, "%s###Tab%i", tabsInfo[tabN].name, tabN);

			//ImGui::PushItemWidth(ImGui::CalcTextSize(tabNameLabel).x + 100);
			//if(!tabsInfo[tabN].isSaved)
			//	ImGui::SetNextItemWidth(ImGui::CalcTextSize(tabsInfo[tabN].name, NULL).x + (style.FramePadding.x * 2) + 15);
			//else
			ImGui::SetNextItemWidth(ImGui::CalcTextSize(tabsInfo[tabN].name, NULL).x + (style.FramePadding.x * 2) + (tabsInfo[tabN].isSaved ? 0 : 15));
			//selectedTab = tabsInfo.size() - 1;
			auto flags = selectedTab == tabN ? ImGuiTabItemFlags_SetSelected : 0;
			flags |= tabsInfo[tabN].isSaved ? 0 : ImGuiTabItemFlags_UnsavedDocument;
			if (ImGui::BeginTabItem(tabNameLabel, NULL, flags))
			{
				ImGui::EndTabItem();
			}
			//ImGui::PopItemWidth();
			//ImGui::PopID();

			if (ImGui::IsItemHovered())
			{
				if (ImGui::IsMouseDown(0))
				{
					selectedTab = tabN;
				}

				auto mouseDelta = io.MouseWheel;
				selectedTab += mouseDelta;
				selectedTab = std::clamp(selectedTab, 0, (int)tabsInfo.size() - 1);
			}

			char popupName[32]{ 0 };
			snprintf(popupName, 32, "Change tab %i name", tabN);

			char popupTextEdit[32]{ 0 };
			snprintf(popupTextEdit, 32, "Ed Txt %i", tabN);

			char tabClosePopupName[48];
			snprintf(tabClosePopupName, 48, "Save before Closing?###TabExit%i", tabN);

			static bool tabExitOpen = true;
			bool openTabClose = false;

			if (ImGui::IsItemFocused())
			{
				if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
				{
#ifdef _DEBUG
					printf("deleting tab: %i\n", tabN);
#endif
					if (tabsInfo.size() > 1)
						if (!tabsInfo[tabN].isSaved)
						{
							ImGui::OpenPopup(tabClosePopupName);
							tabExitOpen = true;
						}
						else
						{
							deletedTabIndex = tabN;
							if (tabN == tabsInfo.size() - 1)
							{
								ImGuiContext& g = *GImGui;
								ImGui::SetFocusID(g.CurrentTabBar->Tabs[tabN-1].ID, ImGui::GetCurrentWindow());
							}
							// Unfocus so the next items won't get deleted (This can also be achieved by checking if DEL was not down)
							//ImGui::SetWindowFocus(nullptr);
						}
				}

				else if (ImGui::IsKeyDown(ImGuiKey_F2))
				{
					ImGui::OpenPopup(popupTextEdit);
				}

				if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
					selectedTab--;

				if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
					selectedTab++;

				if (io.KeyCtrl)
				{
					if (pressedKeys[0] == ImGuiKey_W)
						if (tabsInfo.size() > 1)
							if (!tabsInfo[tabN].isSaved)
							{
								ImGui::OpenPopup(tabClosePopupName);
								tabExitOpen = true;
							}
							else
							{
								deletedTabIndex = tabN;
								if (tabN == tabsInfo.size() - 1)
								{
									ImGuiContext& g = *GImGui;
									ImGui::SetFocusID(g.CurrentTabBar->Tabs[tabN - 1].ID, ImGui::GetCurrentWindow());
								}
							}
				}

				selectedTab = std::clamp(selectedTab, 0, (int)tabsInfo.size() - 1);
			}

			if (ImGui::BeginPopup(popupTextEdit, ImGuiWindowFlags_NoMove))
			{
				ImGui::SetKeyboardFocusHere(0);
				if (ImGui::InputText("Tab name", tabsInfo[tabN].name, TAB_NAME_MAX_SIZE, ImGuiInputTextFlags_AutoSelectAll))
				{
					char defaultTabName[8]{ 0 };
					snprintf(defaultTabName, 8, "Tab %i", tabN);
					tabsInfo[tabN].isSaved = !strcmp(defaultTabName, tabsInfo[tabN].name);
				}
				ImGui::EndPopup();
			}

			if (ImGui::IsItemClicked(1) || (ImGui::IsItemHovered() && (ImGui::IsMouseDoubleClicked(0))))
			{
#ifdef _DEBUG
				printf("Open tab %i tooltip\n", tabN);
#endif
				ImGui::OpenPopup(popupName);
			}

			if (ImGui::BeginPopup(popupName, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
			{
				// Check if this name already exists
				//ImGui::SetKeyboardFocusHere(0);
				if (ImGui::InputText("Tab name", tabsInfo[tabN].name, TAB_NAME_MAX_SIZE, ImGuiInputTextFlags_AutoSelectAll))
				{
					char defaultTabName[8]{ 0 };
					snprintf(defaultTabName, 8, "Tab %i", tabN);
					tabsInfo[tabN].isSaved = !strcmp(defaultTabName, tabsInfo[tabN].name);
				}

				if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape))
					ImGui::CloseCurrentPopup();

				//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 4);
				if (tabsInfo.size() > 1) {
					if (ImGui::Button("Close Tab", { -1, 0 }))
					{
						if (!tabsInfo[tabN].isSaved)
						{
							ImGui::CloseCurrentPopup();
							//ImGui::OpenPopup(tabClosePopupName);
							openTabClose = true;
						}
						else
						{
							deletedTabIndex = tabN;
							ImGui::CloseCurrentPopup();
						}
					}
				}

				ImGui::EndPopup();
			}

			if (openTabClose)
			{
				tabExitOpen = true;
				ImGui::OpenPopup(tabClosePopupName);
			}

			if (ImGui::BeginPopupModal(tabClosePopupName, &tabExitOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				ImGui::Text("Are you sure you want to close this tab?\nAll measurements will be lost if you don't save");

				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 10 });

				auto popupAvail = ImGui::GetContentRegionAvail();

				popupAvail.x -= style.ItemSpacing.x * 2;

				if (ImGui::Button("Save", { popupAvail.x / 3, 0 }))
				{
					if (SaveMeasurements())
						deletedTabIndex = tabN;
					ImGui::CloseCurrentPopup();
					tabExitOpen = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Discard", { popupAvail.x / 3, 0 }))
				{
					deletedTabIndex = tabN;
					ImGui::CloseCurrentPopup();
					tabExitOpen = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", { popupAvail.x / 3, 0 }))
				{
					ImGui::CloseCurrentPopup();
					tabExitOpen = false;
				}

				ImGui::EndPopup();
			}
		}


		if (ImGui::TabItemButton(" + ", ImGuiTabItemFlags_Trailing))
		{
			auto newTab = TabInfo();
			strcat_s<TAB_NAME_MAX_SIZE>(newTab.name, std::to_string(tabsInfo.size()).c_str());
			tabsInfo.push_back(newTab);
			selectedTab = tabsInfo.size() - 1;

			//ImGui::EndTabItem(); 
		}

		//ImGui::Text("Selected Item %i", selectedTab);

		ImGui::EndTabBar();
	}

	if (ImGui::IsItemHovered())
	{
		auto mouseDelta = io.MouseWheel;
		selectedTab += mouseDelta;
		selectedTab = std::clamp(selectedTab, 0, (int)tabsInfo.size() - 1);
	}


	if (deletedTabIndex >= 0)
	{
		tabsInfo.erase(tabsInfo.begin() + deletedTabIndex);
		if(selectedTab >= deletedTabIndex)
			selectedTab = max(selectedTab - 1, 0);
		//char tabNameLabel[48];
		//snprintf(tabNameLabel, 48, "%s###Tab%i", tabsInfo[selectedTab].name, selectedTab);

		//ImGui::SetFocusID(ImGui::GetID(tabNameLabel), ImGui::GetCurrentWindow());
	}

	const auto avail = ImGui::GetContentRegionAvail();

	if (ImGui::BeginChild("LeftChild", { leftTabWidth, avail.y - detectionRectSize.y - 5 }, true))
	{

#ifdef _DEBUG 

		//ImGui::Text("Time is: %ims", clock());
		ImGui::Text("Time is: %lu\xC2\xB5s", micros());

		ImGui::SameLine();

		ImGui::Text("Serial Staus: %i", serialStatus);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (GUI ONLY)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (CPU ONLY)", (micros() - lastFrame) / 1000.f, 1000000.0f / (micros() - lastFrame));

		ImGui::Text("Application average %.3f \xC2\xB5s/frame (%.1f FPS) (CPU ONLY)", averageFrametime, 1000000.f / averageFrametime);

		// The buffer contains just too many frames to display them on the screen (and displaying every 10th or so frame makes no sense). Also smoothing out the data set would hurt the performance too much
		//ImGui::PlotLines("FPS Chart", [](void* data, int idx) { return sinf(float(frames[idx%10]) / 1000); }, frames, AVERAGE_FRAME_COUNT);

		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { avail.x, ImGui::GetFrameHeight() });

#endif // DEBUG

		//ImGui::Combo("Select Port", &selectedPort, Serial::availablePorts.data());

		static ImVec2 restItemsSize;

		ImGui::BeginGroup();

		bool isSelectedConnected = isInternalMode ? isAudioDevConnected : Serial::isConnected;

		ImGui::PushItemWidth(80);
		ImGui::BeginDisabled(isSelectedConnected);
		if (isInternalMode)
		{
			if (ImGui::BeginCombo("Device", availableAudioDevices.size() > 0 ? availableAudioDevices[selectedPort].friendlyName : "0 Found"))
			{
				for (size_t i = 0; i < availableAudioDevices.size(); i++)
				{
					bool isSelected = (selectedPort == i);
					if (ImGui::Selectable(availableAudioDevices[i].friendlyName, isSelected, 0, { 0,0 }, style.FrameRounding))
					{
						if (selectedPort != i)
							Serial::Close();
						selectedPort = i;
					}
				}
				ImGui::EndCombo();
			}
		}
		else
		{
			if (ImGui::BeginCombo("Port", Serial::availablePorts.size() > 0 ? Serial::availablePorts[selectedPort].c_str() : "0 Found"))
			{
				for (size_t i = 0; i < Serial::availablePorts.size(); i++)
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
		}
		ImGui::PopItemWidth();
		ImGui::EndDisabled();

		ImGui::SameLine();

		if (ImGui::Button("Refresh") && serialStatus == Status_Idle)
		{
			if (isInternalMode)
				DiscoverAudioDevices();
			else
				Serial::FindAvailableSerialPorts();
		}

		ImGui::SameLine();
		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });

		if (isSelectedConnected)
		{
			if (ImGui::Button("Disconnect"))
			{
				AttemptDisconnect();
			}
		}
		else
		{
			if (ImGui::Button("Connect") && ((!Serial::availablePorts.empty() && !isInternalMode) || (!availableAudioDevices.empty() && isInternalMode)))
			{
				AttemptConnect();
			}
		}

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

		//ImGui::SameLine();

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
		TOOLTIP("This mode instead of lighting up the rectangle at the bottom will simulate pressing the left mouse button (fire in game).\nTo register the delay between input and shot in game.");

		ImGui::SameLine();

		ImGui::BeginDisabled(isSelectedConnected);
		//ImGui::SetNextItemWidth(ImGui::CalcTextSize(GetInputModeName(InputMode_ModdedMouse)).x + style.ItemSpacing.x*2);
		//if (ImGui::BeginCombo("Mode", GetInputModeName(selectedMode)))
		//{
		//	InputMode oldSelectedMode = selectedMode;
		//	if (ImGui::Selectable(GetInputModeName(InputMode_Normal), selectedMode == InputMode_Normal, 0, { 0,0 }, style.FrameRounding))
		//	{
		//		selectedMode = InputMode_Normal;
		//	}
		//	TOOLTIP("Use Arduino with a button");
		//	if (ImGui::Selectable(GetInputModeName(InputMode_ModdedMouse), selectedMode == InputMode_ModdedMouse, 0, { 0,0 }, style.FrameRounding))
		//	{
		//		selectedMode = InputMode_ModdedMouse;
		//
		//		RAWINPUTDEVICE rawDev;
		//		rawDev.usUsagePage = 1;
		//		rawDev.usUsage = 2;
		//		rawDev.dwFlags = RIDEV_INPUTSINK;
		//		rawDev.hwndTarget = hwnd;
		//		if (!RegisterRawInputDevices(&rawDev, 1, sizeof(rawDev)))
		//			printf("Failed to register raw input device\n");
		//	}
		//	TOOLTIP("Use Arduino with a \"hacked\" mouse connected to it");
		//	if (ImGui::Selectable(GetInputModeName(InputMode_Internal), selectedMode == InputMode_Internal, 0, { 0,0 }, style.FrameRounding))
		//	{
		//		selectedMode = InputMode_Internal;
		//	}
		//	TOOLTIP("Use a light sensor connected to a 3.5mm audio port (Use the \"Run Test\" button)");
		//	ImGui::EndCombo();
		//
		//	if (oldSelectedMode != selectedMode)
		//	{
		//		if (oldSelectedMode == InputMode_ModdedMouse)
		//		{
		//			RAWINPUTDEVICE rawDev;
		//			rawDev.usUsagePage = 1;
		//			rawDev.usUsage = 2;
		//			rawDev.dwFlags = RIDEV_REMOVE;
		//			rawDev.hwndTarget = 0;
		//			if (!RegisterRawInputDevices(&rawDev, 1, sizeof(rawDev)))
		//				printf("Failed to unregister raw input device\n");
		//		}
		//	}
		//}
		if (ImGui::Checkbox("Internal Mode", &isInternalMode))
		{
			int bkpSelected = lastSelectedPort;
			lastSelectedPort = selectedPort;
			selectedPort = bkpSelected;
		}
		ImGui::EndDisabled();
		TOOLTIP("Uses a sensor connected to 3.5mm jack. (More info on Github)")

		ImGui::SameLine();

		if (ImGui::Checkbox("Audio Mode", &isAudioMode))
		{

		}
		TOOLTIP("Measures Audio Latency (Uses a microphone instead of photoresistor)");

		if (isInternalMode) {
			ImGui::SameLine();
			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });
			ImGui::SameLine();

			ImGui::BeginDisabled(!CanDoInternalTest());
			auto cursorPos = ImGui::GetCurrentWindow()->DC.CursorPos;
			if (ImGui::Button("Run Test"))
			{
				StartInternalTest();
			}
			ImGui::EndDisabled();
			auto itemSize = ImGui::GetItemRectSize();
			float frac = ImClamp<float>(((micros() - lastInternalTest) / (float)(1000000 * (uint64_t)WARMUP_AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE)), 0, 1);
			//ImGui::ProgressBar(frac);
			cursorPos.y += ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset + style.FramePadding.y * 2 + ImGui::GetTextLineHeight() + 2;
			cursorPos.x += 5;
			itemSize.x -= 10;
			itemSize.x *= frac;
			//itemSize.x = itemSize.x < 0 ? 0 : itemSize.x;
			ImGui::GetWindowDrawList()->AddLine(cursorPos, { cursorPos.x + itemSize.x, cursorPos.y }, ImGui::GetColorU32(ImGuiCol_SeparatorActive), 2);
			TOOLTIP("It's advised to use Shift+R instead.");
		}
		ImGui::EndGroup();

		restItemsSize = ImGui::GetItemRectSize();

		ImGui::Spacing();

		ImGui::PushFont(boldFont);

		if (ImGui::BeginChild("MeasurementStats", { 0, ImGui::GetTextLineHeightWithSpacing() * 4 + style.WindowPadding.y * 2 - style.ItemSpacing.y + style.FramePadding.y * 2 }, true))
		{
#pragma region OldGraphs
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
#pragma endregion
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

		auto& measurements = tabsInfo[selectedTab].latencyData.measurements;

		// Separate Plots
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.internalPlotColor);
		ImGui::PlotLines("Internal Latency", [](void* data, int idx) { return  tabsInfo[selectedTab].latencyData.measurements[idx].timeInternal / 1000.f; }, measurements.data(), measurements.size(), 0, NULL, FLT_MAX, FLT_MAX, { 0,plotHeight });
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.externalPlotColor);
		ImGui::PlotLines("External Latency", [](void* data, int idx) { return (float)tabsInfo[selectedTab].latencyData.measurements[idx].timeExternal; }, measurements.data(), measurements.size(), 0, NULL, FLT_MAX, FLT_MAX, { 0,plotHeight });
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.inputPlotColor);
		ImGui::PlotLines("Input Latency", [](void* data, int idx) { return  tabsInfo[selectedTab].latencyData.measurements[idx].timePing / 1000.f; }, measurements.data(), measurements.size(), 0, NULL, FLT_MAX, FLT_MAX, { 0,plotHeight });
		ImGui::PopStyleColor(6);

		// Combined Plots
		auto startCursorPos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(startCursorPos);
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.internalPlotColor);
		ImGui::PlotLines("Combined Plots", [](void* data, int idx) { return  tabsInfo[selectedTab].latencyData.measurements[idx].timeInternal / 1000.f; },
			measurements.data(), measurements.size(), 0, NULL,
			tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000, tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000 + 1, { 0,plotHeight });

		ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });
		ImGui::PushStyleColor(ImGuiCol_Border, { 0,0,0,0 });
		ImGui::SetCursorPos(startCursorPos);
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.externalPlotColor);
		ImGui::PlotLines("###ExternalPlot", [](void* data, int idx) { return (float)tabsInfo[selectedTab].latencyData.measurements[idx].timeExternal; },
			measurements.data(), measurements.size(), 0, NULL,
			tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000, tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000 + 1, { 0,plotHeight });

		ImGui::SetCursorPos(startCursorPos);
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.inputPlotColor);
		ImGui::PlotLines("###InputPlot", [](void* data, int idx) { return tabsInfo[selectedTab].latencyData.measurements[idx].timePing / 1000.f; },
			measurements.data(), measurements.size(), 0, NULL,
			tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000, tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000 + 1, { 0,plotHeight });
		ImGui::PopStyleColor(8);
	}

	ImGui::EndChild();

	ImGui::SameLine();
	
	static float leftTabStart = 0;
	ImGui::ResizeSeparator(leftTabWidth, leftTabStart, ImGuiSeparatorFlags_Vertical, {300, 300}, { 6, avail.y - detectionRectSize.y - 5 });

	ImGui::SameLine();

	ImGui::BeginGroup();

	auto tableAvail = ImGui::GetContentRegionAvail();

	if (ImGui::BeginTable("measurementsTable", 5, ImGuiTableFlags_Reorderable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, { tableAvail.x, 200 }))
	{
		ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
		ImGui::TableSetupColumn("Internal Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 1);
		ImGui::TableSetupColumn("External Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 2);
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
		clipper.Begin(_latencyTestsCopy.size(), ImGui::GetTextLineHeightWithSpacing());
		while (clipper.Step())
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				auto& reading = _latencyTestsCopy[i];

				ImGui::PushID(i + 100);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.index);
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.timeInternal / 1000);
				// \xC2\xB5 is the microseconds character (looks like u (not you, just "u"))
				TOOLTIP("%i\xC2\xB5s", reading.timeInternal);
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.timeExternal);
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


		if (wasItemAddedGUI)
			ImGui::SetScrollHereY();


		ImGui::EndTable();
	}

	//ImGui::SameLine();

	// Notes area
	ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 10 });
	ImGui::Text("Measurement notes");
	auto notesAvail = ImGui::GetContentRegionAvail();
	ImGui::InputTextMultiline("Measurements Notes", tabsInfo[selectedTab].latencyData.note, 1000, { tableAvail.x, min(notesAvail.y - detectionRectSize.y - 5, 600) }, ImGuiInputTextFlags_NoHorizontalScroll);
	//ImGui::InputTextMultiline("Measurements Notes", tabsInfo[selectedTab].latencyData.note, 1000, { tableAvail.x, min(tableAvail.y - 200 - detectionRectSize.y - style.WindowPadding.y - style.ItemSpacing.y * 5, 600) - ImGui::GetItemRectSize().y - 8});

	ImGui::EndGroup();

	static float detectionRectStart = 0;
	ImGui::ResizeSeparator(detectionRectSize.y, detectionRectStart, ImGuiSeparatorFlags_Horizontal, {100, 400}, { avail.x, 6 });

	// Color change detection rectangle.
	ImVec2 rectSize{ 0, detectionRectSize.y };
	rectSize.x = detectionRectSize.x == 0 ? windowAvail.x + style.WindowPadding.x + style.FramePadding.x : detectionRectSize.x;
	ImVec2 pos = { 0, windowAvail.y - rectSize.y + style.WindowPadding.y * 2 + style.FramePadding.y * 2 + 10 };
	pos.y += 12;
	ImRect bb{ pos, pos + rectSize };
	ImGui::RenderFrame(
		bb.Min,
		bb.Max,
		// Change the color to white to be detected by the light sensor
		serialStatus == Status_WaitingForResult && !isGameMode && !isAudioMode ? ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1)) : ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 0.f, 0.f, 1.f)),
		false
	);
	//if (ImGui::IsItemFocused())
	//	ImGui::SetFocusID(0, ImGui::GetCurrentWindow());

	if (serialStatus == Status_Idle && currentUserData.performance.showPlots) {
		ImGui::SetCursorPos(pos);
		//rectSize.y -= style.WindowPadding.y/2;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		SetPlotLinesColor({ 1,1,1,0.5 });
		//else
		//	SetPlotLinesColor({ 0,0,0,0.5 });
		if (curAudioDevice)
			ImGui::PlotLines("##audioBufferPlot2", [](void* data, int idx) {return (float)curAudioDevice->audioBuffer[idx]; }, nullptr, TEST_AUDIO_BUFFER_SIZE, 0, 0, (long long)SHRT_MAX / -1, SHRT_MAX / 1, rectSize);
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
	}


	if (ImGui::BeginPopupModal("Set Up", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::Text("Setup configuration");

		ImGui::SeparatorSpace(0, { 0, 5 });

		ImGui::Checkbox("Save config locally", &currentUserData.misc.localUserData);
		TOOLTIP("Chose whether you want to save config file in the same directory as the program, or in the AppData folder");

		ImGui::Spacing();

		if (ImGui::Button("Save", {-1, 0}))
		{
			SaveCurrentUserConfig();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	static bool isExitingWindowOpen = false;

	if (isExiting)
	{


		//for (size_t i = 0; i < tabsInfo.size(); i++)
		//{
		//	if (!tabsInfo[i].isSaved)
		//		unsavedTabs.push_back(i);
		//}

		if (unsavedTabs.size() > 0)
		{
			isExitingWindowOpen = true;
			selectedTab = unsavedTabs[0];
			ImGui::OpenPopup("Exit?");
		}

		if (ImGui::BeginPopupModal("Exit?", &isExitingWindowOpen, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to exit before saving?");
			ImGui::SameLine();
			ImGui::TextUnformatted(tabsInfo[selectedTab].name);

			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 5 });

			if (ImGui::Button("Save and exit"))
			{
				if (SaveMeasurements())
				{
					ImGui::CloseCurrentPopup();
					isExitingWindowOpen = false;
					unsavedTabs.erase(unsavedTabs.begin());
					if (unsavedTabs.size() > 0)
						selectedTab = unsavedTabs[0];
					else
						return 2;
				}
				else
					printf("Could not save the file");
			}
			ImGui::SameLine();
			if (ImGui::Button("Exit"))
			{
				ImGui::CloseCurrentPopup();
				unsavedTabs.erase(unsavedTabs.begin());
				isExitingWindowOpen = false;
				//ImGui::EndPopup();
				if (unsavedTabs.size() > 0)
					selectedTab = unsavedTabs[0];
				else
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
	}

	wasItemAddedGUI = false;
	lastFrameGui = micros();

	return 0;
}

std::chrono::steady_clock::time_point lastTimeGotChar;

void AddMeasurement(UINT internalTime, UINT externalTime, UINT inputTime)
{
	LatencyReading reading{ externalTime, internalTime, inputTime };
	size_t size = tabsInfo[selectedTab].latencyData.measurements.size();

	// Update the stats
	tabsInfo[selectedTab].latencyStats.internalLatency.average = (tabsInfo[selectedTab].latencyStats.internalLatency.average * size) / (size + 1.f) + (internalTime / (size + 1.f));
	tabsInfo[selectedTab].latencyStats.externalLatency.average = (tabsInfo[selectedTab].latencyStats.externalLatency.average * size) / (size + 1.f) + (externalTime / (size + 1.f));
	tabsInfo[selectedTab].latencyStats.inputLatency.average = (tabsInfo[selectedTab].latencyStats.inputLatency.average * size) / (size + 1.f) + (inputTime / (size + 1.f));

	tabsInfo[selectedTab].latencyStats.internalLatency.highest = internalTime > tabsInfo[selectedTab].latencyStats.internalLatency.highest ? internalTime : tabsInfo[selectedTab].latencyStats.internalLatency.highest;
	tabsInfo[selectedTab].latencyStats.externalLatency.highest = externalTime > tabsInfo[selectedTab].latencyStats.externalLatency.highest ? externalTime : tabsInfo[selectedTab].latencyStats.externalLatency.highest;
	tabsInfo[selectedTab].latencyStats.inputLatency.highest = inputTime > tabsInfo[selectedTab].latencyStats.inputLatency.highest ? inputTime : tabsInfo[selectedTab].latencyStats.inputLatency.highest;

	tabsInfo[selectedTab].latencyStats.internalLatency.lowest = internalTime < tabsInfo[selectedTab].latencyStats.internalLatency.lowest ? internalTime : tabsInfo[selectedTab].latencyStats.internalLatency.lowest;
	tabsInfo[selectedTab].latencyStats.externalLatency.lowest = externalTime < tabsInfo[selectedTab].latencyStats.externalLatency.lowest ? externalTime : tabsInfo[selectedTab].latencyStats.externalLatency.lowest;
	tabsInfo[selectedTab].latencyStats.inputLatency.lowest = inputTime < tabsInfo[selectedTab].latencyStats.inputLatency.lowest ? inputTime : tabsInfo[selectedTab].latencyStats.inputLatency.lowest;

	// If there are no measurements done yet set the minimum value to the current one
	if (size == 0)
	{
		tabsInfo[selectedTab].latencyStats.internalLatency.lowest = internalTime;
		tabsInfo[selectedTab].latencyStats.externalLatency.lowest = externalTime;
		tabsInfo[selectedTab].latencyStats.inputLatency.lowest = inputTime;
	}

	wasItemAddedGUI = true;
	reading.index = size;
	tabsInfo[selectedTab].latencyData.measurements.push_back(reading);
	tabsInfo[selectedTab].isSaved = false;
}

void GotSerialChar(char c)
{
	if (c == 0)
		return;

#ifdef _DEBUG
	printf("Got: %c\n", c);
#endif

	// 6 digits should be enough. I doubt the latency can be bigger than 10 seconds (anything greater than 100ms should be discarded)
	const size_t resultBufferSize = 6;
	static BYTE resultBuffer[resultBufferSize]{ 0 };
	static BYTE resultNum = 0;

	static std::chrono::steady_clock::time_point internalStartTime;
	static std::chrono::steady_clock::time_point internalEndTime;

	static std::chrono::steady_clock::time_point pingStartTime;

	// Because we are subtracting 2 similar unsigned long longs, we dont need another unsigned long long, we just need an int
	static unsigned int internalTime = 0;
	static unsigned int externalTime = 0;


	if (!((c >= 97 && c <= 122) || (c >= 48 && c <= 57)))
	{
		externalTime = internalTime = 0;
		serialStatus = Status_Idle;
	}

	switch (serialStatus)
	{
	case Status_Idle:
		// Input (button press) detected (l for light)
		if (c == 'l')
		{
			internalStartTime = std::chrono::high_resolution_clock::now();
			lastInternalTest = micros();
#ifdef _DEBUG
			printf("waiting for results\n");
#endif
			serialStatus = Status_WaitingForResult;

			if (isAudioMode)
				audioPlayer.Play();

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
		return;
		break;
	case Status_WaitingForResult:
		if (resultNum == 0)
			internalEndTime = std::chrono::high_resolution_clock::now();

		// e for end (end of the numbers)
		// All of the code below will have to be moved to a sepearate function in the future when saving/loading from a file will be added. (Little did he know)
		if (c == 'e')
		{
			externalTime = 0;
			internalTime = std::chrono::duration_cast<std::chrono::microseconds>(internalEndTime - internalStartTime).count();
			// Convert the byte array to int
			for (size_t i = 0; i < min(resultNum, resultBufferSize); i++)
			{
				externalTime += resultBuffer[i] * pow<int>(10, (resultNum - i - 1));
			}

			resultNum = 0;
			std::fill_n(resultBuffer, resultBufferSize, 0);

			if (max(externalTime * 1000, internalTime) > 1000000 || min(externalTime * 1000, internalTime) < 1000)
			{
				serialStatus = Status_Idle;
				ZeroMemory(resultBuffer, resultBufferSize);
				break;
			}

			wasMouseClickSent = false;
			wasLMB_Pressed = false;

			//if (!wasModdedMouseTimeUpdated)
			//	moddedMouseTimeClicked += std::chrono::microseconds(375227);
#ifdef _DEBUG
			printf("Final result: %i\n", externalTime);
#endif

			//AddMeasurement(internalTime, externalTime, 0);

			//_write(Serial::fd, "p", 1);
			Serial::Write("p", 1);
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
			unsigned int pingTime = 0;
			//if (selectedMode == InputMode_ModdedMouse) {
			//	pingTime = std::chrono::duration_cast<std::chrono::microseconds>(abs(moddedMouseTimeClicked - internalStartTime)).count();
			//	wasModdedMouseTimeUpdated = false;
			//}
			//else
			pingTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - pingStartTime).count();
			//tabsInfo[selectedTab].latencyData.measurements[tabsInfo[selectedTab].latencyData.measurements.size() - 1].timePing = pingTime;

			//size_t size = tabsInfo[selectedTab].latencyData.measurements.size(); -1;
			// account for the serial timeout delay (this is just an estimate)
			internalTime -= 500000 / SERIAL_UPDATE_TIME;
			externalTime -= 1000 / SERIAL_UPDATE_TIME;
			pingTime -= 1000000 / SERIAL_UPDATE_TIME;
			AddMeasurement(internalTime, externalTime, pingTime);

			//tabsInfo[selectedTab].latencyStats.inputLatency.average = (tabsInfo[selectedTab].latencyStats.inputLatency.average * size) / (size + 1.f) + (pingTime / (size + 1.f));
			//tabsInfo[selectedTab].latencyStats.inputLatency.highest = pingTime > tabsInfo[selectedTab].latencyStats.inputLatency.highest ? pingTime : tabsInfo[selectedTab].latencyStats.inputLatency.highest;
			//tabsInfo[selectedTab].latencyStats.inputLatency.lowest = pingTime < tabsInfo[selectedTab].latencyStats.inputLatency.lowest ? pingTime : tabsInfo[selectedTab].latencyStats.inputLatency.lowest;

			//if (size == 0)
			//{
			//	tabsInfo[selectedTab].latencyStats.inputLatency.lowest = pingTime;
			//}

			serialStatus = Status_Idle;
		}
		break;
	default:
		break;
	}

	// Something was missed, better remove the last measurement
	//if (c == 'l' && serialStatus == Status_WaitingForResult)
	//{
	//	serialStatus = Status_Idle;
	//}

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

bool SetupAudioDevice()
{
	int res = curAudioDevice->AddBuffer(WARMUP_AUDIO_BUFFER_SIZE);
	if (res)
		return false;

	res = curAudioDevice->AddBuffer(TEST_AUDIO_BUFFER_SIZE, true);
	if (res)
		return false;

	return true;
}

bool CanDoInternalTest()
{
	return isInternalMode && curAudioDevice && (micros() > lastInternalTest + (INTERNAL_TEST_DELAY + (rand() / 10)));
}

void DiscoverAudioDevices()
{
	AudioDeviceInfo* devices;
	UINT devCount = GetAudioDevices(&devices);

	availableAudioDevices.clear();

	for (int i = 0; i < devCount; i++)
	{
		auto& dev = devices[i];
		availableAudioDevices.push_back(cAudioDeviceInfo());

		size_t _charsConverted;

		int size_offset = 0;
		int end_offset = 0;
		if (wcsstr(dev.friendlyName, L"Microphone (") == dev.friendlyName) {
			size_offset += 12;
			end_offset += 1;
		}

		char* fName = new char[wcslen(dev.friendlyName) + 1 - size_offset];
		wcstombs_s(&_charsConverted, fName, wcslen(dev.friendlyName) + 1 - size_offset, dev.friendlyName + size_offset, wcslen(dev.friendlyName));

		fName[strlen(fName) - end_offset] = 0;

		char* pName = new char[wcslen(dev.portName) + 1];
		wcstombs_s(&_charsConverted, pName, wcslen(dev.portName) + 1, dev.portName, wcslen(dev.portName));

		availableAudioDevices[i].friendlyName = fName;
		availableAudioDevices[i].portName = pName;
		availableAudioDevices[i].id = dev.id;
		availableAudioDevices[i].WASAPI_id = dev.WASAPI_id;
		availableAudioDevices[i].AudioEnchantments = dev.AudioEnchantments;
	}

	delete[] devices;
}

void StartInternalTest()
{
	if (!CanDoInternalTest())
		return;

	serialStatus = Status_AudioReadying;

	lastInternalTest = micros();

	if (curAudioDevice->GetPlayTime().u.sample > 0)
		curAudioDevice->ResetRecording();
	else
		curAudioDevice->StartRecording();
}

bool GetMonitorModes(DXGI_MODE_DESC* modes, UINT* size)
{
	IDXGIOutput* output;

	if (GUI::g_pSwapChain->GetContainingOutput(&output) != S_OK)
		return false;

	//DXGI_FORMAT_R8G8B8A8_UNORM
	if (output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, size, modes) != S_OK)
	{
		return false;
		printf("Error getting the display modes\n");
	}

	//if (*size > 0)
	//{
	//	auto targetMode = modes[*size - 1];
	//	targetMode.RefreshRate.Numerator = 100000000;
	//	DXGI_MODE_DESC matchMode;
	//	output->FindClosestMatchingMode(&targetMode, &matchMode, GUI::g_pd3dDevice);

	//	printf("Best mode match: %ix%i@%iHz\n", matchMode.Width, matchMode.Height, (matchMode.RefreshRate.Numerator / matchMode.RefreshRate.Denominator));
	//}

	return true;
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

void AttemptConnect()
{
	if (isInternalMode && !availableAudioDevices.empty())
	{
		if (curAudioDevice)
			delete curAudioDevice;

		curAudioDevice = new Waveform_SoundHelper(availableAudioDevices[selectedPort].id, AUDIO_BUFFER_SIZE, 2u, AUDIO_SAMPLE_RATE, 16u, (DWORD_PTR)waveInCallback);
		if (curAudioDevice && curAudioDevice->isCreated)
			isAudioDevConnected = true;

		isAudioDevConnected = SetupAudioDevice();

		if (isAudioDevConnected)
			SetAudioEnchantments(false, availableAudioDevices[selectedPort].WASAPI_id);
	}
	else if(!Serial::availablePorts.empty())
		Serial::Setup(Serial::availablePorts[selectedPort].c_str(), GotSerialChar);
}

void AttemptDisconnect()
{
	if (isInternalMode)
	{
		curAudioDevice->StopRecording();

		SetAudioEnchantments(availableAudioDevices[selectedPort].AudioEnchantments, availableAudioDevices[selectedPort].WASAPI_id);

		if (curAudioDevice)
			delete curAudioDevice;

		curAudioDevice = nullptr;

		isAudioDevConnected = false;
	}
	else {
		Serial::Close();
		serialStatus = Status_Idle;
	}
}

// Returns true if should exit, false otherwise
bool OnExit()
{
	bool isSaved = true;

	unsavedTabs.clear();

	for (size_t i = 0; i < tabsInfo.size(); i++)
	{
		if (!tabsInfo[i].isSaved)
		{
			isSaved = false;
			unsavedTabs.push_back(i);
		}
	}

	if (isSaved)
	{
		// Gui closes itself from the wnd messages
		Serial::Close();

		exit(0);
	}

	isExiting = true;
	return isSaved;
}

void KeyDown(WPARAM key, LPARAM info, bool isPressed)
{
	//enum class ModKey {
	//	ModKey_None = 0,
	//	ModKey_Ctrl = 1,
	//	ModKey_Shift = 2,
	//	ModKey_Alt = 4,
	//}; Just for information

	
	bool wasJustClicked = !(info & 0x40000000);
	static unsigned char modKeyFlags = 0;

	if (key == -1 && info == -1) {
		modKeyFlags = 0;
		return;
	}

	if (isPressed && (key == VK_CONTROL || key == VK_LCONTROL))
		modKeyFlags |= 1;

	if (!isPressed && (key == VK_CONTROL || key == VK_LCONTROL))
		modKeyFlags ^= 1;


	if (isPressed && (key == VK_SHIFT || key == VK_RSHIFT))
		modKeyFlags |= 2;

	if (!isPressed && (key == VK_SHIFT || key == VK_RSHIFT))
		modKeyFlags ^= 2;


	if (isPressed && (key == VK_LMENU || key == VK_RMENU || key == VK_MENU))
		modKeyFlags |= 4;

	if (!isPressed && (key == VK_LMENU || key == VK_RMENU || key == VK_MENU))
		modKeyFlags ^= 4;

	printf("Key mod: %u\n", modKeyFlags);


	// Handle time-sensitive KB input
	if (isInternalMode && key == 'R' && isPressed && modKeyFlags == 2 && !serialStatus == Status_WaitingForResult)
	{
		StartInternalTest();
	}

	if (modKeyFlags == 1 && isPressed)
	{
		if (key == 'S' && wasJustClicked)
		{
			SaveMeasurements();
		}
		else if (key == 'O' && wasJustClicked)
		{
			OpenMeasurements();
		}
		else if (key == 'N' && wasJustClicked)
		{
			auto newTab = TabInfo();
			strcat_s<TAB_NAME_MAX_SIZE>(newTab.name, std::to_string(tabsInfo.size()).c_str());
			tabsInfo.push_back(newTab);
			selectedTab = tabsInfo.size() - 1;
		}
		else if (key == 'Q' && wasJustClicked)
		{
			if (isInternalMode ? isAudioDevConnected : Serial::isConnected)
				AttemptDisconnect();
			else
				AttemptConnect();
		}
	}
	if (modKeyFlags == 3 && key == 'S' && wasJustClicked)
		SaveAsMeasurements();
	if ((modKeyFlags & 4) == 4 && isPressed)
	{
		if (key == 'S' && modKeyFlags == 4 && wasJustClicked)
		{
			SavePackMeasurements();
		}

		if (key == 'S' && (modKeyFlags & 2) == 2 && wasJustClicked)
		{
			SavePackAsMeasurements();
		}
	}
}

//void RawInputEvent(RAWINPUT* ri)
//{
//	if (ri->header.dwType == RIM_TYPEMOUSE)
//	{
//		if ((ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) == RI_MOUSE_BUTTON_1_DOWN) {
//			moddedMouseTimeClicked = std::chrono::high_resolution_clock::now();
//			wasModdedMouseTimeUpdated = true;
//			//printf("mouse button pressed\n");
//		}
//	}
//}

void millisSleep(LONGLONG ns) {
	::LARGE_INTEGER ft;
	ft.QuadPart = -static_cast<int64_t>(ns*10);  // '-' using relative time

	::HANDLE timer = ::CreateWaitableTimer(NULL, TRUE, NULL);
	::SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	::WaitForSingleObject(timer, 20);
	::CloseHandle(timer);
}

// Main code
int main(int argc, char** argv)
{
	hwnd = GUI::Setup(OnGui);
	GUI::onExitFunc = OnExit;

	srand(time(NULL));
		
	localPath = argv[0];

	QueryPerformanceCounter(&StartingTime);

	//if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
	//	printf("Priotity set to the Highest\n");

#ifndef _DEBUG
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#else
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
#endif

	//::ShowWindow(::GetConsoleWindow(), SW_SHOW);

	static size_t frameIndex = 0;
	static uint64_t frameSum = 0;

	bool done = false;

	ImGuiIO& io = ImGui::GetIO();

	leftTabWidth = GUI::windowX/2;

	GUI::KeyDownFunc = KeyDown;
	//GUI::RawInputEventFunc = RawInputEvent;

	auto defaultTab = TabInfo();
	defaultTab.name[4] = '0';
	defaultTab.name[5] = '\0';
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
		currentUserData.performance.guiLockedFps = (monitorModes[modesNum - 1].RefreshRate.Numerator / monitorModes[modesNum - 1].RefreshRate.Denominator) * 2;

#ifdef _DEBUG

		for (UINT i = 0; i < modesNum; i++)
		{
			printf("Mode %i: %ix%i@%iHz\n", i, monitorModes[i].Width, monitorModes[i].Height, (monitorModes[modesNum - 1].RefreshRate.Numerator / monitorModes[modesNum - 1].RefreshRate.Denominator));
		}

#endif // DEBUG

		backupUserData.performance.guiLockedFps = currentUserData.performance.guiLockedFps;

		currentUserData.style.mainColorBrightness = backupUserData.style.mainColorBrightness = .1f;
		currentUserData.style.fontColorBrightness = backupUserData.style.fontColorBrightness = .1f;
		currentUserData.style.fontSize = backupUserData.style.fontSize = 1.f;

		shouldRunConfiguration = true;

		// Note: this style might be a little bit different prior to applying it. (different darker colors)
	}

	// Just imagine what would happen if user for example had a 220 character long username and path to %AppData% + "imgui.ini" would exceed 260. lovely
	auto imguiFileIniPath = (currentUserData.misc.localUserData ? HelperJson::GetLocalUserConfingPath() : HelperJson::GetAppDataUserConfingPath());
	imguiFileIniPath = imguiFileIniPath.substr(0, imguiFileIniPath.find_last_of(L"\\/") + 1) + L"imgui.ini";
	char outputPathBuffer[MAX_PATH];

	wcstombs_s(nullptr, outputPathBuffer, MAX_PATH, imguiFileIniPath.c_str(), imguiFileIniPath.size());

	io.IniFilename = outputPathBuffer;

	Serial::FindAvailableSerialPorts();

	printf("Found %lld serial Ports\n", Serial::availablePorts.size());

	DiscoverAudioDevices();

	// Setup Audio buffer (with noise)
	audioPlayer.Setup();
	audioPlayer.SetBuffer([](int i) { return ((rand() * 2 - RAND_MAX) % RAND_MAX) * 10; });
	//audioPlayer.SetBuffer([](int i) { return rand() % SHRT_MAX; });
	//audioPlayer.SetBuffer([](int i) { return (int)(sinf(i/4.f) * RAND_MAX); });

	//BOOL fScreen;
	//IDXGIOutput* output;
	//GUI::g_pSwapChain->GetFullscreenState(&fScreen, &output);
	//isFullscreen = fScreen;


	//if (Serial::Setup("COM4", GotSerialChar))
	//	printf("Serial Port opened successfuly");
	//else
	//	printf("Error setting up the Serial Port");

	// used to cover the time of rendering a frame when calculating when a next frame should be displayed
	uint64_t lastFrameRenderTime = 0;

	int GUI_Return_Code = 0;

	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	timeBeginPeriod(tc.wPeriodMin);

MainLoop:

	// Main Loop
	while (!done)
	{
		//Serial::HandleInput();

		//if(isGameMode)
		//	HandleGameMode();

		// GUI Loop
		uint64_t curTime = micros();
		if ((curTime - lastFrameGui + (lastFrameRenderTime) >= 1000000 / (currentUserData.performance.VSync ?
			(GUI::MAX_SUPPORTED_FRAMERATE / currentUserData.performance.VSync) - 1 :
			currentUserData.performance.guiLockedFps)) || !currentUserData.performance.lockGuiFps)
		{
			uint64_t frameStartRender = curTime;
			if (GUI_Return_Code = GUI::DrawGui())
				break;
			lastFrameRenderTime = lastFrameGui - frameStartRender;
			curTime += lastFrameRenderTime;
		}

		//uint64_t CPUTime = micros();

		uint64_t newFrame = curTime - lastFrame;

		if (newFrame > 1000000)
		{
			totalFrames++;
			lastFrame = curTime;
			continue;
		}

		// Average frametime calculation
		//printf("Frame: %i\n", frameIndex);
		frameIndex = min(AVERAGE_FRAME_COUNT - 1, frameIndex);
		frameSum -= frames[frameIndex];
		frameSum += newFrame;
		frames[frameIndex] = newFrame;
		frameIndex = (frameIndex + 1) % AVERAGE_FRAME_COUNT;

		averageFrametime = ((float)frameSum) / AVERAGE_FRAME_COUNT;

		totalFrames++;
		lastFrame = curTime;

		// Try to sleep enough to "wake up" right before having to display another frame
		if (!currentUserData.performance.VSync && currentUserData.performance.lockGuiFps)
		{
			uint64_t frameTime = 1000000 / currentUserData.performance.guiLockedFps;
			uint64_t nextFrameIn = lastFrameGui + frameTime - curTime - lastFrameRenderTime;

			if (nextFrameIn > 2500)
				millisSleep((nextFrameIn - 2400));
				//std::this_thread::sleep_for(std::chrono::microseconds(nextFrameIn - 1050));
		}

#ifdef _DEBUG
		// Limit FPS for eco-friendly purposes (Significantly affects the performance) (Windows does not support sub 1 millisecond sleep)
		Sleep(1);
#endif // _DEBUG
	}

	// Check if everything is saved before exiting the program.
	if (!tabsInfo[selectedTab].isSaved && GUI_Return_Code < 1)
	{
		goto MainLoop;
	}

	if (curAudioDevice) {
		curAudioDevice->StopRecording();
		delete curAudioDevice;
	}

	timeEndPeriod(tc.wPeriodMin);
	Serial::Close();
	GUI::Destroy();
}