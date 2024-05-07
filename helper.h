#pragma once

#ifndef LATENCYMETERREFRESHED_HELPER_H
#define LATENCYMETERREFRESHED_HELPER_H

#include "External/ImGui/imgui.h"
#include "structs.h"
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

static ImGuiTableSortSpecs* sortSpec;

namespace helper {

    static unsigned long External2Micros = 1000;

    // idk why I didn't just expose it in GUI.h...
#ifdef _WIN32
    //HWND hwnd; // I have exposed it in GUI namespace
#endif

    /* ---- User Data ----  (Don't ask me why I didn't create structures for these things earlier) */
    inline const int styleColorNum = 2;
    inline const unsigned int colorSize = 16;

    inline ImFont* boldFont;

#ifdef _WIN32
    inline const char* fonts[4]{ "Courier Prime", "Source Sans Pro", "Franklin Gothic", "Lucida Console" };
    inline const int fontIndex[4]{ 0, 2, 4, 5 };
#else
    inline const char* fonts[]{ "Courier Prime", "Source Sans Pro" };
    inline const int fontIndex[]{ 0, 2};
#endif

    /* ---- User Data ---- */
    inline UserData currentUserData{};
    inline UserData backupUserData{};

    inline std::vector<TabInfo> tabsInfo{};

    inline int selectedTab = 0;

    inline bool isFullscreen = false;
    inline bool fullscreenModeOpenPopup = false;
    inline bool fullscreenModeClosePopup = false;


    int LatencyCompare(const void* a, const void* b);

#ifdef _WIN32
    inline LARGE_INTEGER StartingTime{ 0 };
#endif
    uint64_t micros();

    void ApplyStyle(float colors[styleColorNum][4], float brightnesses[styleColorNum]);
    void RevertConfig();
    void SaveCurrentUserConfig();
    bool LoadCurrentUserConfig();
    void ApplyCurrentStyle();
    void SetPlotLinesColor(ImVec4 color);
    ImFont* GetFontBold(int baseFontIndex);
    bool HasConfigChanged();
    void RecalculateStats(bool recalculate_Average = false, int tabIdx = selectedTab);
    void RemoveMeasurement(size_t index);
    bool SaveMeasurements();
    bool SaveAsMeasurements();
    bool SavePackMeasurements();
    bool SavePackAsMeasurements();
    JSON_HANDLE_INFO OpenMeasurements();

} // helper

#endif //LATENCYMETERREFRESHED_HELPER_H
