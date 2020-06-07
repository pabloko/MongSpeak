#pragma once
using namespace std;
class CAudioSession {
public:
	CAudioSession() : fVol(1.0f) {
		pOpus = opus_decoder_create(DEFAULT_SAMPLERATE, OPUS_CHANNELS, NULL);
		opus_decoder_ctl(pOpus, OPUS_SET_BITRATE(DEFAULT_OPUS_BITRATE));
		opus_decoder_ctl(pOpus, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));
		opus_decoder_ctl(pOpus, OPUS_SET_PREDICTION_DISABLED(TRUE));
	}
	~CAudioSession() {
		opus_decoder_destroy(pOpus);
	}
	void SetVol(float vol) {
		fVol = vol;
	}
	void Decode(const vector<uint8_t>* pv) {
		if (fVol == 0.0f) return;
		int samples = opus_decode(pOpus, (const unsigned char*)pv->data() + 3, pv->size() - 3, flTempBuf, sizeof(flTempBuf) / sizeof(short), 0);
		if (fVol != 1.0f && samples > 0) 
			for (int i = 0; i < samples * OPUS_CHANNELS; i++)
				flTempBuf[i] = flTempBuf[i] * fVol;
		if(samples > 0)
			pBuffer.append(&((char*)flTempBuf)[0], &((char*)flTempBuf)[samples * OPUS_CHANNELS * sizeof(short)]);
	}
	string pBuffer;
private:
	OpusDecoder * pOpus;
	float fVol;
	short flTempBuf[2048];
};