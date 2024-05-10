#pragma once

#include <iostream>

#include "../constants.h"

class cAudioDeviceInfo
{
public:
    unsigned int id;
    unsigned int WASAPI_id; // Windows only
    bool AudioEnchantments = false;
    const char* friendlyName;
    const char* portName; // EnumerationName

    cAudioDeviceInfo(const unsigned int id, const unsigned int wasapi_id, bool enchantments, const char* friendlyName, const char* portName)
            : id(id), WASAPI_id(wasapi_id), friendlyName(friendlyName), portName(portName),
              AudioEnchantments(enchantments)
    {
    }

    cAudioDeviceInfo() = default;
};

#ifdef _WIN32
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

	short* recordBuffer = nullptr;
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

		recordBuffer = _audioData;

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

#else
#include <alsa/asoundlib.h>

// Synchronous!
template<typename Ty = int>
class ALSA_AudioDevice {
    unsigned int BUF_FRAMES = 0;
    pollfd* pollDescriptors;
    unsigned int read_frames = 0;
    snd_pcm_uframes_t period_size = 128;
    unsigned int frame_size = 0;

    bool is_loop = false;
public:
    snd_pcm_t *playback_handle = nullptr;
    snd_pcm_t *capture_handle = nullptr;
    unsigned int SAMPLE_RATE = 44100;
    int NUM_CHANNELS = 2;  // Stereo (we don't need it, but most devices dont support mono.........................)
    int BITS_PER_SAMPLE = 16;
    snd_pcm_uframes_t BUFFER_SIZE = SAMPLE_RATE * 2;
    int DEV_ID = 0;
    int NUM_BUFFERS = 1;

    bool isCreated = false;

    Ty* recordBuffer = nullptr;
    Ty* playbackbuffer = nullptr;

    bool isRecording = false;

