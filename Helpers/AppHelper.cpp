#include <chrono>
#include <cstring>
#include <sstream>
#include <fstream>
#include "AppHelper.h"
#include "JsonHelper.h"
#include "../gui.h"
#ifdef __linux__
#include <X11/Xlib.h>
#endif

namespace AppHelper {
    int CompareLatency(const void* a, const void* b)
    {
        LatencyReading* _a = (LatencyReading*)a;
        LatencyReading* _b = (LatencyReading*)b;

        int delta = 0;

        switch (g_SortSpec->Specs->ColumnUserID)
        {
            case 0: delta = _a->index - _b->index;					break;
            case 1: delta = _a->timeExternal - _b->timeExternal;	break;
            case 2: delta = _a->timeInternal - _b->timeInternal;	break;
            case 3: delta = _a->timeInput - _b->timeInput;			break;
            case 4: delta = 0;										break;
        }

        if (g_SortSpec->Specs->SortDirection == ImGuiSortDirection_Ascending)
        {
            return delta > 0 ? +1 : -1;
        }
        else
        {
            return delta > 0 ? -1 : +1;
        }
    }

#ifdef _WIN32

    uint64_t micros()
    {
        // QueryPerformance method is about 15-25% faster than std::chrono implementation of the same code

        LARGE_INTEGER EndingTime, ElapsedMicroseconds;
        LARGE_INTEGER Frequency;

        QueryPerformanceFrequency(&Frequency);

        // Activity to be timed

        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - g_StartingTime.QuadPart;


        //
        // We now have the elapsed number of ticks, along with the
        // number of ticks-per-second. We use these values
        // to convert to the number of elapsed microseconds.
        // To guard against loss-of-precision, we convert
        // to microseconds *before* dividing by ticks-per-second.
        //

        ElapsedMicroseconds.QuadPart *= 1000000;
        ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;

        return ElapsedMicroseconds.QuadPart;
    }

#else

    // Get a time stamp in microseconds.
    uint64_t micros()
    {
        timespec ts{};
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
        uint64_t us = ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);
        static uint64_t start_time = us;
        return us - start_time;
    }
