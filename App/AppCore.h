#pragma once

#include <cstdint>
#include "Audio/AudioProcessor.h"


struct AppState;
namespace AppCore {
	constexpr std::uint32_t INTERNAL_TEST_DELAY = 1195387; // microseconds

	void BindState(AppState &state); // State binding for Audio and Serial handling

	AudioProcessor &GetAudioProcessor();
	void RecordingEnded();
	bool HasRecordingEnded();

	void GotSerialChar(AppState &state, char c);
	void GotSerialCharThunk(char c);

	void ClearData(AppState &state);
	void DiscoverAudioDevices(AppState &state);
	void StartInternalTest(AppState &state);
	bool CanDoInternalTest(const AppState &state, uint64_t time = 0);
	void AttemptConnect(AppState &state);
	void AttemptDisconnect(AppState &state);
	bool AnalyzeAudioData(AppState &state);

	bool Initialize();
	bool OnExit();
	void Update(AppState &state);
} // namespace AppCore
