#pragma once
/*CAudioSession is entirely controlled by CAudioSessionMixer
Provides an interface to decode Opus raw packets and append audio samples to a buffer*/

using namespace std;
class CAudioSession {
public:
	CAudioSession() : fVol(1.0f) {
		//Setup Opus decoder
		pOpus = opus_decoder_create(DEFAULT_SAMPLERATE, OPUS_CHANNELS, NULL);
		opus_decoder_ctl(pOpus, OPUS_SET_BITRATE(DEFAULT_OPUS_BITRATE));
		opus_decoder_ctl(pOpus, OPUS_SET_APPLICATION(OPUS_APPLICATION_VOIP));
		opus_decoder_ctl(pOpus, OPUS_SET_PREDICTION_DISABLED(TRUE));
	}
	~CAudioSession() {
		opus_decoder_destroy(pOpus);
	}
	void SetVol(float vol) {
		//Set sample multiplier
		fVol = vol;
	}
	void Decode(const vector<uint8_t>* pv) {
		//Decode an Opus packet (notice were removing initial 3 bytes [rpcid,sessid] in our protocol)
		int samples = opus_decode(pOpus, (const unsigned char*)pv->data() + 3, pv->size() - 3, flTempBuf, 2048, 0);
		//Compute the volume
		if (fVol != 1.0f && samples > 0) 
			for (int i = 0; i < samples * OPUS_CHANNELS; i++)
				if (fVol == 0.0f)
					flTempBuf[i] = 0.0f;
				else
					flTempBuf[i] = flTempBuf[i] * fVol;
		//Write samples to buffer
		if(samples > 0)
			pBuffer.append(&((char*)flTempBuf)[0], &((char*)flTempBuf)[samples * OPUS_CHANNELS * sizeof(short)]);
	}
	string pBuffer;
private:
	OpusDecoder * pOpus;
	float fVol;
	short flTempBuf[2048 * 2];
};