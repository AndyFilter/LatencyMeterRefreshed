#include <cstring>
#include <cmath>
#include "Sound_Helper.h"

bool AudioProcessor::is_initialized = false;
//AudioProcessor::AudioProcessor() : in_stream(nullptr), playbackIndex(0), isRecording(false), isPlaying(false) {}

AudioProcessor::~AudioProcessor() {
    terminate();
}

bool AudioProcessor::initialize() {
    if(is_initialized) return true;
#ifdef _DEBUG
    printf("INITIALIZING PORT AUDIO!\n");
#endif
    PaError err = Pa_Initialize();
    if(recordedSamples.size() != FRAMES_TO_CAPTURE)
        recordedSamples.resize(FRAMES_TO_CAPTURE);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }
    is_initialized = true;
    return true;
}

void AudioProcessor::terminate() {
#ifdef _DEBUG
    printf("TERMINATING PORT AUDIO!\n");
#endif
    if (isRecording) stopRecording();
    if (isPlaying) stopPlayback();
    Pa_Terminate();
    in_stream = nullptr;
    out_stream = nullptr;
    in_dev_idx = -1;
    out_dev_idx = -1;
    is_initialized = false;
}

void AudioProcessor::restart() {
#ifdef _WIN32
    if(!is_initialized) return;

    PaError _err = Pa_CloseStream(in_stream);
    if (_err != paNoError) {
        std::cerr << "PortAudio (CloseStream) error: " << Pa_GetErrorText(_err) << std::endl;
    }
    in_stream = nullptr;
    out_stream = nullptr;

    Pa_Terminate();
    Pa_Initialize();
#endif
}

bool AudioProcessor::primeRecordingStream(PaDeviceIndex idx) {
    if(!is_initialized) return false;
    if(in_stream && idx == in_dev_idx) {
        Pa_StopStream(&in_stream);
        return true;
    }

    in_stream = nullptr;
    PaError err;
    PaStreamParameters inputParameters;
    inputParameters.device = idx == -1 ? Pa_GetDefaultInputDevice() : idx;

    if (inputParameters.device == paNoDevice || !Pa_GetDeviceInfo(inputParameters.device)) {
        std::cerr << "No default input device.\n";
        return false;
    }
    inputParameters.channelCount = 1;
    inputParameters.sampleFormat = paInt16;
    inputParameters.suggestedLatency = 1;//Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
    inputParameters.hostApiSpecificStreamInfo = nullptr;

#ifdef _DEBUG
    printf("Priming in buffer!\n");
#endif
    in_dev_idx = idx;
    frames_captured = 0;
    //recordedSamples.clear();
    recordedSamples.resize(FRAMES_TO_CAPTURE);
    err = Pa_OpenStream(&in_stream, &inputParameters, nullptr, REC_SAMPLE_RATE, 32, paClipOff, recordCallback, this);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    return true;
}

