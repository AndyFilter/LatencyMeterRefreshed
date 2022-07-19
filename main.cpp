#include <Windows.h>
#include <iostream>
#include <chrono>
#include <fstream>
#include <string.h>

#include "serial.h"
#include "gui.h"
#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_internal.h"
#include "structs.h"

ImVec2 detectionRectSize{ 0, 200 };

float lastFrameGui = 0.0f;
float lastFrame = 0.0f;

// ---- Styling ----
const int styleColorNum = 2;
const unsigned int colorSize = 16;


float accentColor[4];
float accentBrightness = 0.1f;

float fontColor[4];
float fontBrightness = 0.1f;

int selectedFont = 0;
float fontSize = 1.f;
ImFont* boldFont;


float accentColorBak[4];
float accentBrightnessBak = 0.1f;

float fontColorBak[4];
float fontBrightnessBak = 0.1f;

int selectedFontBak = 0;
float fontSizeBak = 1.f;
ImFont* boldFontBak;


const char* fonts[4]{ "Courier Prime", "Source Sans Pro", "Franklin Gothic", "Lucida Console" };
const int fontIndex[4]{ 0, 2, 4, 5 };
// -------

// ---- Functionality ----

SerialStatus serialStatus = Status_Idle;

int selectedPort = 0;
std::vector<LatencyReading> latencyTests;

LatencyStats latencyStats{};
// -------

void GotSerialChar(char c);

