#include <Windows.h>
#include <iostream>
#include <chrono>

#include "gui.h"
#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_internal.h"


float lastFrameGui = 0.0f;
float lastFrame = 0.0f;

float accentColor[4];
float accentBrightness = 1;

float fontColor[4];
float fontBrightness = 1;

bool isSettingOpen = false;

uint64_t micros()
{
	uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::high_resolution_clock::now().time_since_epoch())
		.count();
	return us;
}

// Not the best GUI solution, but it's simple, fast and gets the job done.
int OnGui()
{
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
			if (ImGui::MenuItem("Save", "Ctrl+S")) { /* Do stuff */ }
			if (ImGui::MenuItem("Settings", "")) { isSettingOpen = !isSettingOpen; }
			if (ImGui::MenuItem("Close", "Ctrl+W")) { return 1; }
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGuiStyle& style = ImGui::GetStyle();
	const auto avail = ImGui::GetContentRegionAvail();

	if (isSettingOpen)
	{
		ImGui::SetNextWindowSize({ 480.0f, 480.0f });
		ImGui::Begin("Settings", &isSettingOpen, ImGuiWindowFlags_NoResize);

		static int selectedSettings = 0;
		const auto avail = ImGui::GetContentRegionAvail();

		const char* items[3]{ "Style", "Performance", "Test" };

		// Makes list take ({listBoxSize}*100)% width of the parent
		float listBoxSize = 0.3f;

		ImGui::BeginListBox("##Setting", { avail.x * listBoxSize, avail.y });
		{
			for (int i = 0; i < sizeof(items) / sizeof(items[0]); i++)
			{
				ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (style.ItemSpacing.x / 2) - 2);
				ImVec2 label_size = ImGui::CalcTextSize(items[i], NULL, true);
				bool isSelected = (selectedSettings == i);
				if (ImGui::Selectable(items[i], isSelected, 0, { (avail.x * listBoxSize - (style.ItemSpacing.x) - (style.FramePadding.x * 2)) + 4, label_size.y }, style.FrameRounding))
				{
					selectedSettings = i;
				}
			}
		}
		ImGui::EndListBox();

		//ImGui::SameLine();

		//ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, {50, 200});

		ImGui::SameLine();

		switch (selectedSettings)
		{
		case 0: // Style
		{
			ImGui::BeginChild("Stats", { ((1 - listBoxSize) * avail.x) - style.FramePadding.x * 2, avail.y }, true);

			ImGui::PushID(02);
			ImGui::SliderFloat("Brightness", &accentBrightness, 0, 1);
			ImGui::PopID();

			ImGui::ColorEdit4("Main Color", accentColor);

			auto _accentColor = ImVec4(*(ImVec4*)accentColor) * accentBrightness;
			auto darkerAccent = ImVec4(*(ImVec4*)&_accentColor) * 0.9f;
			auto darkestAccent = ImVec4(*(ImVec4*)&_accentColor) * 0.8f;

			// Alpha has to be set separatly, because it also gets multiplied times brightness.
			auto alphaAccent = ImVec4(*(ImVec4*)accentColor).w;
			_accentColor.w = alphaAccent;
			darkerAccent.w = alphaAccent;
			darkestAccent.w = alphaAccent;

			ImGui::Dummy({ 0, ImGui::GetFrameHeight() / 2 });


			ImVec2 colorsAvail = ImGui::GetContentRegionAvail();

			ImGui::ColorButton("Accent Color", _accentColor, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
			ImGui::ColorButton("Accent Color Dark", darkerAccent, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
			ImGui::ColorButton("Accent Color Darkest", darkestAccent, 0, { colorsAvail.x, ImGui::GetFrameHeight() });

			style.Colors[ImGuiCol_Header] = *(ImVec4*)&_accentColor;
			style.Colors[ImGuiCol_HeaderHovered] = darkerAccent;
			style.Colors[ImGuiCol_HeaderActive] = darkestAccent;


			ImGui::SeparatorSpace(ImGuiLayoutType_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


			ImGui::PushID(12);
			ImGui::SliderFloat("Brightness", &fontBrightness, 0, 1);
			ImGui::PopID();

			ImGui::ColorEdit4("Font Color", fontColor);

			auto _fontColor = ImVec4(*(ImVec4*)fontColor) * fontBrightness;
			auto darkerFont = ImVec4(*(ImVec4*)&_accentColor) * 0.9f;
			auto darkestFont = ImVec4(*(ImVec4*)&_accentColor) * 0.8f;

			// Alpha has to be set separatly, because it also gets multiplied times brightness.
			auto alphaFont = ImVec4(*(ImVec4*)accentColor).w;
			_accentColor.w = alphaFont;
			darkerFont.w = alphaFont;
			darkestFont.w = alphaFont;

			ImGui::Dummy({ 0, ImGui::GetFrameHeight() / 2 });


			ImGui::ColorButton("Font Color", _accentColor, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
			ImGui::ColorButton("Font Color Dark", darkerFont, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
			ImGui::ColorButton("Font Color Darkest", darkestFont, 0, { colorsAvail.x, ImGui::GetFrameHeight() });

			style.Colors[ImGuiCol_Text] = *(ImVec4*)&_accentColor;
			style.Colors[ImGuiCol_TextDisabled] = darkerFont;
			style.Colors[ImGuiCol_HeaderActive] = darkestFont;

			style.Colors[ImGuiCol_Text] = *(ImVec4*)&_fontColor;


			ImGui::EndChild();

			break;
		}
		}

		ImGui::End();
	}

	ImGui::Text("Time is: %ims", clock());

	ImGui::Text("Framerate: %f", 1 / (clock() - lastFrameGui) * 1000);

	lastFrameGui = clock();

	return 0;
}

// Main code
int main(int, char**)
{
	GUI::Setup(OnGui);

	bool done = false;

	uint64_t start = micros();

	memcpy(&accentColor, &ImGui::GetStyle().Colors[ImGuiCol_Header], sizeof(float) * 4);
	memcpy(&fontColor, &ImGui::GetStyle().Colors[ImGuiCol_Text], sizeof(float) * 4);

	// Main Loop
	while (!done)
	{
		lastFrame = micros() - start;

		if (clock() - lastFrameGui > 20)
			if (GUI::DrawGui())
				break;
	}

	GUI::Destroy();
}
