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

void AudioProcessor::Restart() {
#ifdef _WIN32
	if (!isInitialized_)
		return;

	const PaError err = Pa_CloseStream(inStream_);
	if (err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
	}
	inStream_ = nullptr;
	outStream_ = nullptr;

	Pa_Terminate();
	Pa_Initialize();
#endif
}

bool AudioProcessor::PrimeRecordingStream(const PaDeviceIndex idx) {
	if (!isInitialized_)
		return false;

	PaError err;

	if ((inStream_ != nullptr) && idx == inDevIdx_ && Pa_IsStreamActive(inStream_) == 1) {
		if (err = Pa_StopStream(inStream_); err != paNoError) {
			DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
			return false;
		}
		return true;
	}

	inStream_ = nullptr;
	PaStreamParameters inputParameters;
	inputParameters.device = idx == -1 ? Pa_GetDefaultInputDevice() : idx;

	if (inputParameters.device == paNoDevice || (Pa_GetDeviceInfo(inputParameters.device) == nullptr)) {
		DEBUG_ERROR_LOC("No default input device.\n");
		return false;
	}
	inputParameters.channelCount = 1;
	inputParameters.sampleFormat = paInt16;
	inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
	inputParameters.hostApiSpecificStreamInfo = nullptr;

	DEBUG_PRINT("Priming in buffer!\n");

	inDevIdx_ = idx;
	framesCaptured_ = 0;
	//recordedSamples.clear();
	err = Pa_OpenStream(&inStream_, &inputParameters, nullptr, REC_SAMPLE_RATE, SAMPLES_PER_BUFFER, paClipOff, RecordCallback, this);
	if (err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		return false;
	}

	return true;
}

bool AudioProcessor::StartRecording() {
	if (inStream_ == nullptr)
		return false;

	// New test epoch begins here
	const auto epoch = currentEpoch_.fetch_add(1, std::memory_order_relaxed) + 1;
	(void)epoch;

	snapshotReady.store(false, std::memory_order_release);
	snapshotPublished_ = false;
	framesCaptured_ = 0;

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
	if ((outStream_ != nullptr) && outDevIdx_ == idx) {
		StopPlayback();
		return true;
	}
	PaError err;
	if (outDevIdx_ == idx) {
		// Try to close the old stream, but don't panic if it fails
		err = Pa_CloseStream(outStream_);
		if(err != paNoError) {
			DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		}
	}
	outStream_ = nullptr;
	PaStreamParameters outputParameters;
	outputParameters.device = idx == -1 ? Pa_GetDefaultOutputDevice() : idx;
	if (outputParameters.device == paNoDevice) {
		DEBUG_ERROR_LOC("No default output device.\n");
		return false;
	}
	const auto *params = Pa_GetDeviceInfo(outputParameters.device);
	outputParameters.channelCount = 1;
	outputParameters.sampleFormat = paInt16; // assumes OUTPUT_BUFFER_TYPE is short
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
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
	if (outStream_ == nullptr)
		return false;

	if (PaError const err = Pa_StartStream(outStream_); err != paNoError) {
		DEBUG_ERROR_LOC("PortAudio error: %s\n", Pa_GetErrorText(err));
		return false;
	}

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

#ifdef __linux__
	FlushPlaybackBuffer();
#endif

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

	if (statusFlags != 0u) {
		DEBUG_PRINT("status flag = %lu\n", statusFlags);
	}

	if (inputBuffer != nullptr && FRAMES_TO_CAPTURE >= processor->framesCaptured_ + framesPerBuffer) {
		memcpy(processor->captureBuffers[processor->writeBufferIndex_].data() + processor->framesCaptured_,
			   in, framesPerBuffer * sizeof(short));
		processor->framesCaptured_ += framesPerBuffer;
	}

	// Publish snapshot once (first 1/3), for both analysis and GUI plotting
	if (!processor->snapshotPublished_ && processor->framesCaptured_ >= ANALYSIS_FRAMES) {
		memcpy(processor->snapshotBuffer.data(),
			   processor->captureBuffers[processor->writeBufferIndex_].data(),
			   ANALYSIS_FRAMES * sizeof(short));

		std::atomic_thread_fence(std::memory_order_release);
		processor->snapshotEpoch.store(processor->currentEpoch_.load(std::memory_order_relaxed), std::memory_order_release);
		processor->snapshotReady.store(true, std::memory_order_release);

		processor->snapshotPublished_ = true;
	}

	// Keep recording the rest as cooldown/sink, then complete
	if (FRAMES_TO_CAPTURE <= processor->framesCaptured_ + (framesPerBuffer * 2)) {
		DEBUG_PRINT("captured %zu frames, fpb = %lu\n", processor->framesCaptured_, framesPerBuffer);
		processor->writeBufferIndex_ ^= 1;
		processor->framesCaptured_ = 0;
		processor->snapshotPublished_ = false;
		return paComplete;
	}

	return paContinue;
}

int AudioProcessor::PlaybackCallback(const void *, void *outputBuffer, const unsigned long framesPerBuffer,
                                     const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *userData) {
	auto *processor = static_cast<AudioProcessor *>(userData);
	auto *out = static_cast<OUTPUT_BUFFER_TYPE *>(outputBuffer);

	if (outputBuffer != nullptr) {
		memcpy(out, processor->playbackSamples.data() + processor->playbackIndex_,
		       framesPerBuffer * sizeof(OUTPUT_BUFFER_TYPE));
		processor->playbackIndex_ += framesPerBuffer;
	}

	if (processor->playbackSamples.size() <= processor->playbackIndex_ + static_cast<size_t>(framesPerBuffer * 2)) {
		//processor->isPlaying_ = false;
		// Fill the stream with zeroes
		const std::vector<OUTPUT_BUFFER_TYPE> dummyBuffer(SAMPLES_PER_BUFFER, 0);
		// while (Pa_IsStreamActive(processor->outStream_) == 1) {
		// 	Pa_ReadStream(processor->outStream_, (void*)dummyBuffer.data(), SAMPLES_PER_BUFFER);
		// }
		if (Pa_IsStreamActive(processor->outStream_) == 1)
			Pa_WriteStream(processor->outStream_, dummyBuffer.data(), SAMPLES_PER_BUFFER);
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

void AudioProcessor::FlushPlaybackBuffer() {
	if (playbackIndex_ > SAMPLES_PER_BUFFER)
		playbackIndex_ -= SAMPLES_PER_BUFFER;
	else
		playbackIndex_ = 0;

	Pa_StartStream(outStream_);
	const std::vector<OUTPUT_BUFFER_TYPE> dummyBuffer(SAMPLES_PER_BUFFER, 0);
	// while (Pa_IsStreamActive(outStream_) == 1) {
	//  Pa_ReadStream(outStream_, dummyBuffer.data(), SAMPLES_PER_BUFFER);
	// }
	if (Pa_IsStreamActive(outStream_) == 1)
		Pa_WriteStream(outStream_, dummyBuffer.data(), SAMPLES_PER_BUFFER);

	Pa_StopStream(outStream_);
}

void AudioProcessor::FlushRecordBuffer() {
	framesCaptured_ -= SAMPLES_PER_BUFFER;

	Pa_StartStream(inStream_);
	std::vector<OUTPUT_BUFFER_TYPE> dummyBuffer(SAMPLES_PER_BUFFER, 0);
	//while (Pa_IsStreamActive(inStream_) == 1) {
	Pa_ReadStream(inStream_, dummyBuffer.data(), SAMPLES_PER_BUFFER);
	//}

	Pa_StopStream(inStream_);
}
