#pragma once

enum SerialStatus
{
	Status_Idle,
	Status_WaitingForResult,
	Status_WaitingForPing,
};

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
	char note[1000];
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
	bool lockGuiFps;
	int guiLockedFps;

	bool showPlots;
};

struct UserData
{
	StyleData style;
	PerformanceData performance;
};