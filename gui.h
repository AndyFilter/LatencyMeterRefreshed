#pragma once

namespace GUI
{
	void Setup(void (*OnGuiFunc)());
	int DrawGui() noexcept;
	void Destroy() noexcept;
}