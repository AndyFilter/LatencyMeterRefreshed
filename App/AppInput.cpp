#include "AppInput.h"

#include "../GUI/Gui.h"
#include "App/AppCore.h"
#include "Helpers/AppHelper.h"
#include "Models.h"
#include "Serial/Serial.h"

using namespace AppCore;
using namespace AppHelper;

static AppState *g_state = nullptr;

void AppInput::BindState(AppState &state) { g_state = &state; }

#ifdef _WIN32
void AppInput::KeyDown(const WPARAM key, const LPARAM info, const bool isPressed) {
	// enum class ModKey {
	//	ModKey_None = 0,
	//	ModKey_Ctrl = 1,
	//	ModKey_Shift = 2,
	//	ModKey_Alt = 4,
	//  }; Just for information

	const bool wasJustClicked = (info & 0x40000000) == 0;
	static unsigned char modKeyFlags = 0;

	if (key == -1 && info == -1) {
		modKeyFlags = 0;
		return;
	}

	if (isPressed && (key == VK_CONTROL || key == VK_LCONTROL))
		modKeyFlags |= 1;

	if (!isPressed && (key == VK_CONTROL || key == VK_LCONTROL))
		modKeyFlags ^= 1;


	if (isPressed && (key == VK_SHIFT || key == VK_RSHIFT))
		modKeyFlags |= 2;

	if (!isPressed && (key == VK_SHIFT || key == VK_RSHIFT))
		modKeyFlags ^= 2;


	if (isPressed && (key == VK_LMENU || key == VK_RMENU || key == VK_MENU))
		modKeyFlags |= 4;

	if (!isPressed && (key == VK_LMENU || key == VK_RMENU || key == VK_MENU))
		modKeyFlags ^= 4;

	DEBUG_PRINT("Key mod: %u\n", modKeyFlags);


	// Handle time-sensitive KB input
	if (g_state->isInternalMode && key == 'R' && isPressed && modKeyFlags == 2 &&
		g_state->serialStatus != Status_WaitingForResult) {
		StartInternalTest(*g_state);
	}

	if (modKeyFlags == 1 && isPressed) {
		if (key == 'S' && wasJustClicked) {
			SaveMeasurements();
		} else if (key == 'O' && wasJustClicked) {
			OpenMeasurements();
		} else if (key == 'N' && wasJustClicked) {
			auto newTab = TabInfo();
			strcat_s<TAB_NAME_MAX_SIZE>(newTab.name, std::to_string(g_state->tabsInfo.size()).c_str());
			g_state->tabsInfo.push_back(newTab);
			g_state->selectedTab = static_cast<int>(g_state->tabsInfo.size()) - 1;
		} else if (key == 'Q' && wasJustClicked) {
			if (g_state->isInternalMode ? g_state->isAudioDevConnected : Serial::g_isConnected)
				AttemptDisconnect(*g_state);
			else
				AttemptConnect(*g_state);
		}
	}
	if (modKeyFlags == 3 && key == 'S' && wasJustClicked)
		SaveAsMeasurements();
	if ((modKeyFlags & 4) == 4 && isPressed) {
		if (key == 'S' && modKeyFlags == 4 && wasJustClicked) {
			SavePackMeasurements();
		}

		if (key == 'S' && (modKeyFlags & 2) == 2 && wasJustClicked) {
			SavePackAsMeasurements();
		}
	}
}
#else
void AppInput::KeyCallback(GLFWwindow *, const int key, int , const int action, int ) {
	// enum class ModKey {
	//	ModKey_None = 0,
	//	ModKey_Ctrl = 1,
	//	ModKey_Shift = 2,
	//	ModKey_Alt = 4,
	//  }; Just for information

	const bool isPressed = action == GLFW_PRESS || action == GLFW_REPEAT;
	const bool wasJustClicked = action == GLFW_PRESS;
	static unsigned char modKeyFlags = 0;

	// if (key == 0 && action == GLFW_RELEASE) {
	//     modKeyFlags = 0;
	//     return;
	// }

	if (isPressed && (key == GLFW_KEY_RIGHT_CONTROL || key == GLFW_KEY_LEFT_CONTROL))
		modKeyFlags |= 1;

	if (!isPressed && (key == GLFW_KEY_RIGHT_CONTROL || key == GLFW_KEY_LEFT_CONTROL))
		modKeyFlags ^= 1;


	if (isPressed && (key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_SHIFT))
		modKeyFlags |= 2;

	if (!isPressed && (key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_SHIFT))
		modKeyFlags ^= 2;


	if (isPressed && (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT))
		modKeyFlags |= 4;

	if (!isPressed && (key == GLFW_KEY_LEFT_ALT || key == GLFW_KEY_RIGHT_ALT))
		modKeyFlags ^= 4;

	// printf("Key mod: %u\n", modKeyFlags);


	// Handle time-sensitive KB input
	if (g_state->isInternalMode && key == 'R' && isPressed && modKeyFlags == 2 &&
		g_state->serialStatus != Status_WaitingForResult) {
		StartInternalTest(*g_state);
	}

	if (modKeyFlags == 1 && isPressed) {
		if (key == 'S' && wasJustClicked) {
			SaveMeasurements();
		} else if (key == 'O' && wasJustClicked) {
			OpenMeasurements();
		} else if (key == 'N' && wasJustClicked) {
			auto newTab = TabInfo();
			strcat(newTab.name, std::to_string(g_state->tabsInfo.size()).c_str());
			g_state->latencyStatsMutex.lock();
			g_state->tabsInfo.push_back(newTab);
			g_state->latencyStatsMutex.unlock();
			g_state->selectedTab = g_state->tabsInfo.size() - 1;
		} else if (key == 'Q' && wasJustClicked) {
			if (g_state->isInternalMode ? g_state->isAudioDevConnected : Serial::g_isConnected)
				AttemptDisconnect(*g_state);
			else
				AttemptConnect(*g_state);
		}
	}
	if (modKeyFlags == 3 && key == 'S' && wasJustClicked)
		SaveAsMeasurements();
	if ((modKeyFlags & 4) == 4 && isPressed) {
		if (key == 'S' && modKeyFlags == 4 && wasJustClicked) {
			SavePackMeasurements();
		}

		if (key == 'S' && (modKeyFlags & 2) == 2 && wasJustClicked) {
			SavePackAsMeasurements();
		}
	}
}
#endif
