#pragma once

#include <d3d11.h>
#include <vector>

namespace GUI
{
	HWND Setup(int (*OnGuiFunc)());
	int DrawGui() noexcept;
	void Destroy() noexcept;
	void LoadFonts(float fontSizeMultiplier = 1.f);

	inline IDXGISwapChain* g_pSwapChain = NULL;
	inline ID3D11Device* g_pd3dDevice = NULL;
	inline std::vector<IDXGIOutput*> vOutputs;
	inline std::vector<IDXGIAdapter*> vAdapters;
	inline bool (*onExitFunc)();
	inline void (*KeyDownFunc)(WPARAM key, LPARAM info, bool isPressed);

	inline UINT MAX_SUPPORTED_FRAMERATE;
	inline UINT (*VSyncFrame);

	static int windowX = 1080, windowY = 740;
}

#define TOOLTIP(...)					\
   	if (ImGui::IsItemHovered())			\
		ImGui::SetTooltip(__VA_ARGS__); \

#define TOOLTIPFONT(...)														\
   	if (ImGui::IsItemHovered())	{												\
		ImGui::PushFont(io.Fonts->Fonts[fontIndex[currentUserData.style.selectedFont]]);	\
		ImGui::SetTooltip(__VA_ARGS__);											\
		ImGui::PopFont(); } 

#define REVERT(mainVar, bakVar)		\
	if(ImGui::IsItemClicked(1))		\
		*mainVar = bakVar;

#define IS_ONLY_ENTER_PRESSED ((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) && ImGui::GetIO().InputQueueCharacters.size() <= 1)
#define IS_ONLY_ESCAPE_PRESSED (ImGui::IsKeyPressed(ImGuiKey_Escape) && ImGui::GetIO().InputQueueCharacters.size() <= 1)