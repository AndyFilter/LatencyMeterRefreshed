#include "AudioProcessor.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <iostream>
#include "App/Config.h"
#include "Helpers/AppHelper.h"

AudioProcessor::~AudioProcessor() {
	Terminate();
}

bool AudioProcessor::Initialize() {
	if (isInitialized_)
		return true;

	DEBUG_PRINT("Initializing PortAudio!\n");

	if (PaError const err = Pa_Initialize(); err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		return false;
	}
	isInitialized_ = true;
	return true;
}

void AudioProcessor::Terminate() {
	DEBUG_PRINT("Terminating PortAudio!\n");
	if (isRecording_)
		StopRecording();
	if (isPlaying_)
		StopPlayback();

	if (inStream_ != nullptr) {
		Pa_StopStream(inStream_);
		Pa_CloseStream(inStream_);
		inStream_ = nullptr;
	}

	if (outStream_ != nullptr) {
		Pa_StopStream(outStream_);
		Pa_CloseStream(outStream_);
		outStream_ = nullptr;
	}

	Pa_Terminate();
	inDevIdx_ = -1;
	outDevIdx_ = -1;
	isInitialized_ = false;
}

bool AudioProcessor::PrimeRecordingStream(const PaDeviceIndex idx) {
	if (!isInitialized_)
		return false;

	PaError err;

	// Always fully dispose of old stream
	if (inStream_ != nullptr) {
		err = Pa_StopStream(inStream_);
		if (err != paNoError && err != paStreamIsStopped) {
			DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		}

		err = Pa_CloseStream(inStream_);
		if (err != paNoError) {
			DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
			return false;
		}

		inStream_ = nullptr;
	}

	PaStreamParameters inputParameters;
	inputParameters.device = (idx == -1) ? Pa_GetDefaultInputDevice() : idx;

	const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(inputParameters.device);
	if (inputParameters.device == paNoDevice || devInfo == nullptr) {
		DEBUG_ERROR_LOC("No valid input device.\n");
		return false;
	}

	inputParameters.channelCount = 1;
	inputParameters.sampleFormat = paInt16;
	inputParameters.suggestedLatency = devInfo->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = nullptr;

	DEBUG_PRINT("Priming in buffer!\n");

	inDevIdx_ = idx;
	framesCaptured_ = 0;

	err = Pa_OpenStream(&inStream_, &inputParameters, nullptr, REC_SAMPLE_RATE, SAMPLES_PER_BUFFER,paClipOff,
	                    RecordCallback, this);

	if (err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		return false;
	}

	return true;
}

bool AudioProcessor::StartRecording() {
	if (inStream_ == nullptr)
		return false;

	DEBUG_PRINT("Started recording!\n");

	// New test epoch begins here
	const auto epoch = currentEpoch_.fetch_add(1, std::memory_order_relaxed) + 1;
	(void)epoch;

	snapshotReady.store(false, std::memory_order_release);
	snapshotPublished_ = false;
	framesCaptured_ = 0;
	finishedRecording_ = false;

	if (const PaError err = Pa_StartStream(inStream_); err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		return false;
	}

	isRecording_ = true;
	return true;
}

void AudioProcessor::StopRecording() {
	if (!isRecording_ || inStream_ == nullptr)
		return;

	DEBUG_PRINT("Stopped recording!\n");

	if (PaError const err = Pa_StopStream(inStream_); err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
	}

	//FlushRecordBuffer();

	framesCaptured_ = 0;

	//err = Pa_CloseStream(inStream_);
	//if (err != paNoError) {
	//    std::cerr << "PortAudio (CloseStream) error: " << Pa_GetErrorText(err) << std::endl;
	//}

	isRecording_ = false;
}

