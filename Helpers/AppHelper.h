#pragma once

#ifndef LATENCYMETERREFRESHED_APPHELPER_H
#define LATENCYMETERREFRESHED_APPHELPER_H

#include <mutex>

#include "../External/ImGui/imgui.h"
#include "../structs.h"
#include <vector>
#include <string>

#ifdef __linux__
#include <unistd.h>
#define _min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define _max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

#else

#include <windows.h>
#define _min(a,b) min(a,b)
#define _max(a,b) max(a,b)

#endif

#ifndef ZeroMemory
#ifdef __linux__
#define ZeroMemory(ptr, size) (memset(ptr, 0, size))
#endif
#endif

inline ImGuiTableSortSpecs* g_SortSpec;

namespace AppHelper {

    inline unsigned long g_External2Micros = 1000;
    constexpr int STYLE_COLOR_NUM = 2;
    constexpr unsigned int COLOR_SIZE = 16;

    inline ImFont* g_BoldFont;

#ifdef _WIN32
    inline const char* g_Fonts[4]{ "Courier Prime", "Source Sans Pro", "Franklin Gothic", "Lucida Console" };
    inline const int g_FontIndex[4]{ 0, 2, 4, 5 };
#else
    inline const char* g_Fonts[]{ "Courier Prime", "Source Sans Pro" };
    inline constexpr int g_FontIndex[]{ 0, 2 };
#endif

    /* ---- User Data ---- */
    inline UserData g_CurrentUserData{};
    inline UserData g_BackupUserData{};

    inline std::vector<TabInfo> g_TabsInfo{};
    inline std::mutex g_LatencyStatsMutex;

    inline int g_SelectedTab = 0;

    inline bool g_IsFullscreen = false;
    inline bool g_FullscreenModeOpenPopup = false;
    inline bool g_FullscreenModeClosePopup = false;
    inline bool g_WasMeasurementAddedGUI = false;


    int CompareLatency(const void* a, const void* b);

#ifdef _WIN32
    inline LARGE_INTEGER g_StartingTime{ 0 };
#endif
    uint64_t micros();

    void ApplyStyle(float colors[STYLE_COLOR_NUM][4], float brightnesses[STYLE_COLOR_NUM]);
    void RevertConfig();
    void DeleteTab(int tab_idx);
    void SaveCurrentUserConfig();
    bool LoadCurrentUserConfig();
    void ApplyCurrentStyle();
    void SetPlotLinesColor(ImVec4 color);
    ImFont* GetFontBold(int baseFontIndex);
    bool HasConfigChanged();
    void RecalculateStats(bool recalculate_Average = false, int tabIdx = g_SelectedTab);
    void AddMeasurement(UINT internalTime, UINT externalTime, UINT inputTime);
    void RemoveMeasurement(size_t index);
    bool SaveMeasurements();
    bool SaveAsMeasurements();
    bool SavePackMeasurements();
    bool SavePackAsMeasurements();
    JSON_HANDLE_INFO OpenMeasurements();
    bool ExportCSV();
    void mouseClick(); // This might not work for some applications

} // AppHelper

#endif //LATENCYMETERREFRESHED_APPHELPER_H
