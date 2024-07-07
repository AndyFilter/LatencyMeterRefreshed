#include "Sound_Helper.h"

//#include <xaudio2.h>

#ifdef _WIN32

int Waveform_SoundHelper::StartRecording()
{
	// Start recording audio
	result = waveInStart(hWaveIn);
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to start audio recording." << std::endl;
		waveInUnprepareHeader(hWaveIn, &waveHeader[_BufIdx], sizeof(WAVEHDR));
		waveInClose(hWaveIn);
		//delete[] recordBuffer;
		return result;
	}

	//std::cout << "Recording started" << std::endl;

	return 0;
}

int Waveform_SoundHelper::StartLoopRecording()
{
	result = waveInStart(hWaveIn);
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to start audio recording." << std::endl;
		waveInUnprepareHeader(hWaveIn, &waveHeader[_BufIdx], sizeof(WAVEHDR));
		waveInClose(hWaveIn);
		//delete[] recordBuffer;
		return result;
	}

	is_loop = true;

	return 0;
}

int Waveform_SoundHelper::ResetRecording()
{
	result = waveInAddBuffer(hWaveIn, &waveHeader[_BufIdx], sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to add audio buffer to the recording queue." << std::endl;
		waveInUnprepareHeader(hWaveIn, &waveHeader[_BufIdx], sizeof(WAVEHDR));
		waveInClose(hWaveIn);
		//delete[] recordBuffer;
		return result;
	}

	return 0;
}

int Waveform_SoundHelper::StopRecording()
{
	// Stop and reset the audio input device
	result = waveInStop(hWaveIn);
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to stop and reset the audio input device." << std::endl;
		waveInUnprepareHeader(hWaveIn, &waveHeader[_BufIdx], sizeof(WAVEHDR));
		waveInClose(hWaveIn);
		//delete[] recordBuffer;
		return result;
	}

	std::cout << "Recording stopped" << std::endl;

	return 0;
}

int Waveform_SoundHelper::AddBuffer(size_t samplesSize, bool updateAudioBuffer)
{
	//auto size = _audioData.size();
	auto size = _audioDataEndPtr - _audioData;
	_audioDataEndPtr += samplesSize;
	//_audioData.reserve(samplesSize + cap);
	//_audioData.resize(size + samplesSize);

	auto newBlockPlace = &_audioData[0] + size;

	_waveHeaders.resize(_waveHeaders.size() + 1);
	waveHeader = &_waveHeaders[_waveHeaders.size()-1];

	ZeroMemory(newBlockPlace, samplesSize * sizeof(short));

	// Prepare the buffer header
	waveHeader->lpData = (char*)newBlockPlace;
	waveHeader->dwBufferLength = samplesSize * sizeof(short);
	waveHeader->dwBytesRecorded = 0;
	waveHeader->dwUser = 0;
	waveHeader->dwFlags = 0;
	waveHeader->dwLoops = 0;
	result = waveInPrepareHeader(hWaveIn, waveHeader, sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to prepare audio buffer." << std::endl;
		waveInClose(hWaveIn);
		//delete[] recordBuffer;
		return result;
	}

	if (size == 0) {
		// Add the buffer to the recording queue
		result = waveInAddBuffer(hWaveIn, waveHeader, sizeof(WAVEHDR));
		if (result != MMSYSERR_NOERROR)
		{
			std::cout << "Failed to add audio buffer to the recording queue." << std::endl;
			waveInUnprepareHeader(hWaveIn, waveHeader, sizeof(WAVEHDR));
			waveInClose(hWaveIn);
			//delete[] recordBuffer;
			return result;
		}
	}

	waveHeader = &_waveHeaders[_BufIdx];
	if (updateAudioBuffer)
		recordBuffer = newBlockPlace;

	return 0;
}