#endif

    void ApplyStyle(float (*colors)[4], float *brightnesses) {
        auto& style = ImGui::GetStyle();
        for (int i = 0; i < STYLE_COLOR_NUM; i++)
        {
            float brightness = brightnesses[i];

            auto baseColor = ImVec4(*(ImVec4*)colors[i]);
            auto darkerBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness);
            auto darkestBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness * 2);

            // Alpha has to be set separatly, because it also gets multiplied times brightness.
            auto alphaBase = baseColor.w;
            baseColor.w = alphaBase;
            darkerBase.w = alphaBase;
            darkestBase.w = alphaBase;

            switch (i)
            {
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
            }
        }
    }

    void RevertConfig() {
        g_CurrentUserData = g_BackupUserData;

        //std::copy(accentColorBak, accentColorBak + 4, accentColor);
        //std::copy(fontColorBak, fontColorBak + 4, fontColor);

        //accentBrightness = accentBrightnessBak;
        //fontBrightness = fontBrightnessBak;

        //fontSize = fontSizeBak;
        //selectedFont = selectedFontBak;
        //g_BoldFont = boldFontBak;

        //lockGuiFps = lockGuiFpsBak;
        //guiLockedFps = guiLockedFpsBak;

        //showPlots = showPlotsBak;

        ImGuiIO& io = ImGui::GetIO();

        for (int i = 0; i < io.Fonts->Fonts.Size; i++)
        {
            io.Fonts->Fonts[i]->Scale = g_CurrentUserData.style.fontSize;
        }
        io.FontDefault = io.Fonts->Fonts[g_FontIndex[g_CurrentUserData.style.selectedFont]];
    }

    void DeleteTab(int tab_idx)
    {
        if (tab_idx < 0) return;
        std::lock_guard lock(g_LatencyStatsMutex);
        g_TabsInfo.erase(g_TabsInfo.begin() + tab_idx);
        if (g_SelectedTab >= tab_idx)
            g_SelectedTab = _max(g_SelectedTab - 1, 0);
    }

    // Does the calculations and copying for you
    void SaveCurrentUserConfig()
    {
        if (g_BackupUserData.misc.localUserData != g_CurrentUserData.misc.localUserData)
            HelperJson::UserConfigLocationChanged(g_CurrentUserData.misc.localUserData);

        g_BackupUserData = g_CurrentUserData;

        HelperJson::SaveUserData(g_CurrentUserData);
        //delete userData;
    }

    bool LoadCurrentUserConfig()
    {
        //UserData userData;
        if (!HelperJson::GetUserData(g_CurrentUserData))
            return false;

        return true;
    }

    void ApplyCurrentStyle()
    {
        auto& style = ImGui::GetStyle();

        float brightnesses[STYLE_COLOR_NUM]{ g_CurrentUserData.style.mainColorBrightness, g_CurrentUserData.style.fontColorBrightness };
        float* colors[STYLE_COLOR_NUM]{ g_CurrentUserData.style.mainColor, g_CurrentUserData.style.fontColor };

        for (int i = 0; i < STYLE_COLOR_NUM; i++)
        {
            float brightness = brightnesses[i];

            auto baseColor = ImVec4(*(ImVec4*)colors[i]);
            auto darkerBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness);
            auto darkestBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness * 2);

            // Alpha has to be set separatly, because it also gets multiplied times brightness.
            auto alphaBase = baseColor.w;
            baseColor.w = alphaBase;
            darkerBase.w = alphaBase;
            darkestBase.w = alphaBase;

            switch (i)
            {
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
            }
        }
    }

    void SetPlotLinesColor(ImVec4 color)
    {
        auto dark = color * (1 - g_CurrentUserData.style.mainColorBrightness);

        auto& style = ImGui::GetStyle();

        ImGui::PushStyleColor(ImGuiCol_PlotLines, dark);
        ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, color);
    }

    ImFont* GetFontBold(int baseFontIndex)
    {
        ImGuiIO& io = ImGui::GetIO();
        std::string fontDebugName = (std::string)io.Fonts->Fonts[g_FontIndex[baseFontIndex]]->GetDebugName();
        if (fontDebugName.find('-') == std::string::npos || fontDebugName.find(',') == std::string::npos)
            return nullptr;
        auto fontName = fontDebugName.substr(0, fontDebugName.find('-'));
        auto _boldFont = (io.Fonts->Fonts[g_FontIndex[baseFontIndex] + 1]);
        auto boldName = ((std::string)(_boldFont->GetDebugName())).substr(0, fontDebugName.find('-'));
        if (fontName == boldName)
        {
            printf("found bold font %s\n", _boldFont->GetDebugName());
            return _boldFont;
        }
        return nullptr;
    }

    // This could also be done just using a single global variable like "hasStyleChanged", because ImGui elements like FloatSlider, ColorEdit4 return a value (true) if the value has changed.
