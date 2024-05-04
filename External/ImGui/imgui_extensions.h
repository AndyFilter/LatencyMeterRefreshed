#pragma once

#ifndef LATENCYMETERREFRESHED_IMGUI_EXTENSIONS_H
#define LATENCYMETERREFRESHED_IMGUI_EXTENSIONS_H


namespace ImGui
{
	IMGUI_API void HeaderTitle(const char* text, float space = 10);
	IMGUI_API void ResizeSeparator(float& sizeValue, float &startValue, ImGuiSeparatorFlags separatorFlags, ImVec2 sizeCap = {50, 50}, ImVec2 size = { 0,0 });
    IMGUI_API void SeparatorSpace(int flags1 = 0, const ImVec2& size = ImVec2(0, 0));
}

#endif //LATENCYMETERREFRESHED_IMGUI_EXTENSIONS_H