/*
TODO:

+ Add multiple fonts to chose from.
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
	static uint64_t start{ us };
	return us - start;
}

void ApplyStyle(float colors[styleColorNum][4], float brightnesses[styleColorNum])
{
	auto& style = ImGui::GetStyle();
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

			style.Colors[ImGuiCol_TabHovered] = baseColor;
			style.Colors[ImGuiCol_TabActive] = darkerBase;
			style.Colors[ImGuiCol_Tab] = darkestBase;

			style.Colors[ImGuiCol_CheckMark] = baseColor;

			style.Colors[ImGuiCol_PlotLines] = style.Colors[ImGuiCol_PlotHistogram] = darkerBase;
			style.Colors[ImGuiCol_PlotLinesHovered] = style.Colors[ImGuiCol_PlotHistogramHovered] = baseColor;
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

	fontSize = fontSizeBak;
	selectedFont = selectedFontBak;
	boldFont = boldFontBak;

	ImGuiIO& io = ImGui::GetIO();

	for (int i = 0; i < io.Fonts->Fonts.Size; i++)
	{
		io.Fonts->Fonts[i]->Scale = fontSize;
	}
	io.FontDefault = io.Fonts->Fonts[fontIndex[selectedFont]];
}

bool SaveStyleConfig(float colors[styleColorNum][4], float brightnesses[styleColorNum], int fontIndex, float fontSize, size_t size = styleColorNum)
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
	selectedFontBak = selectedFont;
	fontSizeBak = fontSize;
	boldFontBak = boldFont;
	configFile << "F" << fontIndex << std::to_string(fontSize) << std::endl;
	configFile.close();

	std::cout << "File Closed\n";

	return true;
}

// Does the calcualtions and copying for you
void SaveCurrentStyleConfig()
{
	float colors[2][4];
	float brightnesses[2]{ accentBrightness, fontBrightness };
	//std::copy(&accentColor, &accentColor + 2, &colors);
	//std::copy(&fontColor, &fontColor + 2, &colors[1]);
	memcpy(&colors, &accentColor, colorSize);
	memcpy(&colors[1], &fontColor, colorSize);

	SaveStyleConfig(colors, brightnesses, selectedFont, fontSize);
}

ImFont* GetFontBold(int baseFontIndex)
{
	ImGuiIO& io = ImGui::GetIO();
	std::string fontDebugName = (std::string)io.Fonts->Fonts[fontIndex[baseFontIndex]]->GetDebugName();
	if (fontDebugName.find('-') == std::string::npos || fontDebugName.find(',') == std::string::npos)
		return nullptr;
	auto fontName = fontDebugName.substr(0, fontDebugName.find('-'));
	auto _boldFont = (io.Fonts->Fonts[fontIndex[baseFontIndex]+1]);
	auto boldName = ((std::string)(_boldFont->GetDebugName())).substr(0, fontDebugName.find('-'));
	if (fontName == boldName)
	{
		printf("found bold font %s\n", _boldFont->GetDebugName());
		return _boldFont;
	}
	return nullptr;
}

bool ReadStyleConfig(float(&colors)[styleColorNum][4], float(&brightnesses)[styleColorNum], int& fontIndex, float& fontSize)
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
		if (line[0] == 'F')
		{
			fontIndex = line[1] - '0';
			fontSize = std::stof(line.substr(2, line.find("\n")));
			continue;
		}

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

// This could also be done just using a single global variable like "hasStyleChanged", because ImGui elements like FloatSlider, ColorEdit4 return a value (true) if the value has changed.
// But this method would not work as expected when user reverts back the changes or sets the variable to it's original value
bool HasStyleChanged()
{
	bool brightnesses = accentBrightness == accentBrightnessBak && fontBrightness == fontBrightnessBak;
	bool font = selectedFont == selectedFontBak && fontSize == fontSizeBak;
	bool colors{ false };

	// Compare arrays of colors column by column, because otherwise we would just compare pointers to these values which would always yield a false positive result.
	// (Pointers point to different addresses even tho the values at these addresses are the same)
	for (int column = 0; column < styleColorNum; column++)
	{
		if (accentColor[column] != accentColorBak[column])
		{
			colors = false;
			break;
		}

		if (fontColor[column] != fontColorBak[column])
		{
			colors = false;
			break;
		}

		colors = true;
	}

	if (!colors || !brightnesses || !font)
		return true;
	else
		return false;
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
	ImGuiIO& io = ImGui::GetIO();

	if (isSettingOpen)
	{
		ImGui::SetNextWindowSize({ 480.0f, 480.0f });

		bool wasLastSettingsOpen = isSettingOpen;
		if (ImGui::Begin("Settings", &isSettingOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
		{
			// On Settings Window Closed:
			if (wasLastSettingsOpen && !isSettingOpen && HasStyleChanged())
			{
				// Call a popup by name
				ImGui::OpenPopup("Save Style?");
			}

			// Define the messagebox popup
			if (ImGui::BeginPopupModal("Save Style?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				isSettingOpen = true;
				ImGui::PushFont(boldFont);
				ImGui::Text("Do you want to save before exiting?");
				ImGui::PopFont();


				ImGui::Separator();
				ImGui::Dummy({ 0, ImGui::GetTextLineHeight() });

				// These buttons don't really fit the window perfectly with some fonts, I might have to look into that.
				ImGui::BeginGroup();
				if (ImGui::Button("Save"))
				{
					SaveCurrentStyleConfig();
					isSettingOpen = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Discard"))
				{
					RevertStyle();
					isSettingOpen = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndGroup();

				ImGui::EndPopup();
			}


			static int selectedSettings = 0;
			const auto avail = ImGui::GetContentRegionAvail();

			const char* items[3]{ "Style", "Performance", "Test" };

			// Makes list take ({listBoxSize}*100)% width of the parent
			float listBoxSize = 0.3f;

			if (ImGui::BeginListBox("##Setting", { avail.x * listBoxSize, avail.y }))
			{
				for (int i = 0; i < sizeof(items) / sizeof(items[0]); i++)
				{
					//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (style.ItemSpacing.x / 2) - 2);
					ImVec2 label_size = ImGui::CalcTextSize(items[i], NULL, true);
					bool isSelected = (selectedSettings == i);
					//if (ImGui::Selectable(items[i], isSelected, 0, { (avail.x * listBoxSize - (style.ItemSpacing.x) - (style.FramePadding.x * 2)) + 4, label_size.y }, style.FrameRounding))
					if (ImGui::Selectable(items[i], isSelected, 0, { 0, 0 }, style.FrameRounding))
					{
						selectedSettings = i;
					}
				}
				ImGui::EndListBox();
			}

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


				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


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
				memcpy(&colors[1], &fontColor, colorSize);
				ApplyStyle(colors, brightnesses);


				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


				//ImGui::PushFont(io.Fonts->Fonts[fontIndex[selectedFont]]);
				// This has to be done manually (instead of ImGui::Combo()) to be able to change the fonts of each selectable to a corresponding one.
				if (ImGui::BeginCombo("Font", fonts[selectedFont]))
				{
					for (int i = 0; i < (io.Fonts->Fonts.Size / 2) + 1; i++)
					{
						auto selectableSpace = ImGui::GetContentRegionAvail();
						//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2);

						ImGui::PushFont(io.Fonts->Fonts[fontIndex[i]]);

						bool isSelected = (selectedFont == i);
						if (ImGui::Selectable(fonts[i], isSelected, 0, { 0, 0 }, style.FrameRounding))
						{
							if (selectedFont != i) {
								io.FontDefault = io.Fonts->Fonts[fontIndex[i]];
								if (auto _boldFont = GetFontBold(i); _boldFont != nullptr)
									boldFont = _boldFont;
								else
									boldFont = io.Fonts->Fonts[fontIndex[i]];
								selectedFont = i;
							}
						}
						ImGui::PopFont();
					}
					ImGui::EndCombo();
				}
				//ImGui::PopFont();

				if (ImGui::SliderFloat("Font Size", &fontSize, 0.5f, 2, "%.2f"))
				{
					for (int i = 0; i < io.Fonts->Fonts.Size; i++)
					{
						io.Fonts->Fonts[i]->Scale = fontSize;
					}
				}


				ImGui::EndChild();


				ImGui::BeginGroup();

				auto buttonsAvail = ImGui::GetContentRegionAvail();

				if (ImGui::Button("Revert", { (buttonsAvail.x / 2 - style.FramePadding.x), ImGui::GetFrameHeight() }))
				{
					RevertStyle();
				}

				ImGui::SameLine();

				if (ImGui::Button("Save", { (buttonsAvail.x / 2 - style.FramePadding.x), ImGui::GetFrameHeight() }))
				{
					SaveStyleConfig(colors, brightnesses, selectedFont, fontSize);
				}

				ImGui::EndGroup();

				ImGui::EndGroup();

				break;
			}
			}
		}
		ImGui::End();
	}

	if (ImGui::BeginChild("LeftChild", { avail.x / 2 - style.WindowPadding.x, avail.y - detectionRectSize.y + style.WindowPadding.y - 20 }, true))
	{

		ImGui::Text("Time is: %ims", clock());
		ImGui::Text("Time is: %luus", micros());

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (GUI ONLY)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (CPU ONLY)", (micros() - lastFrame) / 1000, 1000000.0f / (micros() - lastFrame));

		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { avail.x, ImGui::GetFrameHeight() });

		//ImGui::Combo("Select Port", &selectedPort, Serial::availablePorts.data());
		ImGui::BeginGroup();
		ImGui::PushItemWidth(80);
		if (ImGui::BeginCombo("Port", Serial::availablePorts[selectedPort].c_str()))
		{
			for (int i = 0; i < Serial::availablePorts.size(); i++)
			{
				bool isSelected = (selectedPort == i);
				if (ImGui::Selectable(Serial::availablePorts[i].c_str(), isSelected, 0, { 0,0 }, style.FrameRounding))
				{
					if (selectedPort != i)
						Serial::Close();
					selectedPort = i;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();

		ImGui::SameLine();

		if (ImGui::Button("Refresh"))
		{
			Serial::FindAvailableSerialPorts();
		}

		ImGui::SameLine();
		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 10, 0 });

		ImGui::BeginDisabled(Serial::isConnected);
		if (ImGui::ButtonEx("Connect"))
		{
			Serial::Setup(Serial::availablePorts[selectedPort].c_str(), GotSerialChar);
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		ImGui::BeginDisabled(!Serial::isConnected);
		if (ImGui::Button("Disconnect"))
		{
			Serial::Close();
		}
		ImGui::EndDisabled();

		ImGui::EndGroup();

		ImGui::Spacing();

		ImGui::PushFont(boldFont);

		if (ImGui::BeginChild("MeasurementStats", { 0,ImGui::GetTextLineHeightWithSpacing() * 4 + style.WindowPadding.y * 2 - style.ItemSpacing.y + style.FramePadding.y}, true))
		{
			/*
			//ImGui::BeginGroup();
			//ImGui::Text("");
			//ImGui::Text("Highest");
			//ImGui::Text("Average");
			//ImGui::Text("Minimum");
			//ImGui::EndGroup();

			//ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();

			//ImGui::BeginGroup();
			//ImGui::Text("Internal");
			//ImGui::Text("%u", latencyStats.internalLatency.highest / 1000);
			//ImGui::Text("%.2f", latencyStats.internalLatency.average / 1000.f);
			//ImGui::Text("%u", latencyStats.internalLatency.lowest / 1000);
			//ImGui::EndGroup();

			//ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();

			//ImGui::BeginGroup();
			//ImGui::Text("External");
			//ImGui::Text("%u", latencyStats.externalLatency.highest);
			//ImGui::Text("%.2f", latencyStats.externalLatency.average);
			//ImGui::Text("%u", latencyStats.externalLatency.lowest);
			//ImGui::EndGroup();

			//ImGui::SameLine(); ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical); ImGui::SameLine();

			//ImGui::BeginGroup();
			//ImGui::Text("Input");
			//ImGui::Text("%u", latencyStats.inputLatency.highest / 1000);
			//ImGui::Text("%.2f", latencyStats.inputLatency.average / 1000.f);
			//ImGui::Text("%u", latencyStats.inputLatency.lowest / 1000);
			//ImGui::EndGroup();
			*/

			if(ImGui::BeginTable("Latency Stats Table", 4, ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoPadOuterX))
			{
				ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(1);
					ImGui::Text("Internal");
					ImGui::TableNextColumn();
					ImGui::Text("External");
					ImGui::TableNextColumn();
					ImGui::Text("Input");

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Highest");
					ImGui::TableNextColumn();
					ImGui::Text("%u", latencyStats.internalLatency.highest / 1000);
					ImGui::TableNextColumn();
					ImGui::Text("%u", latencyStats.externalLatency.highest);
					ImGui::TableNextColumn();
					ImGui::Text("%u", latencyStats.inputLatency.highest / 1000);

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Average");
					ImGui::TableNextColumn();
					ImGui::Text("%.2f", latencyStats.internalLatency.average / 1000);
					ImGui::TableNextColumn();
					ImGui::Text("%.2f", latencyStats.externalLatency.average);
					ImGui::TableNextColumn();
					ImGui::Text("%.2f", latencyStats.inputLatency.average / 1000);

					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::Text("Lowest");
					ImGui::TableNextColumn();
					ImGui::Text("%u", latencyStats.internalLatency.lowest / 1000);
					ImGui::TableNextColumn();
					ImGui::Text("%u", latencyStats.externalLatency.lowest);
					ImGui::TableNextColumn();
					ImGui::Text("%u", latencyStats.inputLatency.lowest / 1000);

				ImGui::EndTable();
			}

			ImGui::EndChild();
		}

		ImGui::PopFont();


		ImGui::EndChild();
	}

	ImGui::SameLine();

	auto tableAvail = ImGui::GetContentRegionAvail();

	if (ImGui::BeginTable("measurementsTable", 4, ImGuiTableFlags_Reorderable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, { avail.x / 2, 200 }))
	{
		ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
		ImGui::TableSetupColumn("External Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 1);
		ImGui::TableSetupColumn("Internal Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 2);
		ImGui::TableSetupColumn("Input Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 2);
		ImGui::TableHeadersRow();

		ImGuiListClipper clipper;
		clipper.Begin(latencyTests.size());
		while (clipper.Step())
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				auto reading = latencyTests[i];

				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%i", i);
				ImGui::TableNextColumn();
				ImGui::Text("%i", reading.timeExternal);
				ImGui::TableNextColumn();
				ImGui::Text("%i", reading.timeInternal / 1000);
				if (ImGui::IsItemHovered())
				{
					// \xC2\xB5 is the microseconds character
					ImGui::SetTooltip("%i\xC2\xB5s", reading.timeInternal);
				}
				ImGui::TableNextColumn();
				ImGui::Text("%i", reading.timePing / 1000);
				if (ImGui::IsItemHovered())
				{
					// \xC2\xB5 is the microseconds character
					ImGui::SetTooltip("%i\xC2\xB5s", reading.timePing);
				}
			}

		ImGui::EndTable();
	}

	// Color change detection rectangle.
	detectionRectSize.x = detectionRectSize.x == 0 ? avail.x + style.WindowPadding.x + style.FramePadding.x : detectionRectSize.x;
	ImVec2 pos = { 0, avail.y - detectionRectSize.y + style.WindowPadding.y * 2 + style.FramePadding.y * 2 };
	ImRect bb{ pos, pos + detectionRectSize };
	ImGui::RenderFrame(
		bb.Min,
		bb.Max,
		// Change the color to white to be detected by the light sensor
		serialStatus == Status_WaitingForResult ? ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1)) : ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 0.f, 0.f, 1)),
		false
	);

	lastFrameGui = micros();

	return 0;
}

