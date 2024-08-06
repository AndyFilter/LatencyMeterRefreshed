#pragma once

// God, I LOVE Microsoft!
#ifdef _MSC_VER
#include "../Dependencies/PortAudio/Include/portaudio.h"
#else
#include "Dependencies/PortAudio/Include/portaudio.h"
#endif
#include <vector>
#include <iostream>
#include "../constants.h"

#define REC_SAMPLE_RATE 44100
#define SAMPLES_PER_BUFFER 256
#define FRAMES_TO_CAPTURE (SAMPLES_PER_BUFFER * 100) // Should be divisible by SAMPLES_PER_BUFFER,
// if not, greater multiple of SAMPLES_PER_BUFFER of frames will be captured.
#define MAIN_BUFFER_SIZE_FRACTION 3 // one-over-x (1/x). How much of the whole buffer (FRAMES_TO_CAPTURE) should be
// used for the audio detection. The rest is to "calm down" the audio processor before the next measurement.
const float MAIN_BUFFER_TIME_SPAN = ((float)FRAMES_TO_CAPTURE / MAIN_BUFFER_SIZE_FRACTION) / REC_SAMPLE_RATE; // in seconds
const float BUFFER_TIME_SPAN = (float)FRAMES_TO_CAPTURE / REC_SAMPLE_RATE; // in seconds

#define OUTPUT_BUFFER_TYPE short

class cAudioDeviceInfo
{
public:
    unsigned int id;
    bool AudioEnchantments = false;
    const char* friendlyName;
    const char* portName; // EnumerationName
    bool isInput = false;

    cAudioDeviceInfo(const unsigned int id, bool enchantments, const char* friendlyName, const char* portName, bool is_input)
            : id(id), friendlyName(friendlyName), portName(portName),
              AudioEnchantments(enchantments), isInput(is_input)
    {
    }

    cAudioDeviceInfo() = default;
};



class AudioProcessor {
public:
    std::vector<short> recordedSamples;
    std::vector<OUTPUT_BUFFER_TYPE> playbackSamples;

    AudioProcessor(void (*_recEndCallback)()) : in_stream(nullptr), playbackIndex(0), isRecording(false), isPlaying(false), recEndCallback(_recEndCallback) {};
    ~AudioProcessor();

    bool initialize();
    void terminate();
    void restart(); // Stops streams, terminates and initializes

    unsigned int GetAudioDevices(std::vector<cAudioDeviceInfo> &devices);

    bool primeRecordingStream(PaDeviceIndex idx); // Prepares the stream for recording
    bool startRecording(); // Requires the stream to be primed beforehand!
    void stopRecording();
    bool primePlaybackStream(PaDeviceIndex idx); // Prepares the stream for playback
    bool startPlayback(); // Requires the stream to be primed beforehand!
    void stopPlayback();

private:
    static int recordCallback(const void *inputBuffer, void *outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo* timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData);

    static int playbackCallback(const void *inputBuffer, void *outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void *userData);

    void flushPlaybackBuffer();
    void flushRecordBuffer();

    PaStream *in_stream;
    PaStream *out_stream = nullptr;
    size_t playbackIndex;
    bool isRecording;
    bool isPlaying;
    size_t frames_captured = 0;
    void (*recEndCallback)();

    PaDeviceIndex in_dev_idx = -1;
    PaDeviceIndex out_dev_idx = -1;

    static bool is_initialized;
};