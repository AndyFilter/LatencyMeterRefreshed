#include "AppCore.h"

#include <array>
#include <chrono>
#include <cmath>
#include <iostream>

#include "../GUI/Gui.h"
#include "Helpers/AppHelper.h"
#include "Serial/Serial.h"

using namespace AppHelper;

namespace AppCore {
	static AppState *g_State = nullptr;

	void BindState(AppState &state) { g_State = &state; }

	AudioProcessor g_audioProcessor;
	auto GetAudioProcessor() -> AudioProcessor & { return g_audioProcessor; }

	bool g_hasPartialRecordingEnded = false;
	std::uint64_t g_lastAnalyzedSnapshotEpoch = 0;
	void RecordingEnded() {
		if (g_State == nullptr) {
			return;
		}
		const auto epoch = g_audioProcessor.snapshotEpoch.load(std::memory_order_acquire);
		if (epoch == 0 || epoch == g_lastAnalyzedSnapshotEpoch)
			return;
		g_lastAnalyzedSnapshotEpoch = epoch;
		g_hasPartialRecordingEnded = true;
		g_State->serialStatus = Status_Idle;
		AnalyzeAudioData(*g_State);
	}

	bool HasRecordingEnded() {
		return g_hasPartialRecordingEnded;
	}

	void GotSerialCharThunk(const char c) {
		if (g_State == nullptr)
			return;

		GotSerialChar(*g_State, c);
	}

	void GotSerialChar(AppState &state, const char c) {
		if (c == 0 || !Serial::g_isConnected)
			return;

		DEBUG_PRINT("Got: %c\n", c);

		constexpr size_t RESULT_BUFFER_SIZE = 6;
		static std::array<std::uint8_t, RESULT_BUFFER_SIZE> resultBuffer{0};
		static std::uint8_t resultNum = 0;

		static std::chrono::high_resolution_clock::time_point internalStartTime;
		static std::chrono::high_resolution_clock::time_point internalEndTime;
		static std::chrono::high_resolution_clock::time_point pingStartTime;

		static unsigned int internalTime = 0;
		static unsigned int externalTime = 0;

		if ((c < 97 || c > 122) && (c < 48 || c > 57)) {
			externalTime = internalTime = 0;
			state.serialStatus = Status_Idle;
		}

		switch (state.serialStatus.load()) {
			case Status_Idle:
				if (c == 'l') {
					internalStartTime = std::chrono::high_resolution_clock::now();
					state.lastInternalTest = GetTimeMicros();

					DEBUG_PRINT("Waiting for results\n");

					state.serialStatus = Status_WaitingForResult;

					if (state.isAudioMode)
						g_audioProcessor.StartPlayback();

					if (state.isGameMode)
						MouseClick();
				}
				return;
			case Status_WaitingForResult:
				if (resultNum == 0)
					internalEndTime = std::chrono::high_resolution_clock::now();

				if (c == 'm' || c == 'u') {
					externalTime = 0;
					internalTime = static_cast<unsigned int>(
						std::chrono::duration_cast<std::chrono::microseconds>(internalEndTime - internalStartTime)
						.count());
					for (size_t i = 0; i < std::min(static_cast<size_t>(resultNum), RESULT_BUFFER_SIZE); i++) {
						externalTime += static_cast<unsigned int>(resultBuffer[i] * std::pow(10, (resultNum - i - 1)));
					}

					resultNum = 0;
					std::fill_n(resultBuffer.begin(), RESULT_BUFFER_SIZE, static_cast<std::uint8_t>(0));
					externalTime *= (c == 'u') ? 1 : 1000;

					if (std::max(externalTime, internalTime) > 1000000 || std::min(externalTime, internalTime) < 1000) {
						state.serialStatus = Status_Idle;
						break;
					}

					DEBUG_PRINT("Final result: %i\n", externalTime);

					Serial::Write("p", 1);
					pingStartTime = std::chrono::high_resolution_clock::now();
					state.serialStatus = Status_WaitingForPing;
				} else {
					const int digit = c - '0';
					if (resultNum < RESULT_BUFFER_SIZE) {
						resultBuffer[resultNum] = static_cast<int>(digit);
						resultNum += 1;
					}
				}
				break;
			case Status_WaitingForPing:
				if (c == 'b') {
					const unsigned int pingTime =
							static_cast<unsigned int>(std::chrono::duration_cast<std::chrono::microseconds>(
										std::chrono::high_resolution_clock::now() - pingStartTime)
								.count());
					AddMeasurement(internalTime, externalTime, pingTime);
					state.serialStatus = Status_Idle;
				}
				break;
			default:
				break;
		}
	}

	void ClearData(AppState &state) {
		std::scoped_lock const lock(g_State->latencyStatsMutex);
		state.serialStatus = Status_Idle;
		g_State->tabsInfo[g_State->selectedTab].latencyData.measurements.clear();
		g_State->tabsInfo[g_State->selectedTab].latencyStats = LatencyStats();
		g_State->tabsInfo[g_State->selectedTab].isSaved = true;
		ZeroMemory(g_State->tabsInfo[g_State->selectedTab].latencyData.note, 1000);
	}

	void DiscoverAudioDevices(AppState &state) {
		state.availableAudioDevices.clear();
		g_audioProcessor.Initialize();
		try {
			g_audioProcessor.GetAudioDevices(state.availableAudioDevices);
		}
		catch (std::exception &e) {
			DEBUG_ERROR_LOC("Error while discovering audio devices: %s\n", e.what());
		}

		if (!state.availableAudioDevices.empty()) {
			state.selectedPort %= state.availableAudioDevices.size();
			state.selectedOutputAudioDeviceIdx %= state.availableAudioDevices.size();

			bool in = false;
			bool out = false;
			for (size_t i = 0; i < state.availableAudioDevices.size() && (!in || !out); i++) {
				const auto &dev = state.availableAudioDevices[i];
				if (dev.isInput && !in) {
					in = true;
					if (state.isInternalMode)
						state.selectedPort = i;
					else
						state.lastSelectedPort = i;
				}

				if (!dev.isInput && !out) {
					out = true;
					state.selectedOutputAudioDeviceIdx = i;
				}
			}
		}
	}

