#pragma once

#include <cstdint>
#include <vector>
#include "../External/ImGui/imgui.h"

struct AppState;

namespace UIComponents {
	void SetupData();

    int DrawMenuBar(AppState& state, bool& isSettingOpen, bool& badVerPopupOpen);
    void DrawSettingsWindow(AppState& state, bool& isSettingOpen);
    void DrawPopups(AppState& state);

	void DrawTabs(AppState& state);
	void DrawConnectionPanel(AppState& state, std::uint64_t curTime);
	void DrawLatencyStatsPanel(AppState& state);
	void DrawPlotsPanel(AppState& state);
	void DrawMeasurementsTable(AppState& state, ImVec2 spaceAvail);
	void DrawNotesPanel(AppState& state, ImVec2 availSize);
	void DrawDetectionRect(const AppState& state, const ImVec2& detectionRectSize, const ImVec2& windowAvail);

    void HandleShortcuts(AppState& state);
}
