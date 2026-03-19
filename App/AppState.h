#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include "../GUI/Gui.h"
#include "Audio/AudioProcessor.h"
#include "Models.h"

struct ImGuiTableSortSpecs;

struct AppState {
	std::atomic<SerialStatus> serialStatus = Status_Idle;
	InputMode selectedMode = InputMode_Normal;
	std::atomic<uint64_t> lastInternalTest = 0;
	size_t lastInternalGateSampleIdx = 0;

	bool isAudioDevConnected = false;
	bool isPlaybackDevConnected = false;
	std::vector<AudioDeviceInfo> availableAudioDevices;
	PaDeviceIndex selectedOutputAudioDeviceIdx = 0;

	bool isGameMode = false;
	bool isInternalMode = false;
	bool isAudioMode = false;

	int selectedPort = 0;
	int lastSelectedPort = 0;

	bool shouldRunConfiguration = false;

	// Frametime in microseconds
	float averageFrametime = 0;
	unsigned long long totalFrames = 0;

	bool isExiting = false;
	std::vector<int> unsavedTabs;

	UserData currentUserData{};
	UserData backupUserData{};

	std::vector<TabInfo> tabsInfo{};
	std::mutex latencyStatsMutex;

	int selectedTab = 0;

	bool isFullscreen = false;
	bool fullscreenModeOpenPopup = false;
	bool fullscreenModeClosePopup = false;
	bool wasMeasurementAddedGUI = false;

	ImGuiTableSortSpecs *sortSpec;

#ifdef _WIN32
	UINT modesNum = 256;
	DXGI_MODE_DESC monitorModes[256];
#endif
};
