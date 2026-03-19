#include "AppHelper.h"
#include <array>
#include <cstring>
#include <fstream>
#include <sstream>
#include "../GUI/Gui.h"
#include "JsonHelper.h"
#ifdef __linux__
#include <X11/Xlib.h>
#endif

namespace AppHelper {
	static AppState *g_State = nullptr;

	void BindState(AppState &state) { g_State = &state; }

	int CompareLatency(const void *a, const void *b) {
		const LatencyReading *_a = (LatencyReading *) a;
		const LatencyReading *_b = (LatencyReading *) b;

		int delta = 0;

		switch (g_State->sortSpec->Specs->ColumnUserID) {
			case 0:
				delta = _a->index - _b->index;
				break;
			case 1:
				delta = _a->timeExternal - _b->timeExternal;
				break;
			case 2:
				delta = _a->timeInternal - _b->timeInternal;
				break;
			case 3:
				delta = _a->timeInput - _b->timeInput;
				break;
			case 4:
			default:
				delta = 0;
				break;
		}

		if (g_State->sortSpec->Specs->SortDirection == ImGuiSortDirection_Ascending) {
			return delta > 0 ? +1 : -1;
		}
		return delta > 0 ? -1 : +1;

	}

#ifdef _WIN32

	uint64_t GetTimeMicros() {
		// QueryPerformance method is about 15-25% faster than std::chrono implementation of the same code

		LARGE_INTEGER endingTime;
		LARGE_INTEGER elapsedMicroseconds;
		LARGE_INTEGER frequency;

		QueryPerformanceFrequency(&frequency);

		QueryPerformanceCounter(&endingTime);
		elapsedMicroseconds.QuadPart = endingTime.QuadPart - g_StartingTime.QuadPart;

		elapsedMicroseconds.QuadPart *= 1000000;
		elapsedMicroseconds.QuadPart /= frequency.QuadPart;

		return elapsedMicroseconds.QuadPart;
	}

#else

	// Get a time stamp in microseconds.
	uint64_t GetTimeMicros() {
		timespec ts{};
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
		const uint64_t us = (static_cast<uint64_t>(ts.tv_sec) * 1000000) + (static_cast<uint64_t>(ts.tv_nsec) / 1000);
		static uint64_t const kStartTime = us;
		return us - kStartTime;
	}
#endif

	void ApplyStyle(const std::array<float*, 4> &colors, const std::array<float, 4>  brightnesses) {
		auto &style = ImGui::GetStyle();
		for (int i = 0; i < STYLE_COLOR_NUM; i++) {
			const float brightness = brightnesses[i];

			auto baseColor = ImVec4(*reinterpret_cast<ImVec4 *>(colors[i]));
			auto darkerBase = ImVec4(baseColor) * (1 - brightness);
			auto darkestBase = ImVec4(baseColor) * (1 - (brightness * 2));

			// Alpha has to be set separately, because it also gets multiplied times brightness.
			const auto alphaBase = baseColor.w;
			baseColor.w = alphaBase;
			darkerBase.w = alphaBase;
			darkestBase.w = alphaBase;

			switch (i) {
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
				default: break;
			}
		}
	}

	void RevertConfig() {
		g_State->currentUserData = g_State->backupUserData;

		// std::copy(accentColorBak, accentColorBak + 4, accentColor);
		// std::copy(fontColorBak, fontColorBak + 4, fontColor);

		// accentBrightness = accentBrightnessBak;
		// fontBrightness = fontBrightnessBak;

		// fontSize = fontSizeBak;
		// selectedFont = selectedFontBak;
		// g_boldFont = boldFontBak;

		// lockGuiFps = lockGuiFpsBak;
		// guiLockedFps = guiLockedFpsBak;

		// showPlots = showPlotsBak;

		ImGuiIO &io = ImGui::GetIO();

		for (int i = 0; i < io.Fonts->Fonts.Size; i++) {
			io.Fonts->Fonts[i]->Scale = g_State->currentUserData.style.fontSize;
		}
		io.FontDefault = io.Fonts->Fonts[FONT_INDEX[g_State->currentUserData.style.selectedFont]];
	}

	void DeleteTab(const int tabIdx) {
		if (tabIdx < 0)
			return;
		std::scoped_lock const lock(g_State->latencyStatsMutex);
		g_State->tabsInfo.erase(g_State->tabsInfo.begin() + tabIdx);
		if (g_State->selectedTab >= tabIdx)
			g_State->selectedTab = std::max(g_State->selectedTab - 1, 0);
	}