int Waveform_SoundHelper::SwapToNextBuffer()
{
	_BufIdx = (_BufIdx + 1) % _waveHeaders.size();

	result = waveInAddBuffer(hWaveIn, &_waveHeaders[_BufIdx], sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to swap buffers" << std::endl;
		return result;
	}
	printf("Using buffer[%i]\n", _BufIdx);

	return 0;
}

MMTIME Waveform_SoundHelper::GetPlayTime()
{
	MMTIME time{0};
	// Retrieve timestamp
	result = waveInGetPosition(hWaveIn, &time, sizeof(time));
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to retrieve current position" << std::endl;
		return time;
	}

	return time;
}

void Waveform_SoundHelper::InternalAudioCallback(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	Waveform_SoundHelper* obj = (Waveform_SoundHelper*)dwInstance;

	switch (uMsg)
	{
	case WIM_DATA:
	{
		if (obj->is_loop)
		{
			obj->ResetRecording();
		}
	}
	break;
	default:
		break;
	}

	if(obj->AudioCallBack)
		obj->AudioCallBack(hwi, uMsg, dwInstance, dwParam1, dwParam2);
}

Waveform_SoundHelper::~Waveform_SoundHelper()
{
	// Clean up resources
	result = waveInUnprepareHeader(hWaveIn, &waveHeader[_BufIdx], sizeof(WAVEHDR));
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to unprepare audio buffer." << std::endl;
		waveInClose(hWaveIn);
		delete[] _audioData;
		return;
	}

	delete[] _audioData;

	// Close the audio input device
	result = waveInClose(hWaveIn);
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to close audio input device." << std::endl;
		delete[] _audioData;
		return;
	}
}

DEFINE_GUID(CLSID_PolicyConfig, 0x870af99c, 0x171d, 0x4f9e, 0xaf, 0x0d, 0xe6, 0x3d, 0xf4, 0x0c, 0x2b, 0xc9);
MIDL_INTERFACE("f8679f50-850a-41cf-9c72-430f290290c8")
IPolicyConfig : public IUnknown
{
public:
 virtual HRESULT STDMETHODCALLTYPE GetMixFormat(PCWSTR pszDeviceName, WAVEFORMATEX * *ppFormat) = 0;
 virtual HRESULT STDMETHODCALLTYPE GetDeviceFormat(PCWSTR pszDeviceName, bool bDefault, WAVEFORMATEX** ppFormat) = 0;
 virtual HRESULT STDMETHODCALLTYPE ResetDeviceFormat(PCWSTR pszDeviceName) = 0;
 virtual HRESULT STDMETHODCALLTYPE SetDeviceFormat(PCWSTR pszDeviceName, WAVEFORMATEX* ppEndpointFormatFormat, WAVEFORMATEX* pMixFormat) = 0;
 virtual HRESULT STDMETHODCALLTYPE GetProcessingPeriod(PCWSTR pszDeviceName, bool bDefault, PINT64 pmftDefaultPeriod, PINT64 pmftMinimumPeriod) = 0;
 virtual HRESULT STDMETHODCALLTYPE SetProcessingPeriod(PCWSTR pszDeviceName, PINT64 pmftPeriod) = 0;
 virtual HRESULT STDMETHODCALLTYPE GetShareMode(PCWSTR pszDeviceName, struct DeviceShareMode* pMode) = 0;
 virtual HRESULT STDMETHODCALLTYPE SetShareMode(PCWSTR pszDeviceName, struct DeviceShareMode* pMode) = 0;
 virtual HRESULT STDMETHODCALLTYPE GetPropertyValue(PCWSTR pszDeviceName, BOOL bFxStore, const PROPERTYKEY& pKey, PROPVARIANT* pv) = 0;
 virtual HRESULT STDMETHODCALLTYPE SetPropertyValue(PCWSTR pszDeviceName, BOOL bFxStore, const PROPERTYKEY& pKey, PROPVARIANT* pv) = 0;
 virtual HRESULT STDMETHODCALLTYPE SetDefaultEndpoint(PCWSTR pszDeviceName, ERole eRole) = 0;
 virtual HRESULT STDMETHODCALLTYPE SetEndpointVisibility(PCWSTR pszDeviceName, bool bVisible) = 0;
};