// But this method would not work as expected when user reverts back the changes or sets the variable to it's original value
    bool HasConfigChanged()
    {
        bool brightnesses = g_CurrentUserData.style.mainColorBrightness == g_BackupUserData.style.mainColorBrightness && g_CurrentUserData.style.fontColorBrightness == g_BackupUserData.style.fontColorBrightness;
        bool font = g_CurrentUserData.style.selectedFont == g_BackupUserData.style.selectedFont && g_CurrentUserData.style.fontSize == g_BackupUserData.style.fontSize;
        bool performance = g_CurrentUserData.performance.guiLockedFps == g_BackupUserData.performance.guiLockedFps &&
                           g_CurrentUserData.performance.lockGuiFps == g_BackupUserData.performance.lockGuiFps &&
                           g_CurrentUserData.performance.showPlots == g_BackupUserData.performance.showPlots &&
                           g_CurrentUserData.performance.VSync == g_BackupUserData.performance.VSync;

        bool colors{ false };
        // Compare arrays of colors column by column, because otherwise we would just compare pointers to these values which would always yield a false positive result.
        // (Pointers point to different addresses even tho the values at these addresses are the same)
        for (int column = 0; column < 4; column++)
        {
            if (g_CurrentUserData.style.mainColor[column] != g_BackupUserData.style.mainColor[column])
            {
                colors = false;
                break;
            }

            if (g_CurrentUserData.style.fontColor[column] != g_BackupUserData.style.fontColor[column])
            {
                colors = false;
                break;
            }

            if (g_CurrentUserData.style.internalPlotColor[column] != g_BackupUserData.style.internalPlotColor[column])
            {
                colors = false;
                break;
            }

            if (g_CurrentUserData.style.externalPlotColor[column] != g_BackupUserData.style.externalPlotColor[column])
            {
                colors = false;
                break;
            }

            if (g_CurrentUserData.style.inputPlotColor[column] != g_BackupUserData.style.inputPlotColor[column])
            {
                colors = false;
                break;
            }

            colors = true;
        }

        if (!colors || !brightnesses || !font || !performance)
            return true;
        else
            return false;
    }

    void RecalculateStats(bool recalculate_Average, int tabIdx)
    {
        size_t size = g_TabsInfo[tabIdx].latencyData.measurements.size();
        if (size <= 0)
            return;
        if (recalculate_Average)
        {
            g_TabsInfo[tabIdx].latencyStats.externalLatency.highest = 0;
            g_TabsInfo[tabIdx].latencyStats.internalLatency.highest = 0;
            g_TabsInfo[tabIdx].latencyStats.inputLatency.highest = 0;

            unsigned int extSum = 0, intSum = 0, inpSum = 0;

            for (size_t i = 0; i < size; i++)
            {
                auto& test = g_TabsInfo[tabIdx].latencyData.measurements[i];

                if (i == 0)
                {
                    g_TabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
                    g_TabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
                    g_TabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timeInput;
                }

                extSum += test.timeExternal;
                intSum += test.timeInternal;
                inpSum += test.timeInput;

                if (test.timeExternal > g_TabsInfo[tabIdx].latencyStats.externalLatency.highest)
                {
                    g_TabsInfo[tabIdx].latencyStats.externalLatency.highest = test.timeExternal;
                }
                else if (test.timeExternal < g_TabsInfo[tabIdx].latencyStats.externalLatency.lowest)
                {
                    g_TabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
                }

                if (test.timeInternal > g_TabsInfo[tabIdx].latencyStats.internalLatency.highest)
                {
                    g_TabsInfo[tabIdx].latencyStats.internalLatency.highest = test.timeInternal;
                }
                else if (test.timeInternal < g_TabsInfo[tabIdx].latencyStats.internalLatency.lowest)
                {
                    g_TabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
                }

                if (test.timeInput > g_TabsInfo[tabIdx].latencyStats.inputLatency.highest)
                {
                    g_TabsInfo[tabIdx].latencyStats.inputLatency.highest = test.timeInput;
                }
                else if (test.timeInput < g_TabsInfo[tabIdx].latencyStats.inputLatency.lowest)
                {
                    g_TabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timeInput;
                }
            }

            g_TabsInfo[tabIdx].latencyStats.externalLatency.average = static_cast<float>(extSum) / size;
            g_TabsInfo[tabIdx].latencyStats.internalLatency.average = static_cast<float>(intSum) / size;
            g_TabsInfo[tabIdx].latencyStats.inputLatency.average = static_cast<float>(inpSum) / size;
        }
        else
        {
            for (size_t i = 0; i < size; i++)
            {
                auto& test = g_TabsInfo[tabIdx].latencyData.measurements[i];

                if (i == 0)
                {
                    g_TabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
                    g_TabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
                    g_TabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timeInput;
                }

                if (test.timeExternal > g_TabsInfo[tabIdx].latencyStats.externalLatency.highest)
                {
                    g_TabsInfo[tabIdx].latencyStats.externalLatency.highest = test.timeExternal;
                }
                else if (test.timeExternal < g_TabsInfo[tabIdx].latencyStats.externalLatency.lowest)
                {
                    g_TabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
                }

                if (test.timeInternal > g_TabsInfo[tabIdx].latencyStats.internalLatency.highest)
                {
                    g_TabsInfo[tabIdx].latencyStats.internalLatency.highest = test.timeInternal;
                }
                else if (test.timeInternal < g_TabsInfo[tabIdx].latencyStats.internalLatency.lowest)
                {
                    g_TabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
                }

                if (test.timeInput > g_TabsInfo[tabIdx].latencyStats.inputLatency.highest)
                {
                    g_TabsInfo[tabIdx].latencyStats.inputLatency.highest = test.timeInput;
                }
                else if (test.timeInput < g_TabsInfo[tabIdx].latencyStats.inputLatency.lowest)
                {
                    g_TabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timeInput;
                }
            }
        }
    }

    void AddMeasurement(UINT internalTime, UINT externalTime, UINT inputTime)
    {
        std::lock_guard lock(g_LatencyStatsMutex);

	    LatencyReading reading{ externalTime, internalTime, inputTime };
	    size_t size = g_TabsInfo[g_SelectedTab].latencyData.measurements.size();

	    // Update the stats
	    g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.average = (g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.average * size) / (size + 1.f) + (internalTime / (size + 1.f));
	    g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.average = (g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.average * size) / (size + 1.f) + (externalTime / (size + 1.f));
	    g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.average = (g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.average * size) / (size + 1.f) + (inputTime / (size + 1.f));

	    g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.highest = internalTime > g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.highest ? internalTime : g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.highest;
	    g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.highest = externalTime > g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.highest ? externalTime : g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.highest;
	    g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.highest = inputTime > g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.highest ? inputTime : g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.highest;

	    g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.lowest = internalTime < g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.lowest ? internalTime : g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.lowest;
	    g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.lowest = externalTime < g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.lowest ? externalTime : g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.lowest;
	    g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.lowest = inputTime < g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.lowest ? inputTime : g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.lowest;

	    // If there are no measurements done yet set the minimum value to the current one
	    if (size == 0)
	    {
		    g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.lowest = internalTime;
		    g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.lowest = externalTime;
		    g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.lowest = inputTime;
	    }

        g_WasMeasurementAddedGUI = true;
	    reading.index = size;
	    g_TabsInfo[g_SelectedTab].latencyData.measurements.push_back(reading);
	    g_TabsInfo[g_SelectedTab].isSaved = false;
    }

    // This code is very much unoptimized, but it has low optimization priority just because of how rarely this function is called
    void RemoveMeasurement(size_t index)
    {
        std::lock_guard lock(g_LatencyStatsMutex);
        auto result = g_TabsInfo[g_SelectedTab].latencyData.measurements[index];
        size_t size = g_TabsInfo[g_SelectedTab].latencyData.measurements.size();
        bool findMax = false, findMin = false;

        // Fix stats
        if (size > 1)
        {
            auto _latencyStats = g_TabsInfo[g_SelectedTab].latencyStats;
            _latencyStats.externalLatency.average = ((g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.average * size) - result.timeExternal) / (size - 1);
            _latencyStats.internalLatency.average = ((g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.average * size) - result.timeInternal) / (size - 1);
            _latencyStats.inputLatency.average = ((g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.average * size) - result.timeInput) / (size - 1);

            g_TabsInfo[g_SelectedTab].latencyStats = LatencyStats();
            ZeroMemory(&g_TabsInfo[g_SelectedTab].latencyStats, sizeof(g_TabsInfo[g_SelectedTab].latencyStats));
            g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.average = _latencyStats.externalLatency.average;
            g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.average = _latencyStats.internalLatency.average;
            g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.average = _latencyStats.inputLatency.average;

            // In most cases a measurement won't be an edge case so It's better to check if it is and only then try to find a new edge value
            //if(result.timeExternal == g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.highest || result.timeInternal == g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.highest || result.timeInput == g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.highest ||
            //	result.timeExternal == g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.lowest || result.timeInternal == g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.lowest || result.timeInput == g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.lowest)
            for (size_t i = 0; i < size; i++)
            {
                if (index == i)
                    continue;

                auto& test = g_TabsInfo[g_SelectedTab].latencyData.measurements[i];

                if (i == 0 || (index == 0 && i == 1))
                {
                    g_TabsInfo[g_SelectedTab].latencyStats.externalLatency.lowest = test.timeExternal;
                    g_TabsInfo[g_SelectedTab].latencyStats.internalLatency.lowest = test.timeInternal;
                    g_TabsInfo[g_SelectedTab].latencyStats.inputLatency.lowest = test.timeInput;
                }

                if (i > index)
                    test.index -= 1;
            }

            g_TabsInfo[g_SelectedTab].latencyData.measurements.erase(g_TabsInfo[g_SelectedTab].latencyData.measurements.begin() + index);
            RecalculateStats();
        }
        else
        {
            g_TabsInfo[g_SelectedTab].latencyStats = LatencyStats();
            g_TabsInfo[g_SelectedTab].latencyData.measurements.clear();
            g_TabsInfo[g_SelectedTab].isSaved = true;
        }
    }

    bool OpenSaveDialogue(const char* defaultName, const std::vector<std::string>& extensions, char* outFilePath, size_t bufferSize, const char* title = "Save As", void* parentWindow = nullptr)
    {
#ifdef _WIN32
        // Save window state
        bool wasFullscreen = g_IsFullscreen;
        if (g_IsFullscreen)
        {
            GUI::SetFullScreenState(false);
            g_IsFullscreen = false;
            g_FullscreenModeOpenPopup = false;
            g_FullscreenModeClosePopup = false;
        }

        char filenameBuffer[MAX_PATH] = {0};
        strncpy_s(filenameBuffer, defaultName, MAX_PATH-1);

        // Build filter string: "EXT1\0*.ext1\EXT2\0*.ext2\0\0"
        std::vector<char> filter;
        for (const auto& ext : extensions) {
            std::string description = ext;
            for (auto& c : description) c = toupper(c); // Uppercase description

            filter.insert(filter.end(), description.begin(), description.end());
            filter.push_back('\0');
            std::string pattern = "*." + ext;
            filter.insert(filter.end(), pattern.begin(), pattern.end());
            filter.push_back('\0');
        }
        filter.push_back('\0');

        OPENFILENAMEA ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = (HWND)parentWindow;
        ofn.lpstrFile = filenameBuffer;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = filter.data();
        ofn.lpstrTitle = title.c_str();
        ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;

        if (!extensions.empty()) {
            ofn.lpstrDefExt = extensions[0].c_str();
        }

        bool res = GetSaveFileNameA(&ofn);
        if (res) {
            strncpy_s(outFilePath, bufferSize, filenameBuffer, _TRUNCATE);
        }

        if (wasFullscreen)
        {
            GUI::SetFullScreenState(true);
            g_IsFullscreen = true;
        }

        return res;

#else
        std::string command = std::string("zenity --save --file-selection --title=") + title + "\"Save As\" --filename=\"";
        command += defaultName;
        command += "\"";

        for (const auto& ext : extensions) {
            command += " --file-filter=\"*.";
            command += ext;
            command += "\"";
        }

        FILE* f = popen(command.c_str(), "r");
        if (!f) return false;

        char tempBuffer[1024];
        if (!fgets(tempBuffer, sizeof(tempBuffer), f)) {
            pclose(f);
            return false;
        }
        pclose(f);

        // Remove trailing newline
        size_t len = strlen(tempBuffer);
        if (len > 0 && tempBuffer[len-1] == '\n') {
            tempBuffer[len-1] = 0;
            len--;
        }

        std::string filename = tempBuffer;

        // Add default extension if missing
        if (!extensions.empty() && !filename.empty()) {
            std::string ext = "." + extensions[0];
            if (filename.size() < ext.size() ||
                filename.substr(filename.size() - ext.size()) != ext) {
                filename += ext;
            }
        }

        // Copy to output buffer
        strncpy(outFilePath, filename.c_str(), bufferSize);
        outFilePath[bufferSize-1] = '\0';
        return true;
#endif
    }

    bool SaveMeasurements()
    {
        //if (g_TabsInfo[g_SelectedTab].latencyData.measurements.size() < 1)
        //	return false;

        std::lock_guard lock(g_LatencyStatsMutex);

        if (std::strlen(g_TabsInfo[g_SelectedTab].savePath) == 0)
            return SaveAsMeasurements();

        HelperJson::SaveLatencyTests(g_TabsInfo[g_SelectedTab], g_TabsInfo[g_SelectedTab].savePath);
        g_TabsInfo[g_SelectedTab].isSaved = true;

        return true;
    }

    // Locking `g_LatencyStatsMutex` should be done by the caller
    bool SaveAsMeasurements()
    {
        //if (g_TabsInfo[g_SelectedTab].latencyData.measurements.size() < 1)
        //	return false;

        char filePath[260];

#ifdef _WIN32
        if (OpenSaveDialogue("name", {"json"}, filePath, sizeof(filePath), "Save As", GUI::hwnd)) {
            strncpy_s(g_TabsInfo[g_SelectedTab].savePathPack, filePath, MAX_PATH);
            SavePackMeasurements();
        }
#else
        if (OpenSaveDialogue("name", {"json"}, filePath, sizeof(filePath), "Save As")) {
            strncpy(g_TabsInfo[g_SelectedTab].savePathPack, filePath, MAX_PATH);
            SavePackMeasurements();
        }
#endif

        return g_TabsInfo[g_SelectedTab].isSaved;
    }

    bool SavePackMeasurements()
    {
        std::lock_guard lock(g_LatencyStatsMutex);

        char* packSavePath = nullptr;
        for (auto& tab : g_TabsInfo) {
            if (packSavePath && strcmp(packSavePath, tab.savePathPack) != 0)
            {
                packSavePath = nullptr;
                break;
            }
            if(strlen(tab.savePathPack) != 0)
                packSavePath = tab.savePathPack;
        }

        if (packSavePath == nullptr)
            return SavePackAsMeasurements();

        HelperJson::SaveLatencyTestsPack(g_TabsInfo, packSavePath);
        for(auto& tab : g_TabsInfo)
            tab.isSaved = true;

        return true;
    }

    // Locking `g_LatencyStatsMutex` should be done by the caller
    bool SavePackAsMeasurements()
    {
        char filePath[260];

#ifdef _WIN32
        if (OpenSaveDialogue("MyPack", {"json"}, filePath, sizeof(filePath), "Save Pack As", GUI::hwnd)) {
            for (auto& tab : g_TabsInfo) {
                strncpy_s(tab.savePathPack, filePath, MAX_PATH);
            }
            // User selected a file - filePath contains the path
            SavePackMeasurements();
        }
#else
        if (OpenSaveDialogue("MyPack", {"json"}, filePath, sizeof(filePath), "Save Pack As")) {
            for (auto& tab : g_TabsInfo) {
                strncpy(tab.savePathPack, filePath, MAX_PATH);
            }
            // User selected a file - filePath contains the path
            SavePackMeasurements();
        }
#endif

        return g_TabsInfo[g_SelectedTab].isSaved;
    }

    bool ExportCSV() {
        char filePath[260];

        auto res = false;
#ifdef _WIN32
        res = OpenSaveDialogue("MyData", {"csv"}, filePath, sizeof(filePath), "Save CSV As", GUI::hwnd);
#else
        res = OpenSaveDialogue("MyData", {"csv"}, filePath, sizeof(filePath), "Save CSV As");
#endif

        if (res) {
            std::ofstream outfile(filePath);
            if (outfile.bad())
                return false;

            outfile << g_TabsInfo[g_SelectedTab].ExportAsCSV();
        }

        return false;
    }

    JSON_HANDLE_INFO OpenMeasurements()
    {
        std::lock_guard lock(g_LatencyStatsMutex);
        JSON_HANDLE_INFO out_res = JSON_HANDLE_INFO_SUCCESS;
#ifdef _WIN32
        bool wasFullscreen = g_IsFullscreen;
        if (g_IsFullscreen)
        {
            GUI::SetFullScreenState(false);
            g_IsFullscreen = false;
            g_FullscreenModeOpenPopup = false;
            g_FullscreenModeClosePopup = false;
        }

        char filename[MAX_PATH*10];
        const char szExt[] = "json\0";

        ZeroMemory(filename, sizeof(filename));

        OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = GUI::hwnd;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = sizeof(filename);
        ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
        ofn.Flags = OFN_DONTADDTORECENT | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

        if (GetOpenFileName(&ofn))
        {
            //ClearData();
            size_t added_tabs = 0;
            bool isMultiSelect = !filename[ofn.nFileOffset-1];
            if (isMultiSelect) {
                std::string baseDir = filename;
                char* pFileName = filename;
                pFileName += baseDir.length() + 1;
                while (*pFileName) {
                    size_t new_tabs = HelperJson::GetLatencyTests(g_TabsInfo, (baseDir + "\\" + pFileName).c_str());
                    // json version mismatch
                    for (size_t i = g_TabsInfo.size() - new_tabs; i < g_TabsInfo.size(); i++)
                    {
                        if (*g_TabsInfo[i].name == 0)
                        {
                            std::string pureFileName = pFileName;
                            pureFileName = pureFileName.substr(0, pureFileName.find_last_of(".json") - 4);
                            strcpy_s<TAB_NAME_MAX_SIZE>(g_TabsInfo[i].name, pureFileName.c_str());
                        }

                        // json version mismatch
                        if(g_TabsInfo[i].saved_ver != JSON_VERSION)
                            out_res |= JSON_HANDLE_INFO_BAD_VERSION;
                    }
                    added_tabs += new_tabs;
                    pFileName += strlen(pFileName) + 1;
                }
            }
            else {
                added_tabs = HelperJson::GetLatencyTests(g_TabsInfo, filename);
                // json version mismatch
                if(g_TabsInfo.back().saved_ver != JSON_VERSION)
                    out_res |= JSON_HANDLE_INFO_BAD_VERSION;
                strcpy_s(g_TabsInfo[g_SelectedTab].savePath, filename);
            }
            g_SelectedTab += added_tabs;

            for (size_t i = g_TabsInfo.size() - added_tabs; i < g_TabsInfo.size(); i++)
            {
                if (!g_TabsInfo[i].latencyData.measurements.empty())
                    RecalculateStats(true, i);

                if (*g_TabsInfo[i].name == 0 && !isMultiSelect)
                {
                    std::string pureFileName = filename;
                    size_t lastSlash = pureFileName.find_last_of('\\');
                    pureFileName = pureFileName.substr(lastSlash + 1, pureFileName.find_last_of(".json") - lastSlash - 5);
                    strcpy_s<TAB_NAME_MAX_SIZE>(g_TabsInfo[i].name, pureFileName.c_str());
                }
            }

            //if (!g_TabsInfo[g_SelectedTab].latencyData.measurements.empty())
            //	RecalculateStats(true);
        }

#else
        char filename[MAX_PATH];
        FILE *f = popen("(zenity --multiple --separator='\t' --file-selection --title=\"Select a measurements file\" --file-filter=\"\"*.json\"\")", "r");
        auto res = fgets(filename, MAX_PATH, f);
        if(res) {
            for (int i = 0; i < strlen(res); i++) res[i] = res[i] == '\t' ? '\n' : res[i]; // can't have "\n" as a
            // separator, so we set it to "\t" and then replace it with newline
            std::istringstream files(res);
            std::string line;
            size_t added_tabs = 0;
            while (std::getline(files, line)) {
                printf("Adding from file: %s\n", line.c_str());
                size_t new_tabs = HelperJson::GetLatencyTests(g_TabsInfo, line.c_str());

                for (size_t i = g_TabsInfo.size() - new_tabs; i < g_TabsInfo.size(); i++) {
                    // Copy the filename to the tab's name
                    if (*g_TabsInfo[i].name == 0) {
                        std::string pureFileName = line;
                        size_t lastSlash = pureFileName.find_last_of('/');
                        pureFileName = pureFileName.substr(lastSlash + 1,
                                                           pureFileName.find_last_of(".json") - 5 - lastSlash);
                        pureFileName.copy(g_TabsInfo[i].name, TAB_NAME_MAX_SIZE);
                    }

                    // json version mismatch
                    if(g_TabsInfo[i].saved_ver != JSON_VERSION)
                            out_res |= JSON_HANDLE_INFO_BAD_VERSION;

                    // Set the save path
                    line.copy(new_tabs == 1 ? g_TabsInfo[i].savePath : g_TabsInfo[i].savePathPack, MAX_PATH);
                }
                added_tabs += new_tabs;
            }
            printf("res = %s\n", res);

            for (size_t i = g_TabsInfo.size() - added_tabs; i < g_TabsInfo.size(); i++) {
                if (!g_TabsInfo[i].latencyData.measurements.empty())
                    RecalculateStats(true, i);
            }
        }
        pclose(f);

        //glfwRequestWindowAttention(GUI::window);
        //glfwShowWindow(GUI::window);
        //glfwMaximizeWindow(GUI::window);
        //glfwRestoreWindow(GUI::window);

#endif

#ifdef _WIN32
        // Add this to preferences or smth
        if (wasFullscreen)
        {
            GUI::SetFullScreenState(true);
            g_IsFullscreen = true;
        }
#endif

        return out_res;
    }

