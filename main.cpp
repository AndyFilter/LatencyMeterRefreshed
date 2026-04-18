#ifdef _WIN32
#ifdef NDEBUG
#pragma comment(linker, "/SUBSYSTEM:windows")
#endif
#define NOMINMAX
#include <Windows.h>
#include <tchar.h>
#endif
#include <cstring>
#include <fstream>
#include "App/AppCore.h"
#include "App/AppGui.h"
#include "App/AppInput.h"
#include "App/AppState.h"
#include "App/Config.h"
#include "App/Models.h"
#include "Audio/AudioProcessor.h"
#include "External/ImGui/imgui.h"
#include "External/ImGui/imgui_internal.h"
#include "GUI/Gui.h"
#include "GUI/UIComponents.h"
#include "Helpers/AppHelper.h"
#include "Helpers/JsonHelper.h"
#include "Serial/Serial.h"

using namespace AppHelper;
using namespace AppCore;
using namespace UIComponents;
using namespace AppGui;
using namespace AppInput;

static AppState g_appState;

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
- Add option to display frametime (?) [Should be something like (2/(framerate)) * 1000]
+ See what can be done about the inefficient Serial Comm code
- Inform user of all the available keyboard shortcuts
+ Clean-up the audio code
+ Handle unplugging audio devices mid test
- Arduino debug mode (print sensor values etc.)
- Overall better communication with Arduino
- A way to change the Audio Latency "beep" wave function / frequency
- User-friendly notifications about errors and such

*/

#ifdef _WIN32
bool GetMonitorModes(DXGI_MODE_DESC *modes, UINT *size) {
	IDXGIOutput *output;

	if (Gui::g_pSwapChain->GetContainingOutput(&output) != S_OK)
		return false;

	// DXGI_FORMAT_R8G8B8A8_UNORM
	if (output->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, 0, size, modes) != S_OK) {
		fprintf(stderr, "Error getting the display modes\n");
		return false;
	}

	// if (*size > 0)
	//{
	//	auto targetMode = modes[*size - 1];
	//	targetMode.RefreshRate.Numerator = 100000000;
	//	DXGI_MODE_DESC matchMode;
	//	output->FindClosestMatchingMode(&targetMode, &matchMode, Gui::g_pd3dDevice);

	//	printf("Best mode match: %ix%i@%iHz\n", matchMode.Width, matchMode.Height, (matchMode.RefreshRate.Numerator /
	// matchMode.RefreshRate.Denominator));
	//}

	return true;
}
#endif


// Returns true if should exit, false otherwise
bool LocalOnExit() {
	bool isSaved = true;

	g_appState.unsavedTabs.clear();

	for (size_t i = 0; i < g_appState.tabsInfo.size(); i++) {
		if (!g_appState.tabsInfo[i].isSaved) {
			isSaved = false;
			g_appState.unsavedTabs.push_back(static_cast<int>(i));
		}
	}

	if (isSaved) {
		// Gui closes itself from the wnd messages
		Serial::Close();
		OnExit();

		exit(0);
	}

	g_appState.isExiting = true;
	return isSaved;
}