void SetAudioEnchantments(bool enabled, _In_ UINT devID)
{
	IMMDeviceEnumerator* pDeviceEnumerator;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (LPVOID*)&pDeviceEnumerator);
	if (SUCCEEDED(hr))
	{
		IMMDeviceCollection* pDevices;
		hr = pDeviceEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pDevices);
		if (SUCCEEDED(hr))
		{
			IMMDevice* device;
			pDevices->Item(devID, &device);
			LPWSTR pwstrEndpointId = NULL;
			device->GetId(&pwstrEndpointId);

			IPolicyConfig* pPolicyConfig;
			hr = CoCreateInstance(CLSID_PolicyConfig, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pPolicyConfig));
			if (SUCCEEDED(hr))
			{
				PROPVARIANT var;
				PropVariantInit(&var);
				hr = pPolicyConfig->GetPropertyValue(pwstrEndpointId, TRUE, PKEY_AudioEndpoint_Disable_SysFx, &var);
				var.uiVal = (USHORT)!enabled;
				hr = pPolicyConfig->SetPropertyValue(pwstrEndpointId, TRUE, PKEY_AudioEndpoint_Disable_SysFx, &var);
				pPolicyConfig->Release();
			}
		}
		pDeviceEnumerator->Release();
	}
}

UINT GetAudioDevices(_Out_ AudioDeviceInfo** devices)
{
	IMMDeviceEnumerator* pEnumerator;

	const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
	const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
	HRESULT hr = CoCreateInstance(
		CLSID_MMDeviceEnumerator, NULL,
		CLSCTX_ALL, IID_IMMDeviceEnumerator,
		(void**)&pEnumerator);

	IMMDeviceCollection* pDevices;
	pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pDevices);

	UINT devCount = 0;
	pDevices->GetCount(&devCount);

	
	*devices = new AudioDeviceInfo[devCount];

	for (int i = 0; i < devCount; i++)
	{
		IMMDevice* device;
		pDevices->Item(devCount - i - 1, &device);

		IPropertyStore* pProps = NULL;
		hr = device->OpenPropertyStore(
			STGM_READ, &pProps);

		PROPVARIANT varName;
		pProps->GetValue(PKEY_Device_FriendlyName, &varName);

		PROPVARIANT varEnumName;
		pProps->GetValue(PKEY_Device_EnumeratorName, &varEnumName);

		LPWSTR pwstrEndpointId = NULL;
		device->GetId(&pwstrEndpointId);
		IPolicyConfig* pPolicyConfig;
		hr = CoCreateInstance(CLSID_PolicyConfig, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pPolicyConfig));
		PROPVARIANT var;
		PropVariantInit(&var);
		hr = pPolicyConfig->GetPropertyValue(pwstrEndpointId, TRUE, PKEY_AudioEndpoint_Disable_SysFx, &var);
		pPolicyConfig->Release();

		UINT WaveformID = 0;
		
		int devs = waveInGetNumDevs();
		for (int j = 0; j < devs; j++)
		{
			MMRESULT res;

			WAVEINCAPSW* caps = new WAVEINCAPSW();
			res = waveInGetDevCapsW(j, caps, sizeof(WAVEINCAPSW));

			if (wcsncmp(varName.pwszVal, caps->szPname, wcslen(caps->szPname)) == 0)
				WaveformID = j;
		}

		(*devices)[i] = AudioDeviceInfo(WaveformID, devCount - i - 1, !var.uiVal, varName.pwszVal, varEnumName.pwszVal);

		device->Release();
		pProps->Release();
	}

	pEnumerator->Release();
	pDevices->Release();

	return devCount;
}

// -------------------- Output Audio --------------------

#define WAV_PLAY_TIME_FRAC 32
#define WAV_HEADER_DATA_SIZE 44100/WAV_PLAY_TIME_FRAC