	// Does the calculations and copying for you
	void SaveCurrentUserConfig() {
		if (g_State->backupUserData.misc.localUserData != g_State->currentUserData.misc.localUserData)
			HelperJson::UserConfigLocationChanged(g_State->currentUserData.misc.localUserData);

		g_State->backupUserData = g_State->currentUserData;

		HelperJson::SaveUserData(g_State->currentUserData);
		// delete userData;
	}

	bool LoadCurrentUserConfig() {
		// UserData userData;
		return HelperJson::GetUserData(g_State->currentUserData);
	}

	void LoadAndHandleNewUserConfig() {
		auto &io = ImGui::GetIO();

		if (LoadCurrentUserConfig()) {
			ApplyCurrentStyle();

			// Set the default colors to the variables with a type conversion (ImVec4 -> float[4]) (could be done with
			// std::copy, but the performance advantage it gives is just immeasurable, especially for a single time
			// execution code)

			g_State->backupUserData = g_State->currentUserData;

			Gui::LoadFonts(g_State->currentUserData.style.fontSize);

			for (int i = 0; i < io.Fonts->Fonts.Size; i++) {
				io.Fonts->Fonts[i]->Scale = g_State->currentUserData.style.fontSize;
			}

			io.FontDefault = io.Fonts->Fonts[FONT_INDEX[g_State->currentUserData.style.selectedFont]];

			auto *const boldFont = GetFontBold(g_State->currentUserData.style.selectedFont);

			if (boldFont != nullptr)
				g_boldFont = boldFont;
			else
				g_boldFont = io.Fonts->Fonts[FONT_INDEX[g_State->currentUserData.style.selectedFont]];
		} else {
			const auto &styleColors = ImGui::GetStyle().Colors;
			memcpy(&g_State->currentUserData.style.mainColor, &(styleColors[ImGuiCol_Header]), COLOR_SIZE);
			memcpy(&g_State->currentUserData.style.fontColor, &(styleColors[ImGuiCol_Text]), COLOR_SIZE);

			memcpy(&g_State->backupUserData.style.mainColor, &(styleColors[ImGuiCol_Header]), COLOR_SIZE);
			memcpy(&g_State->backupUserData.style.fontColor, &(styleColors[ImGuiCol_Text]), COLOR_SIZE);

			Gui::LoadFonts(1);

			g_State->currentUserData.performance.lockGuiFps = g_State->backupUserData.performance.lockGuiFps = true;

			g_State->currentUserData.performance.showPlots = g_State->backupUserData.performance.showPlots = true;

			g_State->currentUserData.performance.VSync = g_State->backupUserData.performance.VSync = 0u;

#ifdef _WIN32
			DEBUG_PRINT("found %u monitor modes\n", g_State->modesNum);
			g_State->currentUserData.performance.guiLockedFps =
					(g_State->monitorModes[g_State->modesNum - 1].RefreshRate.Numerator /
					 g_State->monitorModes[g_State->modesNum - 1].RefreshRate.Denominator) *
					2;

#ifdef _DEBUG

			for (UINT i = 0; i < g_State->modesNum; i++) {
				DEBUG_PRINT("Mode %i: %ix%i@%iHz\n", i, g_State->monitorModes[i].Width, g_State->monitorModes[i].Height,
					   (g_State->monitorModes[g_State->modesNum - 1].RefreshRate.Numerator /
						g_State->monitorModes[g_State->modesNum - 1].RefreshRate.Denominator));
			}

#endif // DEBUG

#endif // _WIN32

			g_State->backupUserData.performance.guiLockedFps = g_State->currentUserData.performance.guiLockedFps;

			g_State->currentUserData.style.mainColorBrightness = g_State->backupUserData.style.mainColorBrightness =
					.1f;
			g_State->currentUserData.style.fontColorBrightness = g_State->backupUserData.style.fontColorBrightness =
					.1f;
			g_State->currentUserData.style.fontSize = g_State->backupUserData.style.fontSize = 1.f;

			g_State->shouldRunConfiguration = true;

			// Note: this style might be a little bit different prior to applying it. (different darker colors)
		}
	}

	void ApplyCurrentStyle() {
		auto &style = ImGui::GetStyle();

		const float brightnesses[STYLE_COLOR_NUM]{g_State->currentUserData.style.mainColorBrightness,
												  g_State->currentUserData.style.fontColorBrightness};
		float *colors[STYLE_COLOR_NUM]{g_State->currentUserData.style.mainColor,
									   g_State->currentUserData.style.fontColor};

		for (int i = 0; i < STYLE_COLOR_NUM; i++) {
			const float brightness = brightnesses[i];

			auto baseColor = ImVec4(*reinterpret_cast<ImVec4 *>(colors[i]));
			auto darkerBase = ImVec4(baseColor) * (1 - brightness);
			auto darkestBase = ImVec4(baseColor) * (1 - brightness * 2);

			// Alpha has to be set separately, because it also gets multiplied times brightness.
			const auto alphaBase = baseColor.w;
			baseColor.w = alphaBase;
			darkerBase.w = alphaBase;
			darkestBase.w = alphaBase;

			switch (i) {
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
				default:
					break;
			}
		}
	}

