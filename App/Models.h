#pragma once

#include <string>
#include <vector>
#include "Config.h"

using namespace Config;

enum SerialStatus {
	Status_Idle,
	Status_WaitingForResult,
	Status_AudioReadying,
	Status_WaitingForPing,
};

enum InputMode {
	InputMode_Normal,
	InputMode_ModdedMouse,
	InputMode_Internal,
};

inline const char *GetInputModeName(const InputMode im) {
	switch (im) {
		case InputMode_Normal:
			return "Normal";
		case InputMode_ModdedMouse:
			return "Modded Mouse";
		case InputMode_Internal:
			return "Internal";
		default:
			return "Unknown";
	}
}

struct LatencyReading {
	unsigned int timeExternal; // in micros
	unsigned int timeInternal; // in micros
	unsigned int timeInput; // in micros
	unsigned int index;
};

struct LatencyParameters {
	float average;
	unsigned int highest;
	unsigned int lowest;
};

struct LatencyStats {
	LatencyParameters externalLatency;
	LatencyParameters internalLatency;
	LatencyParameters inputLatency;
};

struct LatencyData {
	std::vector<LatencyReading> measurements;
	char note[1000]{0};
};

struct TabInfo {
	LatencyData latencyData{};
	LatencyStats latencyStats{};

	bool isSaved = true;
	char savePath[MAX_PATH]{0};
	char savePathPack[MAX_PATH]{0};

	int saved_ver = JSON_VERSION;

	char name[TAB_NAME_MAX_SIZE]{"Tab "};

	std::string ExportAsCSV() const {
		std::string res;

		res += name;
		res += CSV_SEP;
		res += "Internal Latency [ms]";
		res += CSV_SEP;
		res += "External Latency [ms]";
		res += CSV_SEP;
		res += "Input Latency [ms]";
		res += CSV_SEP;

		// Add all measurements
		for (int i = 0; i < latencyData.measurements.size(); i++) {
			res += '\n';
			res += std::to_string(i) + CSV_SEP;
			res += std::to_string(latencyData.measurements[i].timeInternal / 1000.0) + CSV_SEP;
			res += std::to_string(latencyData.measurements[i].timeExternal / 1000.0) + CSV_SEP;
			res += std::to_string(latencyData.measurements[i].timeInput / 1000.0) + CSV_SEP;
		}

		// Add statistics
		res += "\n\n";
		res += "Average:\nType";
		res += CSV_SEP;
		res += "Internal Latency [ms]";
		res += CSV_SEP;
		res += "External Latency [ms]";
		res += CSV_SEP;
		res += "Input Latency [ms]";
		res += CSV_SEP;
		res += "\nLatency";
		res += CSV_SEP;
		res += std::to_string(latencyStats.internalLatency.average / 1000.0) + CSV_SEP;
		res += std::to_string(latencyStats.externalLatency.average / 1000.0) + CSV_SEP;
		res += std::to_string(latencyStats.inputLatency.average / 1000.0) + CSV_SEP;

		return res;
	}
};

struct StyleData {
	float mainColor[4];
	float mainColorBrightness;

	float fontColor[4];
	float fontColorBrightness;

	float internalPlotColor[4]{0.1f, 0.8f, 0.2f, 1.f};
	float externalPlotColor[4]{0.3f, 0.3f, 0.9f, 1.f};
	float inputPlotColor[4]{0.8f, 0.1f, 0.2f, 1.f};


	int selectedFont;
	float fontSize;
};

struct PerformanceData {
	bool lockGuiFps = true;
	int guiLockedFps = 60;

	bool showPlots;

	// float maxSerialReadFrequency = 600;

	unsigned int VSync;
};

struct MiscData {
	bool localUserData = false;
};

struct UserData {
	StyleData style;
	PerformanceData performance;
	MiscData misc;
};

enum JSON_HANDLE_INFO {
	JSON_HANDLE_INFO_SUCCESS = 0,
	JSON_HANDLE_INFO_ERROR = 1,
	JSON_HANDLE_INFO_BAD_VERSION = (1 << 1),
};

inline JSON_HANDLE_INFO operator|(const JSON_HANDLE_INFO a, const JSON_HANDLE_INFO b) {
	return static_cast<JSON_HANDLE_INFO>(a + b);
}
inline JSON_HANDLE_INFO operator&(const JSON_HANDLE_INFO a, const JSON_HANDLE_INFO b) {
	return static_cast<JSON_HANDLE_INFO>(a * b);
}
inline JSON_HANDLE_INFO &operator|=(JSON_HANDLE_INFO &a, const JSON_HANDLE_INFO b) { return a = a | b; }
inline JSON_HANDLE_INFO &operator&=(JSON_HANDLE_INFO &a, const JSON_HANDLE_INFO b) { return a = a & b; }
