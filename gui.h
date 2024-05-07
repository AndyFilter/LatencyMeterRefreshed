#pragma once

#ifdef _WIN32
#include <d3d11.h>
#else
#include "External/ImGui/imgui_impl_glfw.h"
#include "External/ImGui/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"
#endif
#include <vector>

#include "constants.h"

namespace GUI
{
#ifdef _WIN32
	HWND Setup(int (*OnGuiFunc)());
#else
    int Setup(int (*OnGuiFunc)());

#endif
	int DrawGui() noexcept;
	void Destroy() noexcept;
	void LoadFonts(float fontSizeMultiplier = 1.f);
    bool GetFullScreenState();
    int SetFullScreenState(bool state);

    inline bool (*onExitFunc)();

    inline UINT MAX_SUPPORTED_FRAMERATE = 144;
    inline UINT (*VSyncFrame);

#ifdef _WIN32
	inline IDXGISwapChain* g_pSwapChain = NULL;
	inline ID3D11Device* g_pd3dDevice = NULL;
	inline std::vector<IDXGIOutput*> vOutputs;
	inline std::vector<IDXGIAdapter*> vAdapters;
	inline void (*KeyDownFunc)(WPARAM key, LPARAM info, bool isPressed);
	inline void (*RawInputEventFunc)(RAWINPUT *rawInput);
    inline HWND hwnd = nullptr;

#else
    inline GLFWwindow* window;

    bool RegKeyCallback(GLFWkeyfun func);
#endif

	static int windowX = 1080, windowY = 740;
    static int Y_MARGIN = 37;
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