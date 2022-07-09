#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <string.h>

#include "gui.h"
#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_internal.h"


float lastFrameGui = 0.0f;
float lastFrame = 0.0f;

// ---- Styling ----
const int styleColorNum = 2;
const unsigned int colorSize = 16;

float accentColor[4];
float accentBrightness = 0.1f;

float fontColor[4];
float fontBrightness = 0.1f;


float accentColorBak[4];
float accentBrightnessBak = 0.1f;

float fontColorBak[4];
float fontBrightnessBak = 0.1f;
// -------

/*
TODO:
- Add multiple fonts to chose from.
- Add a way to change background color (?)
- Add the functionality...

Learned:
- VS debugger has no idea about what the type of the pointer is, even tho it's explicitly stated... (-2h)
*/

bool isSettingOpen = false;

uint64_t micros()
{
	uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::high_resolution_clock::now().time_since_epoch())
		.count();
	static uint64_t start{us};
	return us - start;
}

void ApplyStyle(float colors[styleColorNum][4], float brightnesses[styleColorNum])
{
	auto&style = ImGui::GetStyle();
	for (int i = 0; i < styleColorNum; i++)
	{
		float brightness = brightnesses[i];

		auto baseColor = ImVec4(*(ImVec4*)colors[i]);
		auto darkerBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness);
		auto darkestBase = ImVec4(*(ImVec4*)&baseColor) * (1 - brightness * 2);

		// Alpha has to be set separatly, because it also gets multiplied times brightness.
		auto alphaBase = baseColor.w;
		baseColor.w = alphaBase;
		darkerBase.w = alphaBase;
		darkestBase.w = alphaBase;

		switch (i)
		{
		case 0:
			style.Colors[ImGuiCol_Header] = baseColor;
			style.Colors[ImGuiCol_HeaderHovered] = darkerBase;
			style.Colors[ImGuiCol_HeaderActive] = darkestBase;
			break;
		case 1:
			style.Colors[ImGuiCol_Text] = baseColor;
			style.Colors[ImGuiCol_TextDisabled] = darkerBase;
			break;
		}
	}
}

void RevertStyle()
{
	std::copy(accentColorBak, accentColorBak + 4, accentColor);
	std::copy(fontColorBak, fontColorBak + 4, fontColor);

	accentBrightness = accentBrightnessBak;
	fontBrightness = fontBrightnessBak;
}

bool SaveStyleConfig(float colors[styleColorNum][4], float brightnesses[styleColorNum], size_t size = styleColorNum)
{
	char buffer[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, buffer);

	auto filePath = std::string(buffer).find("Debug") != std::string::npos ? "Style.cfg" : "Debug/Style.cfg";

	std::ofstream configFile(filePath);
	for (int i = 0; i < size; i++)
	{
		auto color = colors[i];
		auto packedColor = ImGui::ColorConvertFloat4ToU32(*(ImVec4*)(color));
		configFile << i << packedColor << "," << std::to_string(brightnesses[i]) << std::endl;

		switch (i)
		{
		case 0:
			std::copy(color, color + 4, accentColorBak);
			accentBrightnessBak = brightnesses[i];
			break;
		case 1:
			std::copy(color, color + 4, fontColorBak);
			fontBrightnessBak = brightnesses[i];
			break;
		}
	}
	configFile.close();

	std::cout << "File Closed\n";

	return true;
}

