#pragma once

#include "App/AppState.h"

namespace AppGui {
	void BindState(AppState &state);
	int OnGuiThunk();
	int OnGui(AppState &state);
} // namespace AppGui
