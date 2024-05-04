#ifdef _WIN32
#include <Windows.h>
#include <tchar.h>
#endif
#include <iostream>
#include <chrono>
#include <fstream>
#include <cstring>

#include "serial.h"
#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_internal.h"
#include "structs.h"
#include "helperJson.h"
#include "constants.h"
#include "gui.h"
#include "Audio/Sound_Helper.h"
#include "External/ImGui/imgui_extensions.h"
#include <algorithm>
#include <unistd.h>
#include "helper.h"

using namespace helper;


// idk why I didn't just expose it in GUI.h...
#ifdef _WIN32
HWND hwnd;
#endif

ImVec2 detectionRectSize{ 0, 200 };

uint64_t lastFrameGui = 0;
uint64_t lastFrame = 0;

// Frametime in microseconds
float averageFrametime = 0;
const unsigned int AVERAGE_FRAME_COUNT = 1000;
uint64_t frames[AVERAGE_FRAME_COUNT]{ 0 };
unsigned long long totalFrames = 0;


// ---- Functionality ----

SerialStatus serialStatus = Status_Idle;

int selectedPort = 0;
int lastSelectedPort = 0;

InputMode selectedMode = InputMode_Normal;

uint64_t lastInternalTest = 0;
bool isAudioDevConnected = false;
std::vector<cAudioDeviceInfo> availableAudioDevices;
#ifdef _WIN32
Waveform_SoundHelper* curAudioDevice;
WinAudio_Player audioPlayer;
const UINT AUDIO_SAMPLE_RATE = 10000;//44100U;
const UINT TEST_AUDIO_BUFFER_SIZE = AUDIO_SAMPLE_RATE / 10;
const UINT WARMUP_AUDIO_BUFFER_SIZE = AUDIO_SAMPLE_RATE / 2;
const UINT AUDIO_BUFFER_SIZE = WARMUP_AUDIO_BUFFER_SIZE + TEST_AUDIO_BUFFER_SIZE;
const UINT INTERNAL_TEST_DELAY = 1195387; // in microseconds
#else
#define AUDIO_DATA_FORMAT int
ALSA_AudioDevice<AUDIO_DATA_FORMAT>* curAudioDevice;
UINT AUDIO_SAMPLE_RATE = 44100;//44100;//44100U;
UINT TEST_AUDIO_BUFFER_SIZE = AUDIO_SAMPLE_RATE / 4;
const UINT WARMUP_AUDIO_BUFFER_SIZE = AUDIO_SAMPLE_RATE / 2; // On Unix this isn't really a "buffer", but just a delay
UINT AUDIO_BUFFER_SIZE = TEST_AUDIO_BUFFER_SIZE;
const UINT INTERNAL_TEST_DELAY = 995387; // in microseconds
#endif

std::chrono::steady_clock::time_point moddedMouseTimeClicked;
bool wasModdedMouseTimeUpdated = false;


// --------


//bool isSaved = true;
//char savePath[MAX_PATH]{ 0 };

bool isExiting = false;
std::vector<int> unsavedTabs;


bool isGameMode = false;
bool isInternalMode = false;
bool isAudioMode = false;
bool wasLMB_Pressed = false;
bool wasMouseClickSent = false;

bool wasMeasurementAddedGUI = false;

bool shouldRunConfiguration = false;

float leftTabWidth = 400;

// Forward declarations
void GotSerialChar(char c);
void ClearData();
void DiscoverAudioDevices();
void StartInternalTest();
void AddMeasurement(UINT internalTime, UINT externalTime, UINT inputTime);
bool CanDoInternalTest(uint64_t time = 0);
bool SetupAudioDevice();
void AttemptConnect();
void AttemptDisconnect();


/*
TODO:

+ Add multiple fonts to chose from.
- Add a way to change background color (?)
+ Add the functionality...
+ Saving measurement records to a json file.
+ Open a window to select which save file you want to open
+ Better fullscreen implementation (ESCAPE to exit etc.)
+ Max Gui fps slider in settings
+ Load the font in earlier selected size (?)
+ Movable black Square for color detection (?) Check for any additional latency this might involve
+ Clear / Reset button
+ Game integration. Please don't get me banned again (needs testing) Update: (testing done)
+ Fix overwriting saves
+ Unsaved work notification
+ Unsaved tabs alert
- Custom Fonts (?)
+ Multiple tabs for measurements
+ Move IO to a separate thread (ONLY if it doesn't introduce additional latency)
+ Fix measurement index when removing (or don't, I don' know)
+ Reset status on disconnect and clear
+ Auto scroll measurements on new added
+ Changing tab name sets the status to unsaved
+ Fix too many tabs obscuring tabs beneath them
+ Check if ANY tab is unsaved when closing
+ Pack Save (Save all the tabs into a single file) (?)
+ Configuration Screen
+ Pack fonts into a byte array and load them that way.
- Add option to display frametime (?)
- See what can be done about the inefficient Serial Comm code
- Inform user of all the available keyboard shortcuts
- Clean-up the audio code
- Handle unplugging audio devices mid test
- Arduino debug mode (print sensor values etc.)
- Overall better communication with Arduino
- A way to change the Audio Latency "beep" wave function / frequency

*/

bool isSettingOpen = false;

#ifdef _WIN32
bool AnalyzeData()
{
	UINT iter_offset = 00;
	const UINT BUFFER_MAX_VALUE = (1 << (sizeof(short) * 8)) / 2 - 1;
	const int BUFFER_THRESHOLD = BUFFER_MAX_VALUE / 5;

	//serialStatus = Status_Idle;

	int baseAvg = 0;
	const short AvgCount = 10;
	//const int REMAINDER_COUNTDOWN_VALUE = 10;
	//int isBufferRemainder = 0; // Value at which last measurement ended gets carried to the current buffer, we have to deal with it.

	for (int i = iter_offset; i < TEST_AUDIO_BUFFER_SIZE; i++)
	{
		if (i - iter_offset < AvgCount) {
			baseAvg += curAudioDevice->recordBuffer[i];
			continue;
		}
		else if (i - iter_offset == AvgCount) {
			baseAvg /= AvgCount;
			baseAvg = abs(baseAvg);
			//if (baseAvg > BUFFER_THRESHOLD)
			//	isBufferRemainder = REMAINDER_COUNTDOWN_VALUE;
		}

		//if (isBufferRemainder && abs(curAudioDevice->recordBuffer[i]) < BUFFER_THRESHOLD) {
		//	isBufferRemainder--;

		//	if (!isBufferRemainder) {
		//		iter_offset = i - REMAINDER_COUNTDOWN_VALUE;
		//		baseAvg = 0;
		//	}
		//	continue;
		//}

		short sample = abs(curAudioDevice->recordBuffer[i] - baseAvg);

		if (sample >= BUFFER_THRESHOLD / (isAudioMode ? 4 : 1))
		{
			float microsElapsed = (1000000 * (i - iter_offset)) / AUDIO_SAMPLE_RATE;
#ifdef _DEBUG
			std::cout << "latency: " << microsElapsed << ", base val: " << baseAvg << std::endl;
#endif // _DEBUG
			if (baseAvg > BUFFER_MAX_VALUE * 0.9f || microsElapsed <= 1000)
				return false;
			AddMeasurement(microsElapsed, 0, 0);
			return true;
		}
	}
}

// Callback function called by waveInOpen
//HWAVEIN, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR
void CALLBACK waveInCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	switch (uMsg)
	{
	case WIM_DATA:
	{
		////if(serialStatus != Status_Idle)
		////	curAudioDevice->SwapToNextBuffer();
		//serialStatus = Status_Idle;
		////((Waveform_SoundHelper*)dwInstance)->GetPlayTime();
		////short sample = curAudioDevice->GetPlayTime().u.sample;
		////detection_Color = ImColor(0.f, 0.f, 0.f, 1.f);
		////printf("new data\n");
		//AnalyzeData();

		
		static bool swapped_buffers = false;
		static bool reverted_buffers = false;
		if (serialStatus != Status_Idle && !swapped_buffers) {

			swapped_buffers = true;
			reverted_buffers = false;
			curAudioDevice->SwapToNextBuffer();
			if (isAudioMode)
				audioPlayer.Play();
			serialStatus = Status_WaitingForResult;
			break;
		}
		swapped_buffers = false;
		if (!reverted_buffers) {
			reverted_buffers = true;
			AnalyzeData();
			serialStatus = Status_Idle;
			curAudioDevice->SwapToNextBuffer();
		}
		//printf("new data\n");
	}
	break;
	default:
		break;
	}
}

#else

// Same function as on Windows
bool AnalyzeData() {
    UINT iter_offset = 0;
    const size_t BUFFER_MAX_VALUE = (1ull << (sizeof(AUDIO_DATA_FORMAT) * 8)) / 2 - 1;
    const size_t BUFFER_THRESHOLD = BUFFER_MAX_VALUE / 5;

    //serialStatus = Status_Idle;

    int baseAvg = 0;
    const short AvgCount = 10;
    //const int REMAINDER_COUNTDOWN_VALUE = 10;
    //int isBufferRemainder = 0; // Value at which last measurement ended gets carried to the current buffer, we have to deal with it.

    for (int i = iter_offset; i < TEST_AUDIO_BUFFER_SIZE; i+=2)
    {
        // try to account for the base value in the buffer
        if (i - iter_offset < AvgCount) {
            baseAvg += curAudioDevice->recordBuffer[i];
            continue;
        }
        else if (i - iter_offset == AvgCount) {
            baseAvg /= AvgCount;
            baseAvg = abs(baseAvg);

            if(baseAvg > BUFFER_MAX_VALUE / 3) {
                // BAD DATA! Possibly bring the volume down to fix it
                return false;
            }
        }


        AUDIO_DATA_FORMAT sample = abs(curAudioDevice->recordBuffer[i] - baseAvg);

        if (sample >= BUFFER_THRESHOLD / (isAudioMode ? 4 : 1))
        {
            float microsElapsed = (1000000 * (i / 2 - iter_offset)) / AUDIO_SAMPLE_RATE;
#ifdef _DEBUG
            std::cout << "latency: " << microsElapsed << ", base val: " << baseAvg << ", at sample: " << i << std::endl;
#endif // _DEBUG
            // Could add a popup to notify user after a couple of failed attempts that smth is prolly wrong...
            if (baseAvg > BUFFER_MAX_VALUE * 0.9f || microsElapsed <= 1000)
                return false;
            AddMeasurement(microsElapsed, 0, 0);
            return true;
        }
    }
    return false;
}

#endif

static int FilterValidPath(ImGuiInputTextCallbackData* data)
{
	if (std::find(std::begin(invalidPathCharacters), std::end(invalidPathCharacters), data->EventChar) != std::end(invalidPathCharacters))  return 1;
	return 0;
}

void ClearData()
{
	serialStatus = Status_Idle;
	tabsInfo[selectedTab].latencyData.measurements.clear();
	tabsInfo[selectedTab].latencyStats = LatencyStats();
	tabsInfo[selectedTab].isSaved = true;
	ZeroMemory(tabsInfo[selectedTab].latencyData.note, 1000);
}

