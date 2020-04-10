#pragma once

extern CNetwork* g_network;

class CAudioStream : CAudioQueue {
public:
	CAudioStream() {
		pOpus = opus_encoder_create(DEFAULT_SAMPLERATE, OPUS_CHANNELS, OPUS_APPLICATION_VOIP, NULL);
		opus_encoder_ctl(pOpus, OPUS_SET_BITRATE(DEFAULT_OPUS_BITRATE));
		opus_encoder_ctl(pOpus, OPUS_SET_PREDICTION_DISABLED(TRUE));
		fVol = 1.0f;
		iInputMethod = VK_F3;
		bIsInput = FALSE;
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
	void DoTask(int len, char* data, WAVEFORMATEX* wf) {
		if (fVol == 0.0f) {
			CompareInput(FALSE);
			return;
		}
		if (iInputMethod > 0) {
			if (!GetAsyncKeyState(iInputMethod)) {
				CompareInput(FALSE);
				return;
			}
		}
		float* pData = (float*)data;
		if (iInputMethod < 0) {
			float vu[2];
			vu_meter_float(pData, vu, len);
			float tolerance = (iInputMethod * -1) / 100.0f;
			if (vu[0] < tolerance) {
				CompareInput(FALSE);
				return;
			}
		}
		CompareInput(TRUE);
		if (fVol != 1.0f) 
			for (int i = 0; i < len * OPUS_CHANNELS; i++)
				pData[i] = pData[i] * fVol;
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
};