	void SetPlotLinesColor(ImVec4 color) {
		const auto dark = color * (1 - g_State->currentUserData.style.mainColorBrightness);

		ImGui::PushStyleColor(ImGuiCol_PlotLines, dark);
		ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, color);
	}

	ImFont *GetFontBold(const int baseFontIndex) {
		const ImGuiIO &io = ImGui::GetIO();
		std::string const fontDebugName = io.Fonts->Fonts[FONT_INDEX[baseFontIndex]]->GetDebugName();
		if (fontDebugName.find('-') == std::string::npos || fontDebugName.find(',') == std::string::npos)
			return nullptr;
		const auto fontName = fontDebugName.substr(0, fontDebugName.find('-'));
		auto *const boldFont = (io.Fonts->Fonts[FONT_INDEX[baseFontIndex] + 1]);
		const auto boldName = static_cast<std::string>(boldFont->GetDebugName()).substr(0, fontDebugName.find('-'));
		if (fontName == boldName) {
			printf("found bold font %s\n", boldFont->GetDebugName());
			return boldFont;
		}
		return nullptr;
	}

	// This could also be done just using a single global variable like "hasStyleChanged", because ImGui elements like
	// FloatSlider, ColorEdit4 return a value (true) if the value has changed. But this method would not work as
	// expected when the user reverts back the changes or sets the variable to its original value
	bool HasConfigChanged() {
		const bool brightnesses =
				g_State->currentUserData.style.mainColorBrightness ==
						g_State->backupUserData.style.mainColorBrightness &&
				g_State->currentUserData.style.fontColorBrightness == g_State->backupUserData.style.fontColorBrightness;
		const bool font = g_State->currentUserData.style.selectedFont == g_State->backupUserData.style.selectedFont &&
						  g_State->currentUserData.style.fontSize == g_State->backupUserData.style.fontSize;
		const bool performance =
				g_State->currentUserData.performance.guiLockedFps == g_State->backupUserData.performance.guiLockedFps &&
				g_State->currentUserData.performance.lockGuiFps == g_State->backupUserData.performance.lockGuiFps &&
				g_State->currentUserData.performance.showPlots == g_State->backupUserData.performance.showPlots &&
				g_State->currentUserData.performance.VSync == g_State->backupUserData.performance.VSync;

		bool colors{false};
		// Compare arrays of colors column by column, because otherwise we would just compare pointers to these values
		// which would always yield a false positive result. (Pointers point to different addresses even tho the values
		// at these addresses are the same)
		for (int column = 0; column < 4; column++) {
			if (g_State->currentUserData.style.mainColor[column] != g_State->backupUserData.style.mainColor[column]) {
				colors = false;
				break;
			}

			if (g_State->currentUserData.style.fontColor[column] != g_State->backupUserData.style.fontColor[column]) {
				colors = false;
				break;
			}

			if (g_State->currentUserData.style.internalPlotColor[column] !=
				g_State->backupUserData.style.internalPlotColor[column]) {
				colors = false;
				break;
			}

			if (g_State->currentUserData.style.externalPlotColor[column] !=
				g_State->backupUserData.style.externalPlotColor[column]) {
				colors = false;
				break;
			}

			if (g_State->currentUserData.style.inputPlotColor[column] !=
				g_State->backupUserData.style.inputPlotColor[column]) {
				colors = false;
				break;
			}

			colors = true;
		}

		return !colors || !brightnesses || !font || !performance;
	}

	void RecalculateStats(const bool recalculate_Average, const int tabIdx) {
		const size_t size = g_State->tabsInfo[tabIdx].latencyData.measurements.size();
		if (size <= 0)
			return;
		if (recalculate_Average) {
			g_State->tabsInfo[tabIdx].latencyStats.externalLatency.highest = 0;
			g_State->tabsInfo[tabIdx].latencyStats.internalLatency.highest = 0;
			g_State->tabsInfo[tabIdx].latencyStats.inputLatency.highest = 0;

			unsigned int extSum = 0;
			unsigned int intSum = 0;
			unsigned int inpSum = 0;

			for (size_t i = 0; i < size; i++) {
				const auto &test = g_State->tabsInfo[tabIdx].latencyData.measurements[i];

				if (i == 0) {
					g_State->tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
					g_State->tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
					g_State->tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timeInput;
				}

				extSum += test.timeExternal;
				intSum += test.timeInternal;
				inpSum += test.timeInput;

				if (test.timeExternal > g_State->tabsInfo[tabIdx].latencyStats.externalLatency.highest) {
					g_State->tabsInfo[tabIdx].latencyStats.externalLatency.highest = test.timeExternal;
				} else if (test.timeExternal < g_State->tabsInfo[tabIdx].latencyStats.externalLatency.lowest) {
					g_State->tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
				}

				if (test.timeInternal > g_State->tabsInfo[tabIdx].latencyStats.internalLatency.highest) {
					g_State->tabsInfo[tabIdx].latencyStats.internalLatency.highest = test.timeInternal;
				} else if (test.timeInternal < g_State->tabsInfo[tabIdx].latencyStats.internalLatency.lowest) {
					g_State->tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
				}

				if (test.timeInput > g_State->tabsInfo[tabIdx].latencyStats.inputLatency.highest) {
					g_State->tabsInfo[tabIdx].latencyStats.inputLatency.highest = test.timeInput;
				} else if (test.timeInput < g_State->tabsInfo[tabIdx].latencyStats.inputLatency.lowest) {
					g_State->tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timeInput;
				}
			}

			g_State->tabsInfo[tabIdx].latencyStats.externalLatency.average =
					static_cast<float>(extSum / static_cast<int>(size));
			g_State->tabsInfo[tabIdx].latencyStats.internalLatency.average =
					static_cast<float>(intSum / static_cast<int>(size));
			g_State->tabsInfo[tabIdx].latencyStats.inputLatency.average =
					static_cast<float>(inpSum / static_cast<int>(size));
		} else {
			for (size_t i = 0; i < size; i++) {
				const auto &test = g_State->tabsInfo[tabIdx].latencyData.measurements[i];

				if (i == 0) {
					g_State->tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
					g_State->tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
					g_State->tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timeInput;
				}

				if (test.timeExternal > g_State->tabsInfo[tabIdx].latencyStats.externalLatency.highest) {
					g_State->tabsInfo[tabIdx].latencyStats.externalLatency.highest = test.timeExternal;
				} else if (test.timeExternal < g_State->tabsInfo[tabIdx].latencyStats.externalLatency.lowest) {
					g_State->tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
				}

				if (test.timeInternal > g_State->tabsInfo[tabIdx].latencyStats.internalLatency.highest) {
					g_State->tabsInfo[tabIdx].latencyStats.internalLatency.highest = test.timeInternal;
				} else if (test.timeInternal < g_State->tabsInfo[tabIdx].latencyStats.internalLatency.lowest) {
					g_State->tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
				}

				if (test.timeInput > g_State->tabsInfo[tabIdx].latencyStats.inputLatency.highest) {
					g_State->tabsInfo[tabIdx].latencyStats.inputLatency.highest = test.timeInput;
				} else if (test.timeInput < g_State->tabsInfo[tabIdx].latencyStats.inputLatency.lowest) {
					g_State->tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timeInput;
				}
			}
		}
	}

	void AddMeasurement(const std::uint32_t internalTime, const std::uint32_t externalTime,
						const std::uint32_t inputTime) {
		std::scoped_lock const lock(g_State->latencyStatsMutex);

		LatencyReading reading{externalTime, internalTime, inputTime};
		const size_t size = g_State->tabsInfo[g_State->selectedTab].latencyData.measurements.size();

		// Update the stats
		g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.average =
				((g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.average *
				 static_cast<float>(size)) /
						(size + 1.f)) +
				(static_cast<float>(internalTime) / (static_cast<float>(size) + 1.f));
		g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.average =
				((g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.average *
				 static_cast<float>(size)) /
						(static_cast<float>(size) + 1.f)) +
				(static_cast<float>(externalTime) / (static_cast<float>(size) + 1.f));
		g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.average =
				((g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.average * static_cast<float>(size)) /
						(static_cast<float>(size) + 1.f)) +
				(static_cast<float>(inputTime) / (static_cast<float>(size) + 1.f));

		g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.highest =
				internalTime > g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.highest
						? internalTime
						: g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.highest;
		g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.highest =
				externalTime > g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.highest
						? externalTime
						: g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.highest;
		g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.highest =
				inputTime > g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.highest
						? inputTime
						: g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.highest;

		g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.lowest =
				internalTime < g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.lowest
						? internalTime
						: g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.lowest;
		g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.lowest =
				externalTime < g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.lowest
						? externalTime
						: g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.lowest;
		g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.lowest =
				inputTime < g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.lowest
						? inputTime
						: g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.lowest;

		// If there are no measurements done yet set the minimum value to the current one
		if (size == 0) {
			g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.lowest = internalTime;
			g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.lowest = externalTime;
			g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.lowest = inputTime;
		}

		g_State->wasMeasurementAddedGUI = true;
		reading.index = static_cast<unsigned int>(size);
		g_State->tabsInfo[g_State->selectedTab].latencyData.measurements.push_back(reading);
		g_State->tabsInfo[g_State->selectedTab].isSaved = false;
	}

	// This code is very much unoptimized, but it has low optimization priority just because of how rarely this function
	// is called
	void RemoveMeasurement(const size_t index) {
		std::scoped_lock const lock(g_State->latencyStatsMutex);
		const auto result = g_State->tabsInfo[g_State->selectedTab].latencyData.measurements[index];

		// Fix stats
		if (const size_t size = g_State->tabsInfo[g_State->selectedTab].latencyData.measurements.size(); size > 1) {
			auto [externalLatency, internalLatency, inputLatency] =
					g_State->tabsInfo[g_State->selectedTab].latencyStats;
			externalLatency.average = ((g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.average *
										static_cast<float>(size)) -
									   static_cast<float>(result.timeExternal)) /
									  (static_cast<float>(size) - 1);
			internalLatency.average = ((g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.average *
										static_cast<float>(size)) -
									   static_cast<float>(result.timeInternal)) /
									  (static_cast<float>(size) - 1);
			inputLatency.average = ((g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.average *
									 static_cast<float>(size)) -
									static_cast<float>(result.timeInput)) /
								   (static_cast<float>(size) - 1);

			g_State->tabsInfo[g_State->selectedTab].latencyStats = LatencyStats();
			ZeroMemory(&g_State->tabsInfo[g_State->selectedTab].latencyStats,
					   sizeof(g_State->tabsInfo[g_State->selectedTab].latencyStats));
			g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.average = externalLatency.average;
			g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.average = internalLatency.average;
			g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.average = inputLatency.average;

			// In most cases a measurement won't be an edge case so It's better to check if it is and only then try to
			// find a new edge value
			// if(result.timeExternal == g_state->tabsInfo[g_state->selectedTab].latencyStats.externalLatency.highest ||
			// result.timeInternal == g_state->tabsInfo[g_state->selectedTab].latencyStats.internalLatency.highest ||
			// result.timeInput == g_state->tabsInfo[g_state->selectedTab].latencyStats.inputLatency.highest ||
			//	result.timeExternal == g_state->tabsInfo[g_state->selectedTab].latencyStats.externalLatency.lowest ||
			// result.timeInternal == g_state->tabsInfo[g_state->selectedTab].latencyStats.internalLatency.lowest ||
			// result.timeInput == g_state->tabsInfo[g_state->selectedTab].latencyStats.inputLatency.lowest)
			for (size_t i = 0; i < size; i++) {
				if (index == i)
					continue;

				auto &[timeExternal, timeInternal, timeInput, test_idx] =
						g_State->tabsInfo[g_State->selectedTab].latencyData.measurements[i];

				if (i == 0 || (index == 0 && i == 1)) {
					g_State->tabsInfo[g_State->selectedTab].latencyStats.externalLatency.lowest = timeExternal;
					g_State->tabsInfo[g_State->selectedTab].latencyStats.internalLatency.lowest = timeInternal;
					g_State->tabsInfo[g_State->selectedTab].latencyStats.inputLatency.lowest = timeInput;
				}

				if (i > index)
					test_idx -= 1;
			}

			g_State->tabsInfo[g_State->selectedTab].latencyData.measurements.erase(
					g_State->tabsInfo[g_State->selectedTab].latencyData.measurements.begin() + static_cast<int>(index));
			RecalculateStats();
		} else {
			g_State->tabsInfo[g_State->selectedTab].latencyStats = LatencyStats();
			g_State->tabsInfo[g_State->selectedTab].latencyData.measurements.clear();
			g_State->tabsInfo[g_State->selectedTab].isSaved = true;
		}
	}

	bool OpenSaveDialogue(const char *defaultName, const std::vector<std::string> &extensions, char *outFilePath,
						  const size_t bufferSize, const char *title = "Save As", void *parentWindow = nullptr) {
#ifdef _WIN32
		// Save window state
		const bool wasFullscreen = g_State->isFullscreen;
		if (g_State->isFullscreen) {
			Gui::SetFullScreenState(0);
			g_State->isFullscreen = false;
			g_State->fullscreenModeOpenPopup = false;
			g_State->fullscreenModeClosePopup = false;
		}

		char filenameBuffer[MAX_PATH] = {0};
		strncpy_s(filenameBuffer, defaultName, MAX_PATH - 1);

		// Build filter string: "EXT1\0*.ext1\EXT2\0*.ext2\0\0"
		std::vector<char> filter;
		for (const auto &ext: extensions) {
			std::string description = ext;
			for (auto &c: description)
				c = static_cast<char>(toupper(c)); // Uppercase description

			filter.insert(filter.end(), description.begin(), description.end());
			filter.push_back('\0');
			std::string pattern = "*." + ext;
			filter.insert(filter.end(), pattern.begin(), pattern.end());
			filter.push_back('\0');
		}
		filter.push_back('\0');

		OPENFILENAMEA ofn = {0};
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = static_cast<HWND>(parentWindow);
		ofn.lpstrFile = filenameBuffer;
		ofn.nMaxFile = MAX_PATH;
		ofn.lpstrFilter = filter.data();
		ofn.lpstrTitle = title;
		ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;

		if (!extensions.empty()) {
			ofn.lpstrDefExt = extensions[0].c_str();
		}

		const bool res = GetSaveFileNameA(&ofn);
		if (res) {
			strncpy_s(outFilePath, bufferSize, filenameBuffer, _TRUNCATE);
		}

		if (wasFullscreen) {
			Gui::SetFullScreenState(true);
			g_State->isFullscreen = true;
		}

		return res;

#else
		std::string command =
				std::string("zenity --save --file-selection --title=") + title + "\"Save As\" --filename=\"";
		command += defaultName;
		command += "\"";

		for (const auto &ext: extensions) {
			command += " --file-filter=\"*.";
			command += ext;
			command += "\"";
		}

		FILE *f = popen(command.c_str(), "r");
		if (!f)
			return false;

		char tempBuffer[1024];
		if (fgets(tempBuffer, sizeof(tempBuffer), f) == nullptr) {
			pclose(f);
			return false;
		}
		pclose(f);

		// Remove trailing newline
		size_t len = strlen(tempBuffer);
		if (len > 0 && tempBuffer[len - 1] == '\n') {
			tempBuffer[len - 1] = 0;
			len--;
		}

		std::string filename = tempBuffer;

		// Add default extension if missing
		if (!extensions.empty() && !filename.empty()) {
			const std::string ext = "." + extensions[0];
			if (filename.size() < ext.size() || filename.substr(filename.size() - ext.size()) != ext) {
				filename += ext;
			}
		}

		// Copy to output buffer
		strncpy(outFilePath, filename.c_str(), bufferSize);
		outFilePath[bufferSize - 1] = '\0';
		return true;
#endif
	}

	bool SaveMeasurements() {
		// if (g_state->tabsInfo[g_state->selectedTab].latencyData.measurements.size() < 1)
		//	return false;

		const std::scoped_lock lock(g_State->latencyStatsMutex);

		if (std::strlen(g_State->tabsInfo[g_State->selectedTab].savePath) == 0)
			return SaveAsMeasurements();

		HelperJson::SaveLatencyTests(g_State->tabsInfo[g_State->selectedTab],
									 g_State->tabsInfo[g_State->selectedTab].savePath);
		g_State->tabsInfo[g_State->selectedTab].isSaved = true;

		return true;
	}

	// Locking `g_state->latencyStatsMutex` should be done by the caller
	bool SaveAsMeasurements() {
		// if (g_state->tabsInfo[g_state->selectedTab].latencyData.measurements.size() < 1)
		//	return false;

		std::array<char, 260> filePath {' '};

#ifdef _WIN32
		if (OpenSaveDialogue("name", {"json"}, filePath.data(), sizeof(filePath), "Save As", Gui::hwnd)) {
			strncpy_s(g_State->tabsInfo[g_State->selectedTab].savePathPack, filePath.data(), MAX_PATH);
			SavePackMeasurements();
		}
#else
		if (OpenSaveDialogue("name", {"json"}, filePath.data(), sizeof(filePath), "Save As")) {
			strncpy(g_State->tabsInfo[g_State->selectedTab].savePathPack, filePath.data(), MAX_PATH);
			SavePackMeasurements();
		}
#endif

		return g_State->tabsInfo[g_State->selectedTab].isSaved;
	}

	bool SavePackMeasurements() {
		const std::scoped_lock lock(g_State->latencyStatsMutex);

		char *packSavePath = nullptr;
		for (auto &tab: g_State->tabsInfo) {
			if ((packSavePath != nullptr) && strcmp(packSavePath, tab.savePathPack) != 0) {
				packSavePath = nullptr;
				break;
			}
			if (strlen(tab.savePathPack) != 0)
				packSavePath = tab.savePathPack;
		}

		if (packSavePath == nullptr)
			return SavePackAsMeasurements();

		HelperJson::SaveLatencyTestsPack(g_State->tabsInfo, packSavePath);
		for (auto &tab: g_State->tabsInfo)
			tab.isSaved = true;

		return true;
	}

	// Locking `g_state->latencyStatsMutex` should be done by the caller
	bool SavePackAsMeasurements() {
		std::array<char, MAX_PATH> filePath {' '};

#ifdef _WIN32
		if (OpenSaveDialogue("MyPack", {"json"}, filePath.data(), sizeof(filePath), "Save Pack As", Gui::hwnd)) {
			for (auto &tab: g_State->tabsInfo) {
				strncpy_s(tab.savePathPack, filePath.data(), MAX_PATH);
			}
			// User selected a file - filePath contains the path
			SavePackMeasurements();
		}
#else
		if (OpenSaveDialogue("MyPack", {"json"}, filePath.data(), sizeof(filePath), "Save Pack As")) {
			for (auto &tab: g_State->tabsInfo) {
				strncpy(tab.savePathPack, filePath.data(), MAX_PATH);
			}
			// User selected a file - filePath contains the path
			SavePackMeasurements();
		}
#endif

		return g_State->tabsInfo[g_State->selectedTab].isSaved;
	}

	bool ExportCSV() {
		std::array<char, MAX_PATH> filePath {' '};

		auto res = false;
#ifdef _WIN32
		res = OpenSaveDialogue("MyData", {"csv"}, filePath.data(), sizeof(filePath), "Save CSV As", Gui::hwnd);
#else
		res = OpenSaveDialogue("MyData", {"csv"}, filePath.data(), sizeof(filePath), "Save CSV As");
#endif

		if (res) {
			std::ofstream outfile(filePath.data());
			if (outfile.bad())
				return false;

			outfile << g_State->tabsInfo[g_State->selectedTab].ExportAsCSV();
		}

		return res;
	}

	JSON_HANDLE_INFO OpenMeasurements() {
		std::scoped_lock const lock(g_State->latencyStatsMutex);
		JSON_HANDLE_INFO outRes = JSON_HANDLE_INFO_SUCCESS;
#ifdef _WIN32
		const bool wasFullscreen = g_State->isFullscreen;
		if (g_State->isFullscreen) {
			Gui::SetFullScreenState(0);
			g_State->isFullscreen = false;
			g_State->fullscreenModeOpenPopup = false;
			g_State->fullscreenModeClosePopup = false;
		}

		char filename[MAX_PATH * 10];
		constexpr char szExt[] = "json\0";

		ZeroMemory(filename, sizeof(filename));

		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));

		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = Gui::hwnd;
		ofn.lpstrFile = filename;
		ofn.nMaxFile = sizeof(filename);
		ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
		ofn.Flags = OFN_DONTADDTORECENT | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

		if (GetOpenFileName(&ofn)) {
			// ClearData();
			size_t added_tabs = 0;
			const bool isMultiSelect = !filename[ofn.nFileOffset - 1];
			if (isMultiSelect) {
				const std::string baseDir = filename;
				const char *pFileName = filename;
				pFileName += baseDir.length() + 1;
				while (*pFileName) {
					const size_t new_tabs =
							HelperJson::GetLatencyTests(g_State->tabsInfo, (baseDir + "\\" + pFileName).c_str());
					// json version mismatch
					for (size_t i = g_State->tabsInfo.size() - new_tabs; i < g_State->tabsInfo.size(); i++) {
						if (*g_State->tabsInfo[i].name == 0) {
							std::string pureFileName = pFileName;
							pureFileName = pureFileName.substr(0, pureFileName.find_last_of(".json") - 4);
							strcpy_s<TAB_NAME_MAX_SIZE>(g_State->tabsInfo[i].name, pureFileName.c_str());
						}

						// json version mismatch
						if (g_State->tabsInfo[i].saved_ver != JSON_VERSION)
							outRes |= JSON_HANDLE_INFO_BAD_VERSION;
					}
					added_tabs += new_tabs;
					pFileName += strlen(pFileName) + 1;
				}
			} else {
				added_tabs = HelperJson::GetLatencyTests(g_State->tabsInfo, filename);
				// json version mismatch
				if (g_State->tabsInfo.back().saved_ver != JSON_VERSION)
					outRes |= JSON_HANDLE_INFO_BAD_VERSION;
				strcpy_s(g_State->tabsInfo[g_State->selectedTab].savePath, filename);
			}
			g_State->selectedTab += static_cast<int>(added_tabs);

			for (size_t i = g_State->tabsInfo.size() - added_tabs; i < g_State->tabsInfo.size(); i++) {
				if (!g_State->tabsInfo[i].latencyData.measurements.empty())
					RecalculateStats(true, static_cast<int>(i));

				if (*g_State->tabsInfo[i].name == 0 && !isMultiSelect) {
					std::string pureFileName = filename;
					const size_t lastSlash = pureFileName.find_last_of('\\');
					pureFileName =
							pureFileName.substr(lastSlash + 1, pureFileName.find_last_of(".json") - lastSlash - 5);
					strcpy_s<TAB_NAME_MAX_SIZE>(g_State->tabsInfo[i].name, pureFileName.c_str());
				}
			}

			// if (!g_state->tabsInfo[g_state->selectedTab].latencyData.measurements.empty())
			//	RecalculateStats(true);
		}