bool AudioProcessor::PrimePlaybackStream(const PaDeviceIndex idx) {
	if (!isInitialized_)
		return false;
	PaError err;
	if (outStream_ != nullptr) {
		StopPlayback();

		err = Pa_CloseStream(outStream_);
		if (err != paNoError) {
			DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		}

		outStream_ = nullptr;
	}
	PaStreamParameters outputParameters;
	outputParameters.device = idx == -1 ? Pa_GetDefaultOutputDevice() : idx;
	if (outputParameters.device == paNoDevice) {
		DEBUG_ERROR_LOC("No default output device.\n");
		return false;
	}
	const auto *params = Pa_GetDeviceInfo(outputParameters.device);
	outputParameters.channelCount = 1;
	outputParameters.sampleFormat = paInt16; // assumes OUTPUT_BUFFER_TYPE is short
	outputParameters.suggestedLatency = params->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;

	DEBUG_PRINT("Priming out buffer!\n");

	playbackIndex_ = 0;
	outDevIdx_ = idx;
	err = Pa_OpenStream(&outStream_, nullptr, &outputParameters, params->defaultSampleRate, SAMPLES_PER_BUFFER,
	                    paClipOff, PlaybackCallback, this);
	if (err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		return false;
	}

	return true;
}

bool AudioProcessor::StartPlayback() {
	if (outStream_ == nullptr) {
		DEBUG_ERROR_LOC("No output stream!\n");
		return false;
	}

	finishedPlayback_ = false;

	if (PaError const err = Pa_StartStream(outStream_); err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		return false;
	}

	DEBUG_PRINT("Started playback!\n");

	isPlaying_ = true;
	return true;
}

void AudioProcessor::StopPlayback() {
	if (!isPlaying_ || outStream_ == nullptr)
		return;

	if (PaError const err = Pa_StopStream(outStream_); err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
	}

	DEBUG_PRINT("Stopped playback!\n");

	playbackIndex_ = 0;
	//err = Pa_CloseStream(outStream_);
	//if (err != paNoError) {
	//    std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
	//}

	//outStream_ = nullptr;

	isPlaying_ = false;
}

int AudioProcessor::RecordCallback(const void *inputBuffer, void *, const unsigned long framesPerBuffer,
								   const PaStreamCallbackTimeInfo *, const PaStreamCallbackFlags statusFlags,
								   void *userData) {
	auto *processor = static_cast<AudioProcessor *>(userData);
	const auto *in = static_cast<const short *>(inputBuffer);

	if (processor->finishedRecording_)
		return paComplete;

	if (statusFlags != 0u) {
		DEBUG_PRINT("status flag = %lu\n", statusFlags);
	}

	if (inputBuffer != nullptr && FRAMES_TO_CAPTURE >= processor->framesCaptured_ + framesPerBuffer) {
		memcpy(processor->captureBuffers[processor->writeBufferIndex_].data() + processor->framesCaptured_,
			   in, framesPerBuffer * sizeof(short));
		processor->framesCaptured_ += framesPerBuffer;
	}

	// Publish a snapshot once (first 1/MAIN_BUFFER_SIZE_FRACTION_RECIPROCAL), for both analysis and GUI plotting
	if (!processor->snapshotPublished_ && processor->framesCaptured_ >= ANALYSIS_FRAMES) {
		DEBUG_PRINT("Publishing snapshot!\n");
		memcpy(processor->snapshotBuffer.data(),
			   processor->captureBuffers[processor->writeBufferIndex_].data(),
			   ANALYSIS_FRAMES * sizeof(short));

		std::atomic_thread_fence(std::memory_order_release);
		processor->snapshotEpoch.store(processor->currentEpoch_.load(std::memory_order_relaxed), std::memory_order_release);
		processor->snapshotReady.store(true, std::memory_order_release);

		processor->snapshotPublished_ = true;
	}

	// Keep recording the rest as cooldown/sink, then complete
	if (processor->framesCaptured_ >= FRAMES_TO_CAPTURE && processor->snapshotPublished_) {
		DEBUG_PRINT("captured %zu frames, fpb = %lu\n", processor->framesCaptured_, framesPerBuffer);
		processor->writeBufferIndex_ ^= 1;
		processor->framesCaptured_ = 0;
		processor->finishedRecording_ = true;
		return paComplete;
	}

	return paContinue;
}

