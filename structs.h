#pragma once

enum SerialStatus
{
	Status_Idle,
	Status_WaitingForResult,
	Status_Ping,
};

struct LatencyReading
{
	int timeExternal; // in millis
	int timeInternal; // in micros
};