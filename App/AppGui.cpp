#include "AppGui.h"
#include "../GUI/Gui.h"
#include "../GUI/UIComponents.h"
#include "AppCore.h"
#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_extensions.h"
#include "External/ImGui/imgui_internal.h"
#include "Helpers/AppHelper.h"
#include "Serial/Serial.h"

using namespace UIComponents;
using namespace AppHelper;
using namespace AppCore;

static AppState *g_State = nullptr;

void AppGui::BindState(AppState &state) { g_State = &state; }

int AppGui::OnGuiThunk() {
	if (g_State == nullptr)
		return 0;
	return OnGui(*g_State);
}

int AppGui::OnGui(AppState &state) {
	static bool isSettingOpen = false;
	static ImVec2 detectionRectSize{0, 200};
	static float leftTabWidth = Gui::WINDOW_X / 2;

	const ImGuiStyle &style = ImGui::GetStyle();

	if (state.shouldRunConfiguration) {
		ImGui::OpenPopup("Set Up");
		state.shouldRunConfiguration = false;
	}

	bool badVerPopupOpen = false;
	if (const int ret = DrawMenuBar(state, isSettingOpen, badVerPopupOpen); ret != 0)
		return ret;
	HandleShortcuts(state);
	DrawSettingsWindow(state, isSettingOpen);
	DrawPopups(state);

	const auto windowAvail = ImGui::GetContentRegionAvail();
	const uint64_t curTime = GetTimeMicros();

	if (state.serialStatus != Status_Idle && curTime > (state.lastInternalTest + Serial::SERIAL_NO_RESPONSE_DELAY))
		state.serialStatus = Status_Idle;

	// TABS
	DrawTabs(state);

	const auto avail = ImGui::GetContentRegionAvail();

	if (ImGui::BeginChild("LeftChild", {leftTabWidth, avail.y - detectionRectSize.y - 5}, true)) {
#ifdef _DEBUG

		// ImGui::Text("Time is: %ims", clock());
		ImGui::Text("Time is: %lu\xC2\xB5s", GetTimeMicros());

		ImGui::SameLine();

		ImGui::Text("Serial Staus: %i", state.serialStatus.load());

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (Gui ONLY)", 1000.0f / ImGui::GetIO().Framerate,
					ImGui::GetIO().Framerate);

		// ImGui::Text("Application average %.3f us/frame (%.1f FPS) (CPU ONLY)", (micros() - lastFrame) / 1000.f,
		// 1000000.0f / (micros() - lastFrame));

		ImGui::Text("Application average %.3f \xC2\xB5s/frame (%.1f FPS) (CPU ONLY)", state.averageFrametime,
					1000000.f / state.averageFrametime);

		// The buffer contains just too many frames to display them on the screen (and displaying every 10th or so frame
		// makes no sense). Also smoothing out the data set would hurt the performance too much
		// ImGui::PlotLines("FPS Chart", [](void* data, int idx) { return sinf(float(frames[idx%10]) / 1000); }, frames,
		// AVERAGE_FRAME_COUNT);

		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, {avail.x, ImGui::GetFrameHeight()});

