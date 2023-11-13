#pragma once

#include <iostream>
#include <windows.h>
#include <mmsystem.h>
#include <comdef.h>

#include <setupapi.h>  
#include <initguid.h>  // Put this in to get rid of linker errors.  
#include <devpkey.h>  // Property keys defined here are now defined inline. 

#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>

#include <functiondiscoverykeys.h>
#include <vector>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

class AudioDeviceInfo
{
public:
	UINT id;
	UINT WASAPI_id;
	bool AudioEnchantments = false;
	LPCWSTR friendlyName;
	LPCWSTR portName; // EnumerationName

	AudioDeviceInfo(const UINT id, const UINT wasapi_id, bool enchantments, const LPCWSTR friendlyName, const LPCWSTR portName)
		: id(id), WASAPI_id(wasapi_id), friendlyName(friendlyName), portName(portName),
		AudioEnchantments(enchantments)
	{
	}

	AudioDeviceInfo() = default;
};

class cAudioDeviceInfo
{
public:
	UINT id;
	UINT WASAPI_id;
	bool AudioEnchantments = false;
	const char* friendlyName;
	const char* portName; // EnumerationName

	cAudioDeviceInfo(const UINT id, const UINT wasapi_id, bool enchantments, const char* friendlyName, const char* portName)
		: id(id), WASAPI_id(wasapi_id), friendlyName(friendlyName), portName(portName),
		AudioEnchantments(enchantments)
	{
	}

	cAudioDeviceInfo() = default;
};

void SetAudioEnchantments(bool enabled, _In_ UINT devID);
UINT GetAudioDevices(_Out_ AudioDeviceInfo** devices);

class Waveform_SoundHelper
{
	HWAVEIN hWaveIn{0};
	WAVEFORMATEX waveFormat{0};
	MMRESULT result{0};
	WAVEHDR* waveHeader = nullptr;
	std::vector<WAVEHDR> _waveHeaders;
	//WAVEHDR _waveHeaders[bufferCount];
	//std::vector<short> _audioData;
	short* _audioData = nullptr;
	short* _audioDataEndPtr = nullptr;
	int _BufIdx = 0;

	bool is_loop = false;
public:
	const int SAMPLE_RATE = 44100;
	const int NUM_CHANNELS = 1;  // Mono
	const int BITS_PER_SAMPLE = 16;
	const int BUFFER_SIZE = SAMPLE_RATE * 2;
	const int DEV_ID = 0;
	const int NUM_BUFFERS = 1;

	bool isCreated = false;

	short* audioBuffer = nullptr;
	typedef void (*waveInAudioCallBack)(HWAVEIN, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

	void (*AudioCallBack)(HWAVEIN, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

	Waveform_SoundHelper(UINT DeviceId = 0, UINT buf_size = 44100 * 2, UINT bufCount = 1, UINT sampleRate = 44100, UINT bitsPerSample = 16, DWORD_PTR callBack = 0, int num_ch = 1) :
		SAMPLE_RATE(sampleRate), NUM_CHANNELS(num_ch), BITS_PER_SAMPLE(bitsPerSample), BUFFER_SIZE(buf_size), NUM_BUFFERS(bufCount), AudioCallBack((waveInAudioCallBack)callBack), DEV_ID(DeviceId)
	{
		// Set up the wave format
		waveFormat.wFormatTag = WAVE_FORMAT_PCM;
		waveFormat.nChannels = NUM_CHANNELS;
		waveFormat.nSamplesPerSec = SAMPLE_RATE;
		waveFormat.wBitsPerSample = BITS_PER_SAMPLE;
		waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
		waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
		waveFormat.cbSize = 0;

		// Open the audio input device
		result = waveInOpen(&hWaveIn, DEV_ID, &waveFormat, (DWORD_PTR)InternalAudioCallback, (DWORD_PTR)this, CALLBACK_FUNCTION | WAVE_FORMAT_DIRECT);
		if (result != MMSYSERR_NOERROR)
		{
			std::cout << "Failed to open audio input device." << std::endl;
			return;
		}

		_audioData = new short[BUFFER_SIZE];
		_audioDataEndPtr = _audioData;

		memset(_audioData, 0, sizeof(short) * BUFFER_SIZE);

		audioBuffer = _audioData;

		_waveHeaders.reserve(NUM_BUFFERS);

		isCreated = true;
	};

	int StartRecording();
	int StartLoopRecording();

	int ResetRecording();
	int StopRecording();

	int AddBuffer(size_t samplesSize, bool updateAudioBuffer = false);
	int SwapToNextBuffer();

	MMTIME GetPlayTime();

private:
	// Internal Callback function called by waveInOpen
	static void CALLBACK InternalAudioCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

public:

	~Waveform_SoundHelper();
};

struct WAV_HEADER;

class WinAudio_Player {
	WAV_HEADER* header;

public:
	bool Setup();
	void SetBuffer(int (*func)(int));
	void Play();
};