bool AudioProcessor::startRecording() {
    PaError err = Pa_StartStream(in_stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    isRecording = true;
    return true;
}

void AudioProcessor::stopRecording() {
    if (!isRecording) return;

#ifdef _DEBUG
    printf("Stopping recording\n");
#endif

    PaError err = Pa_StopStream(in_stream);
    if (err != paNoError) {
        std::cerr << "PortAudio (StopStream) error: " << Pa_GetErrorText(err) << std::endl;
    }

    //flushRecordBuffer();

    frames_captured = 0;

    //err = Pa_CloseStream(in_stream);
    //if (err != paNoError) {
    //    std::cerr << "PortAudio (CloseStream) error: " << Pa_GetErrorText(err) << std::endl;
    //}

    isRecording = false;
}

bool AudioProcessor::primePlaybackStream(PaDeviceIndex idx) {
    if(!is_initialized) return false;
    if(out_stream && out_dev_idx == idx) {
        stopPlayback();
        return true;
    }
    PaError err;
    if(out_dev_idx == idx) {
        // Try to close the old stream, but don't panic if it fails
        Pa_CloseStream(out_stream);
        //if(err != paNoError) {
        //    std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        //}
    }
    out_stream = nullptr;
    PaStreamParameters outputParameters;
    outputParameters.device = idx == -1 ? Pa_GetDefaultOutputDevice() : idx;
    if (outputParameters.device == paNoDevice) {
        std::cerr << "No default output device.\n";
        return false;
    }
    auto params = Pa_GetDeviceInfo(outputParameters.device);
    outputParameters.channelCount = 1;
    outputParameters.sampleFormat = paInt16; // assumes OUTPUT_BUFFER_TYPE is short
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;

#ifdef _DEBUG
    printf("Priming out buffer!\n");
#endif
    playbackIndex = 0;
    out_dev_idx = idx;
    err = Pa_OpenStream(&out_stream, nullptr, &outputParameters, params->defaultSampleRate, SAMPLES_PER_BUFFER,
                        paClipOff, playbackCallback, this);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    return true;
}

bool AudioProcessor::startPlayback() {
    PaError err = Pa_StartStream(out_stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return false;
    }

    isPlaying = true;
    return true;
}

void AudioProcessor::stopPlayback() {
    if (!isPlaying) return;

    PaError err;

    err = Pa_StopStream(out_stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

#ifdef _DEBUG
    printf("Stopped playback!\n");
#endif

#ifdef __linux__
    flushPlaybackBuffer();
#endif

    playbackIndex = 0;
    //err = Pa_CloseStream(out_stream);
    //if (err != paNoError) {
    //    std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    //}

    //out_stream = nullptr;

    isPlaying = false;
}

int AudioProcessor::recordCallback(const void *inputBuffer, void *outputBuffer,
                                   unsigned long framesPerBuffer,
                                   const PaStreamCallbackTimeInfo* timeInfo,
                                   PaStreamCallbackFlags statusFlags,
                                   void *userData) {
    AudioProcessor *processor = static_cast<AudioProcessor*>(userData);
    const short *in = static_cast<const short*>(inputBuffer);

    static bool called_end = false;

    if(statusFlags) {
        printf("status flag = %lu", statusFlags);
    }

    if(framesPerBuffer != 576) {
        //printf("Frames Per Buffer = %lu\n", framesPerBuffer);
    }

    if (inputBuffer != nullptr && processor->recordedSamples.size() >= processor->frames_captured + framesPerBuffer) {
        memcpy(processor->recordedSamples.data() + processor->frames_captured, in, framesPerBuffer * sizeof(short));
        processor->frames_captured += framesPerBuffer;
    }

    if(processor->frames_captured >= FRAMES_TO_CAPTURE / MAIN_BUFFER_SIZE_FRACTION && !called_end) {
        processor->recEndCallback();
        called_end = true;
    }

    if(processor->recordedSamples.size() <= processor->frames_captured + framesPerBuffer * 2) {
#ifdef _DEBUG
        printf("captured %zu frames, fpb = %lu\n", processor->frames_captured, framesPerBuffer);
#endif
        called_end = false;
        //processor->stopRecording();
        return paComplete;
    }

    return paContinue;
}

int AudioProcessor::playbackCallback(const void *inputBuffer, void *outputBuffer,
                                     unsigned long framesPerBuffer,
                                     const PaStreamCallbackTimeInfo* timeInfo,
                                     PaStreamCallbackFlags statusFlags,
                                     void *userData) {
    AudioProcessor *processor = static_cast<AudioProcessor*>(userData);
    OUTPUT_BUFFER_TYPE *out = static_cast<OUTPUT_BUFFER_TYPE*>(outputBuffer);

    if(outputBuffer) {
        memcpy(out, processor->playbackSamples.data() + processor->playbackIndex, framesPerBuffer * sizeof(OUTPUT_BUFFER_TYPE));
        processor->playbackIndex += framesPerBuffer;
    }

    if(processor->playbackSamples.size() <= processor->playbackIndex + framesPerBuffer * 2) {
        return paComplete;
    }

    return paContinue;
}

unsigned int AudioProcessor::GetAudioDevices(std::vector<cAudioDeviceInfo> &devices) {
    if(!is_initialized) return 1;

    int numDevices = Pa_GetDeviceCount();
    if (numDevices < 0) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(numDevices) << std::endl;
        return 0;
    }

    const PaDeviceInfo *deviceInfo;
    const PaHostApiInfo *hostApiInfo;
    for (int i = 0; i < numDevices; ++i) {
        deviceInfo = Pa_GetDeviceInfo(i);
        hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

#ifdef _DEBUG
        std::cout << "Device #" << i << ": " << deviceInfo->name << std::endl;
        std::cout << "  Host API: " << hostApiInfo->name << std::endl;
#endif

        size_t name_size = strlen(deviceInfo->name);
        size_t host_api_size = strlen(hostApiInfo->name);
        char* szName = new char[name_size + host_api_size + 1 + 3];
#ifdef _WIN32
        strcpy_s(szName + 1, host_api_size + 1, hostApiInfo->name);
        szName[0] = '('; szName[host_api_size + 1] = ')'; szName[host_api_size + 2] = ' ';
        strcpy_s(szName + host_api_size + 3, name_size + 1, deviceInfo->name);
#else
        memcpy(szName + 1, hostApiInfo->name, host_api_size + 1);
        szName[0] = '('; szName[host_api_size + 1] = ')'; szName[host_api_size + 2] = ' ';
        memcpy(szName + host_api_size + 3, deviceInfo->name, name_size + 1);
#endif
        cAudioDeviceInfo device(i, false, szName, hostApiInfo->name, false);

        PaStreamParameters params;
        params.device = i;
        params.channelCount = 1;
        params.sampleFormat = paInt16;
        params.hostApiSpecificStreamInfo = nullptr;
        params.suggestedLatency = 0.01;

        if (deviceInfo->maxInputChannels > 0) {
            PaError err =  Pa_IsFormatSupported(&params, nullptr, REC_SAMPLE_RATE);
            if(err == paNoError) {
                device.isInput = true;
                devices.push_back(device);
            }
            else {
                printf("Didnt add device (%s), error: %s\n", device.friendlyName, Pa_GetErrorText(err));
            }
        }

        if(deviceInfo->maxOutputChannels > 0) {
            PaError err = Pa_IsFormatSupported(nullptr, &params, deviceInfo->defaultSampleRate);
            if(err == paNoError || err == paSampleFormatNotSupported) {
                device.isInput = false;
                devices.push_back(device);
            }
            else {
                printf("Didnt add device (%s), error: %s\n", device.friendlyName, Pa_GetErrorText(err));
            }
        }
    }

    return 0;
}

void AudioProcessor::flushPlaybackBuffer() {
    playbackIndex -= SAMPLES_PER_BUFFER;

    Pa_StartStream(out_stream);
    std::vector<OUTPUT_BUFFER_TYPE> dummyBuffer(SAMPLES_PER_BUFFER, 0);
    while (Pa_IsStreamActive(out_stream) == 1) {
        Pa_ReadStream(out_stream, dummyBuffer.data(), SAMPLES_PER_BUFFER);
    }

    Pa_StopStream(out_stream);
}

void AudioProcessor::flushRecordBuffer() {
    frames_captured -= SAMPLES_PER_BUFFER;

    Pa_StartStream(in_stream);
    std::vector<OUTPUT_BUFFER_TYPE> dummyBuffer(SAMPLES_PER_BUFFER, 0);
    //while (Pa_IsStreamActive(in_stream) == 1) {
        Pa_ReadStream(in_stream, dummyBuffer.data(), SAMPLES_PER_BUFFER);
    //}

    Pa_StopStream(in_stream);
}
