#include <chrono>
#include <cstring>
#include <sstream>
#include "helper.h"
#include "helperJson.h"
#include "gui.h"
#ifdef __linux__
#include <X11/Xlib.h>
#endif

namespace helper {
    int LatencyCompare(const void* a, const void* b)
    {
        LatencyReading* _a = (LatencyReading*)a;
        LatencyReading* _b = (LatencyReading*)b;

        int delta = 0;

        switch (sortSpec->Specs->ColumnUserID)
        {
            case 0: delta = _a->index - _b->index;					break;
            case 1: delta = _a->timeExternal - _b->timeExternal;	break;
            case 2: delta = _a->timeInternal - _b->timeInternal;	break;
            case 3: delta = _a->timePing - _b->timePing;			break;
            case 4: delta = 0;										break;
        }

        if (sortSpec->Specs->SortDirection == ImGuiSortDirection_Ascending)
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
        //auto timeNow = std::chrono::high_resolution_clock::now();
        //uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(timeNow.time_since_epoch()).count();
        //static const uint64_t start{ us };
        //return us - start;

        //FILETIME ft;
        //GetSystemTimeAsFileTime(&ft);
        //ULARGE_INTEGER li;
        //li.LowPart = ft.dwLowDateTime;
        //li.HighPart = ft.dwHighDateTime;
        //unsigned long long valueAsHns = li.QuadPart;
        //unsigned long long valueAsUs = valueAsHns / 10;

        //return valueAsUs;


        // QueryPerformance method is about 15-25% faster than std::chrono implementation of the same code

        LARGE_INTEGER EndingTime, ElapsedMicroseconds;
        LARGE_INTEGER Frequency;

        QueryPerformanceFrequency(&Frequency);

        // Activity to be timed

        QueryPerformanceCounter(&EndingTime);
        ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;


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
        for (int i = 0; i < styleColorNum; i++)
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
        currentUserData = backupUserData;

        //std::copy(accentColorBak, accentColorBak + 4, accentColor);
        //std::copy(fontColorBak, fontColorBak + 4, fontColor);

        //accentBrightness = accentBrightnessBak;
        //fontBrightness = fontBrightnessBak;

        //fontSize = fontSizeBak;
        //selectedFont = selectedFontBak;
        //boldFont = boldFontBak;

        //lockGuiFps = lockGuiFpsBak;
        //guiLockedFps = guiLockedFpsBak;

        //showPlots = showPlotsBak;

        ImGuiIO& io = ImGui::GetIO();

        for (int i = 0; i < io.Fonts->Fonts.Size; i++)
        {
            io.Fonts->Fonts[i]->Scale = currentUserData.style.fontSize;
        }
        io.FontDefault = io.Fonts->Fonts[fontIndex[currentUserData.style.selectedFont]];
    }

    // Does the calculations and copying for you
    void SaveCurrentUserConfig()
    {
        if (backupUserData.misc.localUserData != currentUserData.misc.localUserData)
            HelperJson::UserConfigLocationChanged(currentUserData.misc.localUserData);

        backupUserData = currentUserData;

        HelperJson::SaveUserData(currentUserData);
        //delete userData;
    }

    bool LoadCurrentUserConfig()
    {
        //UserData userData;
        if (!HelperJson::GetUserData(currentUserData))
            return false;

        return true;
    }

