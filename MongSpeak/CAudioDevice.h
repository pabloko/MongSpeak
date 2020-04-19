#pragma once
extern WebWindow* g_webWindow;

/*CAudioDevice: Spawns a thread to control an audio device over WASAPI shared, timer'ed mode*/

class CAudioDevice {
public:
	CAudioDevice(IMMDevice* mDev, EDataFlow mMode) : pDevice(mDev), pDeviceDirection(mMode) {
		//Start the multimedia thread
		hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)CAudioDevice::AudioThread, (void*)this, NULL, NULL);
	}
	~CAudioDevice() {
		//Cleanup on delete
		bWorking = FALSE;
		TerminateThread(hThread, 0);
	}
	void SetAudioQueue(CAudioQueue* aq) {
		//Link this instance to a CAudioQueue derivated class (audio exchange)
		pAudioQueue = aq;
	}
	CAudioQueue* GetAudioQueue() {
		//Returns CAudioQueude derivated class associated with this instance
		return pAudioQueue;
	}
	DWORD AudioProcess() {
		HRESULT hr = S_OK;
		//Initialice COM on current thread
		hr = CoInitialize(NULL);
		if (FAILED(hr)) return -__LINE__;
		CoUninitializeCleanup release_CoInitialize;
		//Set MMCSS (Multimedia characteristics) on this thread to "Audio" preset
		DWORD nTaskIndex = 0;
		HANDLE hTask = AvSetMmThreadCharacteristics(L"Audio", &nTaskIndex);
		if (NULL == hTask) return HRESULT_FROM_WIN32(GetLastError());
		AvRevertMmThreadCharacteristicsRelease release_AvSetMmThreadCharacteristics(hTask);
		//Activate the provided IMMDevice, dtoring will take care of calling Stop()
		hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
		if (FAILED(hr)) return hr;
		ReleaseAndStopIUnknown release_IAudioClient(pAudioClient);
		//Query the audio device to get its parameters
		hr = pAudioClient->GetDevicePeriod(&hDefaultDevicePeriod, &hMinimumDevicePeriod);
		if (FAILED(hr)) return hr;
		hr = pAudioClient->GetMixFormat(&pWaveFormat);
		if (FAILED(hr)) return hr;
		CoTaskMemFreeRelease release_pWaveFormat(pWaveFormat);
		//Windows apis always use FLOAT for each sample, here we coerce this value to use SHORT
		switch (pWaveFormat->wFormatTag) {
		case WAVE_FORMAT_IEEE_FLOAT: {
			pWaveFormat->wFormatTag = WAVE_FORMAT_PCM;
			pWaveFormat->wBitsPerSample = 16;
			pWaveFormat->nBlockAlign = pWaveFormat->nChannels * pWaveFormat->wBitsPerSample / 8;
			pWaveFormat->nAvgBytesPerSec = pWaveFormat->nBlockAlign * pWaveFormat->nSamplesPerSec;
		}break;
		case WAVE_FORMAT_EXTENSIBLE: {
		PWAVEFORMATEXTENSIBLE pEx = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(pWaveFormat);
		if (IsEqualGUID(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT, pEx->SubFormat)) {
			pEx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
			pEx->Samples.wValidBitsPerSample = 16;
			pWaveFormat->wBitsPerSample = 16;
			pWaveFormat->nBlockAlign = pWaveFormat->nChannels * pWaveFormat->wBitsPerSample / 8;
			pWaveFormat->nAvgBytesPerSec = pWaveFormat->nBlockAlign * pWaveFormat->nSamplesPerSec;
		} else return -16;
		}  break;
		
		default: {
			return -16;
		} break;
		}
		nBlockAlign = pWaveFormat->nBlockAlign;
		//Setting up an event for IAudioClient semaphore
		hEvent = CreateEvent(nullptr, false, false, nullptr);
		HandleRelease release_hEvent(hEvent);
		//Setting up an audio resampler, so CAudioQueue derivated task always has the same
		SpeexResamplerState* pResampler = NULL;
		if (pWaveFormat->nSamplesPerSec != DEFAULT_SAMPLERATE)
			bNeedResample = TRUE;
		short fResampled[9048];
		DWORD dwFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
		/*if (pDeviceMode == EDataFlow::eRender && pDeviceDirection == EDataFlow::eCapture)
		dwFlags |= AUDCLNT_STREAMFLAGS_LOOPBACK;*/
		//Initialize IAudioClient in shared mode and coerced waveformat
		hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, dwFlags, hMinimumDevicePeriod, 0, pWaveFormat, nullptr);
		if (FAILED(hr)) return hr;
		//This event will be locked until next packet is avalible
		hr = pAudioClient->SetEventHandle(hEvent);
		if (FAILED(hr)) return hr;
		//IAudioClient is ready, start it.
		hr = pAudioClient->Start();
		if (FAILED(hr)) return hr;
		//Get default device's frames-per-packet value, at least it shouldn't be higher
		hr = pAudioClient->GetBufferSize(&hNumBufferFrames);
		if (FAILED(hr)) return hr;
		//If device is an input, obtain IAudioCaptureClient service, and setup resampler
		if (pDeviceDirection == EDataFlow::eCapture) {
			hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pAudioCaptureClient);
			if (FAILED(hr)) return hr;
			if (bNeedResample)
				pResampler = speex_resampler_init(pWaveFormat->nChannels, pWaveFormat->nSamplesPerSec, DEFAULT_SAMPLERATE, 5, NULL);
		}
		//If device is an output, obtain IAudioRenderClient service, and setup resampler
		ReleaseIUnknown release_pAudioCaptureClient(pAudioCaptureClient);
		if (pDeviceDirection == EDataFlow::eRender) {
			hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pAudioRenderClient);
			if (FAILED(hr)) return hr;
			if (bNeedResample)
				pResampler = speex_resampler_init(pWaveFormat->nChannels, DEFAULT_SAMPLERATE, pWaveFormat->nSamplesPerSec, 5, NULL);
		}
		ReleaseIUnknown release_pAudioRenderClient(pAudioRenderClient);
		ReleaseSpeexResampler release_pResampler(pResampler);
		//This loop will run while processing audio
		while (bWorking) {
			BYTE* pData;
			//Wait until device is ready to serve a packet
			WaitForSingleObject(hEvent, INFINITE);
			UINT32 NumPaddingFrames = 0;
			//Todo: sometimes delete and this thread dont sync, prevent it from reading nullptr
			if ((DWORD)pAudioClient == 0xDDDDDDDD) 
				return 666;
			//As we are using default device's frames-per-packet, padding represents how many to skip at the end of this value
			hr = pAudioClient->GetCurrentPadding(&NumPaddingFrames);
			if (FAILED(hr)) return hr;
			UINT32 numAvailableFrames = hNumBufferFrames - NumPaddingFrames;
			//If device is an output
			if (pDeviceDirection == EDataFlow::eRender) {
				//Todo: never should be 0, this could indicate a problem, never happened tho...
				if (numAvailableFrames == 0) continue;
				//Obtain writable audio packet from IAudioRenderClient
				hr = pAudioRenderClient->GetBuffer(numAvailableFrames, &pData);
				if (FAILED(hr)) return hr;
				//Pointer to data
				BYTE* ppData = pData;
				//If a CAudioQueue instance is associated to this instance, exchange the packet
				if (pAudioQueue != nullptr)
					//If resampler is needed (most likely)
					if (bNeedResample) {
						//Recalculate samples asked to CAudioQueue to write
						UINT32 szResampledLen = numAvailableFrames,
						szInputLen = numAvailableFrames * DEFAULT_SAMPLERATE / pWaveFormat->nSamplesPerSec;
						//Replace pointer to internal buffer
						ppData = (BYTE*)fResampled;
						//Call CAudioQueue derivated class, asking it to write the buffer
						pAudioQueue->DoTask(szInputLen, (char*)ppData, pWaveFormat);
						//Resample the obtained buffer to the audio data pointer
						speex_resampler_process_interleaved_int(pResampler, (short*)ppData, &szInputLen, (short*)pData, &szResampledLen);
					}
					else 
						//If no resampling is needed, just call the CAudioQueue instance and ask to write directly to audio buffer
						if (pAudioQueue != nullptr)
							pAudioQueue->DoTask(numAvailableFrames, (char*)ppData, pWaveFormat);
				//Mandatory release will actually queue the buffer
				hr = pAudioRenderClient->ReleaseBuffer(numAvailableFrames, 0);
				if (FAILED(hr)) return hr;
			}
			//If device is an input
			if (pDeviceDirection == EDataFlow::eCapture) {
				//Obtain audio packet from IAudioCaptureClient
				DWORD pFlags = NULL;
				hr = pAudioCaptureClient->GetBuffer(&pData, &numAvailableFrames, &pFlags, NULL, NULL);
				if (FAILED(hr)) return hr;
				//Todo: flags indicate not valid audio being present
				//wprintf(L"iac flags %d\n", pFlags);
				if (pFlags > 0) {
					//Todo: when data_discontinuity flag is present theres audible glitches, try to mitigate
					if (pFlags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
						//Todo: this trick solves the issue 95% of time
						Sleep(10); //trick: try to compensate underflow with overflow
						iDiscontinuities++; //Discontinuity counter update
						iRecoverDisc = 0; //Reset discontinuity recovery value
						if (iDiscontinuities > 10) { //More than 10 discontinuities, take some action
							iDiscontinuities = 0;
							wprintf(L"IRRECOVERABLE DEVICE ALERT\n");
							//Sending the UI an event to reset the audio device (it was fast...)
							if (g_webWindow)
								g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -666, (wchar_t*)L"Irrecoverable");
							//Todo: We could try to read packets til buffer is 0, anyway release and break this instance...
							pAudioCaptureClient->ReleaseBuffer(numAvailableFrames);
							return AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY;
						}
					}
					//Not sending this packet to CAudioQueue instance since its worthless, but at least make a call, it may have some work to do...
					if (pAudioQueue != nullptr)
						pAudioQueue->DoTask(0, NULL, pWaveFormat);
				} else {
					//Update discontinuity recovery value
					iRecoverDisc++;
					if (iRecoverDisc > 10)
						iDiscontinuities = 0, iRecoverDisc = 0;
					//Pointer to audio packet data
					BYTE* ppData = pData;
					UINT32 resampled_numAvailableFrames = numAvailableFrames;
					//If we need a resample (most likely)
					if (bNeedResample) {
						UINT32 szResampledLen = numAvailableFrames,
							szInputLen = numAvailableFrames;
						//Resample the audio data to a internal buffer
						speex_resampler_process_interleaved_int(pResampler, (short*)pData, &szInputLen, fResampled, &szResampledLen);
						//Replace pointer to data to our internal buffer, and also update number of frames
						ppData = (BYTE*)fResampled;
						resampled_numAvailableFrames = szResampledLen;
					}
					//Call the CAudioQueue derivated class
					if (pAudioQueue != nullptr)
						pAudioQueue->DoTask(resampled_numAvailableFrames, (char*)ppData, pWaveFormat);
				}
				//Mandatory release
				hr = pAudioCaptureClient->ReleaseBuffer(numAvailableFrames);
				if (FAILED(hr)) return hr;
			}
		}
		bWorking = FALSE;
		return hr;
	}
	//Helper to run the thread
	static DWORD AudioThread(void* pParam) {
		DWORD hr = ((CAudioDevice*)pParam)->AudioProcess();
		((CAudioDevice*)pParam)->bWorking = FALSE;
		return hr;
	}
private:
	BOOL bWorking = TRUE;
	IMMDevice* pDevice = nullptr;
	IAudioClient* pAudioClient = nullptr;
	IAudioRenderClient* pAudioRenderClient = nullptr;
	IAudioCaptureClient* pAudioCaptureClient = nullptr;
	WAVEFORMATEX* pWaveFormat;
	REFERENCE_TIME hDefaultDevicePeriod = 0;
	REFERENCE_TIME hMinimumDevicePeriod = 0;
	UINT32 hNumBufferFrames = 0;
	EDataFlow pDeviceDirection;
	UINT32 nBlockAlign;
	HANDLE hEvent = nullptr;
	HANDLE hThread = nullptr;
	BOOL bNeedResample = FALSE;
	CAudioQueue* pAudioQueue = nullptr;
	UINT32 iDiscontinuities = 0, iRecoverDisc=0;
};