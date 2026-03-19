#pragma once

#ifdef _WIN32
#define NOMINMAX
#include <d3d11.h>
#else
#include "External/ImGui/imgui_impl_glfw.h"
#include "External/ImGui/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"
#endif
#include <vector>

#include "../App/Config.h"

namespace Gui {
#ifdef _WIN32
	HWND Setup(int (*onGuiFunc)());
#else
	int Setup(int (*onGuiFunc)());

#endif
	int DrawGui() noexcept;
	void Destroy() noexcept;
	void LoadFonts(float fontSizeMultiplier = 1.f);
	bool GetFullScreenState();
#ifdef _WIN32
	int SetFullScreenState(BOOL state);
#else
	int SetFullScreenState(bool state);
#endif
	inline bool (*g_onExitFunc)();

	inline std::uint32_t g_maxSupportedFramerate = 144;
	inline std::uint32_t(*g_vSyncFrame);

#ifdef _WIN32
	inline IDXGISwapChain *g_pSwapChain = NULL;
	inline ID3D11Device *g_pd3dDevice = NULL;
	inline std::vector<IDXGIOutput *> vOutputs;
	inline std::vector<IDXGIAdapter *> vAdapters;
	inline void (*KeyDownFunc)(WPARAM key, LPARAM info, bool isPressed);
	[[maybe_unused]] inline void (*RawInputEventFunc)(RAWINPUT *rawInput);
	inline HWND hwnd = nullptr;

#else
	inline GLFWwindow *g_window;

	bool RegKeyCallback(GLFWkeyfun func);
#endif

	constexpr int WINDOW_X = 1080, WINDOW_Y = 740;
	constexpr int MARGIN_Y = 37;
} // namespace Gui

#define RIGHT_CLICK_REVERTIBLE(mainVar, bakVar)                                                                        \
	if (ImGui::IsItemClicked(1))                                                                                       \
		*mainVar = bakVar;

#define IS_ONLY_ENTER_PRESSED                                                                                          \
	((ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)) &&                             \
	 ImGui::GetIO().InputQueueCharacters.size() <= 1)
#define IS_ONLY_ESCAPE_PRESSED (ImGui::IsKeyPressed(ImGuiKey_Escape) && ImGui::GetIO().InputQueueCharacters.size() <= 1)
