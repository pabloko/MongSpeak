#pragma once

extern CNetwork* g_network;

class CAudioStream : CAudioQueue {
public:
	CAudioStream() {
		pOpus = opus_encoder_create(DEFAULT_SAMPLERATE, OPUS_CHANNELS, OPUS_APPLICATION_VOIP, NULL);
		opus_encoder_ctl(pOpus, OPUS_SET_BITRATE(DEFAULT_OPUS_BITRATE));
		opus_encoder_ctl(pOpus, OPUS_SET_PREDICTION_DISABLED(TRUE));
	}
	~CAudioStream() {
		opus_encoder_destroy(pOpus);
		if (pDeviceIn != NULL)
			pDeviceIn->SetAudioQueue(NULL);
	}
	void SetDeviceIn(CAudioDevice* dev) {
		if (pDeviceIn != NULL && dev == NULL)
			pDeviceIn->SetAudioQueue(NULL);
		pDeviceIn = dev;
		if (dev != NULL)
			dev->SetAudioQueue(this);
	}
	void DoTask(int len, char* data, WAVEFORMATEX* wf) {
		if (!GetAsyncKeyState(VK_F3)) 
			return;
		int samples = opus_encode_float(pOpus, (const float*)data, len, szOpusBuffer, sizeof(szOpusBuffer));
		if (samples < 1) return;
		string pv(&szOpusBuffer[0], &szOpusBuffer[samples]);
		///pVecBuffer.push_back(pv);
		/*if (g_mix != NULL)
			g_mix->GetSession(0)->Decode(&pv); //todo:remove*/
		if (g_network) 
			g_network->Send(RPCID::OPUS_DATA, &pv);
	};
	//vector<string> pVecBuffer;
private:
	OpusEncoder* pOpus;
	CAudioDevice* pDeviceIn;
	unsigned char szOpusBuffer[2048];
};