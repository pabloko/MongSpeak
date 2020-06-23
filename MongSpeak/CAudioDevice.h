#pragma once
extern WebWindow*				g_webWindow;
static GUID g_audio_id =		GUID_NULL;
#define REFTIMES_PER_SEC		10000000
#define REFTIMES_PER_MILLISEC	10000

class CAudioDevice {
public:
	CAudioDevice(IMMDevice* mDev, EDataFlow mMode) : pDevice(mDev), pDeviceDirection(mMode) {
		if (g_audio_id == GUID_NULL) 
			CoCreateGuid(&g_audio_id);
		hThread = CreateThread(NULL, NULL, AudioThread, this, NULL, NULL);
	}
	~CAudioDevice() {
		bWorking = FALSE;
		TerminateThread(hThread, 0);
	}
	void SetAudioQueue(CAudioQueue* aq) {
		pAudioQueue = aq;
	}
	CAudioQueue* GetAudioQueue() {
		return pAudioQueue;
	}
	DWORD AudioProcess() {
		//Sleep(100);
		HRESULT hr = S_OK;
		hr = CoInitialize(NULL);
		if (FAILED(hr)) return -__LINE__;
		CoUninitializeCleanup release_CoInitialize;
		DWORD nTaskIndex = 0;
		HANDLE hTask = AvSetMmThreadCharacteristics(L"Audio", &nTaskIndex);
		if (NULL == hTask) return HRESULT_FROM_WIN32(GetLastError());
		AvRevertMmThreadCharacteristicsRelease release_AvSetMmThreadCharacteristics(hTask);
		hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&pAudioClient);
		if (FAILED(hr)) return hr;
		ReleaseAndStopIUnknown release_IAudioClient(pAudioClient);
		hr = pAudioClient->GetDevicePeriod(&hDefaultDevicePeriod, &hMinimumDevicePeriod);
		if (FAILED(hr)) return hr;
		hr = pAudioClient->GetMixFormat(&pWaveFormat);
		if (FAILED(hr)) return hr;
		CoTaskMemFreeRelease release_pWaveFormat(pWaveFormat);
		switch (pWaveFormat->wFormatTag) {
			case WAVE_FORMAT_IEEE_FLOAT: {
				pWaveFormat->wFormatTag = WAVE_FORMAT_PCM;
				pWaveFormat->wBitsPerSample = 16;
				pWaveFormat->nBlockAlign = pWaveFormat->nChannels * pWaveFormat->wBitsPerSample / 8;
				pWaveFormat->nAvgBytesPerSec = pWaveFormat->nBlockAlign * pWaveFormat->nSamplesPerSec;
			} break;
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
		hEvent = CreateEvent(nullptr, false, false, nullptr);
		HandleRelease release_hEvent(hEvent);
		SpeexResamplerState* pResampler = NULL;
		if (pWaveFormat->nSamplesPerSec != DEFAULT_SAMPLERATE)
			bNeedResample = TRUE;
		short fResampled[9048];
		DWORD dwFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST;
		/*if (pDeviceMode == EDataFlow::eRender && pDeviceDirection == EDataFlow::eCapture)
		dwFlags |= AUDCLNT_STREAMFLAGS_LOOPBACK;*/
		hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, dwFlags, hMinimumDevicePeriod, 0, pWaveFormat, &g_audio_id);
		if (FAILED(hr)) return hr;
		hr = pAudioClient->SetEventHandle(hEvent);
		if (FAILED(hr)) return hr;
		hr = pAudioClient->Start();
		if (FAILED(hr)) return hr;
		hr = pAudioClient->GetBufferSize(&hNumBufferFrames);
		if (FAILED(hr)) return hr;
		if (pDeviceDirection == EDataFlow::eCapture) {
			hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pAudioCaptureClient);
			if (FAILED(hr)) return hr;
			if (bNeedResample)
				pResampler = speex_resampler_init(pWaveFormat->nChannels, pWaveFormat->nSamplesPerSec, DEFAULT_SAMPLERATE, 5, NULL);
		}
		ReleaseIUnknown release_pAudioCaptureClient(pAudioCaptureClient);
		if (pDeviceDirection == EDataFlow::eRender) {
			hr = pAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&pAudioRenderClient);
			if (FAILED(hr)) return hr;
			if (bNeedResample)
				pResampler = speex_resampler_init(pWaveFormat->nChannels, DEFAULT_SAMPLERATE, pWaveFormat->nSamplesPerSec, 5, NULL);
		}
		ReleaseIUnknown release_pAudioRenderClient(pAudioRenderClient);
		ReleaseSpeexResampler release_pResampler(pResampler);
		while (bWorking) {
			BYTE* pData;
			WaitForSingleObject(hEvent, (hDefaultDevicePeriod / 1000));

			if (pDeviceDirection == EDataFlow::eRender) {

				UINT32 NumPaddingFrames = 0;
				hr = pAudioClient->GetCurrentPadding(&NumPaddingFrames);
				if (FAILED(hr)) return hr;
				UINT32 numAvailableFrames = hNumBufferFrames - NumPaddingFrames;

				if (numAvailableFrames == 0) continue;
				hr = pAudioRenderClient->GetBuffer(numAvailableFrames, &pData);
				if (hr==AUDCLNT_E_DEVICE_INVALIDATED || hr == AUDCLNT_E_SERVICE_NOT_RUNNING) return hr;
				BYTE* ppData = pData;
				if (pAudioQueue != nullptr)
					if (bNeedResample) {
						UINT32 szResampledLen = numAvailableFrames,
						szInputLen = numAvailableFrames * DEFAULT_SAMPLERATE / pWaveFormat->nSamplesPerSec;
						ppData = (BYTE*)fResampled;
						pAudioQueue->DoTask(szInputLen, (char*)ppData, pWaveFormat);
						speex_resampler_process_interleaved_int(pResampler, (short*)ppData, &szInputLen, (short*)pData, &szResampledLen);
					} else {
						if (pAudioQueue != nullptr)
							pAudioQueue->DoTask(numAvailableFrames, (char*)ppData, pWaveFormat);
					}
				hr = pAudioRenderClient->ReleaseBuffer(numAvailableFrames, 0);
				if (hr == AUDCLNT_E_DEVICE_INVALIDATED || hr == AUDCLNT_E_SERVICE_NOT_RUNNING) return hr;
			}

			if (pDeviceDirection == EDataFlow::eCapture) {

				UINT32 packetSize = 0;
				pAudioCaptureClient->GetNextPacketSize(&packetSize);
				while (packetSize != 0) {
					UINT32 numAvailableFrames = 0;
					DWORD pFlags = NULL;
					hr = pAudioCaptureClient->GetBuffer(&pData, &numAvailableFrames, &pFlags, NULL, NULL);
					if (hr == AUDCLNT_E_DEVICE_INVALIDATED || hr == AUDCLNT_E_SERVICE_NOT_RUNNING) return hr;
					if (pFlags != 0) {
						ZeroMemory(pData, numAvailableFrames);
						/*if (pFlags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY) {
							iDiscontinuities++;
							iRecoverDisc = 0;
							if (iDiscontinuities == 10) {
								g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -666, (wchar_t*)L"Irrecoverable");
								bWorking = FALSE;
							}
						}
						if (pAudioQueue != nullptr) pAudioQueue->DoTask(0, NULL, pWaveFormat);*/
					} else {
						/*iRecoverDisc++;
						if (iRecoverDisc > 10)
							iDiscontinuities = 0, iRecoverDisc = 0;*/
						BYTE* ppData = pData;
						UINT32 resampled_numAvailableFrames = numAvailableFrames;
						if (bNeedResample) {
							UINT32 szResampledLen = numAvailableFrames,
								szInputLen = numAvailableFrames;
							speex_resampler_process_interleaved_int(pResampler, (short*)pData, &szInputLen, fResampled, &szResampledLen);
							ppData = (BYTE*)fResampled;
							resampled_numAvailableFrames = szResampledLen;
						}
						if (pAudioQueue != nullptr) pAudioQueue->DoTask(resampled_numAvailableFrames, (char*)ppData, pWaveFormat);
					}
					hr = pAudioCaptureClient->ReleaseBuffer(numAvailableFrames);
					if (hr == AUDCLNT_E_DEVICE_INVALIDATED || hr == AUDCLNT_E_SERVICE_NOT_RUNNING) return hr;
					pAudioCaptureClient->GetNextPacketSize(&packetSize);
				}
			}
		}
		return hr;
	}
	static DWORD WINAPI AudioThread(void* pParam) {
		DWORD hr = ((CAudioDevice*)pParam)->AudioProcess();
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
	UINT32 iDiscontinuities = 0, iRecoverDisc = 0;
};