typedef struct WAV_HEADER {
	/* RIFF Chunk Descriptor */
	uint8_t RIFF[4] = { 'R', 'I', 'F', 'F' }; // RIFF Header Magic header
	uint32_t ChunkSize = 44 + (sizeof(short) * WAV_HEADER_DATA_SIZE);            // File size
	uint8_t WAVE[4] = { 'W', 'A', 'V', 'E' }; // WAVE Header
	uint8_t fmt[4] = { 'f', 'm', 't', ' ' }; // FMT header
	uint32_t Subchunk1Size = 16;           // Size of the fmt chunk
	uint16_t AudioFormat = 1; // Audio format 1 = PCM
	uint16_t NumOfChan = 1;   // Number of channels 1=Mono, 2=Stereo
	uint32_t SamplesPerSec = WAV_HEADER_DATA_SIZE * WAV_PLAY_TIME_FRAC;   // Sampling Frequency in Hz
	uint32_t bytesPerSec = SamplesPerSec * 2; // bytes per second
	uint16_t blockAlign = 2;          // 2=16-bit mono, 4=16-bit stereo
	uint16_t bitsPerSample = 16;      // Number of bits per sample
	/* "data" sub-chunk */
	uint8_t Subchunk2ID[4] = { 'd', 'a', 't', 'a' }; // "data"  string
	uint32_t Subchunk2Size = sizeof(short) * WAV_HEADER_DATA_SIZE;            // data chunk size
	short data[WAV_HEADER_DATA_SIZE];
};

bool WinAudio_Player::Setup()
{
	header = new WAV_HEADER();
	return true;
}

void WinAudio_Player::SetBuffer(int (*func)(int))
{
	for (int i = 0; i < WAV_HEADER_DATA_SIZE; i++)
	{
		header->data[i] = func(i);
	}
}

void WinAudio_Player::Play()
{
	PlaySound((LPCSTR)header, GetModuleHandle(NULL), SND_MEMORY | SND_ASYNC);
}

#else

#include <vector>

UINT GetAudioDevices(cAudioDeviceInfo** devices) {
    std::vector<cAudioDeviceInfo> vec_devices;

    char **hints;
    /* Enumerate sound devices */
    int err = snd_device_name_hint(-1, "pcm", (void***)&hints);
    if (err != 0)
        return 0;

    int idx = 0;

    //void** hint;
    for (char** hint = hints; *hint != nullptr; ++hint) {
        const char* deviceName = snd_device_name_get_hint(*hint, "NAME");
        const char* IO_type = snd_device_name_get_hint(*hint, "IOID");
        auto dev_name_str = std::string(deviceName);

        if (deviceName && dev_name_str != "null" &&
        (dev_name_str.find("hw") != std::string::npos && // Leave devices with "hw" in name
        (!IO_type ||std::string(IO_type) == "Input") || dev_name_str == "default")) // leave only input devices
        {
            char* name_copy;
            name_copy = new char[strlen(deviceName) - (dev_name_str == "default" ? 0 : 0)];
            strcpy(name_copy, deviceName + (dev_name_str == "default" ? 0 : 0));
            vec_devices.emplace_back(idx, idx, false, name_copy, name_copy);
            //std::cout << "Device Name: " << deviceName << ", Description: " << deviceDesc << std::endl;
        }

        free((void *) IO_type);
        free((void *) deviceName);
        idx++;
    }

    //char** n = hints;
    //while (*n != NULL) {
    //
    //    char *name = snd_device_name_get_hint(*n, "NAME");
    //
    //    if (name != NULL && 0 != strcmp("null", name) ) {
    //        //cAudioDeviceInfo dev = cAudioDeviceInfo(idx, idx, false, name, name);
    //        //vec_devices.push_back(dev);
    //
    //        char* name_copy;
    //
    //        if(strstr(name, "hw:") == name) {
    //            int comma = 0;//strlen(name) - (strchr(name, ',') - name);
    //            name_copy = new char[strlen(name) - 8 - comma];
    //            //memcpy(name_copy, name + 8, strlen(name) - 8 - comma);
    //            strcpy(name_copy, name + 8);
    //            vec_devices.push_back(cAudioDeviceInfo(idx, idx, false, name_copy, name_copy));
    //        }
    //        else if (strstr(name, "default") == name) {
    //            name_copy = new char[strlen(name)];
    //            strcpy(name_copy, name);
    //            vec_devices.emplace_back(idx, idx, false, name_copy, name_copy);
    //        }
    //        //else { continue; }
    //
    //        //memcpy(name_copy, name, strlen(name_copy) * sizeof(char));
    //        //vec_devices.emplace_back(idx, idx, false, name_copy, name_copy);
    //        //printf("found %s\n", name);
    //        free(name);
    //    }
    //    idx++;
    //    n++;
    //}

    //Free hint buffer too
    snd_device_name_free_hint((void**)hints);

    *devices = new cAudioDeviceInfo[vec_devices.size()];
    for(int i = 0; i < vec_devices.size(); i++)
        (*devices)[i] = vec_devices[i];
    //std::copy(vec_devices.begin(), vec_devices.end(), devices);

    return vec_devices.size();
}