	static auto RandInRange(const int lower, const int upper) -> int { return lower + (rand() % (upper - lower)); }

	bool CanDoInternalTest(const AppState &state, uint64_t time) {
		time = time == 0 ? GetTimeMicros() : time;
		static int randomVal = RandInRange(2, 10);
		static bool setRandomVal = false;

		if (state.isInternalMode && state.isAudioDevConnected && (state.isPlaybackDevConnected || !state.isAudioMode) &&
		    (time > state.lastInternalTest + (INTERNAL_TEST_DELAY + randomVal))) {
			setRandomVal = false;
			return true;
		}

		if (!setRandomVal)
			randomVal = RandInRange(80000, 400000);
		setRandomVal = true;
		return false;
	}

	void StartInternalTest(AppState &state) {
		if (!CanDoInternalTest(state)) {
			return;
		}

		g_hasPartialRecordingEnded = false;
		state.serialStatus = Status_WaitingForResult;
		state.lastInternalTest = GetTimeMicros();
		if (!g_audioProcessor.StartRecording()) {
			g_hasPartialRecordingEnded = true;
			state.serialStatus = Status_Idle;
			return;
		}

		DEBUG_PRINT("Starting Rec took %ldus\n", GetTimeMicros() - state.lastInternalTest);

		if (state.isAudioMode)
			g_audioProcessor.StartPlayback();
	}

	void AttemptConnect(AppState &state) {
		try {
			if (state.isInternalMode && !state.availableAudioDevices.empty()) {
				if (state.isAudioDevConnected)
					g_audioProcessor.Terminate();

				state.selectedPort %= state.availableAudioDevices.size();
				state.selectedOutputAudioDeviceIdx %= state.availableAudioDevices.size();
				state.isAudioDevConnected =
						g_audioProcessor.Initialize() &&
						g_audioProcessor.PrimeRecordingStream(state.availableAudioDevices[state.selectedPort].id);

				if (state.isAudioMode)
					state.isPlaybackDevConnected = g_audioProcessor.PrimePlaybackStream(
							state.availableAudioDevices[state.selectedOutputAudioDeviceIdx].id);

				if (state.isAudioDevConnected) {
					state.lastInternalTest = GetTimeMicros();
					state.serialStatus = Status_WaitingForResult;
				}
			} else {
				if (Serial::g_isConnected)
					Serial::Close();

				const std::string port = Serial::g_availablePorts.empty() ? "" : Serial::g_availablePorts[state.selectedPort];
				Serial::Setup(port.c_str(), AppCore::GotSerialCharThunk);
			}
		}
		catch (std::exception &e) {
			DEBUG_ERROR_LOC("Error while connecting: %s\n", e.what());
		}
	}

	void AttemptDisconnect(AppState &state) {
		if (state.isInternalMode) {
			state.isAudioDevConnected = false;
			state.isPlaybackDevConnected = false;
			g_audioProcessor.StopRecording();
			g_audioProcessor.StopPlayback();
			g_audioProcessor.Terminate();
		} else {
			Serial::Close();
		}
		state.serialStatus = Status_Idle;
	}

	// Called from recEndCallback
	bool AnalyzeAudioData(AppState &state) {
		state.lastInternalGateSampleIdx = 0;
		constexpr std::uint32_t BUFFER_MAX_VALUE = ((1 << (sizeof(short) * 8)) / 2) - 1;
		const int bufferThreshold = BUFFER_MAX_VALUE / 5 / (state.isAudioMode ? 2 : 1);

		int baseAvg = 0;
		constexpr short AVG_COUNT = 10;

		const auto *samplesBuffer = g_audioProcessor.GetSnapshotBuffer();
		for (size_t sampleIdx = 0; sampleIdx < FRAMES_TO_CAPTURE / MAIN_BUFFER_SIZE_FRACTION_RECIPROCAL; sampleIdx++) {
			if (sampleIdx < AVG_COUNT) {
				baseAvg += samplesBuffer[sampleIdx];
				continue;
			}
			if (sampleIdx == AVG_COUNT) {
				baseAvg /= AVG_COUNT;
				baseAvg = std::abs(baseAvg);
			}

			if (const auto sample = static_cast<short>(std::abs(samplesBuffer[sampleIdx] - baseAvg));
				sample >= bufferThreshold) {
				const auto microsElapsed = static_cast<float>(1000000ull * (sampleIdx) / REC_SAMPLE_RATE);
				if (static_cast<float>(baseAvg) > BUFFER_MAX_VALUE * 0.9f || microsElapsed <= 1000)
					return false;
				DEBUG_PRINT("Found sample over the threshold\n");
				AddMeasurement(static_cast<std::uint32_t>(microsElapsed), 0, 0);
				state.lastInternalGateSampleIdx = sampleIdx;
				return true;
			}
		}
		return false;
	}

	bool Initialize() {
		g_audioProcessor.Initialize();
		return true;
	}

	bool OnExit() {
		g_audioProcessor.Terminate();
		return true;
	}

	void Update(AppState &state) {
		if (!g_hasPartialRecordingEnded && GetAudioProcessor().snapshotReady.load()) {
			DEBUG_PRINT("Ready to analyse!\n");
			RecordingEnded();
		}
	}
} // namespace AppCore
