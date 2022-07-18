#pragma once

enum SerialStatus
{
	Status_Idle,
	Status_WaitingForResult,
	Status_WaitingForPing,
};

struct LatencyReading
{
	int timeExternal; // in millis
	int timeInternal; // in micros
	int timePing; // in micros
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
};