void GotSerialChar(char c)
{
	printf("Got: %c\n", c);

	// 5 numbers should be enough. I doubt the latency can be bigger than 10 seconds (anything greater than 100ms should be discarded)
	static BYTE resultBuffer[5]{};
	static BYTE resultNum = 0;

	static uint64_t internalStartTime = 0;
	static uint64_t internalEndTime = 0;

	static uint64_t pingStartTime = 0;

	switch (serialStatus)
	{
	case Status_Idle:
		// Input (button press) detected (l for light)
		if (c == 'l')
		{
			internalStartTime = micros();
			printf("waiting for results\n");
			serialStatus = Status_WaitingForResult;
		}
		break;
	case Status_WaitingForResult:
		internalEndTime = micros();
		// e for end (end of the numbers)
		// All of the code below will have to be moved to a sepearate function in the future when saving/loading from a file will be added.
		if (c == 'e')
		{
			// Because we are subtracting 2 similar unsigned longs longs, we dont need another unsigned long long, we just need and int
			unsigned int internalTime = internalEndTime - internalStartTime;
			int externalTime = 0;
			// Convert the byte array to int
			for (int i = 0; i < resultNum; i++)
			{
				externalTime += resultBuffer[i] * pow<int>(10, (resultNum - i - 1));
			}

			printf("Final result: %i\n", externalTime);
			LatencyReading reading{ externalTime, internalTime };
			size_t size = latencyTests.size();

			// Update the stats
			latencyStats.internalLatency.average = (latencyStats.internalLatency.average * size) / (size + 1.f) + (internalTime / (size + 1.f));
			latencyStats.externalLatency.average = (latencyStats.externalLatency.average * size) / (size + 1.f) + (externalTime / (size + 1.f));

			latencyStats.internalLatency.highest = internalTime > latencyStats.internalLatency.highest ? internalTime : latencyStats.internalLatency.highest;
			latencyStats.externalLatency.highest = externalTime > latencyStats.externalLatency.highest ? externalTime : latencyStats.externalLatency.highest;

			latencyStats.internalLatency.lowest = internalTime < latencyStats.internalLatency.lowest ? internalTime : latencyStats.internalLatency.lowest;
			latencyStats.externalLatency.lowest = externalTime < latencyStats.externalLatency.lowest ? externalTime : latencyStats.externalLatency.lowest;

			// If there are not measurements done yet set the minimum value to the current one
			if (size == 0)
			{
				latencyStats.internalLatency.lowest = internalTime;
				latencyStats.externalLatency.lowest = externalTime;
			}

			latencyTests.push_back(reading);
			resultNum = 0;
			std::fill_n(resultBuffer, 5, 0);
			pingStartTime = micros();
			Serial::Write("p", 1);
			serialStatus = Status_WaitingForPing;
		}
		else
		{
			int digit = c - '0';
			resultBuffer[resultNum] = digit;
			resultNum += 1;
		}
		break;
	case Status_WaitingForPing:
		if (c == 'b')
		{
			unsigned int pingTime = micros() - pingStartTime;
			latencyTests[latencyTests.size() - 1].timePing = pingTime;

			size_t size = latencyTests.size() - 1;

			latencyStats.inputLatency.average = (latencyStats.inputLatency.average * size) / (size + 1.f) + (pingTime / (size + 1.f));
			latencyStats.inputLatency.highest = pingTime > latencyStats.inputLatency.highest ? pingTime : latencyStats.inputLatency.highest;
			latencyStats.inputLatency.lowest = pingTime < latencyStats.inputLatency.lowest ? pingTime : latencyStats.inputLatency.lowest;

			if (size == 0)
			{
				latencyStats.inputLatency.lowest = pingTime;
			}

			serialStatus = Status_Idle;
		}
		break;
	default:
		break;
	}
}

