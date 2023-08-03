#include "Sound_Helper.h"

int Waveform_SoundHelper::StartRecording()
{
	// Start recording audio
	result = waveInStart(hWaveIn);
	if (result != MMSYSERR_NOERROR)
	{
		std::cout << "Failed to start audio recording." << std::endl;
		waveInUnprepareHeader(hWaveIn, &waveHeader[_BufIdx], sizeof(WAVEHDR));
		waveInClose(hWaveIn);
		//delete[] audioBuffer;
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
		//delete[] audioBuffer;
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
		//delete[] audioBuffer;
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
		//delete[] audioBuffer;
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
		//delete[] audioBuffer;
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
			//delete[] audioBuffer;
			return result;
		}
	}

	waveHeader = &_waveHeaders[_BufIdx];
	if (updateAudioBuffer)
		audioBuffer = newBlockPlace;

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
		for (int i = 0; i < devs; i++)
		{
			MMRESULT res;

			WAVEINCAPSW* caps = new WAVEINCAPSW();
			res = waveInGetDevCapsW(i, caps, sizeof(WAVEINCAPSW));

			if (wcsncmp(varName.pwszVal, caps->szPname, wcslen(caps->szPname)) == 0)
				WaveformID = i;
		}

		(*devices)[i] = AudioDeviceInfo(WaveformID, devCount - i - 1, !var.uiVal, varName.pwszVal, varEnumName.pwszVal);

		device->Release();
		pProps->Release();
	}

	pEnumerator->Release();
	pDevices->Release();

	return devCount;
}