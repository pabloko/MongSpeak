#pragma once
/*CAudioStream will work with an input CAudioDevice and run in its thread
Provides capturing audio, processing it with SpeexDSP, encoding to Opus and sending to network*/
extern CNetwork* g_network;
extern vector<wstring> g_jsStack;
//Speex preprocessor want "CBR" audio packets, so fix this value here, docs said shoudl represent 10-20ms of audio
#define PREPROCESS_SAMPLES (160*2)

class CAudioStream : CAudioQueue {
public:
	CAudioStream() : fVol(1.0f), iInputMethod(-666), bIsInput(FALSE), bSendVu(FALSE), fVuSumMax(0.0f), fVuSumMin(0.0f), lVuCount(0), fTol(0.0f) {
		//Setup Opus encoder
		pOpus = opus_encoder_create(DEFAULT_SAMPLERATE, OPUS_CHANNELS, OPUS_APPLICATION_VOIP, NULL);
		opus_encoder_ctl(pOpus, OPUS_SET_BITRATE(DEFAULT_OPUS_BITRATE));
		opus_encoder_ctl(pOpus, OPUS_SET_PREDICTION_DISABLED(TRUE));
		//Setup SpeexDSP preprocessor features (VAD, AGC, Denoise) this values are [todo]
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
	}
	~CAudioStream() {
		speex_preprocess_state_destroy(pSpeexPreprocessor);
		opus_encoder_destroy(pOpus);
		if (pDeviceIn != NULL)
			pDeviceIn->SetAudioQueue(NULL);
	}
	void SetInputMethod(int input) {
		//Set how to activate voice sending
		//0=always on
		//-666=speex vad
		//>0 = virtual key, fe: VK_F3
		//<0 = % of decibles *-1, fe=-24:24% (limit -200, weird huh)
		iInputMethod = input;
	}
	void SetVol(float vol) {
		//Set master input volume multiplicator
		fVol = vol;
	}
	void SetDeviceIn(CAudioDevice* dev) {
		//Connect this instance to a CAudioDevice instance
		//If provided is NULL, were cleaning the house, so remove this instance of previously used CAudioDevice
		if (pDeviceIn != NULL && dev == NULL)
			pDeviceIn->SetAudioQueue(NULL);
		//Else, connect CAudioDevice and notify it we're accepting its data
		pDeviceIn = dev;
		if (dev != NULL)
			dev->SetAudioQueue(this);
	}
	void CompareInput(BOOL in) {
		//Each audio packet, we compare the input method with its state, if has changed, notify the network
		if (bIsInput != in) {
			bIsInput = in;
			vector<uint8_t> pv;
			if (bIsInput)
				rpc_write_short(&pv, 10);
			else
				rpc_write_short(&pv, 11);
			g_network->Send(RPCID::UI_COMMAND, &pv);
		}
	}
	void SendVU(BOOL vu) {
		//Helper, enabling it will send to UI fast decibel % data (used in settings panel)
		bSendVu = vu;
	}
	//CAudioQueue task called by CAudioDevice
	void DoTask(int len, char* data, WAVEFORMATEX* wf) {
		//Volume is muted, dont continue
		if (fVol == 0.0f)
			return CompareInput(FALSE);
		//Check KEYCODE input (dont do it if we're sending vu data, as it wont get processed)
		if (!bSendVu && iInputMethod > 0) 
			if (!GetAsyncKeyState(iInputMethod)) 
				return CompareInput(FALSE);
		short* pData = (short*)data;
		//Compute volume on audio samples
		if (fVol != 1.0f) 
			for (int i = 0; i < len * wf->nChannels; i+= wf->nChannels)
				pData[i] = pData[i] * fVol;
		//Check MANUAL-VAD input
		if ((iInputMethod < 0 && iInputMethod > -200) || bSendVu) {
			//Obtain power of audio
			lVuCount += len;
			for (int i = 0; i < len * wf->nChannels; i += wf->nChannels) {
				float vl = ((float)pData[i + 0]) / (float)32768; // (float)(pData[i + 0] - 32767);
				fVuSumMin = (vl < fVuSumMin ? vl : fVuSumMin);
				fVuSumMax = (vl > fVuSumMax ? vl : fVuSumMax);
			}
			//Limit sending vu data to UI to FRAMERATE/20
			if (lVuCount > (wf->nSamplesPerSec / 20)) {
				//Compute power to decibels, log2(x)*6.02 should be faster than log10(x)*20
				fTol = log2(max(fVuSumMax, -fVuSumMin)) * 6.02f;
				//Adjust db to % in -40 - 0 db dynamic range
				if (fTol < -40.0f) fTol = -40.0f;
				fTol += 40.0f; fTol = (fTol * 100.0f) / 40.0f;
				if (bSendVu) 
					g_jsStack.push_back(wstring_format(L"onEvent(%d, %d, %3.0f);", RPCID::UI_COMMAND, -3, fTol));
				fVuSumMax = 0.0f; fVuSumMin = 0.0f; lVuCount = 0;
			}
			//Compare db % with toggle value
			if (iInputMethod < 0 && iInputMethod > -200) {
				float tolerance = iInputMethod * -1;
				if (fTol < tolerance)
					return CompareInput(FALSE);
			}
		}
		//We previously didnt checked this, so check now
		if (bSendVu && iInputMethod > 0) {
			if (!GetAsyncKeyState(iInputMethod)) 
				return CompareInput(FALSE);
		}
		//Deinterleave audio data, we want only 1 channel
		int c = 0;
		for (int i = 0; i < len * wf->nChannels; i += wf->nChannels) 
			fDeinterleaved[c] = pData[i]; c++;
		//Append deinterleaved audio to a circular buffer
		char* dein = (char*)fDeinterleaved;
		pVecAudioBuffer.append(&dein[0], &dein[len * sizeof(short)]);
		//Speex dsp needs same number of audio samples each call, and probably opus will benefit from it too
		while (pVecAudioBuffer.length() > (PREPROCESS_SAMPLES * sizeof(short))) {
			//Pass buffer to SpeexDSP preprocessor, it will rewrite it
			int vad = speex_preprocess_run(pSpeexPreprocessor, (short*)pVecAudioBuffer.data());
			//wprintf(L"VAD %d\n", vad);
			//Encode the buffer to Opus
			int opus_bytes = opus_encode(pOpus, (short*)pVecAudioBuffer.data(), PREPROCESS_SAMPLES, szOpusBuffer, sizeof(szOpusBuffer));
			//Remove the used buffer from circular buffer
			pVecAudioBuffer.erase(pVecAudioBuffer.begin(), pVecAudioBuffer.begin() + (PREPROCESS_SAMPLES * sizeof(short)));
			//Check SPEEXVAD input
			if (iInputMethod==-666 && vad == 0)
				return CompareInput(FALSE);
			//We got here, no input method returned, so audio can be sent to network
			CompareInput(TRUE);
			if (g_network && opus_bytes > 0)
				g_network->Send(RPCID::OPUS_DATA, (char*)szOpusBuffer, opus_bytes);
		}
	};
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
	SpeexPreprocessState* pSpeexPreprocessor;
	std::string pVecAudioBuffer;
};