int AudioProcessor::PlaybackCallback(const void *, void *outputBuffer, const unsigned long framesPerBuffer,
                                     const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *userData) {
	auto *processor = static_cast<AudioProcessor *>(userData);
	auto *out = static_cast<OUTPUT_BUFFER_TYPE *>(outputBuffer);

	if (processor->finishedPlayback_)
		return paComplete;

	if (outputBuffer != nullptr) {
		const unsigned long framesLeft = processor->playbackSamples.size() - processor->playbackIndex_;
		const unsigned long framesToCopy = std::min(framesPerBuffer, framesLeft);

		memcpy(out, processor->playbackSamples.data() + processor->playbackIndex_,
		       framesToCopy * sizeof(OUTPUT_BUFFER_TYPE));

		if (framesToCopy < framesPerBuffer) {
			memset(out + framesToCopy, 0,
			       (framesPerBuffer - framesToCopy) * sizeof(OUTPUT_BUFFER_TYPE));
		}

		processor->playbackIndex_ += framesToCopy;
		DEBUG_PRINT("Playback: %llu/%llu\n", processor->playbackIndex_, processor->playbackSamples.size());
	}

	if (processor->playbackIndex_ >= processor->playbackSamples.size()) {
		DEBUG_PRINT("Playback buffer is full!\n");

		processor->finishedPlayback_ = true;
		return paComplete;
	}

	return paContinue;
}

unsigned int AudioProcessor::GetAudioDevices(std::vector<AudioDeviceInfo> &devices) const {
	if (!isInitialized_)
		return 1;

	const int numDevices = Pa_GetDeviceCount();
	if (numDevices < 0) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(numDevices));
		return 0;
	}

	for (int i = 0; i < numDevices; ++i) {
		const PaDeviceInfo *deviceInfo = Pa_GetDeviceInfo(i);
		const PaHostApiInfo *hostApiInfo = Pa_GetHostApiInfo(deviceInfo->hostApi);

		DEBUG_PRINT("Device #%d: %s\n", i, deviceInfo->name);
		DEBUG_PRINT("  Host API: %s\n", hostApiInfo->name);
		DEBUG_PRINT("  Lowest latency: %f s\n", Pa_GetDeviceInfo(i)->defaultLowInputLatency);

		const size_t nameSize = strlen(deviceInfo->name);
		const size_t hostApiSize = strlen(hostApiInfo->name);
		std::string szName;
		szName.reserve(nameSize + hostApiSize + 1 + 3);
		szName += hostApiInfo->name;
		szName += " (";
		szName += deviceInfo->name;
		szName += ")";
		AudioDeviceInfo device(i, false, szName, hostApiInfo->name, false);

		PaStreamParameters params;
		params.device = i;
		params.channelCount = 1;
		params.sampleFormat = paInt16;
		params.hostApiSpecificStreamInfo = nullptr;
		params.suggestedLatency = Pa_GetDeviceInfo(i)->defaultLowInputLatency;

		if (deviceInfo->maxInputChannels > 0) {
			if (const PaError err = Pa_IsFormatSupported(&params, nullptr, REC_SAMPLE_RATE); err == paNoError) {
				device.isInput = true;
				devices.push_back(device);
			} else {
				printf("Didnt add device (%s), error: %s\n", device.friendlyName.c_str(), Pa_GetErrorText(err));
			}
		}

		if (deviceInfo->maxOutputChannels > 0) {
			if (const PaError err = Pa_IsFormatSupported(nullptr, &params, deviceInfo->defaultSampleRate);
				err == paNoError || err == paSampleFormatNotSupported) {
				device.isInput = false;
				devices.push_back(device);
			} else {
				printf("Didnt add device (%s), error: %s\n", device.friendlyName.c_str(), Pa_GetErrorText(err));
			}
		}
	}

	return 0;
}