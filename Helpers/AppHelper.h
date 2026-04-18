#pragma once

#include <array>
#include "../App/Models.h"
#include "../External/ImGui/imgui.h"
#include "App/AppState.h"

#ifdef __linux__
#include <unistd.h>
#else
#define NOMINMAX
#include <windows.h>
#endif

#ifndef ZeroMemory
#ifdef __linux__
#define ZeroMemory(ptr, size) (memset(ptr, 0, size))
#endif
#endif

#ifdef _DEBUG
#define DEBUG_PRINT(...) printf("[%.6f] ", AppHelper::GetTimeMicros() / 1e6f); printf( __VA_ARGS__ )
#define DEBUG_ERROR(...) fprintf( stderr, "[%.6f] ", AppHelper::GetTimeMicros() / 1e6f); fprintf( stderr, __VA_ARGS__ )
#define DEBUG_ERROR_LOC(...) fprintf( stderr, "[%.6f] [%s:%d] ", AppHelper::GetTimeMicros() / 1e6f, __FILE__, __LINE__); fprintf( stderr, __VA_ARGS__ )
#else
#define DEBUG_PRINT(...) do{ } while ( 0 )
#define DEBUG_ERROR(...) do{ } while ( 0 )
#define DEBUG_ERROR_LOC(...) do{ } while ( 0 )
#endif

namespace AppHelper {
	void BindState(AppState &state);

	constexpr int STYLE_COLOR_NUM = 2;
	constexpr unsigned int COLOR_SIZE = 16;

	inline ImFont *g_boldFont;

#ifdef _WIN32
	inline LARGE_INTEGER g_StartingTime{0};
	inline constexpr std::array<const char*, 4> FONTS {"Courier Prime", "Source Sans Pro", "Franklin Gothic", "Lucida Console"};
	inline constexpr std::array<int, 4> FONT_INDEX {0, 2, 4, 5};
#else
	inline constexpr std::array<const char*, 2> FONTS {"Courier Prime", "Source Sans Pro"};
	inline constexpr std::array<int, 2> FONT_INDEX {0, 2};
#endif

	uint64_t GetTimeMicros();

	int CompareLatency(const void *a, const void *b);
	void ApplyStyle(std::array<float*, 4> &colors, std::array<float, 4>  brightnesses);
	void RevertConfig();
	void DeleteTab(int tabIdx);
	void SaveCurrentUserConfig();
	bool LoadCurrentUserConfig();
	void LoadAndHandleNewUserConfig();
	void ApplyCurrentStyle();
	void SetPlotLinesColor(ImVec4 color);
	ImFont *GetFontBold(int baseFontIndex);
	bool HasConfigChanged();
	void RecalculateStats(bool recalculate_Average = false, int tabIdx = 0);
	void AddMeasurement(std::uint32_t internalTime, std::uint32_t externalTime, std::uint32_t inputTime);
	void RemoveMeasurement(size_t index);
	bool SaveMeasurements();
	bool SaveAsMeasurements();
	bool SavePackMeasurements();
	bool SavePackAsMeasurements();
	JSON_HANDLE_INFO OpenMeasurements();
	bool ExportCSV();
	void MouseClick(); // This might not work for some applications

} // namespace AppHelper