    ALSA_AudioDevice(UINT DeviceId, UINT& buf_size, UINT& sampleRate) :
            SAMPLE_RATE(sampleRate), BUFFER_SIZE(buf_size), DEV_ID(DeviceId)
    {
        int err;

        char* device;
        char **hints;
        if ((err = snd_device_name_hint(-1, "pcm", (void***)&hints)) < 0) {
            fprintf (stderr, "cannot enumerate devices\n", device, snd_strerror (err)); exit (1);
        }
        device = snd_device_name_get_hint(*(hints + DeviceId), "NAME");
        snd_device_name_free_hint((void**)hints);

        snd_pcm_format_t format =   typeid(Ty) == typeid(int) ? SND_PCM_FORMAT_S32_LE :
                                    typeid(Ty) == typeid(short) ? SND_PCM_FORMAT_S16_LE : SND_PCM_FORMAT_FLOAT;

        if ((err = snd_pcm_open (&capture_handle, device, SND_PCM_STREAM_CAPTURE, 1)) < 0) {
            fprintf (stderr, "cannot open audio device %s (%s)\n", device, snd_strerror (err));
            return;
        }

        snd_pcm_hw_params_t* hwParams;
        snd_pcm_hw_params_alloca(&hwParams);
        snd_pcm_hw_params_any(capture_handle, hwParams);
        int dir = 0;

        if((err = snd_pcm_hw_params_test_format(capture_handle, hwParams, format)) < 0) {
            fprintf (stderr, "Audio device doesnt support the given format %s (%s)\n", device, snd_strerror (err));
            return;
        }

        //snd_pcm_hw_params_set_period_size_near(capture_handle, hwParams, &desired_period, &dir);
        //snd_pcm_hw_params_set_period_time(capture_handle, hwParams, desired_period, dir);
        //err = snd_pcm_hw_params(capture_handle, hwParams);
        snd_pcm_hw_params_set_access(capture_handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
        snd_pcm_hw_params_set_format(capture_handle, hwParams, format);
        snd_pcm_hw_params_set_channels(capture_handle, hwParams, NUM_CHANNELS);
        snd_pcm_hw_params_set_rate_near(capture_handle, hwParams, &SAMPLE_RATE, &dir);
        snd_pcm_hw_params_set_buffer_size_near(capture_handle, hwParams, &BUFFER_SIZE);
        printf("got sample rate = %i\n", SAMPLE_RATE);
        snd_pcm_hw_params_set_period_size_near(capture_handle, hwParams, &period_size, &dir);
        snd_pcm_hw_params_set_rate_resample(capture_handle, hwParams, 1);
        snd_pcm_hw_params_set_drain_silence(capture_handle, hwParams, 1);

        if ((err = snd_pcm_hw_params(capture_handle, hwParams)) < 0) {
            fprintf(stderr, "sw_params setting error: %s\n", snd_strerror(err));
            return;
        }

        frame_size = BUFFER_SIZE * NUM_CHANNELS;

        buf_size = BUFFER_SIZE;
        sampleRate = SAMPLE_RATE;

        BUF_FRAMES = BUFFER_SIZE * NUM_CHANNELS;
        recordBuffer = new Ty[BUF_FRAMES];

        //if ((err = snd_pcm_set_params(capture_handle, format,  SND_PCM_ACCESS_RW_INTERLEAVED, NUM_CHANNELS, SAMPLE_RATE, 0, 0)) < 0) {
        //    fprintf(stderr, "capture open error: %s\n", snd_strerror(err)); exit(1);
        //}

        printf("Got %lu period\n", period_size);

        int nfds = snd_pcm_poll_descriptors_count(capture_handle);
        pollDescriptors = new pollfd[nfds];

        err = snd_pcm_poll_descriptors(capture_handle, pollDescriptors, nfds);

        snd_output_t* output = nullptr;
        err = snd_output_stdio_attach(&output, stdout, 0);

        //snd_pcm_sw_params_t*  sw_params;
        //snd_pcm_sw_params_current(capture_handle, sw_params);
        //snd_pcm_sw_params_dump(sw_params, output);

        std::cout << "HW params:" << std::endl;
        snd_pcm_hw_params_t* hw_params;
        err = snd_pcm_hw_params_malloc(&hw_params);
        if ((err = snd_pcm_hw_params_current(capture_handle, hw_params)) < 0) {
            fprintf(stderr, "hw_params setting error: %s\n", snd_strerror(err));
            return;
        }
        snd_pcm_hw_params_dump(hwParams, output);

        //snd_pcm_nonblock(capture_handle, SND_PCM_NONBLOCK);

        //snd_async_handler_t *pcm_callback;
        //
        //snd_async_add_pcm_handler(&pcm_callback, capture_handle, MyCallback, this);

        //snd_pcm_sw_params_t* sw_params;
        //snd_pcm_sw_params_current(capture_handle, sw_params);
        //snd_pcm_sw_params_malloc(&sw_params);
        //snd_pcm_sw_params_set_stop_threshold(capture_handle, sw_params, 1000000);

        //free(device);

        isCreated = true;
    }

    ~ALSA_AudioDevice() {
        delete[] recordBuffer;
        delete[] playbackbuffer;
        delete[] pollDescriptors;
        if(playback_handle)
            snd_pcm_close (playback_handle);
        if(capture_handle)
            snd_pcm_close (capture_handle);
    }

    //int StartRecordingBlocking();
    //int StartRecording();
    //int ClearRecordBuffer();
    //bool UpdateState();
    ////int StartLoopRecording();
    ////
    ////int ResetRecording();
    ////int StopRecording();
    //int PlayAudio();

    int StartRecording() {
        int err = 0;
        if((snd_pcm_start(capture_handle) < 0)) {
            fprintf (stderr, "Starting recording failed (%s)\n", snd_strerror (err));
            return err;
        }

        read_frames = 0;
        isRecording = true;

        return 0;
    }

    int StartRecordingBlocking() {
        int err = 0;
        if ((err = snd_pcm_readi (capture_handle, recordBuffer, BUF_FRAMES)) != BUF_FRAMES) {
            fprintf (stderr, "read from audio interface failed (%s)\n", snd_strerror (err));
        }
        return err;
    }

    // I might use this for audio latency tests
    int PlayAudio() {
        int err = 0;
        if ((err = snd_pcm_writei (playback_handle, playbackbuffer, BUF_FRAMES)) != BUF_FRAMES) {
            fprintf (stderr, "write to audio interface failed (%s)\n", snd_strerror (err)); exit (1);
        }
        return err;
    }

    int ClearRecordBuffer() {
        int err = 0;
        //auto start = micros1();
        //snd_pcm_drain(capture_handle);
        if((err = snd_pcm_drop(capture_handle)) < 0) {
            fprintf(stderr, "dropping capture failed: (%s)\n", snd_strerror (err)); return err;
        }
        if((err = snd_pcm_prepare(capture_handle)) < 0) {
            fprintf(stderr, "dropping capture failed: (%s)\n", snd_strerror (err)); return err;
        }
        if((err = snd_pcm_reset(capture_handle)) < 0) {
            fprintf(stderr, "resetting capture failed: (%s)\n", snd_strerror (err)); return err;
        }
        //printf("Took %luus to reset\n", micros1() - start);
        return 0;
    }

// Pretty much just waits for new data, and if there is any, just accumulates it
// Doesn't matter if we are reading it too slowly, it won't affect the final result
// Thats because we are forcing period time to be as big as possible (bigger than 512...)
    bool UpdateState() {

        // Dont even try to read if there is nothing to be read
        auto status = snd_pcm_avail(capture_handle);
        if(status <= 0)
            return false;

        printf("status: %i\n", status);

        int err;
        int ret = poll(pollDescriptors, 1, 0); // Timeout of 0ms
        if (ret == -1) {
            std::cerr << "Error in poll: " << strerror(errno) << std::endl;
            return false;
        } else if (ret == 0) {
            return false; // No Events
        }
        // Check if there is data available
        if (pollDescriptors[0].revents & POLLIN)
        {
            // Read data from the PCM device
            //size_t offset = (read_frames / period_size) * period_size;
            auto* tmp_buf = new Ty[status * 2];
            //err = snd_pcm_readi(capture_handle, tmp_buf, status);
            err = snd_pcm_readi(capture_handle, recordBuffer + read_frames, status);
            if (err < 0) {
                delete[] tmp_buf;
                std::cerr << "Error reading from PCM device: " << snd_strerror(err) << std::endl;
                return false;
            }

            printf("read frames: %i, new frames: %i, status: %i\n", read_frames, err, status);

            if(status < err)
                exit(1);

            //for(int i = 0; i < err; i++) {
            //    int cur_frame = i + read_frames;
            //    if(cur_frame >= BUFFER_SIZE)
            //        break;
            //    if(cur_frame % 2 == 0) {
            //        recordBuffer[cur_frame / 2] = tmp_buf[i];
            //    }
            //    else {
            //        recordBuffer[cur_frame / 2 + (BUFFER_SIZE / 2)] = tmp_buf[i];
            //    }
            //
            //    //printf("frame %i\n", cur_frame);
            //};

            delete[] tmp_buf;

            //printf("Got %i new frames\n", err);
            //printf("overall: %u\n", read_frames);
            //if(is_first_cn)
            read_frames += err*2;

            //pollDescriptors[0].revents = 0;
            if(read_frames >= BUFFER_SIZE) {
                isRecording = false;
                return true;
            }
            else return false;
        }
    }
};

UINT GetAudioDevices(cAudioDeviceInfo** devices);
#endif