// Not the best GUI solution, but it's simple, fast and gets the job done.
int OnGui()
{
	ImGuiStyle& style = ImGui::GetStyle();

	if (shouldRunConfiguration)
	{
		ImGui::OpenPopup("Set Up");
		shouldRunConfiguration = false;
	}

	if (ImGui::BeginMenuBar())
	{
		short openFullscreenCommand = 0;
        bool badVerPopupOpen = false;
		//static bool saveMeasurementsPopup = false;
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open", "Ctrl+O")) {
                auto res = OpenMeasurements();
                if(res & JSON_HANDLE_INFO_BAD_VERSION) {
                    badVerPopupOpen = true;
                    //ImGui::OpenPopup("JSON version mismatch!");
                    //printf("BAD VERSION!\n");
                }
            }
			if (ImGui::MenuItem("Save", "Ctrl+S")) { SaveMeasurements(); }
			if (ImGui::MenuItem("Save as", "Ctrl+Shift+S")) { SaveAsMeasurements(); }
			if (ImGui::MenuItem("Save Pack", "Alt+S")) { SavePackMeasurements(); }
			if (ImGui::MenuItem("Save Pack As", "Alt+Shift+S")) { SavePackAsMeasurements(); }
			if (ImGui::MenuItem("Fullscreen", "Alt+Enter")) {
                //GUI::SetFullScreenState(isFullscreen);

				if (!isFullscreen && !fullscreenModeOpenPopup)
					openFullscreenCommand = 1;
				else if (!fullscreenModeClosePopup && !fullscreenModeOpenPopup)
					openFullscreenCommand = -1;
			}
			if (ImGui::MenuItem("Settings", "")) { isSettingOpen = !isSettingOpen; }
			if (ImGui::MenuItem("Close", "")) { return 1; }
			ImGui::EndMenu();
		}

		// Draw FPS
		{
			auto menuBarAvail = ImGui::GetContentRegionAvail();

			const float width = ImGui::CalcTextSize("FPS12345FPS    FPS123456789FPS").x;

			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (menuBarAvail.x - width));
			ImGui::BeginGroup();

			ImGui::Text("FPS:");
			ImGui::SameLine();
			ImGui::Text("%.1f GUI", ImGui::GetIO().Framerate);
			ImGui::SameLine();
			// Right click total frames to reset it
			ImGui::Text(" %.1f CPU", 1000000.f / averageFrametime);
			static uint64_t totalFramerateStartTime{ micros() };
			// The framerate is sometimes just too high to be properly displayed by the moving average and it's "small" buffer. So this should show average framerate over the entire program lifespan
			TOOLTIP("Avg. framerate: %.1f", ((float)totalFrames / (micros() - totalFramerateStartTime)) * 1000000.f);
			if (ImGui::IsItemClicked(1))
			{
				totalFramerateStartTime = micros();
				totalFrames = 0;
			}

			ImGui::EndGroup();
		}
		ImGui::EndMenuBar();

		if (openFullscreenCommand == 1) {
			ImGui::OpenPopup("Enter Fullscreen mode?");
			fullscreenModeOpenPopup = true;
		}
		else if (openFullscreenCommand == -1) {
			ImGui::OpenPopup("Exit Fullscreen mode?");
			fullscreenModeClosePopup = true;
		}

        if(badVerPopupOpen)
            ImGui::OpenPopup("JSON version mismatch!");
	}

    if (ImGui::BeginPopupModal("JSON version mismatch!", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
    {
        ImGui::Text("Some of the imported files are either from an older or newer version of the program\nThe data might be corrupted or wrong!");
        if (ImGui::Button("Ok", {-1, 0}))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }


	const auto windowAvail = ImGui::GetContentRegionAvail();
	ImGuiIO& io = ImGui::GetIO();
    uint64_t curTime = micros();

	if (serialStatus != Status_Idle && curTime > (lastInternalTest + SERIAL_NO_RESPONSE_DELAY))
	{
		serialStatus = Status_Idle;
	}


	// Handle Shortcuts
	ImGuiKey pressedKeys[ImGuiKey_COUNT]{static_cast<ImGuiKey>(0)};
	size_t addedKeys = 0;

	//ZeroMemory(pressedKeys, ImGuiKey_COUNT);

	for (auto key = static_cast<ImGuiKey>(0); key < ImGuiKey_COUNT; (*(int*)&key)++)
	{
		bool isPressed = ImGui::IsKeyPressed(key, false);

		// Shift, ctrl and alt
		if ((key >= 641 && key <= 643) || (key >= 527 && key <= 533))
			continue;

		if (ImGui::IsLegacyKey(key))
			continue;
		
		if (isPressed) {
			pressedKeys[addedKeys++] = key;
		}
	}

	static bool wasEscapeUp{ true };

	if (addedKeys == 1)
	{
		//const char* name = ImGui::GetKeyName(pressedKeys[0]);
		if (io.KeyCtrl)
		{
			// Moved to the Main function
			//io.ClearInputKeys();
			//if (pressedKeys[0] == ImGuiKey_S)
			//{
			//	printf("Save intent\n");
			//	SaveMeasurements();
			//}
			//if (pressedKeys[0] == ImGuiKey_O)
			//{
			//	OpenMeasurements();
			//}
			//else if (pressedKeys[0] == ImGuiKey_N)
			//{
			//	auto newTab = TabInfo();
			//	strcat_s<TAB_NAME_MAX_SIZE>(newTab.name, std::to_string(tabsInfo.size()).c_str());
			//	tabsInfo.push_back(newTab);
			//}
			// Doesn't work yet!
			if (pressedKeys[0] == ImGuiKey_W)
			{
				char tabClosePopupName[48];
				snprintf(tabClosePopupName, 48, "Save before Closing?###TabExit%i", selectedTab);

				ImGui::OpenPopup(tabClosePopupName);
			}
		}
		else if (io.KeyAlt)
		{
			//io.ClearInputKeys();

			// Going fullscreen popup currently with a small issue

			// Enter
			if (pressedKeys[0] == ImGuiKey_Enter || pressedKeys[0] == ImGuiKey_KeypadEnter)
			{
				//GUI::g_pSwapChain->GetFullscreenState((BOOL*)&isFullscreen, nullptr);
                isFullscreen = GUI::GetFullScreenState();
				if (!isFullscreen && !fullscreenModeOpenPopup)
				{
					ImGui::OpenPopup("Enter Fullscreen mode?");
					fullscreenModeOpenPopup = true;
				}
				else if (!fullscreenModeClosePopup && !fullscreenModeOpenPopup)
				{
					ImGui::OpenPopup("Exit Fullscreen mode?");
					fullscreenModeClosePopup = true;
				}
			}
		}
		else if (!io.KeyShift && pressedKeys[0] == ImGuiKey_Escape)
		{
			bool isFS = GUI::GetFullScreenState();;
			//GUI::g_pSwapChain->GetFullscreenState(&isFS, nullptr);
			isFullscreen = isFS;
			if (isFullscreen && !fullscreenModeClosePopup && wasEscapeUp)
			{
				wasEscapeUp = false;
				ImGui::OpenPopup("Exit Fullscreen mode?");
				fullscreenModeClosePopup = true;
				wasEscapeUp = false;
			}

		}
	}

	if (ImGui::BeginPopupModal("Enter Fullscreen mode?", &fullscreenModeOpenPopup, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to enter Fullscreen mode?");
		TOOLTIP("Press Escape to Exit");
		ImGui::SeparatorSpace(0, { 0, 10 });
		if (ImGui::Button("Yes") || (IS_ONLY_ENTER_PRESSED && !io.KeyAlt))
		{
			// I'm pretty sure I should also resize the swapchain buffer to use the new resolution, but maybe later. It looks kind of goofy on displays with odd resolution :shrug:
			//GUI::g_pSwapChain->ResizeBuffers(0, 1080, 1920, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
			//UINT modesNum = 256;
			//DXGI_MODE_DESC monitorModes[256];
			//GUI::vOutputs[1]->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &modesNum, monitorModes);
			//DXGI_MODE_DESC mode = monitorModes[modesNum - 1];
			//GUI::g_pSwapChain->ResizeTarget(&mode);
			//GUI::g_pSwapChain->ResizeBuffers(0, 1080, 1920, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
			if (GUI::SetFullScreenState(true) == 0)
				isFullscreen = true;
			else
				isFullscreen = false;

			ImGui::CloseCurrentPopup();
			fullscreenModeOpenPopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("No") || IS_ONLY_ESCAPE_PRESSED)
		{
			ImGui::CloseCurrentPopup();
			fullscreenModeOpenPopup = false;
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Exit Fullscreen mode?", &fullscreenModeClosePopup, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to exit Fullscreen mode?");
		ImGui::SeparatorSpace(0, { 0, 10 });

		// ImGui doesn't yet support focusing a button

		//ImGui::PushAllowKeyboardFocus(true);
		//ImGui::PushID("ExitFSYes");
		//ImGui::SetKeyboardFocusHere();
		//bool isYesDown = ImGui::Button("Yes");
		//ImGui::PopID();
		////ImGui::SetKeyboardFocusHere(-1);
		//auto curWindow = ImGui::GetCurrentWindow();
		//ImGui::PopAllowKeyboardFocus();
		//ImGui::SetFocusID(curWindow->GetID("ExitFSYes"), curWindow);

		if (ImGui::Button("Yes") || IS_ONLY_ENTER_PRESSED)
		{
            if (GUI::SetFullScreenState(false) == 0)
				isFullscreen = false;
            else
                isFullscreen = true;

			ImGui::CloseCurrentPopup();
			fullscreenModeClosePopup = false;
		}
		ImGui::SameLine();
		if (ImGui::Button("No") || (IS_ONLY_ESCAPE_PRESSED && wasEscapeUp))
		{
			wasEscapeUp = false;
			ImGui::CloseCurrentPopup();
			fullscreenModeClosePopup = false;
		}
		ImGui::EndPopup();
	}

	if (ImGui::IsKeyReleased(ImGuiKey_Escape))
		wasEscapeUp = true;

	// following way might be better, but it requires a use of some weird values like 0xF for CTRL+S
	//if (io.InputQueueCharacters.size() == 1)
	//{
	//	ImWchar c = io.InputQueueCharacters[0];
	//	if (((c > ' ' && c <= 255) ? (char)c : '?' == 's') && io.KeyCtrl)
	//	{
	//		SaveMeasurements();
	//	}

	//}


	if (isSettingOpen)
	{
		ImGui::SetNextWindowSize({ 480.0f, 480.0f });

		bool wasLastSettingsOpen = isSettingOpen;
		if (ImGui::Begin("Settings", &isSettingOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
		{
			// On Settings Window Closed:
			if (wasLastSettingsOpen && !isSettingOpen && HasConfigChanged())
			{
				// Call a popup by name
				ImGui::OpenPopup("Save Style?");
			}

			// Define the messagebox popup
			if (ImGui::BeginPopupModal("Save Style?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
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
					SaveCurrentUserConfig();
					isSettingOpen = false;
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Discard"))
				{
					RevertConfig();
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

			const char* items[3]{ "Style", "Performance", "Misc" };

			// Makes list take ({listBoxSize}*100)% width of the parent
			float listBoxSize = 0.3f;

			if (ImGui::BeginListBox("##Setting", { avail.x * listBoxSize, avail.y }))
			{
				for (int i = 0; i < sizeof(items) / sizeof(items[0]); i++)
				{
					//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (style.ItemSpacing.x / 2) - 2);
					//ImVec2 label_size = ImGui::CalcTextSize(items[i], NULL, true);
					bool isSelected = (selectedSettings == i);
					//if (ImGui::Selectable(items[i], isSelected, 0, { (avail.x * listBoxSize - (style.ItemSpacing.x) - (style.FramePadding.x * 2)) + 4, label_size.y }, style.FrameRounding))
					if (ImGui::Selectable(items[i], isSelected, 0, { 0, 0 }))
					{
						selectedSettings = i;
					}
				}
				ImGui::EndListBox();
			}

			ImGui::SameLine();

			ImGui::BeginGroup();

			switch (selectedSettings)
			{
			case 0: // Style
			{
				ImGui::BeginChild("Style", { ((1 - listBoxSize) * avail.x) - style.FramePadding.x * 2, avail.y - ImGui::GetFrameHeightWithSpacing() }, true);

				ImGui::ColorEdit4("Main Color", currentUserData.style.mainColor);

				ImGui::PushID(02);
				ImGui::SliderFloat("Brightness", &currentUserData.style.mainColorBrightness, -0.5f, 0.5f);
				REVERT(&currentUserData.style.mainColorBrightness, backupUserData.style.mainColorBrightness);
				ImGui::PopID();

				auto _accentColor = ImVec4(*(ImVec4*)currentUserData.style.mainColor);
				auto darkerAccent = ImVec4(*(ImVec4*)&_accentColor) * (1 - currentUserData.style.mainColorBrightness);
				auto darkestAccent = ImVec4(*(ImVec4*)&_accentColor) * (1 - currentUserData.style.mainColorBrightness * 2);

				// Alpha has to be set separatly, because it also gets multiplied times brightness.
				auto alphaAccent = ImVec4(*(ImVec4*)currentUserData.style.mainColor).w;
				_accentColor.w = alphaAccent;
				darkerAccent.w = alphaAccent;
				darkestAccent.w = alphaAccent;

				ImGui::Dummy({ 0, ImGui::GetFrameHeight() / 2 });

				ImVec2 colorsAvail = ImGui::GetContentRegionAvail();

				ImGui::ColorButton("Accent Color", (ImVec4)_accentColor, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
				ImGui::ColorButton("Accent Color Dark", darkerAccent, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
				ImGui::ColorButton("Accent Color Darkest", darkestAccent, 0, { colorsAvail.x, ImGui::GetFrameHeight() });


				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


				ImGui::ColorEdit4("Font Color", currentUserData.style.fontColor);

				ImGui::PushID(12);
				ImGui::SliderFloat("Brightness", &currentUserData.style.fontColorBrightness, -1.0f, 1.0f, "%.3f");
				REVERT(&currentUserData.style.fontColorBrightness, backupUserData.style.fontColorBrightness);
				ImGui::PopID();

				auto _fontColor = ImVec4(*(ImVec4*)currentUserData.style.fontColor);
				auto darkerFont = ImVec4(*(ImVec4*)&_fontColor) * (1 - currentUserData.style.fontColorBrightness);

				// Alpha has to be set separatly, because it also gets multiplied times brightness.
				auto alphaFont = ImVec4(*(ImVec4*)currentUserData.style.fontColor).w;
				_fontColor.w = alphaFont;
				darkerFont.w = alphaFont;

				ImGui::Dummy({ 0, ImGui::GetFrameHeight() / 2 });

				ImGui::ColorButton("Font Color", _fontColor, 0, { colorsAvail.x, ImGui::GetFrameHeight() });
				ImGui::ColorButton("Font Color Dark", darkerFont, 0, { colorsAvail.x, ImGui::GetFrameHeight() });

				//ImGui::SetCursorPosY(avail.y - ImGui::GetFrameHeight() - style.WindowPadding.y);

				//float colors[2][4];
				//float brightnesses[2]{ accentBrightness, fontBrightness };
				//memcpy(&colors, &accentColor, colorSize);
				//memcpy(&colors[1], &fontColor, colorSize);
				//ApplyStyle(colors, brightnesses);

				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


				ImGui::ColorEdit4("Int. Plot", currentUserData.style.internalPlotColor);
				TOOLTIP("Internal plot color");
				ImGui::ColorEdit4("Ext. Plot", currentUserData.style.externalPlotColor);
				TOOLTIP("External plot color");
				ImGui::ColorEdit4("Input Plot", currentUserData.style.inputPlotColor);
				TOOLTIP("Input plot color");

				ApplyCurrentStyle();


				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { colorsAvail.x * 0.9f, ImGui::GetFrameHeight() });


				//ImGui::PushFont(io.Fonts->Fonts[fontIndex[selectedFont]]);
				// This has to be done manually (instead of ImGui::Combo()) to be able to change the fonts of each selectable to a corresponding one.
				if (ImGui::BeginCombo("Font", fonts[currentUserData.style.selectedFont]))
				{
					for (int i = 0; i < ((io.Fonts->Fonts.Size - 1) / 2) + 1; i++)
					{
						//auto selectableSpace = ImGui::GetContentRegionAvail();
						//ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 2);

						ImGui::PushFont(io.Fonts->Fonts[fontIndex[i]]);

						bool isSelected = (currentUserData.style.selectedFont == i);
						if (ImGui::Selectable(fonts[i], isSelected, 0, { 0, 0 }))
						{
							if (currentUserData.style.selectedFont != i) {
								io.FontDefault = io.Fonts->Fonts[fontIndex[i]];
								if (auto _boldFont = GetFontBold(i); _boldFont != nullptr)
									boldFont = _boldFont;
								else
									boldFont = io.Fonts->Fonts[fontIndex[i]];
								currentUserData.style.selectedFont = i;
							}
						}
						ImGui::PopFont();
					}
					ImGui::EndCombo();
				}
				//ImGui::PopFont();

				bool hasFontSizeChanged = ImGui::SliderFloat("Font Size", &currentUserData.style.fontSize, 0.5f, 2, "%.2f");
				REVERT(&currentUserData.style.fontSize, backupUserData.style.fontSize);
				if (hasFontSizeChanged || ImGui::IsItemClicked(1))
				{
					for (int i = 0; i < io.Fonts->Fonts.Size; i++)
					{
						io.Fonts->Fonts[i]->Scale = currentUserData.style.fontSize;
					}
				}


				ImGui::EndChild();

				break;
			}
			// Performance
			case 1:
				if (ImGui::BeginChild("Performance", { ((1 - listBoxSize) * avail.x) - style.FramePadding.x * 2, avail.y - ImGui::GetFrameHeightWithSpacing() }, true))
				{
					auto performanceAvail = ImGui::GetContentRegionAvail();
					ImGui::Checkbox("###LockGuiFpsCB", &currentUserData.performance.lockGuiFps);
					TOOLTIP("It's strongly recommended to keep GUI FPS locked for the best performance");

					ImGui::BeginDisabled(!currentUserData.performance.lockGuiFps);
					ImGui::SameLine();
					ImGui::PushItemWidth(performanceAvail.x - ImGui::GetItemRectSize().x - style.FramePadding.x * 3 - ImGui::CalcTextSize("GUI Refresh Rate").x);
					//ImGui::SliderInt("GUI FPS", &guiLockedFps, 30, 360, "%.1f");
					ImGui::DragInt("GUI Refresh Rate", &currentUserData.performance.guiLockedFps, .5f, 30, 480);
					REVERT(&currentUserData.performance.guiLockedFps, backupUserData.performance.guiLockedFps);
					ImGui::PopItemWidth();
					ImGui::EndDisabled();

					ImGui::Spacing();

					ImGui::Checkbox("Show Plots", &currentUserData.performance.showPlots);
					TOOLTIP("Plots can have a small impact on the performance");

					ImGui::Spacing();

					//ImGui::Checkbox("VSync", &currentUserData.performance.VSync);
					ImGui::SliderInt("VSync", &(int&)currentUserData.performance.VSync, 0, 4, "%d", ImGuiSliderFlags_AlwaysClamp);
					TOOLTIP("This setting synchronizes your monitor with the program to avoid screen tearing.\n(Adds a significant amount of latency)");

					ImGui::EndChild();
				}

				break;

				// Misc
			case 2:
				if ((ImGui::BeginChild("Misc", { ((1 - listBoxSize) * avail.x) - style.FramePadding.x * 2, avail.y - ImGui::GetFrameHeightWithSpacing() }, true)))
				{
					ImGui::Checkbox("Save config locally", &currentUserData.misc.localUserData);
#ifdef _WIN32
					TOOLTIP("Chose whether you want to save config file in the same directory as the program, or in the AppData folder");
#else
                    TOOLTIP("Chose whether you want to save config file in the same directory as the program, or in the /home/{usr}/.LatencyMeter folder");
#endif

					ImGui::EndChild();
				}

				break;
			}

			ImGui::BeginGroup();

			auto buttonsAvail = ImGui::GetContentRegionAvail();

			if (ImGui::Button("Revert", { (buttonsAvail.x / 2 - style.FramePadding.x), ImGui::GetFrameHeight() }))
			{
				RevertConfig();
			}

			ImGui::SameLine();

			if (ImGui::Button("Save", { (buttonsAvail.x / 2 - style.FramePadding.x), ImGui::GetFrameHeight() }))
			{
				SaveCurrentUserConfig();
				//SaveStyleConfig(colors, brightnesses, selectedFont, fontSize);
			}

			ImGui::EndGroup();

			ImGui::EndGroup();
		}
		ImGui::End();
	}


	// TABS

	int deletedTabIndex = -1;

	if (ImGui::BeginTabBar("TestsTab", ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_FittingPolicyScroll))
	{

		for (size_t tabN = 0; tabN < tabsInfo.size(); tabN++)
		{
			//char idText[24];
			//snprintf(idText, 24, "Tab Item %i", tabN);
			//ImGui::PushID(idText);

			char tabNameLabel[48];
			snprintf(tabNameLabel, 48, "%s###Tab%i", tabsInfo[tabN].name, tabN);

			//ImGui::PushItemWidth(ImGui::CalcTextSize(tabNameLabel).x + 100);
			//if(!tabsInfo[tabN].isSaved)
			//	ImGui::SetNextItemWidth(ImGui::CalcTextSize(tabsInfo[tabN].name, NULL).x + (style.FramePadding.x * 2) + 15);
			//else
			ImGui::SetNextItemWidth(ImGui::CalcTextSize(tabsInfo[tabN].name, NULL).x + (style.FramePadding.x * 2) + (tabsInfo[tabN].isSaved ? 0 : 15));
			//selectedTab = tabsInfo.size() - 1;
			auto flags = selectedTab == tabN ? ImGuiTabItemFlags_SetSelected : 0;
			flags |= tabsInfo[tabN].isSaved ? 0 : ImGuiTabItemFlags_UnsavedDocument;
			if (ImGui::BeginTabItem(tabNameLabel, NULL, flags))
			{
				ImGui::EndTabItem();
			}
			//ImGui::PopItemWidth();
			//ImGui::PopID();

			if (ImGui::IsItemHovered())
			{
				if (ImGui::IsMouseDown(0))
				{
					selectedTab = tabN;
				}

				auto mouseDelta = io.MouseWheel;
				selectedTab += mouseDelta;
				selectedTab = std::clamp(selectedTab, 0, (int)tabsInfo.size() - 1);
			}

			char popupName[32]{ 0 };
			snprintf(popupName, 32, "Change tab %i name", tabN);

			char popupTextEdit[32]{ 0 };
			snprintf(popupTextEdit, 32, "Ed Txt %i", tabN);

			char tabClosePopupName[48];
			snprintf(tabClosePopupName, 48, "Save before Closing?###TabExit%i", tabN);

			static bool tabExitOpen = true;
			bool openTabClose = false;

			if (ImGui::IsItemFocused())
			{
				if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
				{
#ifdef _DEBUG
					printf("deleting tab: %i\n", tabN);
#endif
					if (tabsInfo.size() > 1)
						if (!tabsInfo[tabN].isSaved)
						{
							ImGui::OpenPopup(tabClosePopupName);
							tabExitOpen = true;
						}
						else
						{
							deletedTabIndex = tabN;
							if (tabN == tabsInfo.size() - 1)
							{
								ImGuiContext& g = *GImGui;
								ImGui::SetFocusID(g.CurrentTabBar->Tabs[tabN-1].ID, ImGui::GetCurrentWindow());
							}
							// Unfocus so the next items won't get deleted (This can also be achieved by checking if DEL was not down)
							//ImGui::SetWindowFocus(nullptr);
						}
				}

				else if (ImGui::IsKeyDown(ImGuiKey_F2))
				{
					ImGui::OpenPopup(popupTextEdit);
				}

				if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
					selectedTab--;

				if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
					selectedTab++;

				if (io.KeyCtrl)
				{
					if (pressedKeys[0] == ImGuiKey_W)
						if (tabsInfo.size() > 1)
							if (!tabsInfo[tabN].isSaved)
							{
								ImGui::OpenPopup(tabClosePopupName);
								tabExitOpen = true;
							}
							else
							{
								deletedTabIndex = tabN;
								if (tabN == tabsInfo.size() - 1)
								{
									ImGuiContext& g = *GImGui;
									ImGui::SetFocusID(g.CurrentTabBar->Tabs[tabN - 1].ID, ImGui::GetCurrentWindow());
								}
							}
				}

				selectedTab = std::clamp(selectedTab, 0, (int)tabsInfo.size() - 1);
			}

			if (ImGui::BeginPopup(popupTextEdit, ImGuiWindowFlags_NoMove))
			{
				ImGui::SetKeyboardFocusHere(0);
				if (ImGui::InputText("Tab name", tabsInfo[tabN].name, TAB_NAME_MAX_SIZE, ImGuiInputTextFlags_AutoSelectAll))
				{
					char defaultTabName[8]{ 0 };
					snprintf(defaultTabName, 8, "Tab %i", tabN);
					tabsInfo[tabN].isSaved = !strcmp(defaultTabName, tabsInfo[tabN].name);
				}
				ImGui::EndPopup();
			}

			if (ImGui::IsItemClicked(1) || (ImGui::IsItemHovered() && (ImGui::IsMouseDoubleClicked(0))))
			{
#ifdef _DEBUG
				printf("Open tab %i tooltip\n", tabN);
#endif
				ImGui::OpenPopup(popupName);
			}

			if (ImGui::BeginPopup(popupName, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
			{
				// Check if this name already exists
				//ImGui::SetKeyboardFocusHere(0);
				if (ImGui::InputText("Tab name", tabsInfo[tabN].name, TAB_NAME_MAX_SIZE, ImGuiInputTextFlags_AutoSelectAll))
				{
					char defaultTabName[8]{ 0 };
					snprintf(defaultTabName, 8, "Tab %i", tabN);
					tabsInfo[tabN].isSaved = !strcmp(defaultTabName, tabsInfo[tabN].name);
				}

				if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Escape))
					ImGui::CloseCurrentPopup();

				//ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 4);
				if (tabsInfo.size() > 1) {
					if (ImGui::Button("Close Tab", { -1, 0 }))
					{
						if (!tabsInfo[tabN].isSaved)
						{
							ImGui::CloseCurrentPopup();
							//ImGui::OpenPopup(tabClosePopupName);
							openTabClose = true;
						}
						else
						{
							deletedTabIndex = tabN;
							ImGui::CloseCurrentPopup();
						}
					}
				}

				ImGui::EndPopup();
			}

			if (openTabClose)
			{
				tabExitOpen = true;
				ImGui::OpenPopup(tabClosePopupName);
			}

			if (ImGui::BeginPopupModal(tabClosePopupName, &tabExitOpen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings))
			{
				ImGui::Text("Are you sure you want to close this tab?\nAll measurements will be lost if you don't save");

				ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 10 });

				auto popupAvail = ImGui::GetContentRegionAvail();

				popupAvail.x -= style.ItemSpacing.x * 2;

				if (ImGui::Button("Save", { popupAvail.x / 3, 0 }))
				{
					if (SaveMeasurements())
						deletedTabIndex = tabN;
					ImGui::CloseCurrentPopup();
					tabExitOpen = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Discard", { popupAvail.x / 3, 0 }))
				{
					deletedTabIndex = tabN;
					ImGui::CloseCurrentPopup();
					tabExitOpen = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel", { popupAvail.x / 3, 0 }))
				{
					ImGui::CloseCurrentPopup();
					tabExitOpen = false;
				}

				ImGui::EndPopup();
			}
		}


		if (ImGui::TabItemButton(" + ", ImGuiTabItemFlags_Trailing))
		{
			auto newTab = TabInfo();
#ifdef _WIN32
			strcat_s<TAB_NAME_MAX_SIZE>(newTab.name, std::to_string(tabsInfo.size()).c_str());
#else
            strcat(newTab.name, std::to_string(tabsInfo.size()).c_str());
#endif
			tabsInfo.push_back(newTab);
			selectedTab = tabsInfo.size() - 1;

			//ImGui::EndTabItem(); 
		}

		//ImGui::Text("Selected Item %i", selectedTab);

		ImGui::EndTabBar();
	}

	if (ImGui::IsItemHovered())
	{
		auto mouseDelta = io.MouseWheel;
		selectedTab += mouseDelta;
		selectedTab = std::clamp(selectedTab, 0, (int)tabsInfo.size() - 1);
	}


	if (deletedTabIndex >= 0)
	{
		tabsInfo.erase(tabsInfo.begin() + deletedTabIndex);
		if(selectedTab >= deletedTabIndex)
			selectedTab = _max(selectedTab - 1, 0);
		//char tabNameLabel[48];
		//snprintf(tabNameLabel, 48, "%s###Tab%i", tabsInfo[selectedTab].name, selectedTab);

		//ImGui::SetFocusID(ImGui::GetID(tabNameLabel), ImGui::GetCurrentWindow());
	}

	const auto avail = ImGui::GetContentRegionAvail();

	if (ImGui::BeginChild("LeftChild", { leftTabWidth, avail.y - detectionRectSize.y - 5 }, true))
	{

#ifdef _DEBUG 

		//ImGui::Text("Time is: %ims", clock());
		ImGui::Text("Time is: %lu\xC2\xB5s", micros());

		ImGui::SameLine();

		ImGui::Text("Serial Staus: %i", serialStatus);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS) (GUI ONLY)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		//ImGui::Text("Application average %.3f us/frame (%.1f FPS) (CPU ONLY)", (micros() - lastFrame) / 1000.f, 1000000.0f / (micros() - lastFrame));

		ImGui::Text("Application average %.3f \xC2\xB5s/frame (%.1f FPS) (CPU ONLY)", averageFrametime, 1000000.f / averageFrametime);

		// The buffer contains just too many frames to display them on the screen (and displaying every 10th or so frame makes no sense). Also smoothing out the data set would hurt the performance too much
		//ImGui::PlotLines("FPS Chart", [](void* data, int idx) { return sinf(float(frames[idx%10]) / 1000); }, frames, AVERAGE_FRAME_COUNT);

		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { avail.x, ImGui::GetFrameHeight() });

#endif // _DEBUG

		//ImGui::Combo("Select Port", &selectedPort, Serial::availablePorts.data());

		static ImVec2 restItemsSize;

		ImGui::BeginGroup();

		bool isSelectedConnected = isInternalMode ? isAudioDevConnected : Serial::isConnected;

		ImGui::PushItemWidth(80);
		ImGui::BeginDisabled(isSelectedConnected);

		if (isInternalMode)
		{
//#ifdef _WIN32
			if (ImGui::BeginCombo("Device", !availableAudioDevices.empty() ? availableAudioDevices[selectedPort].friendlyName : "0 Found"))
			{
				for (size_t i = 0; i < availableAudioDevices.size(); i++)
				{
					bool isSelected = (selectedPort == i);
					if (ImGui::Selectable(availableAudioDevices[i].friendlyName, isSelected, 0, { 0,0 }))
					{
						if (selectedPort != i)
							Serial::Close();
						selectedPort = i;
					}
				}
				ImGui::EndCombo();
			}
//#endif
		}
		else
		{
			if (ImGui::BeginCombo("Port", !Serial::availablePorts.empty() ? Serial::availablePorts[selectedPort].c_str() : "0 Found"))
			{
				for (size_t i = 0; i < Serial::availablePorts.size(); i++)
				{
					bool isSelected = (selectedPort == i);
					if (ImGui::Selectable(Serial::availablePorts[i].c_str(), isSelected, 0, { 0,0 }))
					{
						if (selectedPort != i)
							Serial::Close();
						selectedPort = i;
					}
				}
				ImGui::EndCombo();
			}
		}
		ImGui::PopItemWidth();
		ImGui::EndDisabled();

		ImGui::SameLine();

		if (ImGui::Button("Refresh") && serialStatus == Status_Idle)
		{
			if (isInternalMode)
				DiscoverAudioDevices();
			else
				Serial::FindAvailableSerialPorts();
		}

		ImGui::SameLine();
		//ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });

		if (isSelectedConnected)
		{
			if (ImGui::Button("Disconnect"))
			{
				AttemptDisconnect();
			}
		}
		else
		{
			if (ImGui::Button("Connect") && ((!Serial::availablePorts.empty() && !isInternalMode) || (!availableAudioDevices.empty() && isInternalMode)))
			{
				AttemptConnect();
			}
		}

		ImGui::SameLine();
		ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });

		if (ImGui::Button("Clear"))
		{
			ImGui::OpenPopup("Clear?");
		}

		if (ImGui::BeginPopupModal("Clear?", NULL, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to clear EVERYTHING?");

			ImGui::SeparatorSpace(0, { 0, 10 });

			if (ImGui::Button("Yes") || IS_ONLY_ENTER_PRESSED)
			{
				ClearData();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("No") || IS_ONLY_ESCAPE_PRESSED)
			{
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		//ImGui::SameLine();

		ImGui::EndGroup();

		auto portItemsSize = ImGui::GetItemRectSize();

		if (portItemsSize.x + restItemsSize.x + 30 < ImGui::GetContentRegionAvail().x)
		{
			ImGui::SameLine();
			ImGui::Dummy({ -10, 0 });
			ImGui::SameLine();
			//ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
			//ImGui::SameLine();

			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });
		}
		else
			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 0 });

		ImGui::BeginGroup();

#ifdef _WIN32
		ImGui::Checkbox("Game Mode", &isGameMode);
		TOOLTIP("This mode instead of lighting up the rectangle at the bottom will simulate pressing the left mouse button (fire in game).\nTo register the delay between input and shot in game.");
#else
#warning "DISABLED GAME MODE"
#endif

		ImGui::SameLine();

		ImGui::BeginDisabled(isSelectedConnected);
		if (ImGui::Checkbox("Internal Mode", &isInternalMode))
		{
			int bkpSelected = lastSelectedPort;
			lastSelectedPort = selectedPort;
			selectedPort = bkpSelected;
		}
		ImGui::EndDisabled();
		TOOLTIP("Uses a sensor connected to 3.5mm jack. (More info on Github)")

		ImGui::SameLine();

#ifdef _WIN32
		if (ImGui::Checkbox("Audio Mode", &isAudioMode))
		{

		}
		TOOLTIP("Measures Audio Latency (Uses a microphone instead of photoresistor)");
#else
#warning "DISABLED AUDIO MODE"
#endif

		if (isInternalMode) {
			ImGui::SameLine();
			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Vertical, { 0, 0 });
			ImGui::SameLine();

			ImGui::BeginDisabled(!CanDoInternalTest(curTime));
			auto cursorPos = ImGui::GetCurrentWindow()->DC.CursorPos;
			if (ImGui::Button("Run Test"))
			{
				StartInternalTest();
			}
			ImGui::EndDisabled();
			auto itemSize = ImGui::GetItemRectSize();
			float frac = ImClamp<float>(((curTime - lastInternalTest) / (float)(1000 * (uint64_t)WARMUP_AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE + (INTERNAL_TEST_DELAY))), 0, 1);
			//ImGui::ProgressBar(frac);
			cursorPos.y += ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset + style.FramePadding.y * 2 + ImGui::GetTextLineHeight() + 2;
			cursorPos.x += 5;
			itemSize.x -= 10;
			itemSize.x *= frac;
			//itemSize.x = itemSize.x < 0 ? 0 : itemSize.x;
			ImGui::GetWindowDrawList()->AddLine(cursorPos, { cursorPos.x + itemSize.x, cursorPos.y }, ImGui::GetColorU32(ImGuiCol_SeparatorActive), 2);
			TOOLTIP("It's advised to use Shift+R instead.");
		}
		ImGui::EndGroup();

		restItemsSize = ImGui::GetItemRectSize();

		ImGui::Spacing();

		ImGui::PushFont(boldFont);

		if (ImGui::BeginChild("MeasurementStats", { 0, ImGui::GetTextLineHeightWithSpacing() * 4 + style.WindowPadding.y * 2 - style.ItemSpacing.y + style.FramePadding.y * 2 }, true))
		{
			if (ImGui::BeginTable("Latency Stats Table", 4, ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoPadOuterX))
			{
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(1);
				ImGui::Text("Internal");
				TOOLTIPFONT("Time measured by the computer from the moment it got the start signal (button press) to the signal that the light intensity change was spotted");
				ImGui::TableNextColumn();
				ImGui::Text("External");
				TOOLTIPFONT("Time measured by the microcontroller from the button press to the change of light intensity");
				ImGui::TableNextColumn();
				ImGui::Text("Input");
				TOOLTIPFONT("Time between computer sending a message to the microcontroller and receiving it back (Ping)");

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Highest");
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.externalLatency.highest / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.inputLatency.highest / 1000);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Average");
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", tabsInfo[selectedTab].latencyStats.internalLatency.average / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", tabsInfo[selectedTab].latencyStats.externalLatency.average / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%.2f", tabsInfo[selectedTab].latencyStats.inputLatency.average / 1000);

				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::Text("Lowest");
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.internalLatency.lowest / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.externalLatency.lowest / 1000);
				ImGui::TableNextColumn();
				ImGui::Text("%u", tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000);

				ImGui::EndTable();
			}

			ImGui::EndChild();
		}

		ImGui::PopFont();
	}

	if (currentUserData.performance.showPlots)
	{
		auto plotsAvail = ImGui::GetContentRegionAvail();
		auto plotHeight = _min(_max((plotsAvail.y - (4 * style.FramePadding.y)) / 4, 40), 100);

		auto& measurements = tabsInfo[selectedTab].latencyData.measurements;

		// Separate Plots
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.internalPlotColor);
		ImGui::PlotLines("Internal Latency", [](void* data, int idx) { return  tabsInfo[selectedTab].latencyData.measurements[idx].timeInternal / 1000.f; }, measurements.data(), measurements.size(), 0, NULL, FLT_MAX, FLT_MAX, { 0,plotHeight });
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.externalPlotColor);
		ImGui::PlotLines("External Latency", [](void* data, int idx) { return tabsInfo[selectedTab].latencyData.measurements[idx].timeExternal / 1000.f; }, measurements.data(), measurements.size(), 0, NULL, FLT_MAX, FLT_MAX, { 0,plotHeight });
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.inputPlotColor);
		ImGui::PlotLines("Input Latency", [](void* data, int idx) { return  tabsInfo[selectedTab].latencyData.measurements[idx].timePing / 1000.f; }, measurements.data(), measurements.size(), 0, NULL, FLT_MAX, FLT_MAX, { 0,plotHeight });
		ImGui::PopStyleColor(6);

		// Combined Plots
		auto startCursorPos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(startCursorPos);
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.internalPlotColor);
		ImGui::PlotLines("Combined Plots", [](void* data, int idx) { return  tabsInfo[selectedTab].latencyData.measurements[idx].timeInternal / 1000.f; },
			measurements.data(), measurements.size(), 0, NULL,
			tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000, tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000 + 1, { 0,plotHeight });

		ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });
		ImGui::PushStyleColor(ImGuiCol_Border, { 0,0,0,0 });
		ImGui::SetCursorPos(startCursorPos);
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.externalPlotColor);
		ImGui::PlotLines("###ExternalPlot", [](void* data, int idx) { return (float)tabsInfo[selectedTab].latencyData.measurements[idx].timeExternal / 1000.f; },
			measurements.data(), measurements.size(), 0, NULL,
			tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000, tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000 + 1, { 0,plotHeight });

		ImGui::SetCursorPos(startCursorPos);
		SetPlotLinesColor(*(ImVec4*)currentUserData.style.inputPlotColor);
		ImGui::PlotLines("###InputPlot", [](void* data, int idx) { return tabsInfo[selectedTab].latencyData.measurements[idx].timePing / 1000.f; },
			measurements.data(), measurements.size(), 0, NULL,
			tabsInfo[selectedTab].latencyStats.inputLatency.lowest / 1000, tabsInfo[selectedTab].latencyStats.internalLatency.highest / 1000 + 1, { 0,plotHeight });
		ImGui::PopStyleColor(8);
	}

	ImGui::EndChild();

	ImGui::SameLine();
	
	static float leftTabStart = 0;
	ImGui::ResizeSeparator(leftTabWidth, leftTabStart, ImGuiSeparatorFlags_Vertical, {300, 300}, { 6, avail.y - detectionRectSize.y - 5 });

	ImGui::SameLine();

	ImGui::BeginGroup();

	auto tableAvail = ImGui::GetContentRegionAvail();

	if (ImGui::BeginTable("measurementsTable", 5, ImGuiTableFlags_Reorderable | ImGuiTableFlags_SortMulti | ImGuiTableFlags_Sortable | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, { tableAvail.x, 200 }))
	{
		ImGui::TableSetupColumn("Index", ImGuiTableColumnFlags_DefaultSort | ImGuiTableColumnFlags_WidthFixed, 0.0f, 0);
		ImGui::TableSetupColumn("Internal Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 1);
		ImGui::TableSetupColumn("External Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 2);
		ImGui::TableSetupColumn("Input Latency (ms)", ImGuiTableColumnFlags_WidthStretch, 0.0f, 3);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoSort, 0.0f, 4);
		ImGui::TableHeadersRow();

		// Copy of the original vector has to be used because there is a possibility that some of the items will be removed from it. In which case changes will be updated on the next frame
		std::vector<LatencyReading> _latencyTestsCopy = tabsInfo[selectedTab].latencyData.measurements;

		if (ImGuiTableSortSpecs* sorts_specs = ImGui::TableGetSortSpecs())
			if (sorts_specs->SpecsDirty)
			{
				sortSpec = sorts_specs;
				if (tabsInfo[selectedTab].latencyData.measurements.size() > 1)
					qsort(&tabsInfo[selectedTab].latencyData.measurements[0], (size_t)tabsInfo[selectedTab].latencyData.measurements.size(), sizeof(tabsInfo[selectedTab].latencyData.measurements[0]), LatencyCompare);
				sortSpec = NULL;
				sorts_specs->SpecsDirty = false;
			}

		ImGuiListClipper clipper;
		clipper.Begin(_latencyTestsCopy.size(), ImGui::GetTextLineHeightWithSpacing());
		while (clipper.Step())
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++)
			{
				auto& reading = _latencyTestsCopy[i];

				ImGui::PushID(i + 100);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.index);
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.timeInternal / 1000);
				/* \xC2\xB5 is the microseconds character (looks like u (not you, just "u")) */
				TOOLTIP("%i\xC2\xB5s", reading.timeInternal);
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.timeExternal/1000);
                TOOLTIP("%i\xC2\xB5s", reading.timeExternal);
				ImGui::TableNextColumn();

				ImGui::Text("%i", reading.timePing / 1000);
				TOOLTIP("%i\xC2\xB5s", reading.timePing);

				ImGui::TableNextColumn();

				if (ImGui::SmallButton("X"))
				{
					RemoveMeasurement(i);
				}

				ImGui::PopID();
			}


		if (wasMeasurementAddedGUI)
			ImGui::SetScrollHereY();


		ImGui::EndTable();
	}

	//ImGui::SameLine();

	// Notes area
	ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 10 });
	ImGui::Text("Measurement notes");
	auto notesAvail = ImGui::GetContentRegionAvail();
	ImGui::InputTextMultiline("Measurements Notes", tabsInfo[selectedTab].latencyData.note, 1000, {tableAvail.x, _min(notesAvail.y - detectionRectSize.y - 5, 600) }, ImGuiInputTextFlags_NoHorizontalScroll);
	//ImGui::InputTextMultiline("Measurements Notes", tabsInfo[selectedTab].latencyData.note, 1000, { tableAvail.x, _min(tableAvail.y - 200 - detectionRectSize.y - style.WindowPadding.y - style.ItemSpacing.y * 5, 600) - ImGui::GetItemRectSize().y - 8});

	ImGui::EndGroup();

	static float detectionRectStart = 0;
	ImGui::ResizeSeparator(detectionRectSize.y, detectionRectStart, ImGuiSeparatorFlags_Horizontal, {100, 400}, { avail.x, 6 });

    // Start recording only when we actually change the color (Just an idea)
    // I tested it and turns out it doesn't really make a difference );
    //static SerialStatus last_serialStatus{serialStatus};
    //
    //if(serialStatus == Status_WaitingForResult && last_serialStatus != Status_WaitingForResult){
    //    curAudioDevice->ClearRecordBuffer();
    //    curAudioDevice->StartRecording();
    //}
    //
    //last_serialStatus = serialStatus;

	// Color change detection rectangle.
	ImVec2 rectSize{ 0, detectionRectSize.y };
	rectSize.x = detectionRectSize.x == 0 ? windowAvail.x + style.WindowPadding.x + style.FramePadding.x : detectionRectSize.x;
	ImVec2 pos = { 0, windowAvail.y - rectSize.y + style.WindowPadding.y * 2 + style.FramePadding.y * 2 + 10 };
	pos.y += 12;
	ImRect bb{ pos, pos + rectSize };
	ImGui::RenderFrame(
		bb.Min,
		bb.Max,
		// Change the color to white to be detected by the light sensor
		serialStatus == Status_WaitingForResult && !isGameMode && !isAudioMode ? ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 1.f, 1.f, 1)) : ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 0.f, 0.f, 1.f)),
		false
	);
	//if (ImGui::IsItemFocused())
	//	ImGui::SetFocusID(0, ImGui::GetCurrentWindow());

	if (serialStatus == Status_Idle && currentUserData.performance.showPlots) {
		ImGui::SetCursorPos(pos);
		//rectSize.y -= style.WindowPadding.y/2;
		ImGui::PushStyleColor(ImGuiCol_FrameBg, { 0,0,0,0 });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		SetPlotLinesColor({ 1,1,1,0.5 });
		//else
		//	SetPlotLinesColor({ 0,0,0,0.5 });
#ifdef _WIN32
		if (curAudioDevice)
			ImGui::PlotLines("##audioBufferPlot2", [](void* data, int idx) {return (float)curAudioDevice->recordBuffer[idx]; }, nullptr, TEST_AUDIO_BUFFER_SIZE, 0, 0, (long long)SHRT_MAX / -1, SHRT_MAX / 1, rectSize);
#else
        if (curAudioDevice)
            ImGui::PlotLines("##audioBufferPlot2",[](void* data, int idx) {return (float)curAudioDevice->recordBuffer[idx < (TEST_AUDIO_BUFFER_SIZE / 2) ? (idx * 2) : (idx * 2 + 1 - (TEST_AUDIO_BUFFER_SIZE))]; },
                             nullptr, TEST_AUDIO_BUFFER_SIZE/1, 0, 0,
                             std::numeric_limits<AUDIO_DATA_FORMAT>::min(),
                             std::numeric_limits<AUDIO_DATA_FORMAT>::max(), rectSize);

#endif
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
	}


	if (ImGui::BeginPopupModal("Set Up", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
	{
		ImGui::Text("Setup configuration");

		ImGui::SeparatorSpace(0, { 0, 5 });

		ImGui::Checkbox("Save config locally", &currentUserData.misc.localUserData);
#ifdef _WIN32
		TOOLTIP("Chose whether you want to save config file in the same directory as the program, or in the AppData folder");
#else
        TOOLTIP("Chose whether you want to save config file in the same directory as the program, or in the /home/ folder");
#endif

		ImGui::Spacing();

		if (ImGui::Button("Save", {-1, 0}))
		{
			SaveCurrentUserConfig();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	static bool isExitingWindowOpen = false;

	if (isExiting)
	{


		//for (size_t i = 0; i < tabsInfo.size(); i++)
		//{
		//	if (!tabsInfo[i].isSaved)
		//		unsavedTabs.push_back(i);
		//}

		if (unsavedTabs.size() > 0)
		{
			isExitingWindowOpen = true;
			selectedTab = unsavedTabs[0];
			ImGui::OpenPopup("Exit?");
		}

		if (ImGui::BeginPopupModal("Exit?", &isExitingWindowOpen, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Are you sure you want to exit before saving?");
			ImGui::SameLine();
			ImGui::TextUnformatted(tabsInfo[selectedTab].name);

			ImGui::SeparatorSpace(ImGuiSeparatorFlags_Horizontal, { 0, 5 });

			if (ImGui::Button("Save and exit"))
			{
				if (SaveMeasurements())
				{
					ImGui::CloseCurrentPopup();
					isExitingWindowOpen = false;
					unsavedTabs.erase(unsavedTabs.begin());
					if (unsavedTabs.size() > 0)
						selectedTab = unsavedTabs[0];
					else
						return 2;
				}
				else
					printf("Could not save the file");
			}
			ImGui::SameLine();
			if (ImGui::Button("Exit"))
			{
				ImGui::CloseCurrentPopup();
				unsavedTabs.erase(unsavedTabs.begin());
				isExitingWindowOpen = false;
				//ImGui::EndPopup();
				if (unsavedTabs.size() > 0)
					selectedTab = unsavedTabs[0];
				else
					return 2;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
				isExitingWindowOpen = false;
				isExiting = false;
			}
			ImGui::EndPopup();
		}
	}

    wasMeasurementAddedGUI = false;
	lastFrameGui = micros();

	return 0;
}

std::chrono::high_resolution_clock::time_point lastTimeGotChar;

void AddMeasurement(UINT internalTime, UINT externalTime, UINT inputTime)
{
	LatencyReading reading{ externalTime, internalTime, inputTime };
	size_t size = tabsInfo[selectedTab].latencyData.measurements.size();

	// Update the stats
	tabsInfo[selectedTab].latencyStats.internalLatency.average = (tabsInfo[selectedTab].latencyStats.internalLatency.average * size) / (size + 1.f) + (internalTime / (size + 1.f));
	tabsInfo[selectedTab].latencyStats.externalLatency.average = (tabsInfo[selectedTab].latencyStats.externalLatency.average * size) / (size + 1.f) + (externalTime / (size + 1.f));
	tabsInfo[selectedTab].latencyStats.inputLatency.average = (tabsInfo[selectedTab].latencyStats.inputLatency.average * size) / (size + 1.f) + (inputTime / (size + 1.f));

	tabsInfo[selectedTab].latencyStats.internalLatency.highest = internalTime > tabsInfo[selectedTab].latencyStats.internalLatency.highest ? internalTime : tabsInfo[selectedTab].latencyStats.internalLatency.highest;
	tabsInfo[selectedTab].latencyStats.externalLatency.highest = externalTime > tabsInfo[selectedTab].latencyStats.externalLatency.highest ? externalTime : tabsInfo[selectedTab].latencyStats.externalLatency.highest;
	tabsInfo[selectedTab].latencyStats.inputLatency.highest = inputTime > tabsInfo[selectedTab].latencyStats.inputLatency.highest ? inputTime : tabsInfo[selectedTab].latencyStats.inputLatency.highest;

	tabsInfo[selectedTab].latencyStats.internalLatency.lowest = internalTime < tabsInfo[selectedTab].latencyStats.internalLatency.lowest ? internalTime : tabsInfo[selectedTab].latencyStats.internalLatency.lowest;
	tabsInfo[selectedTab].latencyStats.externalLatency.lowest = externalTime < tabsInfo[selectedTab].latencyStats.externalLatency.lowest ? externalTime : tabsInfo[selectedTab].latencyStats.externalLatency.lowest;
	tabsInfo[selectedTab].latencyStats.inputLatency.lowest = inputTime < tabsInfo[selectedTab].latencyStats.inputLatency.lowest ? inputTime : tabsInfo[selectedTab].latencyStats.inputLatency.lowest;

	// If there are no measurements done yet set the minimum value to the current one
	if (size == 0)
	{
		tabsInfo[selectedTab].latencyStats.internalLatency.lowest = internalTime;
		tabsInfo[selectedTab].latencyStats.externalLatency.lowest = externalTime;
		tabsInfo[selectedTab].latencyStats.inputLatency.lowest = inputTime;
	}

    wasMeasurementAddedGUI = true;
	reading.index = size;
	tabsInfo[selectedTab].latencyData.measurements.push_back(reading);
	tabsInfo[selectedTab].isSaved = false;
}

void GotSerialChar(char c)
{
	if (c == 0 || !Serial::isConnected)
		return;

#ifdef _DEBUG
	printf("Got: %c\n", c);
#endif

	// 6 digits should be enough. I doubt the latency can be bigger than 10 seconds (anything greater than 100us should be discarded)
	const size_t resultBufferSize = 6;
	static BYTE resultBuffer[resultBufferSize]{ 0 };
	static BYTE resultNum = 0;

	static std::chrono::high_resolution_clock::time_point internalStartTime;
	static std::chrono::high_resolution_clock::time_point internalEndTime;

	static std::chrono::high_resolution_clock::time_point pingStartTime;

	// Because we are subtracting 2 similar unsigned long longs, we dont need another unsigned long long, we just need an int
	static unsigned int internalTime = 0;
	static unsigned int externalTime = 0;


	if (!((c >= 97 && c <= 122) || (c >= 48 && c <= 57)))
	{
		externalTime = internalTime = 0;
		serialStatus = Status_Idle;
	}

	switch (serialStatus)
	{
	case Status_Idle:
		// Input (button press) detected (l for light)
		if (c == 'l')
		{
			internalStartTime = std::chrono::high_resolution_clock::now();
			lastInternalTest = micros();
#ifdef _DEBUG
			printf("waiting for results\n");
#endif
			serialStatus = Status_WaitingForResult;

#ifdef _WIN32
			if (isAudioMode)
				audioPlayer.Play();
#endif

			if (isGameMode)
			{
#ifdef _WIN32
				INPUT Inputs[2] = { 0 };

				Inputs[0].type = INPUT_MOUSE;
				Inputs[0].mi.dx = 0;
				Inputs[0].mi.dy = 0;
				Inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

				Inputs[1].type = INPUT_MOUSE;
				Inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

				// Send input as 2 different input packets because some programs read only one flag from packets
				if (SendInput(2, Inputs, sizeof(INPUT)))
				{
					wasMouseClickSent = false;
					wasLMB_Pressed = false;
				}
#endif
			}
		}
		return;
		break;
	case Status_WaitingForResult:
		if (resultNum == 0)
			internalEndTime = std::chrono::high_resolution_clock::now();

		// e for end (end of the numbers)
		// All the code below will have to be moved to a separate function in the future when saving/loading from a file will be added. (Little did he know)
		if (c == 'e')
		{
			externalTime = 0;
			internalTime = std::chrono::duration_cast<std::chrono::microseconds>(internalEndTime - internalStartTime).count();
			// Convert the byte array to int
			for (size_t i = 0; i < _min(resultNum, resultBufferSize); i++)
			{
				externalTime += resultBuffer[i] * pow<int>(10, (resultNum - i - 1));
			}

			resultNum = 0;
			std::fill_n(resultBuffer, resultBufferSize, 0);

            externalTime *= External2Micros;

			if (_max(externalTime, internalTime) > 1000000 || _min(externalTime, internalTime) < 1000)
			{
				serialStatus = Status_Idle;
				ZeroMemory(resultBuffer, resultBufferSize);
				break;
			}

			wasMouseClickSent = false;
			wasLMB_Pressed = false;

			//if (!wasModdedMouseTimeUpdated)
			//	moddedMouseTimeClicked += std::chrono::microseconds(375227);
#ifdef _DEBUG
			printf("Final result: %i\n", externalTime);
#endif

			//AddMeasurement(internalTime, externalTime, 0);

			//_write(Serial::fd, "p", 1);
			Serial::Write("p", 1);
			pingStartTime = std::chrono::high_resolution_clock::now();
			//fwrite(&ch, sizeof(char), 1, Serial::hFile);
			//fflush(Serial::hFile);
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
			unsigned int pingTime = 0;
			//if (selectedMode == InputMode_ModdedMouse) {
			//	pingTime = std::chrono::duration_cast<std::chrono::microseconds>(abs(moddedMouseTimeClicked - internalStartTime)).count();
			//	wasModdedMouseTimeUpdated = false;
			//}
			//else
			pingTime = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - pingStartTime).count();
			//tabsInfo[selectedTab].latencyData.measurements[tabsInfo[selectedTab].latencyData.measurements.size() - 1].timePing = pingTime;

			//size_t size = tabsInfo[selectedTab].latencyData.measurements.size(); -1;
			// account for the serial timeout delay (this is just an estimate)
#ifdef _WIN32
			internalTime -= 500000 / SERIAL_UPDATE_TIME;
			externalTime -= 500000 / SERIAL_UPDATE_TIME;
			pingTime -= 1000000 / SERIAL_UPDATE_TIME;
#endif
			AddMeasurement(internalTime, externalTime, pingTime);

			//tabsInfo[selectedTab].latencyStats.inputLatency.average = (tabsInfo[selectedTab].latencyStats.inputLatency.average * size) / (size + 1.f) + (pingTime / (size + 1.f));
			//tabsInfo[selectedTab].latencyStats.inputLatency.highest = pingTime > tabsInfo[selectedTab].latencyStats.inputLatency.highest ? pingTime : tabsInfo[selectedTab].latencyStats.inputLatency.highest;
			//tabsInfo[selectedTab].latencyStats.inputLatency.lowest = pingTime < tabsInfo[selectedTab].latencyStats.inputLatency.lowest ? pingTime : tabsInfo[selectedTab].latencyStats.inputLatency.lowest;

			//if (size == 0)
			//{
			//	tabsInfo[selectedTab].latencyStats.inputLatency.lowest = pingTime;
			//}

			serialStatus = Status_Idle;
		}
		break;
	default:
		break;
	}

	// Something was missed, better remove the last measurement
	//if (c == 'l' && serialStatus == Status_WaitingForResult)
	//{
	//	serialStatus = Status_Idle;
	//}

#ifdef _DEBUG
	printf("char receive delay: %ld\n", std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - lastTimeGotChar).count());
#endif
	lastTimeGotChar = std::chrono::high_resolution_clock::now();
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

bool SetupAudioDevice()
{
#ifdef _WIN32
	int res = curAudioDevice->AddBuffer(WARMUP_AUDIO_BUFFER_SIZE);
	if (res)
		return false;

	res = curAudioDevice->AddBuffer(TEST_AUDIO_BUFFER_SIZE, true);
	if (res)
		return false;

	return true;
#else
    return true;
#endif
}

// [lower, upper)
int RandInRange(int _lower, int _upper) {
    return _lower + (rand() % (_upper - _lower));
}

bool CanDoInternalTest(uint64_t timeNow)
{
    timeNow = timeNow == 0 ? micros() : timeNow;
    static int random_val = RandInRange(2, 10);
    static bool set_random_val = false; // used to generate only one random value for each returned true
            // this is because "rand()" is kinda slow and we want to reduce its usage as much as possible
#ifdef _WIN32
	if(isInternalMode && curAudioDevice && (timeNow > lastInternalTest + (INTERNAL_TEST_DELAY + random_val))) {
        set_random_val = false;
        return true;
    }
#else
    if(isInternalMode && curAudioDevice && (timeNow > lastInternalTest + ((1000*WARMUP_AUDIO_BUFFER_SIZE / AUDIO_SAMPLE_RATE) + INTERNAL_TEST_DELAY + random_val))) {
        set_random_val = false;
        return true;
    }
#endif

    if(!set_random_val)
        random_val = RandInRange(80000, 400000); // 0.08 to 0.4 seconds delay (80ms to 400ms)
    set_random_val = true;
    return false;
}

void DiscoverAudioDevices()
{
#ifdef _WIN32
	AudioDeviceInfo* devices;
	UINT devCount = GetAudioDevices(&devices);

	availableAudioDevices.clear();

	for (int i = 0; i < devCount; i++)
	{
		auto& dev = devices[i];
		availableAudioDevices.push_back(cAudioDeviceInfo());

		size_t _charsConverted;

		int size_offset = 0;
		int end_offset = 0;
		if (wcsstr(dev.friendlyName, L"Microphone (") == dev.friendlyName) {
			size_offset += 12;
			end_offset += 1;
		}

		char* fName = new char[wcslen(dev.friendlyName) + 1 - size_offset];
		wcstombs_s(&_charsConverted, fName, wcslen(dev.friendlyName) + 1 - size_offset, dev.friendlyName + size_offset, wcslen(dev.friendlyName));

		fName[strlen(fName) - end_offset] = 0;

		char* pName = new char[wcslen(dev.portName) + 1];
		wcstombs_s(&_charsConverted, pName, wcslen(dev.portName) + 1, dev.portName, wcslen(dev.portName));

		availableAudioDevices[i].friendlyName = fName;
		availableAudioDevices[i].portName = pName;
		availableAudioDevices[i].id = dev.id;
		availableAudioDevices[i].WASAPI_id = dev.WASAPI_id;
		availableAudioDevices[i].AudioEnchantments = dev.AudioEnchantments;
	}

	delete[] devices;

#else

    availableAudioDevices.clear();

    cAudioDeviceInfo* a_devs;
    auto cnt = GetAudioDevices(&a_devs);
    availableAudioDevices.reserve(cnt);
    std::copy(a_devs, a_devs + cnt, std::back_inserter(availableAudioDevices));
#ifdef _DEBUG
    for(int i = 0; i < cnt; i++) {
        printf("device %i: %s\n", a_devs[i].id, a_devs[i].friendlyName);
    }
#endif

#endif
}

void StartInternalTest()
{
#ifdef _WIN32
	if (!CanDoInternalTest())
		return;

	serialStatus = Status_AudioReadying;

	lastInternalTest = micros();

	if (curAudioDevice->GetPlayTime().u.sample > 0)
		curAudioDevice->ResetRecording();
	else
		curAudioDevice->StartRecording();

#else
    if (!CanDoInternalTest())
        return;

    serialStatus = Status_WaitingForResult;
    curAudioDevice->ClearRecordBuffer();
    curAudioDevice->StartRecording();
    lastInternalTest = micros();

    //printf("TEST DONE!\n");
#endif
}

#ifdef _WIN32
bool GetMonitorModes(DXGI_MODE_DESC* modes, UINT* size)
{
	IDXGIOutput* output;

	if (GUI::g_pSwapChain->GetContainingOutput(&output) != S_OK)
		return false;

	//DXGI_FORMAT_R8G8B8A8_UNORM
	if (output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, size, modes) != S_OK)
	{
		return false;
		printf("Error getting the display modes\n");
	}

	//if (*size > 0)
	//{
	//	auto targetMode = modes[*size - 1];
	//	targetMode.RefreshRate.Numerator = 100000000;
	//	DXGI_MODE_DESC matchMode;
	//	output->FindClosestMatchingMode(&targetMode, &matchMode, GUI::g_pd3dDevice);

	//	printf("Best mode match: %ix%i@%iHz\n", matchMode.Width, matchMode.Height, (matchMode.RefreshRate.Numerator / matchMode.RefreshRate.Denominator));
	//}

	return true;
}
#endif

void AttemptConnect()
{
	if (isInternalMode && !availableAudioDevices.empty())
	{
#ifdef _WIN32
        delete curAudioDevice;

		curAudioDevice = new Waveform_SoundHelper(availableAudioDevices[selectedPort].id, AUDIO_BUFFER_SIZE, 2u, AUDIO_SAMPLE_RATE, 16u, (DWORD_PTR)waveInCallback);
		if (curAudioDevice && curAudioDevice->isCreated)
			isAudioDevConnected = true;

		isAudioDevConnected = SetupAudioDevice();

		if (isAudioDevConnected)
			SetAudioEnchantments(false, availableAudioDevices[selectedPort].WASAPI_id);

#else


        delete curAudioDevice;

        //curAudioDevice = new ALSA_AudioDevice(availableAudioDevices[selectedPort].id, AUDIO_BUFFER_SIZE, 1u, AUDIO_SAMPLE_RATE, 16u);
        curAudioDevice = new ALSA_AudioDevice<AUDIO_DATA_FORMAT>(availableAudioDevices[selectedPort].id, TEST_AUDIO_BUFFER_SIZE, AUDIO_SAMPLE_RATE);
        if (curAudioDevice != nullptr && curAudioDevice->isCreated) {
            isAudioDevConnected = SetupAudioDevice();
        }
        else {
            delete curAudioDevice;
            curAudioDevice = nullptr;
        }
#endif
	}
	else if(!Serial::availablePorts.empty())
		Serial::Setup(Serial::availablePorts[selectedPort].c_str(), GotSerialChar);
}

void AttemptDisconnect()
{
	if (isInternalMode)
	{
#ifdef _WIN32
		curAudioDevice->StopRecording();

		SetAudioEnchantments(availableAudioDevices[selectedPort].AudioEnchantments, availableAudioDevices[selectedPort].WASAPI_id);

		if (curAudioDevice)
			delete curAudioDevice;

		curAudioDevice = nullptr;

		isAudioDevConnected = false;
#else
        //curAudioDevice->StopRecording();

        if (curAudioDevice)
            delete curAudioDevice;

        curAudioDevice = nullptr;

        isAudioDevConnected = false;
#endif
	}
	else {
		Serial::Close();
		serialStatus = Status_Idle;
	}
}

// Returns true if should exit, false otherwise
bool OnExit()
{
	bool isSaved = true;

	unsavedTabs.clear();

	for (size_t i = 0; i < tabsInfo.size(); i++)
	{
		if (!tabsInfo[i].isSaved)
		{
			isSaved = false;
			unsavedTabs.push_back(i);
		}
	}

	if (isSaved)
	{
		// Gui closes itself from the wnd messages
		Serial::Close();

		exit(0);
	}

	isExiting = true;
	return isSaved;
}

#ifdef _WIN32
void KeyDown(WPARAM key, LPARAM info, bool isPressed)
{
	//enum class ModKey {
	//	ModKey_None = 0,
	//	ModKey_Ctrl = 1,
	//	ModKey_Shift = 2,
	//	ModKey_Alt = 4,
	//}; Just for information

	bool wasJustClicked = !(info & 0x40000000);
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

	printf("Key mod: %u\n", modKeyFlags);


	// Handle time-sensitive KB input
	if (isInternalMode && key == 'R' && isPressed && modKeyFlags == 2 && !serialStatus == Status_WaitingForResult)
	{
		StartInternalTest();
	}

	if (modKeyFlags == 1 && isPressed)
	{
		if (key == 'S' && wasJustClicked)
		{
			SaveMeasurements();
		}
		else if (key == 'O' && wasJustClicked)
		{
			OpenMeasurements();
		}
		else if (key == 'N' && wasJustClicked)
		{
			auto newTab = TabInfo();
			strcat_s<TAB_NAME_MAX_SIZE>(newTab.name, std::to_string(tabsInfo.size()).c_str());
			tabsInfo.push_back(newTab);
			selectedTab = tabsInfo.size() - 1;
		}
		else if (key == 'Q' && wasJustClicked)
		{
			if (isInternalMode ? isAudioDevConnected : Serial::isConnected)
				AttemptDisconnect();
			else
				AttemptConnect();
		}
	}
	if (modKeyFlags == 3 && key == 'S' && wasJustClicked)
		SaveAsMeasurements();
	if ((modKeyFlags & 4) == 4 && isPressed)
	{
		if (key == 'S' && modKeyFlags == 4 && wasJustClicked)
		{
			SavePackMeasurements();
		}

		if (key == 'S' && (modKeyFlags & 2) == 2 && wasJustClicked)
		{
			SavePackAsMeasurements();
		}
	}
}

#else

// Pretty much just a copy of what works on windows
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    //enum class ModKey {
    //	ModKey_None = 0,
    //	ModKey_Ctrl = 1,
    //	ModKey_Shift = 2,
    //	ModKey_Alt = 4,
    //}; Just for information

    bool isPressed = action == GLFW_PRESS || action == GLFW_REPEAT;
    bool wasJustClicked = action == GLFW_PRESS;
    static unsigned char modKeyFlags = 0;

    //if (key == 0 && action == GLFW_RELEASE) {
    //    modKeyFlags = 0;
    //    return;
    //}

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

    //printf("Key mod: %u\n", modKeyFlags);


    // Handle time-sensitive KB input
    if (isInternalMode && key == 'R' && isPressed && modKeyFlags == 2 && serialStatus != Status_WaitingForResult)
    {
        StartInternalTest();
    }

    if (modKeyFlags == 1 && isPressed)
    {
        if (key == 'S' && wasJustClicked)
        {
            SaveMeasurements();
        }
        else if (key == 'O' && wasJustClicked)
        {
            OpenMeasurements();
        }
        else if (key == 'N' && wasJustClicked)
        {
            auto newTab = TabInfo();
            strcat(newTab.name, std::to_string(tabsInfo.size()).c_str());
            tabsInfo.push_back(newTab);
            selectedTab = tabsInfo.size() - 1;
        }
        else if (key == 'Q' && wasJustClicked)
        {
            if (isInternalMode ? isAudioDevConnected : Serial::isConnected)
                AttemptDisconnect();
            else
                AttemptConnect();
        }
    }
    if (modKeyFlags == 3 && key == 'S' && wasJustClicked)
        SaveAsMeasurements();
    if ((modKeyFlags & 4) == 4 && isPressed)
    {
        if (key == 'S' && modKeyFlags == 4 && wasJustClicked)
        {
            SavePackMeasurements();
        }

        if (key == 'S' && (modKeyFlags & 2) == 2 && wasJustClicked)
        {
            SavePackAsMeasurements();
        }
    }
}

#endif

//void RawInputEvent(RAWINPUT* ri)
//{
//	if (ri->header.dwType == RIM_TYPEMOUSE)
//	{
//		if ((ri->data.mouse.usButtonFlags & RI_MOUSE_BUTTON_1_DOWN) == RI_MOUSE_BUTTON_1_DOWN) {
//			moddedMouseTimeClicked = std::chrono::high_resolution_clock::now();
//			wasModdedMouseTimeUpdated = true;
//			//printf("mouse button pressed\n");
//		}
//	}
//}

#ifdef _WIN32
auto millisSleepTimer = ::CreateWaitableTimerEx(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, EVENT_ALL_ACCESS);

// 500us precision
void microsSleep(LONGLONG us) {
	::LARGE_INTEGER ft;
	ft.QuadPart = -static_cast<int64_t>(us * 10);  // '-' using relative time

	//::HANDLE millisSleepTimer = ::CreateWaitableTimer(NULL, TRUE, NULL);
	::SetWaitableTimer(millisSleepTimer, &ft, 0, NULL, NULL, 0);
	::WaitForSingleObject(millisSleepTimer, 200);
	//::CloseHandle(millisSleepTimer);
}
#endif

// Main code
int main(int argc, char** argv)
{
#ifdef _WIN32
	hwnd = GUI::Setup(OnGui);
#else
    GUI::VSyncFrame = &currentUserData.performance.VSync;
    GUI::Setup(OnGui);
    GUI::RegKeyCallback(key_callback);
#endif
	GUI::onExitFunc = OnExit;

	srand(time(NULL));
		
	//localPath = argv[0];

#ifdef _WIN32
	QueryPerformanceCounter(&StartingTime);
#endif

    // Doesnt do that much (anything) and I'd rather not be this "critical"
	//if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
	//	printf("Priotity set to the Highest\n");

#ifdef _WIN32
#ifndef _DEBUG
	::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#else
	::ShowWindow(::GetConsoleWindow(), SW_SHOW);
#endif
#endif

	//::ShowWindow(::GetConsoleWindow(), SW_SHOW);
	//AllocConsole();
	//freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	static size_t frameIndex = 0;
	static uint64_t frameSum = 0;

	bool done = false;

	ImGuiIO& io = ImGui::GetIO();

	leftTabWidth = GUI::windowX/2;

#ifdef _WIN32
	GUI::KeyDownFunc = KeyDown;
	//GUI::RawInputEventFunc = RawInputEvent;
#endif

	auto defaultTab = TabInfo();
	defaultTab.name[4] = '0';
	defaultTab.name[5] = '\0';
	tabsInfo.push_back(defaultTab);

#ifdef _WIN32
	UINT modesNum = 256;
	DXGI_MODE_DESC monitorModes[256];
	GetMonitorModes(monitorModes, &modesNum);

	GUI::MAX_SUPPORTED_FRAMERATE = monitorModes[modesNum - 1].RefreshRate.Numerator;
	GUI::VSyncFrame = &currentUserData.performance.VSync;
#endif

	//float colors[styleColorNum][4];
	//float brightnesses[styleColorNum] = { accentBrightness, fontBrightness };
	//if (ReadStyleConfig(colors, brightnesses, selectedFont, fontSize))
	if (LoadCurrentUserConfig())
	{
		ApplyCurrentStyle();
		//ApplyStyle(colors, brightnesses);

		//// Set the default colors to the variables with a type conversion (ImVec4 -> float[4]) (could be done with std::copy,
		//// but the performance advantage it gives is just immeasurable, especially for a single time execution code)

		backupUserData = currentUserData;

		GUI::LoadFonts(currentUserData.style.fontSize);

		for (int i = 0; i < io.Fonts->Fonts.Size; i++)
		{
			io.Fonts->Fonts[i]->Scale = currentUserData.style.fontSize;
		}

		io.FontDefault = io.Fonts->Fonts[fontIndex[currentUserData.style.selectedFont]];

		auto _boldFont = GetFontBold(currentUserData.style.selectedFont);

		if (_boldFont != nullptr)
			boldFont = _boldFont;
		else
			boldFont = io.Fonts->Fonts[fontIndex[currentUserData.style.selectedFont]];

		//boldFontBak = boldFont;
	}
	else
	{
		auto& styleColors = ImGui::GetStyle().Colors;
		memcpy(&currentUserData.style.mainColor, &(styleColors[ImGuiCol_Header]), colorSize);
		memcpy(&currentUserData.style.fontColor, &(styleColors[ImGuiCol_Text]), colorSize);

		memcpy(&backupUserData.style.mainColor, &(styleColors[ImGuiCol_Header]), colorSize);
		memcpy(&backupUserData.style.fontColor, &(styleColors[ImGuiCol_Text]), colorSize);

		GUI::LoadFonts(1);

		currentUserData.performance.lockGuiFps = backupUserData.performance.lockGuiFps = true;

		currentUserData.performance.showPlots = backupUserData.performance.showPlots = true;

		currentUserData.performance.VSync = backupUserData.performance.VSync = false;

#ifdef _WIN32
		printf("found %u monitor modes\n", modesNum);
		currentUserData.performance.guiLockedFps = (monitorModes[modesNum - 1].RefreshRate.Numerator / monitorModes[modesNum - 1].RefreshRate.Denominator) * 2;

#ifdef _DEBUG

		for (UINT i = 0; i < modesNum; i++)
		{
			printf("Mode %i: %ix%i@%iHz\n", i, monitorModes[i].Width, monitorModes[i].Height, (monitorModes[modesNum - 1].RefreshRate.Numerator / monitorModes[modesNum - 1].RefreshRate.Denominator));
		}

#endif // DEBUG

#endif // _WIN32

		backupUserData.performance.guiLockedFps = currentUserData.performance.guiLockedFps;

		currentUserData.style.mainColorBrightness = backupUserData.style.mainColorBrightness = .1f;
		currentUserData.style.fontColorBrightness = backupUserData.style.fontColorBrightness = .1f;
		currentUserData.style.fontSize = backupUserData.style.fontSize = 1.f;

		shouldRunConfiguration = true;

		// Note: this style might be a little bit different prior to applying it. (different darker colors)
	}

	// Just imagine what would happen if user for example had a 220 character long username and path to %AppData% + "imgui.ini" would exceed 260. lovely
	auto imguiFileIniPath = (currentUserData.misc.localUserData ? HelperJson::GetLocalUserConfingPath() : HelperJson::GetAppDataUserConfigPath());
	imguiFileIniPath = imguiFileIniPath.substr(0, imguiFileIniPath.find_last_of(OS_SPEC_PATH("\\/")) + 1) + OS_SPEC_PATH("imgui.ini");

#ifdef _WIN32
    char outputPathBuffer[MAX_PATH];
	wcstombs_s(nullptr, outputPathBuffer, MAX_PATH, imguiFileIniPath.c_str(), imguiFileIniPath.size());
    io.IniFilename = outputPathBuffer;
#else
    //wcstombs(outputPathBuffer, imguiFileIniPath.c_str(), imguiFileIniPath.size());
    io.IniFilename = imguiFileIniPath.c_str();
#endif


	Serial::FindAvailableSerialPorts();

	printf("Found %lld serial Ports\n", Serial::availablePorts.size());

	DiscoverAudioDevices();

#ifdef _WIN32
	// Setup Audio buffer (with noise)
	audioPlayer.Setup();
	audioPlayer.SetBuffer([](int i) { return ((rand() * 2 - RAND_MAX) % RAND_MAX) * 10; });
	//audioPlayer.SetBuffer([](int i) { return rand() % SHRT_MAX; });
	//audioPlayer.SetBuffer([](int i) { return (int)(sinf(i/4.f) * RAND_MAX); });

	//BOOL fScreen;
	//IDXGIOutput* output;
	//GUI::g_pSwapChain->GetFullscreenState(&fScreen, &output);
	//isFullscreen = fScreen;
#endif

	//if (Serial::Setup("COM4", GotSerialChar))
	//	printf("Serial Port opened successfuly");
	//else
	//	printf("Error setting up the Serial Port");

	// used to cover the time of rendering a frame when calculating when a next frame should be displayed
	uint64_t lastFrameRenderTime = 0;

	int GUI_Return_Code = 0;

#ifdef _WIN32
	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	timeBeginPeriod(tc.wPeriodMin);
#endif

	//size_t cnt = 0;
	//auto start = micros();
	//while (cnt < 500)
	//{
	//	microsSleep(4000);
	//	cnt++;
	//	//std::cout << "callback at time: " << micros();
	//	//printf("callback at time: %lldms\n", micros());
	//}

	//auto end = micros();

	//printf("took: %lld, average rate was %fus\n", (end - start), (end - start) / 500.f);
	//return 0;

MainLoop:

	// Main Loop
	while (!done)
	{
		//Serial::HandleInput();

		//if(isGameMode)
		//	HandleGameMode();

        // Update Audio Input (Linux)
#ifdef __linux__

        if(isInternalMode && curAudioDevice && curAudioDevice->isRecording) {
            bool res = curAudioDevice->UpdateState();
            if (res) {
                //printf("Got results!\n");
                serialStatus = Status_Idle;
                AnalyzeData();
            }
        }

#endif

		// GUI Loop
		uint64_t curTime = micros();
		if ((curTime - lastFrameGui + (lastFrameRenderTime) >= 1000000 / (currentUserData.performance.VSync ?
			(GUI::MAX_SUPPORTED_FRAMERATE / currentUserData.performance.VSync) - 1 :
			currentUserData.performance.guiLockedFps)) || !currentUserData.performance.lockGuiFps || currentUserData.performance.VSync)
		{
			uint64_t frameStartRender = curTime;
			if (GUI_Return_Code = GUI::DrawGui())
				break;
			lastFrameRenderTime = lastFrameGui - frameStartRender;
			curTime += lastFrameRenderTime;
		}

		//uint64_t CPUTime = micros();

		uint64_t newFrame = curTime - lastFrame;

		if (newFrame > 1000000)
		{
			totalFrames++;
			lastFrame = curTime;
			continue;
		}

		// Average frametime calculation
		//printf("Frame: %i\n", frameIndex);
		frameIndex = _min(AVERAGE_FRAME_COUNT - 1, frameIndex);
		frameSum -= frames[frameIndex];
		frameSum += newFrame;
		frames[frameIndex] = newFrame;
		frameIndex = (frameIndex + 1) % AVERAGE_FRAME_COUNT;

		averageFrametime = ((float)frameSum) / AVERAGE_FRAME_COUNT;

		totalFrames++;
		lastFrame = curTime;

		// Try to sleep enough to "wake up" right before having to display another frame
		//if (!currentUserData.performance.VSync && currentUserData.performance.lockGuiFps)
		//{
		//	uint64_t frameTime = 1000000 / currentUserData.performance.guiLockedFps;
		//	uint64_t nextFrameIn = lastFrameGui + frameTime - micros() - lastFrameRenderTime;

		//	printf("Next frame in %lldus\n", nextFrameIn);
		//	
		//	if (nextFrameIn > 2500) {
		//		microsSleep(nextFrameIn - 1250);

		//		nextFrameIn = lastFrameGui + frameTime - micros() - lastFrameRenderTime;
		//		printf("Woke up %lldus before present\n", nextFrameIn);
		//	}

		//	//if (nextFrameIn > 25)
		//	//	microsSleep((nextFrameIn - 24));
		//		//std::this_thread::sleep_for(std::chrono::microseconds(nextFrameIn - 1050));
		//}

        // Actually do it on a semi-realtime OS
#ifdef __linux__
        if(!currentUserData.performance.VSync && currentUserData.performance.lockGuiFps) {
            //static uint64_t last_nextFrameIn;
            uint64_t frameTime = 1000000 / currentUserData.performance.guiLockedFps;
            uint64_t nextFrameIn = lastFrameGui + frameTime - micros() - lastFrameRenderTime;

            //if(last_nextFrameIn > 1000)
            //    printf("woke up %luus before (%luus)\n", nextFrameIn, last_frame_render_time - micros());
            //if(static_cast<int64_t>(nextFrameIn) < 0)
            //    printf("rendered %lius too late!\n", -static_cast<int64_t>(nextFrameIn));

            if (lastFrameGui + frameTime > micros() + lastFrameRenderTime)
                std::this_thread::sleep_for(std::chrono::microseconds(nextFrameIn - 60));

            //last_nextFrameIn = nextFrameIn;
        }
#endif

#ifdef _DEBUG
		// Limit FPS for eco-friendly purposes (Significantly affects the performance) (Windows does not support sub 1 millisecond sleep)*
		//Sleep(1);
#endif // _DEBUG
	}

	// Check if everything is saved before exiting the program.
	if (!tabsInfo[selectedTab].isSaved && GUI_Return_Code < 1)
	{
		goto MainLoop;
	}

#ifdef _WIN32
	if (curAudioDevice) {
		curAudioDevice->StopRecording();
		delete curAudioDevice;
	}
	timeEndPeriod(tc.wPeriodMin);
#else
    delete curAudioDevice;
#endif

	Serial::Close();
	GUI::Destroy();
}