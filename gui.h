#pragma once

namespace GUI
{
	HWND Setup(int (*OnGuiFunc)());
	int DrawGui() noexcept;
	void Destroy() noexcept;
}