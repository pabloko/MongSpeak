#pragma once

extern CNetwork* g_network;

class CAudioStream : CAudioQueue {
public:
	CAudioStream() {
		pOpus = opus_encoder_create(DEFAULT_SAMPLERATE, OPUS_CHANNELS, OPUS_APPLICATION_VOIP, NULL);
		opus_encoder_ctl(pOpus, OPUS_SET_BITRATE(DEFAULT_OPUS_BITRATE));
		opus_encoder_ctl(pOpus, OPUS_SET_PREDICTION_DISABLED(TRUE));
		fVol = 1.0f;
	}
	~CAudioStream() {
		opus_encoder_destroy(pOpus);
		if (pDeviceIn != NULL)
			pDeviceIn->SetAudioQueue(NULL);
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
	void DoTask(int len, char* data, WAVEFORMATEX* wf) {
		if (fVol == 0.0f) return;
		if (!GetAsyncKeyState(VK_F3)) 
			return;

		float* pData = (float*)data;

		if (fVol != 1.0f) 
			for (int i = 0; i < len * OPUS_CHANNELS; i++)
				pData[i] = pData[0] * fVol;
		
		int samples = opus_encode_float(pOpus, (const float*)data, len, szOpusBuffer, sizeof(szOpusBuffer));
		if (samples < 1) return;
		if (g_network) 
			g_network->Send(RPCID::OPUS_DATA, (char*)szOpusBuffer, samples);
	};
	//vector<string> pVecBuffer;
private:
	OpusEncoder* pOpus;
	CAudioDevice* pDeviceIn;
	unsigned char szOpusBuffer[2048];
	float fVol;
};