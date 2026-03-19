#pragma once

#include "App/AppState.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include "GLFW/glfw3.h"
#endif

namespace AppInput {
	void BindState(AppState &state);

#ifdef _WIN32
	void KeyDown(WPARAM key, LPARAM info, bool isPressed);
#else
	void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
#endif
} // namespace AppInput