#endif // _DEBUG
		DrawConnectionPanel(state, curTime);
		ImGui::Spacing();
		DrawLatencyStatsPanel(state);
	}

	DrawPlotsPanel(state);
	ImGui::SameLine();

	static float leftTabStart = 0;
	if (ImGui::GetIO().DisplaySize.x > 100) // Check if the window is minimized
		ImGui::ResizeSeparator(leftTabWidth, leftTabStart, ImGuiSeparatorFlags_Vertical, {300, 300},
							   {6, avail.y - detectionRectSize.y - 5});

	ImGui::SameLine();
	ImGui::BeginGroup();
	auto tableAvail = ImGui::GetContentRegionAvail();
	DrawMeasurementsTable(state, tableAvail);

	// ImGui::SameLine();

	DrawNotesPanel(state, {tableAvail.x, detectionRectSize.y});
	ImGui::EndGroup();

	static float detectionRectStart = 0;
	if (ImGui::GetIO().DisplaySize.x > 100) // Check if the window is minimized
		ImGui::ResizeSeparator(detectionRectSize.y, detectionRectStart, ImGuiSeparatorFlags_Horizontal, {100, 400},
							   {avail.x, 6});

	DrawDetectionRect(state, detectionRectSize, windowAvail);

	if (ImGui::BeginPopupModal("Set Up", nullptr,
							   ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
									   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
		ImGui::Text("Setup configuration");
		ImGui::SeparatorSpace(0, {0, 5});
		ImGui::Checkbox("Save config locally", &state.currentUserData.misc.localUserData);
#ifdef _WIN32
		ImGui::SetItemTooltip(
				"Chose whether you want to save config file in the same directory as the program, or in the AppData "
				"folder");
#else
		ImGui::SetItemTooltip(
				"Chose whether you want to save config file in the same directory as the program, or in the /home/ "
				"folder");
#endif

		ImGui::Spacing();
		if (ImGui::Button("Save", {-1, 0})) {
			SaveCurrentUserConfig();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	if (state.isExiting) {
		if (!state.unsavedTabs.empty()) {
			state.selectedTab = state.unsavedTabs[0];
			ImGui::OpenPopup("Exit?");
		}

		if (ImGui::BeginPopupModal("Exit?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
			ImGui::Text("Are you sure you want to exit before saving?");
			ImGui::SameLine();
			ImGui::Text("(%s)", state.tabsInfo[state.unsavedTabs.front()].name);

			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, {0, 5});

			const float buttonPadding = style.FramePadding.x * 2;
			const float exitButtonWidth = ImGui::CalcTextSize("Exit").x + buttonPadding;
			const float exitSaveButtonWidth = ImGui::CalcTextSize("Exit and save").x + buttonPadding;
			const float cancelButtonWidth = ImGui::CalcTextSize("Cancel").x + buttonPadding;
			const float totalTextWidth = exitButtonWidth + exitSaveButtonWidth + cancelButtonWidth;

			const float availableWidth = ImGui::GetContentRegionAvail().x - (2 * style.ItemSpacing.x);

			const float scale = availableWidth / totalTextWidth;

			if (ImGui::Button("Save and exit", {exitSaveButtonWidth * scale, 0})) {
				if (SaveMeasurements()) {
					ImGui::CloseCurrentPopup();
					state.latencyStatsMutex.lock();
					state.tabsInfo.erase(state.tabsInfo.begin() + state.unsavedTabs.front());
					state.latencyStatsMutex.unlock();
					state.unsavedTabs.erase(state.unsavedTabs.begin());
					if (!state.unsavedTabs.empty())
						state.selectedTab = state.unsavedTabs[0];
					else
						return 2;
				} else
					printf("Could not save the file");
			}

			ImGui::SameLine();
			if (ImGui::Button("Exit", {exitButtonWidth * scale, 0})) {
				ImGui::CloseCurrentPopup();
				state.latencyStatsMutex.lock();
				state.tabsInfo.erase(state.tabsInfo.begin() + state.unsavedTabs.front());
				state.latencyStatsMutex.unlock();
				state.unsavedTabs.erase(state.unsavedTabs.begin());
				// ImGui::EndPopup();
				if (!state.unsavedTabs.empty())
					state.selectedTab = state.unsavedTabs[0];
				else
					return 2;
			}

			ImGui::SameLine();
			if (ImGui::Button("Cancel", {cancelButtonWidth * scale, 0})) {
				ImGui::CloseCurrentPopup();
				state.isExiting = false;
#ifndef _WIN32
				glfwSetWindowShouldClose(Gui::g_window, false);
#endif
			}

			ImGui::EndPopup();
		}
	}

	state.wasMeasurementAddedGUI = false;

	return 0;
}
