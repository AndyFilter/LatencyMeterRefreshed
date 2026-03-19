#pragma once

#include "imgui.h"
#include "imgui_internal.h"

namespace ImGui
{
	IMGUI_API void HeaderTitle(const char* text, float space = 10);
	IMGUI_API void ResizeSeparator(float& sizeValue, float &startValue, ImGuiSeparatorFlags separatorFlags, ImVec2 sizeCap = {50, 50}, ImVec2 size = { 0,0 });
    IMGUI_API void SeparatorSpace(ImGuiSeparatorFlags flags1 = 0, const ImVec2& size = ImVec2(0, 0));
}