#pragma once

namespace ImGui
{
	IMGUI_API void HeaderTitle(const char* text, float space = 10);
	IMGUI_API void ResizeSeparator(float& sizeValue, static float &startValue, ImGuiSeparatorFlags separatorFlags, ImVec2 sizeCap = {50, 50}, ImVec2 size = { 0,0 });
}