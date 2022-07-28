#pragma once

#include <d3d11.h>

namespace GUI
{
	HWND Setup(int (*OnGuiFunc)());
	int DrawGui() noexcept;
	void Destroy() noexcept;
	void LoadFonts(float fontSizeMultiplier = 1.f);

	inline IDXGISwapChain* g_pSwapChain = NULL;
}

#define TOOLTIP(...)					\
   	if (ImGui::IsItemHovered())			\
		ImGui::SetTooltip(__VA_ARGS__); \

#define TOOLTIPFONT(...)														\
   	if (ImGui::IsItemHovered())	{												\
		ImGui::PushFont(io.Fonts->Fonts[fontIndex[selectedFont]]);	\
		ImGui::SetTooltip(__VA_ARGS__);											\
		ImGui::PopFont(); } 

#define REVERT(mainVar, bakVar)		\
	if(ImGui::IsItemClicked(1))		\
		*mainVar = bakVar;

#define IS_ONLY_ENTER_PRESSED ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && ImGui::GetIO().InputQueueCharacters.size() <= 1)
#define IS_ONLY_ESCAPE_PRESSED (ImGui::IsKeyPressed(ImGuiKey_Escape) && ImGui::GetIO().InputQueueCharacters.size() <= 1)