#pragma once

#include <array>
#include <atomic>
#include <portaudio.h>
#include <string>
#include <utility>
#include <vector>

constexpr unsigned int REC_SAMPLE_RATE = 44100;
constexpr unsigned int AUDIO_SAMPLE_RATE = REC_SAMPLE_RATE;
constexpr unsigned int SAMPLES_PER_BUFFER = 256;
constexpr unsigned int FRAMES_TO_CAPTURE = (SAMPLES_PER_BUFFER * 100); // Should be divisible by SAMPLES_PER_BUFFER,
// if not, a greater multiple of SAMPLES_PER_BUFFER of frames will be captured.

// one-over-x (1/x). How much of the whole buffer (FRAMES_TO_CAPTURE) should be used for the audio detection.
// The rest is to "calm down" the audio processor before the next measurement.
constexpr unsigned int MAIN_BUFFER_SIZE_FRACTION_RECIPROCAL = 3;

constexpr unsigned int ANALYSIS_FRAMES = FRAMES_TO_CAPTURE / MAIN_BUFFER_SIZE_FRACTION_RECIPROCAL;
constexpr float MAIN_BUFFER_TIME_SPAN =
    (static_cast<float>(FRAMES_TO_CAPTURE) / MAIN_BUFFER_SIZE_FRACTION_RECIPROCAL) / REC_SAMPLE_RATE; // in seconds
constexpr float BUFFER_TIME_SPAN = static_cast<float>(FRAMES_TO_CAPTURE) / REC_SAMPLE_RATE; // in seconds

#define OUTPUT_BUFFER_TYPE short

class AudioDeviceInfo
{
public:
    unsigned int id{};
    bool audioEnchantments = false;
    std::string friendlyName{};
    const char* portName{}; // EnumerationName
    bool isInput = false;

    AudioDeviceInfo(const unsigned int audioDeviceId, const bool enchantments, std::string friendlyName,
                    const char* portName, const bool is_input) :
        id(audioDeviceId), audioEnchantments(enchantments), friendlyName(std::move(friendlyName)), portName(portName),
        isInput(is_input)
    {
    }

    AudioDeviceInfo() = default;
};


/**
 * @class AudioProcessor
 * @brief Handles audio capture and playback operations using the PortAudio library.
 *
 * The AudioProcessor class provides functionality to record and playback audio
 * using streams. It includes methods to initialise, terminate, prime, start, and
 * stop audio streams for both recording and playback. The class manages internal
 * buffers and device indices for audio operations.
 */
class AudioProcessor
{
public:
	std::vector<OUTPUT_BUFFER_TYPE> playbackSamples;

	// Whole capture buffers (double-buffered write side)
	std::array<std::array<short, FRAMES_TO_CAPTURE>, 2> captureBuffers{};

	// Published snapshot used by BOTH analysis and GUI (first 1/3 only)
	std::array<short, ANALYSIS_FRAMES> snapshotBuffer{};

	std::atomic<bool> snapshotReady{false};
	std::atomic<size_t> snapshotEpoch{0};

    AudioProcessor() = default;
    ~AudioProcessor();

    bool Initialize();
    void Terminate();
    void Restart(); // Stops streams, terminates and initialises

    unsigned int GetAudioDevices(std::vector<AudioDeviceInfo>& devices) const;

	const short* GetSnapshotBuffer() const {
		return snapshotBuffer.data();
	}

    bool PrimeRecordingStream(PaDeviceIndex idx); // Prepares the stream for recording
    bool StartRecording(); // Requires the stream to be primed beforehand!
    void StopRecording();
    bool PrimePlaybackStream(PaDeviceIndex idx); // Prepares the stream for playback
    bool StartPlayback(); // Requires the stream to be primed beforehand!
    void StopPlayback();

private:
    static int RecordCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
                              void* userData);

    static int PlaybackCallback(const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
                                void* userData);

    void FlushPlaybackBuffer();
    void FlushRecordBuffer();

    PaStream* inStream_ = nullptr;
    PaStream* outStream_ = nullptr;

    size_t playbackIndex_{0};

    bool isRecording_ = false;
    bool isPlaying_ = false;
	bool calledEnd_ = false;

	bool snapshotPublished_ = false;
    size_t framesCaptured_ = 0;
	int writeBufferIndex_ = 1;

	std::atomic<size_t> currentEpoch_{0};

    PaDeviceIndex inDevIdx_ = -1;
    PaDeviceIndex outDevIdx_ = -1;

    bool isInitialized_ = false;
};
