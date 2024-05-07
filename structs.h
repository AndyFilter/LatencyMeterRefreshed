#pragma once

#include <vector>
#include "constants.h"

enum SerialStatus
{
	Status_Idle,
	Status_WaitingForResult,
	Status_AudioReadying,
	Status_WaitingForPing,
};

enum InputMode
{
	InputMode_Normal,
	InputMode_ModdedMouse,
	InputMode_Internal,
};
inline const char* GetInputModeName(InputMode im) {
	switch (im)
	{
	case InputMode_Normal:
		return "Normal";
		break;
	case InputMode_ModdedMouse:
		return "Modded Mouse";
		break;
	case InputMode_Internal:
		return "Internal";
		break;
	default:
		break;
	}
}

struct LatencyReading
{
	unsigned int timeExternal; // in millis
	unsigned int timeInternal; // in micros
	unsigned int timePing; // in micros
	unsigned int index;
};

struct LatencyParameters
{
	float average;
	unsigned int highest;
	unsigned int lowest;
};

struct LatencyStats
{
	LatencyParameters externalLatency;
	LatencyParameters internalLatency;
	LatencyParameters inputLatency;
};

struct LatencyData
{
	std::vector<LatencyReading> measurements;
	char note[1000]{ 0 };
};

#define TAB_NAME_MAX_SIZE 64
struct TabInfo
{
	LatencyData latencyData {};
	LatencyStats latencyStats {};

	bool isSaved = true;
	char savePath[260]{ 0 };
	char savePathPack[260]{ 0 };

    int saved_ver = json_version;

	char name[TAB_NAME_MAX_SIZE]{ "Tab " };
};


struct StyleData
{
	float mainColor[4];
	float mainColorBrightness;

	float fontColor[4];
	float fontColorBrightness;

	float internalPlotColor[4]{ 0.1f, 0.8f, 0.2f, 1.f };
	float externalPlotColor[4]{ 0.3f, 0.3f, 0.9f, 1.f };
	float inputPlotColor[4]{ 0.8f, 0.1f, 0.2f, 1.f };


	int selectedFont;
	float fontSize;
};

struct PerformanceData
{
	bool lockGuiFps = true;
	int guiLockedFps = 60;

	bool showPlots;

	//float maxSerialReadFrequency = 600;

	unsigned int VSync;
};

struct MiscData
{
	bool localUserData = false;
};

struct UserData
{
	StyleData style;
	PerformanceData performance;
	MiscData misc;
};

enum JSON_HANDLE_INFO {
    JSON_HANDLE_INFO_SUCCESS = 0,
    JSON_HANDLE_INFO_ERROR = 1,
    JSON_HANDLE_INFO_BAD_VERSION = (1 << 1),
};

inline JSON_HANDLE_INFO operator |(JSON_HANDLE_INFO a, JSON_HANDLE_INFO b) { return static_cast<JSON_HANDLE_INFO>(a + b); }
inline JSON_HANDLE_INFO operator &(JSON_HANDLE_INFO a, JSON_HANDLE_INFO b) { return static_cast<JSON_HANDLE_INFO>(a * b); }
inline JSON_HANDLE_INFO& operator |=(JSON_HANDLE_INFO& a, JSON_HANDLE_INFO b) { return a = a | b; }
inline JSON_HANDLE_INFO& operator &=(JSON_HANDLE_INFO& a, JSON_HANDLE_INFO b) { return a = a & b; }