#else
		std::array<char, MAX_PATH> filename {' '};
		FILE *f = popen("(zenity --multiple --separator='\t' --file-selection --title=\"Select a measurements file\" "
						"--file-filter=\"\"*.json\"\")",
						"r");
		if (auto *res = fgets(filename.data(), MAX_PATH, f)) {
			for (size_t i = 0; i < strlen(res); i++)
				res[i] = res[i] == '\t' ? '\n' : res[i]; // can't have "\n" as a
			// separator, so we set it to "\t" and then replace it with newline
			std::istringstream files(res);
			std::string line;
			size_t addedTabs = 0;
			while (std::getline(files, line)) {
				DEBUG_PRINT("Adding from file: %s\n", line.c_str());
				size_t const newTabs = HelperJson::GetLatencyTests(g_State->tabsInfo, line.c_str());

				for (size_t i = g_State->tabsInfo.size() - newTabs; i < g_State->tabsInfo.size(); i++) {
					// Copy the filename to the tab's name
					if (*g_State->tabsInfo[i].name == 0) {
						std::string pureFileName = line;
						size_t const lastSlash = pureFileName.find_last_of('/');
						pureFileName =
								pureFileName.substr(lastSlash + 1, pureFileName.find_last_of(".json") - 5 - lastSlash);
						pureFileName.copy(g_State->tabsInfo[i].name, TAB_NAME_MAX_SIZE);
					}

					// json version mismatch
					if (g_State->tabsInfo[i].saved_ver != JSON_VERSION)
						outRes |= JSON_HANDLE_INFO_BAD_VERSION;

					// Set the save path
					line.copy(newTabs == 1 ? g_State->tabsInfo[i].savePath : g_State->tabsInfo[i].savePathPack,
							  MAX_PATH);
				}
				addedTabs += newTabs;
			}
			DEBUG_PRINT("res = %s\n", res);

			for (size_t i = g_State->tabsInfo.size() - addedTabs; i < g_State->tabsInfo.size(); i++) {
				if (!g_State->tabsInfo[i].latencyData.measurements.empty())
					RecalculateStats(true, i);
			}
		}
		pclose(f);

		// glfwRequestWindowAttention(Gui::window);
		// glfwShowWindow(Gui::window);
		// glfwMaximizeWindow(Gui::window);
		// glfwRestoreWindow(Gui::window);