#ifdef __linux__
    void mouseClick()
    {
        Display *display = XOpenDisplay(NULL);

        XEvent event;

        if(display == NULL)
        {
            fprintf(stderr, "Cannot initialize the display\n");
            exit(EXIT_FAILURE);
        }

        memset(&event, 0x00, sizeof(event));

        event.type = ButtonPress;
        event.xbutton.button = Button1;
        event.xbutton.same_screen = True;

        XQueryPointer(display, RootWindow(display, DefaultScreen(display)), &event.xbutton.root, &event.xbutton.window, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);

        event.xbutton.subwindow = event.xbutton.window;

        while(event.xbutton.subwindow)
        {
            event.xbutton.window = event.xbutton.subwindow;

            XQueryPointer(display, event.xbutton.window, &event.xbutton.root, &event.xbutton.subwindow, &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y, &event.xbutton.state);
        }

        if(XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0) fprintf(stderr, "Error\n");

        XFlush(display);

        //usleep(100000);

        event.type = ButtonRelease;
        event.xbutton.state = 0x100;

        if(XSendEvent(display, PointerWindow, True, 0xfff, &event) == 0) fprintf(stderr, "Error\n");

        XFlush(display);

        XCloseDisplay(display);
    }
#else
    void mouseClick(){
        //static bool wasLMB_Pressed = false;
        //static bool wasMouseClickSent = false;

        INPUT Inputs[2] = { 0 };

        Inputs[0].type = INPUT_MOUSE;
        Inputs[0].mi.dx = 0;
        Inputs[0].mi.dy = 0;
        Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

        Inputs[1].type = INPUT_MOUSE;
        Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

        // Send input as 2 different input packets because some programs read only one flag from packets
        if (SendInput(2, Inputs, sizeof(INPUT)))
        {
            //wasMouseClickSent = false;
            //wasLMB_Pressed = false;
        }
    }

#endif


} // AppHelper