// Main code
#if defined(_WIN32) && defined(NDEBUG)
int WINAPI WinMain(HINSTANCE  /*hInstance*/, HINSTANCE  /*hPrevInstance*/, PSTR  /*lpCmdLine*/, int  /*nCmdShow*/) {
#else
int main(int, char **) {
#endif
	static uint64_t lastFrameGui = 0;
	static uint64_t lastFrame = 0;
	constexpr unsigned int AVERAGE_FRAME_COUNT = 1000;
	static uint64_t frames[AVERAGE_FRAME_COUNT]{0};

	static size_t frameIndex = 0;
	static uint64_t frameSum = 0;

	AppCore::BindState(g_appState);
	AppInput::BindState(g_appState);
	AppGui::BindState(g_appState);
	AppHelper::BindState(g_appState);

#ifdef _WIN32
	Gui::Setup(OnGuiThunk);
#else
	Gui::g_vSyncFrame = &g_appState.currentUserData.performance.VSync;
	Gui::Setup(OnGuiThunk);
	Gui::RegKeyCallback(KeyCallback);
#endif
	Gui::g_onExitFunc = LocalOnExit;

	srand(time(nullptr));

	GetAudioProcessor().Initialize();
	UIComponents::SetupData();

#ifdef _WIN32
	QueryPerformanceCounter(&g_StartingTime);
#endif

	// Doesnt do that much (anything) and I'd rather not be this "critical"
	// if (SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
	//	printf("Priority set to the Highest\n");

#ifdef _WIN32
#ifndef _DEBUG
	// ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
#else
	//::ShowWindow(::GetConsoleWindow(), SW_SHOW);
#endif
#endif

	//::ShowWindow(::GetConsoleWindow(), SW_SHOW);
	// AllocConsole();
	// freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);

	ImGuiIO &io = ImGui::GetIO();

#ifdef _WIN32
	Gui::KeyDownFunc = KeyDown;
	// Gui::RawInputEventFunc = RawInputEvent;
#endif

	auto defaultTab = TabInfo();
	defaultTab.name[4] = '0';
	defaultTab.name[5] = '\0';
	g_appState.tabsInfo.push_back(defaultTab);

#ifdef _WIN32
	GetMonitorModes(g_appState.monitorModes, &g_appState.modesNum);

	Gui::g_maxSupportedFramerate = g_appState.monitorModes[g_appState.modesNum - 1].RefreshRate.Numerator;
	Gui::g_vSyncFrame = &g_appState.currentUserData.performance.VSync;
#endif

	LoadAndHandleNewUserConfig();

	// Just imagine what would happen if user for example had a 220 character long username and path to %AppData% +
	// "imgui.ini" would exceed 260. lovely
	auto imguiFileIniPath = (g_appState.currentUserData.misc.localUserData ? HelperJson::GetLocalUserConfigPath()
																		   : HelperJson::GetAppDataUserConfigPath());
	imguiFileIniPath = imguiFileIniPath.substr(0, imguiFileIniPath.find_last_of(OS_SPEC_PATH("\\/")) + 1) +
					   OS_SPEC_PATH("imgui.ini");

#ifdef _WIN32
	char outputPathBuffer[MAX_PATH];
	wcstombs_s(nullptr, outputPathBuffer, MAX_PATH, imguiFileIniPath.c_str(), imguiFileIniPath.size());
	io.IniFilename = outputPathBuffer;
#else
	// wcstombs(outputPathBuffer, imguiFileIniPath.c_str(), imguiFileIniPath.size());
	io.IniFilename = imguiFileIniPath.c_str();
#endif

	Serial::FindAvailableSerialPorts();

	DEBUG_PRINT("Found %zu serial Ports\n", Serial::g_availablePorts.size());

	DiscoverAudioDevices(g_appState);

	// Setup Audio buffer (with noise)
	// audioPlayer.SetBuffer([](int i) { return ((rand() * 2 - RAND_MAX) % RAND_MAX) * 10; });
	// audioPlayer.SetBuffer([](int i) { return rand() % SHRT_MAX; });
	// audioPlayer.SetBuffer([](int i) { return (int)(sinf(i/4.f) * RAND_MAX); });
	GetAudioProcessor().playbackSamples.resize(2000);
	for (int i = 0; i < GetAudioProcessor().playbackSamples.size(); i++)
		GetAudioProcessor().playbackSamples[i] = std::sin(i / 10.f) * SHRT_MAX;
	// audioProcessor.playbackSamples[i] = rand();

	// used to cover the time of rendering a frame when calculating when a next frame should be displayed
	uint64_t lastFrameRenderTime = 0;

	int GUI_Return_Code = 0;

#ifdef _WIN32
	TIMECAPS tc;
	timeGetDevCaps(&tc, sizeof(tc));
	timeBeginPeriod(tc.wPeriodMin);
#endif

	constexpr bool done = false;
MainLoop:
	while (!done) {
		uint64_t curTime = GetTimeMicros();

		if (g_appState.isInternalMode && g_appState.isAudioDevConnected) {
			static bool isProcReset = false;

			if (!isProcReset && g_appState.serialStatus == Status_WaitingForResult) {
				isProcReset = true;
			}
			// checks if the audio processor ended writing to the (whole) buffer
			if (isProcReset && g_appState.serialStatus == Status_Idle &&
				(curTime - g_appState.lastInternalTest) >
						((1000 * static_cast<uint64_t>(FRAMES_TO_CAPTURE) / AUDIO_SAMPLE_RATE) + (INTERNAL_TEST_DELAY))) {
				DEBUG_PRINT("Buffer Ended\n");
				GetAudioProcessor().StopRecording();
				GetAudioProcessor().PrimeRecordingStream(g_appState.availableAudioDevices[g_appState.selectedPort].id);
				if (g_appState.isAudioMode)
					GetAudioProcessor().PrimePlaybackStream(
							g_appState.availableAudioDevices[g_appState.selectedOutputAudioDeviceIdx].id);
				isProcReset = false;
			}
		}

		AppCore::Update(g_appState);

		// GUI Loop
		if ((curTime - lastFrameGui + (lastFrameRenderTime) >=
			 1000000 / (g_appState.currentUserData.performance.VSync
								? (Gui::g_maxSupportedFramerate / g_appState.currentUserData.performance.VSync) - 1
								: g_appState.currentUserData.performance.guiLockedFps)) ||
			!g_appState.currentUserData.performance.lockGuiFps || g_appState.currentUserData.performance.VSync) {
			const uint64_t frameStartRender = curTime;
			if ((GUI_Return_Code = Gui::DrawGui()))
				break;
			lastFrameGui = GetTimeMicros();
			lastFrameRenderTime = lastFrameGui - frameStartRender;
			curTime += lastFrameRenderTime;
		}

		// uint64_t CPUTime = micros();

		const uint64_t newFrame = curTime - lastFrame;

		if (newFrame > 1000000) {
			g_appState.totalFrames++;
			lastFrame = curTime;
			continue;
		}

		// Average frametime calculation
		// printf("Frame: %i\n", frameIndex);
		frameIndex = std::min(static_cast<size_t>(AVERAGE_FRAME_COUNT) - 1, frameIndex);
		frameSum -= frames[frameIndex];
		frameSum += newFrame;
		frames[frameIndex] = newFrame;
		frameIndex = (frameIndex + 1) % AVERAGE_FRAME_COUNT;

		g_appState.averageFrametime = static_cast<float>(frameSum) / AVERAGE_FRAME_COUNT;

		g_appState.totalFrames++;
		lastFrame = curTime;

		// Try to sleep enough to "wake up" right before having to display another frame (Only worth doing on Linux)
#ifdef __linux__
		if (g_appState.currentUserData.performance.VSync == 0u && g_appState.currentUserData.performance.lockGuiFps) {
			// static uint64_t last_nextFrameIn;
			const uint64_t frameTime = 1000000 / g_appState.currentUserData.performance.guiLockedFps;
			const uint64_t nextFrameIn = lastFrameGui + frameTime - GetTimeMicros() - lastFrameRenderTime;

			// if(last_nextFrameIn > 1000)
			//     printf("woke up %luus before (%luus)\n", nextFrameIn, last_frame_render_time - micros());
			// if(static_cast<int64_t>(nextFrameIn) < 0)
			//     printf("rendered %lius too late!\n", -static_cast<int64_t>(nextFrameIn));

			if (lastFrameGui + frameTime > GetTimeMicros() + lastFrameRenderTime)
				std::this_thread::sleep_for(std::chrono::microseconds(nextFrameIn - 32));

			// last_nextFrameIn = nextFrameIn;
		}
#endif

#ifdef _DEBUG
		// Limit FPS for eco-friendly purposes (Significantly affects the performance)
		// (Windows does not support sub millisecond sleep)
		// Sleep(1);
#endif // _DEBUG
	}

	// Check if everything is saved before exiting the program.
	if (!g_appState.tabsInfo.empty() && !g_appState.tabsInfo[g_appState.selectedTab].isSaved && GUI_Return_Code < 1) {
		goto MainLoop;
	}

	if (g_appState.isAudioDevConnected) {
		GetAudioProcessor().Terminate();
		// delete curAudioDevice;
	}
#ifdef _WIN32
	timeEndPeriod(tc.wPeriodMin);
#endif

	Serial::Close();
	Gui::Destroy();
}