// Will be removed in some later push
#ifdef LocalSerial
bool SetupSerialPort(char COM_Number)
{
	std::string serialCom = "\\\\.\\COM";
	serialCom += COM_Number;
	hPort = CreateFile(
		serialCom.c_str(),
		GENERIC_WRITE | GENERIC_READ,
		0,
		0,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED, // Make reading async
		NULL
	);

	DCB serialParams;
	serialParams.ByteSize = sizeof(serialParams);

	if (!GetCommState(hPort, &serialParams))
		return false;

	serialParams.BaudRate = 19200; // Can be changed, but this value is fast enought not to introduce any significant latency and pretty reliable
	serialParams.Parity = NOPARITY;
	serialParams.ByteSize = 8;
	serialParams.StopBits = ONESTOPBIT;

	if (!SetCommState(hPort, &serialParams))
		return false;

	COMMTIMEOUTS timeout = { 0 };
	timeout.ReadIntervalTimeout = MAXDWORD;
	timeout.ReadTotalTimeoutConstant = 0;
	timeout.ReadTotalTimeoutMultiplier = 0;
	timeout.WriteTotalTimeoutConstant = 0;
	timeout.WriteTotalTimeoutMultiplier = 0;

	if (SetCommTimeouts(hPort, &timeout))
		return false;

	//SetCommMask(hPort, EV_RXCHAR | EV_ERR); //receive character event

	return true;
}

