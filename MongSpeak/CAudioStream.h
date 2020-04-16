#pragma once

extern CNetwork* g_network;
extern vector<wstring> g_jsStack;

class CAudioStream : CAudioQueue {
public:
	CAudioStream() : fVol(1.0f), iInputMethod(VK_F3), bIsInput(FALSE), bSendVu(FALSE), fVuSumMax(0.0f), fVuSumMin(0.0f), lVuCount(0), fTol(0.0f) {
		pOpus = opus_encoder_create(DEFAULT_SAMPLERATE, OPUS_CHANNELS, OPUS_APPLICATION_VOIP, NULL);
		opus_encoder_ctl(pOpus, OPUS_SET_BITRATE(DEFAULT_OPUS_BITRATE));
		opus_encoder_ctl(pOpus, OPUS_SET_PREDICTION_DISABLED(TRUE));
	}
	~CAudioStream() {
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
		if (iInputMethod < 0 || bSendVu) {
			lVuCount += len;
			for (int i = 0; i < len * wf->nChannels; i += wf->nChannels) {
				float vl = ((float)pData[i + 0]) / (float)32768; // (float)(pData[i + 0] - 32767);
				fVuSumMin = (vl < fVuSumMin ? vl : fVuSumMin);
				fVuSumMax = (vl > fVuSumMax ? vl : fVuSumMax);
			}
			if (lVuCount > (wf->nSamplesPerSec / 20)) {
				fTol = log10(max(fVuSumMax, -fVuSumMin)) * 20;
				if (fTol < -40.0f) fTol = -40.0f;
				fTol += 40.0f; fTol = (fTol * 100.0f) / 40.0f;
				if (bSendVu) 
					g_jsStack.push_back(wstring_format(L"onEvent(%d, %d, %3.0f);", RPCID::UI_COMMAND, -3, fTol));
				fVuSumMax = 0.0f; fVuSumMin = 0.0f; lVuCount = 0;
			}
			if (iInputMethod < 0) {
				float tolerance = iInputMethod * -1;
				if (fTol < tolerance)
					return CompareInput(FALSE);
			}
		}
			
		if (bSendVu && iInputMethod > 0) {
			if (!GetAsyncKeyState(iInputMethod)) 
				return CompareInput(FALSE);
		}
		CompareInput(TRUE);

		int c = 0;
		for (int i = 0; i < len * wf->nChannels; i += wf->nChannels) {
			fDeinterleaved[c] = pData[i]; c++;
		}
		int samples = opus_encode(pOpus, fDeinterleaved, len, szOpusBuffer, sizeof(szOpusBuffer));
		if (g_network && samples > 0) 
			g_network->Send(RPCID::OPUS_DATA, (char*)szOpusBuffer, samples);
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
};