    void ApplyCurrentStyle()
    {
        auto& style = ImGui::GetStyle();

        float brightnesses[styleColorNum]{ currentUserData.style.mainColorBrightness, currentUserData.style.fontColorBrightness };
        float* colors[styleColorNum]{ currentUserData.style.mainColor, currentUserData.style.fontColor };

        for (int i = 0; i < styleColorNum; i++)
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
        auto dark = color * (1 - currentUserData.style.mainColorBrightness);

        auto& style = ImGui::GetStyle();

        ImGui::PushStyleColor(ImGuiCol_PlotLines, dark);
        ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, color);
    }

    ImFont* GetFontBold(int baseFontIndex)
    {
        ImGuiIO& io = ImGui::GetIO();
        std::string fontDebugName = (std::string)io.Fonts->Fonts[fontIndex[baseFontIndex]]->GetDebugName();
        if (fontDebugName.find('-') == std::string::npos || fontDebugName.find(',') == std::string::npos)
            return nullptr;
        auto fontName = fontDebugName.substr(0, fontDebugName.find('-'));
        auto _boldFont = (io.Fonts->Fonts[fontIndex[baseFontIndex] + 1]);
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
        bool brightnesses = currentUserData.style.mainColorBrightness == backupUserData.style.mainColorBrightness && currentUserData.style.fontColorBrightness == backupUserData.style.fontColorBrightness;
        bool font = currentUserData.style.selectedFont == backupUserData.style.selectedFont && currentUserData.style.fontSize == backupUserData.style.fontSize;
        bool performance = currentUserData.performance.guiLockedFps == backupUserData.performance.guiLockedFps &&
                           currentUserData.performance.lockGuiFps == backupUserData.performance.lockGuiFps &&
                           currentUserData.performance.showPlots == backupUserData.performance.showPlots &&
                           currentUserData.performance.VSync == backupUserData.performance.VSync;

        bool colors{ false };
        // Compare arrays of colors column by column, because otherwise we would just compare pointers to these values which would always yield a false positive result.
        // (Pointers point to different addresses even tho the values at these addresses are the same)
        for (int column = 0; column < 4; column++)
        {
            if (currentUserData.style.mainColor[column] != backupUserData.style.mainColor[column])
            {
                colors = false;
                break;
            }

            if (currentUserData.style.fontColor[column] != backupUserData.style.fontColor[column])
            {
                colors = false;
                break;
            }

            if (currentUserData.style.internalPlotColor[column] != backupUserData.style.internalPlotColor[column])
            {
                colors = false;
                break;
            }

            if (currentUserData.style.externalPlotColor[column] != backupUserData.style.externalPlotColor[column])
            {
                colors = false;
                break;
            }

            if (currentUserData.style.inputPlotColor[column] != backupUserData.style.inputPlotColor[column])
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
        size_t size = tabsInfo[tabIdx].latencyData.measurements.size();
        if (size <= 0)
            return;
        if (recalculate_Average)
        {
            tabsInfo[tabIdx].latencyStats.externalLatency.highest = 0;
            tabsInfo[tabIdx].latencyStats.internalLatency.highest = 0;
            tabsInfo[tabIdx].latencyStats.inputLatency.highest = 0;

            unsigned int extSum = 0, intSum = 0, inpSum = 0;

            for (size_t i = 0; i < size; i++)
            {
                auto& test = tabsInfo[tabIdx].latencyData.measurements[i];

                if (i == 0)
                {
                    tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
                    tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
                    tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timePing;
                }

                extSum += test.timeExternal;
                intSum += test.timeInternal;
                inpSum += test.timePing;

                if (test.timeExternal > tabsInfo[tabIdx].latencyStats.externalLatency.highest)
                {
                    tabsInfo[tabIdx].latencyStats.externalLatency.highest = test.timeExternal;
                }
                else if (test.timeExternal < tabsInfo[tabIdx].latencyStats.externalLatency.lowest)
                {
                    tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
                }

                if (test.timeInternal > tabsInfo[tabIdx].latencyStats.internalLatency.highest)
                {
                    tabsInfo[tabIdx].latencyStats.internalLatency.highest = test.timeInternal;
                }
                else if (test.timeInternal < tabsInfo[tabIdx].latencyStats.internalLatency.lowest)
                {
                    tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
                }

                if (test.timePing > tabsInfo[tabIdx].latencyStats.inputLatency.highest)
                {
                    tabsInfo[tabIdx].latencyStats.inputLatency.highest = test.timePing;
                }
                else if (test.timePing < tabsInfo[tabIdx].latencyStats.inputLatency.lowest)
                {
                    tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timePing;
                }
            }

            tabsInfo[tabIdx].latencyStats.externalLatency.average = static_cast<float>(extSum) / size;
            tabsInfo[tabIdx].latencyStats.internalLatency.average = static_cast<float>(intSum) / size;
            tabsInfo[tabIdx].latencyStats.inputLatency.average = static_cast<float>(inpSum) / size;
        }
        else
        {
            for (size_t i = 0; i < size; i++)
            {
                auto& test = tabsInfo[tabIdx].latencyData.measurements[i];

                if (i == 0)
                {
                    tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
                    tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
                    tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timePing;
                }

                if (test.timeExternal > tabsInfo[tabIdx].latencyStats.externalLatency.highest)
                {
                    tabsInfo[tabIdx].latencyStats.externalLatency.highest = test.timeExternal;
                }
                else if (test.timeExternal < tabsInfo[tabIdx].latencyStats.externalLatency.lowest)
                {
                    tabsInfo[tabIdx].latencyStats.externalLatency.lowest = test.timeExternal;
                }

                if (test.timeInternal > tabsInfo[tabIdx].latencyStats.internalLatency.highest)
                {
                    tabsInfo[tabIdx].latencyStats.internalLatency.highest = test.timeInternal;
                }
                else if (test.timeInternal < tabsInfo[tabIdx].latencyStats.internalLatency.lowest)
                {
                    tabsInfo[tabIdx].latencyStats.internalLatency.lowest = test.timeInternal;
                }

                if (test.timePing > tabsInfo[tabIdx].latencyStats.inputLatency.highest)
                {
                    tabsInfo[tabIdx].latencyStats.inputLatency.highest = test.timePing;
                }
                else if (test.timePing < tabsInfo[tabIdx].latencyStats.inputLatency.lowest)
                {
                    tabsInfo[tabIdx].latencyStats.inputLatency.lowest = test.timePing;
                }
            }
        }
    }

    // This code is very much unoptimized, but it has low optimization priority just because of how rarely this function is called
    void RemoveMeasurement(size_t index)
    {
        auto result = tabsInfo[selectedTab].latencyData.measurements[index];
        size_t size = tabsInfo[selectedTab].latencyData.measurements.size();
        bool findMax = false, findMin = false;

        // Fix stats
        if (size > 1)
        {
            auto _latencyStats = tabsInfo[selectedTab].latencyStats;
            _latencyStats.externalLatency.average = ((tabsInfo[selectedTab].latencyStats.externalLatency.average * size) - result.timeExternal) / (size - 1);
            _latencyStats.internalLatency.average = ((tabsInfo[selectedTab].latencyStats.internalLatency.average * size) - result.timeInternal) / (size - 1);
            _latencyStats.inputLatency.average = ((tabsInfo[selectedTab].latencyStats.inputLatency.average * size) - result.timePing) / (size - 1);

            tabsInfo[selectedTab].latencyStats = LatencyStats();
            ZeroMemory(&tabsInfo[selectedTab].latencyStats, sizeof(tabsInfo[selectedTab].latencyStats));
            tabsInfo[selectedTab].latencyStats.externalLatency.average = _latencyStats.externalLatency.average;
            tabsInfo[selectedTab].latencyStats.internalLatency.average = _latencyStats.internalLatency.average;
            tabsInfo[selectedTab].latencyStats.inputLatency.average = _latencyStats.inputLatency.average;

            // In most cases a measurement won't be an edge case so It's better to check if it is and only then try to find a new edge value
            //if(result.timeExternal == tabsInfo[selectedTab].latencyStats.externalLatency.highest || result.timeInternal == tabsInfo[selectedTab].latencyStats.internalLatency.highest || result.timePing == tabsInfo[selectedTab].latencyStats.inputLatency.highest ||
            //	result.timeExternal == tabsInfo[selectedTab].latencyStats.externalLatency.lowest || result.timeInternal == tabsInfo[selectedTab].latencyStats.internalLatency.lowest || result.timePing == tabsInfo[selectedTab].latencyStats.inputLatency.lowest)
            for (size_t i = 0; i < size; i++)
            {
                if (index == i)
                    continue;

                auto& test = tabsInfo[selectedTab].latencyData.measurements[i];

                if (i == 0 || (index == 0 && i == 1))
                {
                    tabsInfo[selectedTab].latencyStats.externalLatency.lowest = test.timeExternal;
                    tabsInfo[selectedTab].latencyStats.internalLatency.lowest = test.timeInternal;
                    tabsInfo[selectedTab].latencyStats.inputLatency.lowest = test.timePing;
                }

                if (i > index)
                    test.index -= 1;
            }

            tabsInfo[selectedTab].latencyData.measurements.erase(tabsInfo[selectedTab].latencyData.measurements.begin() + index);
            RecalculateStats();
        }
        else
        {
            tabsInfo[selectedTab].latencyStats = LatencyStats();
            tabsInfo[selectedTab].latencyData.measurements.clear();
            tabsInfo[selectedTab].isSaved = true;
        }
    }

    bool SaveMeasurements()
    {
        //if (tabsInfo[selectedTab].latencyData.measurements.size() < 1)
        //	return false;

        if (std::strlen(tabsInfo[selectedTab].savePath) == 0)
            return SaveAsMeasurements();

        HelperJson::SaveLatencyTests(tabsInfo[selectedTab], tabsInfo[selectedTab].savePath);
        tabsInfo[selectedTab].isSaved = true;

        return true;
    }

    bool SaveAsMeasurements()
    {
        //if (tabsInfo[selectedTab].latencyData.measurements.size() < 1)
        //	return false;

#ifdef _WIN32
        bool wasFullscreen = isFullscreen;
        if (isFullscreen)
        {
            GUI::SetFullScreenState(false);
            isFullscreen = false;
            fullscreenModeOpenPopup = false;
            fullscreenModeClosePopup = false;
        }


        char filename[MAX_PATH]{ "name" };
        memcpy(filename, tabsInfo[selectedTab].name, TAB_NAME_MAX_SIZE);
        const char szExt[] = "Json\0*.json\0\0";

        OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GUI::hwnd;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
        ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;

        if (GetSaveFileName(&ofn))
        {
            strncpy_s(tabsInfo[selectedTab].savePath, filename, MAX_PATH);

            SaveMeasurements();
        }
        //else
        //	tabsInfo[selectedTab].isSaved = false;
#else

        char filename[MAX_PATH];
        FILE *f = popen(R"(zenity --save --file-selection --title="Save to" --file-filter=""*.json"")", "r");
        auto res = fgets(filename, MAX_PATH, f);
        if(res) {
            res[strlen(res)-1] = 0; // Get rid of the new line character from the end. idk why its even there...
            std::string file(res);
            if(file.find(".json") != file.size() - 5) {
                file += ".json";
            }
            file.copy(tabsInfo[selectedTab].savePath, MAX_PATH);
            SaveMeasurements();
        }
        pclose(f);


#endif

#ifdef _WIN32
        if (wasFullscreen)
        {
            GUI::SetFullScreenState(true);
            isFullscreen = true;
        }
#endif

        return tabsInfo[selectedTab].isSaved;
    }

    bool SavePackMeasurements()
    {
        char* packSavePath = nullptr;
        for (auto& tab : tabsInfo) {
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

        HelperJson::SaveLatencyTestsPack(tabsInfo, packSavePath);
        for(auto& tab : tabsInfo)
            tab.isSaved = true;

        return true;
    }

    bool SavePackAsMeasurements()
    {
#ifdef _WIN32
        bool wasFullscreen = isFullscreen;
        if (isFullscreen)
        {
            GUI::SetFullScreenState(false);
            isFullscreen = false;
            fullscreenModeOpenPopup = false;
            fullscreenModeClosePopup = false;
        }

        char filename[MAX_PATH]{ "PackName" };
        const char szExt[] = "Json\0*.json\0\0";

        OPENFILENAME ofn;
        ZeroMemory(&ofn, sizeof(ofn));

        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = GUI::hwnd;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFilter = ofn.lpstrDefExt = szExt;
        ofn.Flags = OFN_DONTADDTORECENT | OFN_OVERWRITEPROMPT;
        ofn.lpstrTitle = "Save Pack As";

        if (GetSaveFileName(&ofn))
        {
            for (auto& tab : tabsInfo) {
                strncpy_s(tab.savePathPack, filename, MAX_PATH);
            }

            SavePackMeasurements();
        }
        //else
        //	tabsInfo[selectedTab].isSaved = false;

#else

        char filename[MAX_PATH];
        FILE *f = popen(R"(zenity --save --file-selection --title="Save to" --file-filter=""*.json"")", "r");
        auto res = fgets(filename, MAX_PATH, f);
        if(res) {
            res[strlen(res)-1] = 0; // Get rid of the new line character from the end. idk why its even there...
            std::string file(res);
            if(file.find(".json") != file.size() - 5) {
                file += ".json";
            }
            for (auto& tab : tabsInfo) {
                file.copy(tab.savePathPack, MAX_PATH);
            }
            SavePackMeasurements();
        }
        pclose(f);

#endif

#ifdef _WIN32
        if (wasFullscreen)
        {
            GUI::SetFullScreenState(true);
            isFullscreen = true;
        }
#endif

        return tabsInfo[selectedTab].isSaved;
    }

    JSON_HANDLE_INFO OpenMeasurements()
    {
        JSON_HANDLE_INFO out_res = JSON_HANDLE_INFO_SUCCESS;
#ifdef _WIN32
        bool wasFullscreen = isFullscreen;
        if (isFullscreen)
        {
            GUI::SetFullScreenState(false);
            isFullscreen = false;
            fullscreenModeOpenPopup = false;
            fullscreenModeClosePopup = false;
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
                    size_t new_tabs = HelperJson::GetLatencyTests(tabsInfo, (baseDir + "\\" + pFileName).c_str());
                    // json version mismatch
                    for (size_t i = tabsInfo.size() - new_tabs; i < tabsInfo.size(); i++)
                    {
                        if (*tabsInfo[i].name == 0)
                        {
                            std::string pureFileName = pFileName;
                            pureFileName = pureFileName.substr(0, pureFileName.find_last_of(".json") - 4);
                            strcpy_s<TAB_NAME_MAX_SIZE>(tabsInfo[i].name, pureFileName.c_str());
                        }

                        // json version mismatch
                        if(tabsInfo[i].saved_ver != json_version)
                            out_res |= JSON_HANDLE_INFO_BAD_VERSION;
                    }
                    added_tabs += new_tabs;
                    pFileName += strlen(pFileName) + 1;
                }
            }
            else {
                added_tabs = HelperJson::GetLatencyTests(tabsInfo, filename);
                // json version mismatch
                if(tabsInfo.back().saved_ver != json_version)
                    out_res |= JSON_HANDLE_INFO_BAD_VERSION;
                strcpy_s(tabsInfo[selectedTab].savePath, filename);
            }
            selectedTab += added_tabs;

            for (size_t i = tabsInfo.size() - added_tabs; i < tabsInfo.size(); i++)
            {
                if (!tabsInfo[i].latencyData.measurements.empty())
                    RecalculateStats(true, i);

                if (*tabsInfo[i].name == 0 && !isMultiSelect)
                {
                    std::string pureFileName = filename;
                    size_t lastSlash = pureFileName.find_last_of('\\');
                    pureFileName = pureFileName.substr(lastSlash + 1, pureFileName.find_last_of(".json") - lastSlash - 5);
                    strcpy_s<TAB_NAME_MAX_SIZE>(tabsInfo[i].name, pureFileName.c_str());
                }
            }

            //if (!tabsInfo[selectedTab].latencyData.measurements.empty())
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
                size_t new_tabs = HelperJson::GetLatencyTests(tabsInfo, line.c_str());

                for (size_t i = tabsInfo.size() - new_tabs; i < tabsInfo.size(); i++) {
                    // Copy the filename to the tab's name
                    if (*tabsInfo[i].name == 0) {
                        std::string pureFileName = line;
                        size_t lastSlash = pureFileName.find_last_of('/');
                        pureFileName = pureFileName.substr(lastSlash + 1,
                                                           pureFileName.find_last_of(".json") - 5 - lastSlash);
                        pureFileName.copy(tabsInfo[i].name, TAB_NAME_MAX_SIZE);
                    }

                    // json version mismatch
                    if(tabsInfo[i].saved_ver != json_version)
                            out_res |= JSON_HANDLE_INFO_BAD_VERSION;

                    // Set the save path
                    line.copy(new_tabs == 1 ? tabsInfo[i].savePath : tabsInfo[i].savePathPack, MAX_PATH);
                }
                added_tabs += new_tabs;
            }
            printf("res = %s\n", res);

            for (size_t i = tabsInfo.size() - added_tabs; i < tabsInfo.size(); i++) {
                if (!tabsInfo[i].latencyData.measurements.empty())
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
            isFullscreen = true;
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


} // helper