uint64_t micros1()
{
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t us = ((uint64_t)ts.tv_sec * 1000000) + ((uint64_t)ts.tv_nsec / 1000);
    static uint64_t start_time = us;
    return us - start_time;
}
//
//int ALSA_RecDevice::StartRecording() {
//    int err = 0;
//    //snd_pcm_prepare(capture_handle);
//    //usleep(10000);
//    //printf("Reading\n");
//    if((snd_pcm_start(capture_handle) < 0)) {
//        fprintf (stderr, "Starting recording failed (%s)\n", snd_strerror (err));
//        return err;
//    }
//
//    read_frames = 0;
//    isRecording = true;
//
//    return 0;
//    //snd_pcm_uframes_t period_size = 128;
//    //snd_pcm_prepare(capture_handle);
//    ////snd_pcm_wait(capture_handle, 2000);
//    //printf("Reading\n");
//    //snd_pcm_start(capture_handle);
//    ////ClearRecordBuffer();
//    ////usleep(100000);
//    //snd_pcm_wait(capture_handle, 0);
//    //auto num = snd_pcm_readi_nb(capture_handle, recordBuffer, BUF_FRAMES, SAMPLE_RATE);
//    //printf("read %i frames\n", num);
//    //return 0;
//    //snd_pcm_prepare(capture_handle);
//    ////snd_pcm_wait(capture_handle, 2000);
//    //printf("Reading\n");
//    //snd_pcm_start(capture_handle);
//    //
//    ////snd_pcm_prepare(capture_handle);
//    ////sleep(1);
//    ////usleep(200000);
//    //auto state = snd_pcm_state(capture_handle);
//    //printf("state: %s\n", snd_pcm_state_name(state));
//    //if ((err = snd_pcm_readi (capture_handle, recordBuffer, period_size)) < 0) {
//    //    fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
//    //}
//    ////usleep(200000);
//    //if ((err = snd_pcm_readi (capture_handle, recordBuffer + period_size, 512)) < 0) {
//    //    fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
//    //}
//    //printf("read %i frames\n", err);
//    //return err;
//}
//
//int ALSA_RecDevice::StartRecordingBlocking() {
//    int err = 0;
//    if ((err = snd_pcm_readi (capture_handle, recordBuffer, BUF_FRAMES)) != BUF_FRAMES) {
//        fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
//    }
//    return err;
//}
//
//int ALSA_RecDevice::PlayAudio() {
//    int err = 0;
//    if ((err = snd_pcm_writei (playback_handle, playbackBuffer, BUF_FRAMES)) != BUF_FRAMES) {
//        fprintf (stderr, "write to audio interface failed (%s)\n", snd_strerror (err)); exit (1);
//    }
//    return err;
//}
//
//int ALSA_RecDevice::ClearRecordBuffer() {
//    int err = 0;
//    //auto start = micros1();
//    if((err = snd_pcm_drop(capture_handle)) < 0) {
//        fprintf(stderr, "dropping capture failed: (%s)\n", snd_strerror (err)); return err;
//    }
//    if((err = snd_pcm_prepare(capture_handle)) < 0) {
//        fprintf(stderr, "dropping capture failed: (%s)\n", snd_strerror (err)); return err;
//    }
//    if((err = snd_pcm_reset(capture_handle)) < 0) {
//        fprintf(stderr, "resetting capture failed: (%s)\n", snd_strerror (err)); return err;
//    }
//    //printf("Took %luus to reset\n", micros1() - start);
//    return 0;
//}
//
//// Pretty much just waits for new data, and if there is any, just accumulates it
//// Doesn't matter if we are reading it too slowly, it won't affect the final result
//// Thats because we are forcing period time to be as big as possible (bigger than 512...)
//bool ALSA_RecDevice::UpdateState() {
//
//    // Dont even try to read if there is nothing to be read
//    auto status = snd_pcm_avail(capture_handle);
//    if(status <= 0)
//        return false;
//
//    int err;
//    int ret = poll(pollDescriptors, 1, 0); // Timeout of 100ms
//    if (ret == -1) {
//        std::cerr << "Error in poll: " << strerror(errno) << std::endl;
//        return false;
//    } else if (ret == 0) {
//        return false; // No Events
//    }
//    // Check if there is data available
//    if (pollDescriptors[0].revents & POLLIN) {
//        // Read data from the PCM device
//        bool is_first_cn = (read_frames / period_size) % 2 == 0;
//        //size_t offset = (read_frames / period_size) * period_size;
//        auto* tmp_buf = new short[status * 4];
//        err = snd_pcm_readi(capture_handle, tmp_buf, status);
//        if (err < 0) {
//            //delete[] tmp_buf;
//            std::cerr << "Error reading from PCM device: " << snd_strerror(err) << std::endl;
//            return false;
//        }
//
//        printf("read frames: %i, is first = %i, new frames: %i\n", read_frames, is_first_cn, err);
//
//        if(status < err)
//            exit(1);
//
//        for(int i = 0; i < err; i++) {
//            int cur_frame = i + read_frames;
//            if(cur_frame >= BUFFER_SIZE)
//                break;
//            if(cur_frame % 2 == 0) {
//                recordBuffer[cur_frame / 2] = tmp_buf[i];
//            }
//            else {
//                recordBuffer[cur_frame / 2 + (BUFFER_SIZE / 2)] = tmp_buf[i];
//            }
//
//            printf("frame %i, is first = %i\n", cur_frame, is_first_cn);
//            ////if(cur_frame >= BUF_FRAMES)
//            ////    break;
//            //is_first_cn = (cur_frame / period_size) % 2 == 0;
//            //printf("frame %i, is first = %i\n", cur_frame, is_first_cn);
//            ////if(is_first_cn) {
//            //if(cur_frame / 2 + (is_first_cn ? 0 : (BUFFER_SIZE / 2)) >= BUFFER_SIZE)
//            //    exit(1);
//            //if(is_first_cn)
//            //    recordBuffer[cur_frame / 2] = tmp_buf[i];
//            ////recordBuffer[cur_frame / 2 + (is_first_cn ? 0 : (BUFFER_SIZE / 2))] = tmp_buf[i];
//            ////}
//        };
//
//        //delete[] tmp_buf;
//
//        //printf("Got %i new frames\n", err);
//        //printf("overall: %u\n", read_frames);
//        //if(is_first_cn)
//        read_frames += err;
//
//        pollDescriptors[0].revents = 0;
//        if(read_frames >= BUF_FRAMES) {
//            isRecording = false;
//            return true;
//        }
//        else return false;
//    }
//}

#endif
