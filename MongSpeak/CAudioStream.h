#pragma once

extern CNetwork* g_network;
extern ITaskbarList3* g_Taskbar;
extern BOOL bPreventSimultaneousUpload;
extern void set_taskbar_state();

#define PREPROCESS_SAMPLES (160*2)
class CAudioStream : CAudioQueue {
public:
	CAudioStream() : fVol(1.0f), iInputMethod(-666), bIsInput(FALSE), bSendVu(FALSE), fVuSumMax(0.0f), fVuSumMin(0.0f), lVuCount(0), fTol(0.0f) {
		pOpus = opus_encoder_create(DEFAULT_SAMPLERATE, OPUS_CHANNELS, OPUS_APPLICATION_VOIP, NULL);
		opus_encoder_ctl(pOpus, OPUS_SET_BITRATE(DEFAULT_OPUS_BITRATE));
		opus_encoder_ctl(pOpus, OPUS_SET_PREDICTION_DISABLED(TRUE));
#ifdef USING_SPEEXDSP
		FLOAT f; INT i;
		pSpeexPreprocessor = speex_preprocess_state_init(PREPROCESS_SAMPLES, DEFAULT_SAMPLERATE); 
		i = 98;
		speex_preprocess_ctl(pSpeexPreprocessor, SPEEX_PREPROCESS_SET_PROB_START, &i);
		f = 0.3f;
		speex_preprocess_ctl(pSpeexPreprocessor, SPEEX_PREPROCESS_SET_PROB_CONTINUE, &f);
		i = 1;
		speex_preprocess_ctl(pSpeexPreprocessor, SPEEX_PREPROCESS_SET_DENOISE, &i); 
		i = 1;
		speex_preprocess_ctl(pSpeexPreprocessor, SPEEX_PREPROCESS_SET_VAD, &i); 
		i = 1;
		speex_preprocess_ctl(pSpeexPreprocessor, SPEEX_PREPROCESS_SET_AGC, &i); 
#endif
#ifdef USING_FVAD
		pVad = fvad_new();
		fvad_set_mode(pVad, 3);
		fvad_set_sample_rate(pVad, DEFAULT_SAMPLERATE);
#endif
	}
	~CAudioStream() {
#ifdef USING_SPEEXDSP
		speex_preprocess_state_destroy(pSpeexPreprocessor);
#endif
#ifdef USING_FVAD
		fvad_free(pVad);
#endif
		opus_encoder_destroy(pOpus);
		if (pDeviceIn != NULL)
			pDeviceIn->SetAudioQueue(NULL);
	}
	void SetInputMethod(int input) {
		iInputMethod = input;
	}
	void SetVol(float vol) {
		fVol = vol;
	}
	void SetDeviceIn(CAudioDevice* dev) {
		if (pDeviceIn != NULL && dev == NULL)
			pDeviceIn->SetAudioQueue(NULL);
		pDeviceIn = dev;
		if (dev != NULL)
			dev->SetAudioQueue(this);
	}
	void CompareInput(BOOL in) {
		if (bIsInput != in) {
			bIsInput = in;
			vector<uint8_t> pv;
			if (bIsInput)
				rpc_write_short(&pv, 10);
			else
				rpc_write_short(&pv, 11);
			g_network->Send(RPCID::UI_COMMAND, &pv);
			if (g_Taskbar && bPreventSimultaneousUpload == FALSE) {
				if (bIsInput) {
					g_Taskbar->SetProgressState(g_webWindow->hWndWebWindow, TBPF_NORMAL);
					g_Taskbar->SetProgressValue(g_webWindow->hWndWebWindow, 100, 100);
				}
				else set_taskbar_state();
			}
		}
	}
	void SendVU(BOOL vu) {
		bSendVu = vu;
	}
	void DoTask(int len, char* data, WAVEFORMATEX* wf) {
		if (fVol == 0.0f)
			return CompareInput(FALSE);
		if (!bSendVu && iInputMethod > 0) 
			if (!GetAsyncKeyState(iInputMethod)) 
				return CompareInput(FALSE);
		short* pData = (short*)data;
		if (fVol != 1.0f) 
			for (int i = 0; i < len * wf->nChannels; i+= wf->nChannels)
				pData[i] = pData[i] * fVol;
		if ((iInputMethod < 0 && iInputMethod > -200) || bSendVu) {
			lVuCount += len;
			for (int i = 0; i < len * wf->nChannels; i += wf->nChannels) {
				float vl = ((float)pData[i + 0]) / (float)32768; 
				fVuSumMin = (vl < fVuSumMin ? vl : fVuSumMin);
				fVuSumMax = (vl > fVuSumMax ? vl : fVuSumMax);
			}
			if (lVuCount > (wf->nSamplesPerSec / 60)) {
				fTol = log2(max(fVuSumMax, -fVuSumMin)) * 6.02f;
				if (fTol < -40.0f) fTol = -40.0f;
				fTol += 40.0f; fTol = (fTol * 100.0f) / 40.0f;
				if (bSendVu)
					g_webWindow->webForm->QueueCallToEvent(RPCID::UI_COMMAND, -3, (short)fTol);
				fVuSumMax = 0.0f; fVuSumMin = 0.0f; lVuCount = 0;
			}
			if (iInputMethod < 0 && iInputMethod > -200) {
				float tolerance = iInputMethod * -1;
				if (fTol < tolerance)
					return CompareInput(FALSE);
			}
		}
		if (bSendVu && iInputMethod > 0) {
			if (!GetAsyncKeyState(iInputMethod)) 
				return CompareInput(FALSE);
		}
		///CompareInput(TRUE);
		int c = 0;
		for (int i = 0; i < len * wf->nChannels; i += wf->nChannels) {
			fDeinterleaved[c] = pData[i]; c++;
		}
#ifdef USING_SPEEXDSP
		char* dein = (char*)fDeinterleaved;
		pVecAudioBuffer.append(&dein[0], &dein[len * sizeof(short)]);
		while (pVecAudioBuffer.length() > (PREPROCESS_SAMPLES * sizeof(short))) {
			int vad = speex_preprocess_run(pSpeexPreprocessor, (short*)pVecAudioBuffer.data());
			//wprintf(L"VAD %d\n", vad);
			if (iInputMethod == -666 && vad == 0) {
				CompareInput(FALSE);
				pVecAudioBuffer.erase(pVecAudioBuffer.begin(), pVecAudioBuffer.begin() + (PREPROCESS_SAMPLES * sizeof(short)));
				continue;
			}
			/*if (bIsInput == FALSE) {
				float fader = 0.0f;
				float fader_step = 1.0f / PREPROCESS_SAMPLES;
				for (int cc = 0; cc < PREPROCESS_SAMPLES; cc++) {
					if (fader < 1.0f) fader += fader_step;
					fDeinterleaved[cc] = fDeinterleaved[cc] * fader;
				}
			}*/
			CompareInput(TRUE);
			int opus_bytes = opus_encode(pOpus, (short*)pVecAudioBuffer.data(), PREPROCESS_SAMPLES, szOpusBuffer, sizeof(szOpusBuffer));
			pVecAudioBuffer.erase(pVecAudioBuffer.begin(), pVecAudioBuffer.begin() + (PREPROCESS_SAMPLES * sizeof(short)));
			if (opus_bytes > 0)
				pVecOpusBuffer.append(&szOpusBuffer[0], &szOpusBuffer[opus_bytes]);
			/*if (g_network && opus_bytes > 0)
				g_network->Send(RPCID::OPUS_DATA, (char*)szOpusBuffer, opus_bytes);*/
		}

		if (g_network && pVecOpusBuffer.size() > 0) {
			g_network->Send(RPCID::OPUS_DATA, (char*)pVecOpusBuffer.c_str(), pVecOpusBuffer.size());
			pVecOpusBuffer.erase(pVecOpusBuffer.begin(), pVecOpusBuffer.end());
		}
#endif
#ifdef USING_FVAD
		if (iInputMethod == -666) {
			int vad = fvad_process(pVad, fDeinterleaved, len);
			if (vad < 1)
				return CompareInput(FALSE);
		}

		CompareInput(TRUE);
		int opus_bytes = opus_encode(pOpus, fDeinterleaved, len, szOpusBuffer, sizeof(szOpusBuffer));
		g_network->Send(RPCID::OPUS_DATA, (char*)szOpusBuffer, opus_bytes);
#endif
	};
	float GetVol() {
		return fVol;
	}
private:
	OpusEncoder* pOpus;
	CAudioDevice* pDeviceIn;
	unsigned char szOpusBuffer[2048];
	float fVol;
	int iInputMethod;
	BOOL bIsInput;
	BOOL bSendVu;
	DOUBLE fVuSumMax, fVuSumMin;
	LONG lVuCount;
	double fTol;
	short fDeinterleaved[2024];
#ifdef USING_SPEEXDSP
	SpeexPreprocessState* pSpeexPreprocessor;
	std::string pVecAudioBuffer;
	std::string pVecOpusBuffer;
#endif
#ifdef USING_FVAD
	Fvad* pVad;
#endif
};