bool ReadStyleConfig(float (&colors)[styleColorNum][4], float (&brightnesses)[styleColorNum])
{
	char buffer[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, buffer);

	auto filePath = std::string(buffer).find("Debug") != std::string::npos ? "Style.cfg" : "Debug/Style.cfg";

	std::ifstream configFile(filePath);

	if (!configFile.good())
		return false;

	std::string line, colorPart, brightnessPart;
	while (std::getline(configFile, line))
	{
		// First char of the line is the index (just to be sure)
		int index = (int)(line[0] - '0');

		// Color is stored in front of the comma and the brightness after it
		int delimiterPos = line.find(",");
		colorPart = line.substr(1, delimiterPos - 1);
		brightnessPart = line.substr(delimiterPos + 1, line.find("\n"));

		// Extract the color from the string
		auto subInt = stoll(colorPart);
		auto color = ImGui::ColorConvertU32ToFloat4(subInt);
		auto offset = (index * (colorSize));

		// Same with brightness
		float brightness = std::stof(brightnessPart);

		// Set both of the values
		memcpy(&(colors[index][0]), &color, colorSize);
		brightnesses[index] = brightness;
	}

	// Remember to close the handle
	configFile.close();

	std::cout << "File Closed\n";

	return true;
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

		ImGui::SameLine();

		switch (selectedSettings)
		{
		case 0: // Style
		{
			ImGui::BeginGroup();
			ImGui::BeginChild("Style", { ((1 - listBoxSize) * avail.x) - style.FramePadding.x * 2, avail.y - ImGui::GetFrameHeightWithSpacing() }, true);

			ImGui::ColorEdit4("Main Color", accentColor);

			ImGui::PushID(02);
			ImGui::SliderFloat("Brightness", &accentBrightness, -0.5f, 0.5f);
			ImGui::PopID();

			auto _accentColor = ImVec4(*(ImVec4*)accentColor);
			auto darkerAccent = ImVec4(*(ImVec4*)&_accentColor) * (1 - accentBrightness);
			auto darkestAccent = ImVec4(*(ImVec4*)&_accentColor) * (1 - accentBrightness * 2);

			// Alpha has to be set separatly, because it also gets multiplied times brightness.
			auto alphaAccent = ImVec4(*(ImVec4*)accentColor).w;
			_accentColor.w = alphaAccent;
			darkerAccent.w = alphaAccent;
			darkestAccent.w = alphaAccent;

			ImGui::Dummy({ 0, ImGui::GetFrameHeight() / 2 });

			ImVec2 colorsAvail = ImGui::GetContentRegionAvail();

			ImGui::ColorButton("Accent Color", (ImVec4)_accentColor, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
			ImGui::ColorButton("Accent Color Dark", darkerAccent, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
			ImGui::ColorButton("Accent Color Darkest", darkestAccent, 0, { colorsAvail.x, ImGui::GetFrameHeight() });


			ImGui::SeparatorSpace(ImGuiLayoutType_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


			ImGui::ColorEdit4("Font Color", fontColor);

			ImGui::PushID(12);
			ImGui::SliderFloat("Brightness", &fontBrightness, -1.0f, 1.0f, "%.3f");
			ImGui::PopID();

			auto _fontColor = ImVec4(*(ImVec4*)fontColor);
			auto darkerFont = ImVec4(*(ImVec4*)&_fontColor) * (1 - fontBrightness);

			// Alpha has to be set separatly, because it also gets multiplied times brightness.
			auto alphaFont = ImVec4(*(ImVec4*)fontColor).w;
			_fontColor.w = alphaFont;
			darkerFont.w = alphaFont;

			ImGui::Dummy({ 0, ImGui::GetFrameHeight() / 2 });

			ImGui::ColorButton("Font Color", _fontColor, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
			ImGui::ColorButton("Font Color Dark", darkerFont, 0, { colorsAvail.x, ImGui::GetFrameHeight() });

			//ImGui::SetCursorPosY(avail.y - ImGui::GetFrameHeight() - style.WindowPadding.y);

				float colors[2][4];
				float brightnesses[2]{ accentBrightness, fontBrightness };
				memcpy(&colors, &accentColor, colorSize);
				memcpy(&colors[1][0], &fontColor, colorSize);
				ApplyStyle(colors, brightnesses);

			ImGui::EndChild();

			ImGui::BeginGroup();

			auto buttonsAvail = ImGui::GetContentRegionAvail();

			if (ImGui::Button("Revert", { (buttonsAvail.x / 2 - style.FramePadding.x), ImGui::GetFrameHeight()}))
			{
				RevertStyle();
			}

			ImGui::SameLine();

			if (ImGui::Button("Save", { (buttonsAvail.x / 2 - style.FramePadding.x), ImGui::GetFrameHeight() }))
			{
				SaveStyleConfig(colors, brightnesses);
			}

			ImGui::EndGroup();

			ImGui::EndGroup();

			break;
		}
		}


		ImGui::End();
	}

	ImGui::Text("Time is: %ims", clock());
	ImGui::Text("Time is: %luus", micros());

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (GUI ONLY)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	lastFrameGui = micros();

	return 0;
}


// Main code
int main(int, char**)
{
	micros();
	GUI::Setup(OnGui);

	bool done = false;

	float colors[styleColorNum][4];
	float brightnesses[styleColorNum] = { accentBrightness, fontBrightness };
	if (ReadStyleConfig(colors, brightnesses))
	{
		ApplyStyle(colors, brightnesses);

		// Set the default colors to the varaibles with a type conversion (ImVec4 -> float[4]) (could be done with std::copy, but the performance advantage it gives is just unmeasurable, especially for a single time execution code)
		memcpy(&accentColor, &(colors[0][0]), colorSize);
		memcpy(&fontColor, &(colors[1][0]), colorSize);

		accentBrightness = brightnesses[0];
		fontBrightness = brightnesses[1];

		memcpy(&accentColorBak, &(colors[0][0]), colorSize);
		memcpy(&fontColorBak, &(colors[1][0]), colorSize);

		accentBrightnessBak = brightnesses[0];
		fontBrightnessBak = brightnesses[1];
	}
	else
	{
		auto& styleColors = ImGui::GetStyle().Colors;
		memcpy(&accentColor, &(styleColors[ImGuiCol_Header]), colorSize);
		memcpy(&fontColor, &(styleColors[ImGuiCol_Text]), colorSize);

		memcpy(&accentColorBak, &(styleColors[ImGuiCol_Header]), colorSize);
		memcpy(&fontColorBak, &(styleColors[ImGuiCol_Text]), colorSize);

		// Note: this style might be a little bit different prior to applying it. (different darker colors)
	}
	

	// Main Loop
	while (!done)
	{
		//if ((micros()) - lastFrameGui >= 1/*(1000000/79)*/)
		// GUI Loop
		if(micros() - lastFrameGui >= (1/75.f) * 1000000)
			if (GUI::DrawGui())
				break;

		lastFrame = micros();

		// Limit for eco-friendly purposes
		Sleep(1);
	}

	GUI::Destroy();
}
