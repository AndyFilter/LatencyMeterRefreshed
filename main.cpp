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

HANDLE hPort;
DCB serialParams;

std::vector<int> latencyTests;

// -------

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
	ImGuiIO &io = ImGui::GetIO();
	std::string fontDebugName = (std::string)io.Fonts->Fonts[fontIndex[baseFontIndex]]->GetDebugName();
	if (fontDebugName.find('-') == std::string::npos || fontDebugName.find(',') == std::string::npos)
		return nullptr;
	auto fontName = fontDebugName.substr(fontDebugName.find('-'), fontDebugName.find(','));
	if (auto _boldFont = (io.Fonts->Fonts[fontIndex[baseFontIndex + 1]]); ((std::string)(_boldFont->GetDebugName())).find(fontName))
	{
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
	bool colors{false};

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

			if(ImGui::BeginListBox("##Setting", { avail.x * listBoxSize, avail.y }))
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
				memcpy(&colors[1][0], &fontColor, colorSize);
				ApplyStyle(colors, brightnesses);


				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


				//ImGui::PushFont(io.Fonts->Fonts[fontIndex[selectedFont]]);
				// This has to be done manually (instead of ImGui::Combo()) to be able to change the fonts of each selectable to a corresponding one.
				if (ImGui::BeginCombo("Font", fonts[selectedFont]))
				{
					for (int i = 0; i < (io.Fonts->Fonts.Size / 2) + 1; i++)
					{
						auto selectableSpace = ImGui::GetContentRegionAvail();
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2);

						ImGui::PushFont(io.Fonts->Fonts[fontIndex[i]]);

						bool isSelected = (selectedFont == i);
						if (ImGui::Selectable(fonts[i], isSelected, 0, { selectableSpace.x - 4 ,0 }, style.FrameRounding))
						{
							if (selectedFont != i) {
								io.FontDefault = io.Fonts->Fonts[fontIndex[i]];
								if(auto _boldFont = GetFontBold(i); _boldFont != nullptr)
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

	ImGui::Text("Time is: %ims", clock());
	ImGui::Text("Time is: %luus", micros());

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (GUI ONLY)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { avail.x, ImGui::GetFrameHeight() });

	if (ImGui::BeginTable("measurementsTable", 2, ImGuiTableFlags_Reorderable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders, {avail.x / 2, 0}))
	{
		ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
		ImGui::TableSetupColumn("Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 1);
		ImGui::TableHeadersRow();

		ImGui::EndTable();
	}

	lastFrameGui = micros();

	return 0;
}


bool SetupSerialPort(const char COM_Number[1])
{
	std::string serialCom = "\\\\.\\COM";
	serialCom += COM_Number;
	hPort = CreateFile(
		serialCom.c_str(),
		GENERIC_WRITE | GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);

	DCB serialParams;
	serialParams.ByteSize = sizeof(serialParams);

	if (!GetCommState(hPort, &serialParams))
		return false;

	serialParams.BaudRate = 19200;
	serialParams.Parity = NOPARITY;
	serialParams.ByteSize = 8;
	serialParams.StopBits = ONESTOPBIT;

	if (!SetCommState(hPort, &serialParams))
		return false;

	COMMTIMEOUTS timeout = { 0 };
	timeout.ReadIntervalTimeout = 50;
	timeout.ReadTotalTimeoutConstant = 50;
	timeout.ReadTotalTimeoutMultiplier = 50;
	timeout.WriteTotalTimeoutConstant = 50;
	timeout.WriteTotalTimeoutMultiplier = 10;

	if (SetCommTimeouts(hPort, &timeout))
		return false;

	return true;
}

void HandleSerial()
{
	DWORD dwBytesTransferred;
	DWORD dwCommModemStatus{};
	BYTE byte = NULL;

	SetCommMask(hPort, EV_RXCHAR | EV_ERR); //receive character event
	//WaitCommEvent(hPort, &dwCommModemStatus, 0); //wait for character

	//if (dwCommModemStatus & EV_RXCHAR)
	// Does not work.
	ReadFile(hPort, &byte, 1, &dwBytesTransferred, 0); //read 1
	//else if (dwCommModemStatus & EV_ERR)
	//	return;

	if (byte == NULL)
		return;

	printf("Got: %c\n", byte);
}

void CloseSerial()
{
	CloseHandle(hPort);
}

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
		if (auto _boldFont = GetFontBold(fontIndex[selectedFont]); _boldFont != nullptr)
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

	if (SetupSerialPort("4"))
		printf("Serial Port opened successfuly");

	// Main Loop
	while (!done)
	{
		HandleSerial();
		//if ((micros()) - lastFrameGui >= 1/*(1000000/79)*/)
		// GUI Loop
		if (micros() - lastFrameGui >= (1 / 75.f) * 1000000)
			if (GUI::DrawGui())
				break;

		lastFrame = micros();

		// Limit for eco-friendly purposes
		Sleep(1);
	}

	CloseSerial();
	GUI::Destroy();
}
