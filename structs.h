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


struct StyleData
{
	float mainColor[4];
	float mainColorBrightness;

	float fontColor[4];
	float fontColorBrightness;


	int selectedFont;
	float fontSize;
};

struct UserData
{
	StyleData style;
};