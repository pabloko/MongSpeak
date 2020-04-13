#pragma once

extern CNetwork* g_network;
extern vector<wstring> g_jsStack;

class CAudioStream : CAudioQueue {
public:
	CAudioStream() : fVol(1.0f), iInputMethod(VK_F3), bIsInput(FALSE), bSendVu(FALSE), fVuSum(0.0f), lVuCount(0) {
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
		if (fVol == 0.0f) {
			return CompareInput(FALSE);
		}
		if (!bSendVu && iInputMethod > 0) 
			if (!GetAsyncKeyState(iInputMethod)) 
				return CompareInput(FALSE);
		float* pData = (float*)data;
		if (fVol != 1.0f) 
			for (int i = 0; i < len * wf->nChannels; i++)
				pData[i] = pData[i] * fVol;
		if (iInputMethod < 0 || bSendVu) {
			float fTol = 0.0f;
			lVuCount += len;
			for (int i = 0; i < len * wf->nChannels; i = i + wf->nChannels)
				fVuSum += pow(pData[i + 0], 2);
			if (lVuCount > (wf->nSamplesPerSec / 30)) {
				float fTol = sqrt(fVuSum / lVuCount);
				if (bSendVu)
					g_jsStack.push_back(wstring_format(L"onEvent(%d, %d, %4.0f);", RPCID::UI_COMMAND, -3, fTol * 100.0f));
				fVuSum = 0.0f; lVuCount = 0;
			}
			if (iInputMethod < 0) {
				float tolerance = (iInputMethod * -1) / 100.0f;
				if (fTol < tolerance) 
					return CompareInput(FALSE);
			}
		}
		if (bSendVu && iInputMethod > 0) {
			if (!GetAsyncKeyState(iInputMethod)) 
				return CompareInput(FALSE);
		}
		CompareInput(TRUE);
		int samples = opus_encode_float(pOpus, (const float*)data, len, szOpusBuffer, sizeof(szOpusBuffer));
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
	DOUBLE fVuSum;
	LONG lVuCount;
};