void HandleSerial()
{
	DWORD dwBytesTransferred;
	DWORD dwCommModemStatus{};
	BYTE byte = NULL;

	/*
	//if (ReadFile(hPort, &byte, 1, &dwBytesTransferred, NULL))
	//	printf("Got: %c\n", byte);

	//return;

	//SetCommMask(hPort, EV_RXCHAR | EV_ERR); //receive character event
	//WaitCommEvent(hPort, &dwCommModemStatus, 0); //wait for character

	//if (dwCommModemStatus & EV_RXCHAR)
	// Does not work.
	//ReadFile(hPort, &byte, 1, NULL, NULL); //read 1
	//else if (dwCommModemStatus & EV_ERR)
	//	return;

	//OVERLAPPED o = { 0 };
	//o.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	//SetCommMask(hPort, EV_RXCHAR);

	//if (!WaitCommEvent(hPort, &dwCommModemStatus, &o))
	//{
	//	DWORD er = GetLastError();
	//	if (er == ERROR_IO_PENDING)
	//	{
	//		DWORD dwRes = WaitForSingleObject(o.hEvent, 1);
	//		switch (dwRes)
	//		{
	//			case WAIT_OBJECT_0:
	//				if (ReadFile(hPort, &byte, 1, &dwRead, &o))
	//					printf("Got: %c\n", byte);
	//				break;
	//		default:
	//			printf("care off");
	//			break;
	//		}
	//	}
	//	// Check GetLastError for ERROR_IO_PENDING, if I/O is pending then
	//	// use WaitForSingleObject() to determine when `o` is signaled, then check
	//	// the result. If a character arrived then perform your ReadFile.
	//}

		*/

	DWORD dwRead;
	BOOL fWaitingOnRead = FALSE;

	if (osReader.hEvent == NULL)
	{
		printf("Creating a new reader Event\n");
		osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	if (!fWaitingOnRead)
	{
		// Issue read operation.
		if (!ReadFile(hPort, &byte, 1, &dwRead, &osReader))
		{
			if (GetLastError() != ERROR_IO_PENDING)
				printf("IO Error");
			else
				fWaitingOnRead = TRUE;
		}
		else
		{
			// read completed immediately
			if (dwRead)
				GotSerialChar(byte);
		}
	}

	const DWORD READ_TIMEOUT = 1;

	DWORD dwRes;

	if (fWaitingOnRead) {
		dwRes = WaitForSingleObject(osReader.hEvent, READ_TIMEOUT);
		switch (dwRes)
		{
			// Read completed.
		case WAIT_OBJECT_0:
			if (!GetOverlappedResult(hPort, &osReader, &dwRead, FALSE))
				printf("IO Error");
			// Error in communications; report it.
			else
				// Read completed successfully.
				GotSerialChar(byte);

			//  Reset flag so that another opertion can be issued.
			fWaitingOnRead = FALSE;
			break;

			//case WAIT_TIMEOUT:
			//	break;

		default:
			printf("Event Error");
			break;
		}
	}

	/*
		//if (dwCommModemStatus & EV_RXCHAR) {
		//	ReadFile(hPort, &byte, 1, &dwBytesTransferred, 0); //read 1
		//	printf("Got: %c\n", byte);
		//}

		//COMSTAT comStat;
		//DWORD   dwErrors;
		//int bytesToRead = ClearCommError(hPort, &dwErrors, &comStat);

		//if (bytesToRead)
		//{
		//	ReadFile(hPort, &byte, 1, &dwBytesTransferred, 0);
		//	printf("There are %i bytes to read", bytesToRead);
		//	printf("Got: %c\n", byte);
		//}
		*/

	if (byte == NULL)
		return;


}

void CloseSerial()
{
	CloseHandle(hPort);
	CloseHandle(osReader.hEvent);
}
#endif

// Main code
int main(int, char**)
{
	micros();
	GUI::Setup(OnGui);

	bool done = false;

	float colors[styleColorNum][4];
	float brightnesses[styleColorNum] = { accentBrightness, fontBrightness };
	if (ReadStyleConfig(colors, brightnesses, selectedFont, fontSize))
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

		selectedFontBak = selectedFont;
		fontSizeBak = fontSize;

		ImGuiIO& io = ImGui::GetIO();

		for (int i = 0; i < io.Fonts->Fonts.Size; i++)
		{
			io.Fonts->Fonts[i]->Scale = fontSize;
		}
		io.FontDefault = io.Fonts->Fonts[fontIndex[selectedFont]];
		auto _boldFont = GetFontBold(selectedFont);
		if (_boldFont != nullptr)
		{
			boldFont = _boldFont;
			boldFontBak = boldFont;
		}
		else
			boldFont = io.Fonts->Fonts[fontIndex[selectedFont]];

		boldFontBak = boldFont;
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

	Serial::FindAvailableSerialPorts();

	//if (Serial::Setup("COM4", GotSerialChar))
	//	printf("Serial Port opened successfuly");
	//else
	//	printf("Error setting up the Serial Port");

	// Main Loop
	while (!done)
	{
		Serial::HandleInput();

		// GUI Loop
		if (micros() - lastFrameGui >= (1 / 72.f) * 1000000)
			if (GUI::DrawGui())
				break;

		lastFrame = micros();

		// Limit FPS for eco-friendly purposes (Significantly affects the performance) (Windows does not support sub 1 millisecond sleep)
		Sleep(1);
	}

	Serial::Close();
	GUI::Destroy();
}