#endif

#ifdef _WIN32
		// Add this to preferences or smth
		if (wasFullscreen) {
			Gui::SetFullScreenState(1);
			g_State->isFullscreen = true;
		}
#endif

		return outRes;
	}

#ifdef __linux__
	void MouseClick() {
		Display *display = XOpenDisplay(nullptr);

		XEvent event;

		if (display == nullptr) {
			fprintf(stderr, "Cannot initialize the display\n");
			exit(EXIT_FAILURE);
		}

		memset(&event, 0x00, sizeof(event));

		event.type = ButtonPress;
		event.xbutton.button = Button1;
		event.xbutton.same_screen = True;

		XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root, &event.xbutton.window,
					  &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y,
					  &event.xbutton.state);

		event.xbutton.subwindow = event.xbutton.window;

		while (event.xbutton.subwindow != 0u) {
			event.xbutton.window = event.xbutton.subwindow;

			XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow,
						  &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y,
						  &event.xbutton.state);
		}

		if (XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0)
			fprintf(stderr, "Error\n");

		XFlush(display);

		// usleep(100000);

		event.type = ButtonRelease;
		event.xbutton.state = 0x100;

		if (XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0)
			fprintf(stderr, "Error\n");

		XFlush(display);

		XCloseDisplay(display);
	}
#else
	void MouseClick() {
		// static bool wasLMB_Pressed = false;
		// static bool wasMouseClickSent = false;

		INPUT Inputs[2] = {0};

		Inputs[0].type = INPUT_MOUSE;
		Inputs[0].mi.dx = 0;
		Inputs[0].mi.dy = 0;
		Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

		Inputs[1].type = INPUT_MOUSE;
		Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

		// Send input as 2 different input packets because some programs read only one flag from packets
		if (SendInput(2, Inputs, sizeof(INPUT))) {
			// wasMouseClickSent = false;
			// wasLMB_Pressed = false;
		}
	}

#endif


} // namespace AppHelper
