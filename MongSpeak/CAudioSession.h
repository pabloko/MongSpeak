#pragma once
using namespace std;
class CAudioSession {
public:
	CAudioSession() {
		pOpus = opus_decoder_create(DEFAULT_SAMPLERATE, OPUS_CHANNELS, NULL);
		opus_decoder_ctl(pOpus, OPUS_SET_BITRATE(DEFAULT_OPUS_BITRATE));
		opus_decoder_ctl(pOpus, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));
		opus_decoder_ctl(pOpus, OPUS_SET_PREDICTION_DISABLED(TRUE));
	}
	~CAudioSession() {
		opus_decoder_destroy(pOpus);
	}
	void Decode(const vector<uint8_t>* pv) {
		int samples = opus_decode_float(pOpus, (const unsigned char*)pv->data() + 3, pv->size() - 3, flTempBuf, 2048, 0);
		pBuffer.append(&((char*)flTempBuf)[0], &((char*)flTempBuf)[samples * OPUS_CHANNELS * sizeof(float)]);
	}
	string pBuffer;
	float flTempBuf[2048 * 2];
private:
	OpusDecoder * pOpus;
};