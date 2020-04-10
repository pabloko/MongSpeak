#pragma once

class CAudioSessionMixer : CAudioQueue {
public:
	CAudioSessionMixer() {
		fVol = 1.0f;
	}
	~CAudioSessionMixer() {
		ClearSessions();
		if (pDeviceOut != NULL)
			pDeviceOut->SetAudioQueue(NULL);
	}
	void SetVol(float vol) {
		fVol = vol;
	}
	void SetDeviceOut(CAudioDevice* dev) {
		if (pDeviceOut != NULL && dev == NULL)
			pDeviceOut->SetAudioQueue(NULL);
		pDeviceOut = dev;
		if (dev != NULL)
			dev->SetAudioQueue(this);
	}
	CAudioSession* AddSession(WORD id) {
		CAudioSession* ret = new CAudioSession();
		pVecSessions.insert(make_pair(id, ret));
		return ret;
	}
	void RemoveSession(WORD id) {
		map<WORD, CAudioSession*>::const_iterator iter = pVecSessions.find(id);
		if (iter != pVecSessions.end()) {
			delete iter->second;
			pVecSessions.erase(iter);
		}
	}
	CAudioSession* GetSession(WORD id) {
		map<WORD, CAudioSession*>::const_iterator iter = pVecSessions.find(id);
		if (iter != pVecSessions.end())
			return iter->second;
		return AddSession(id);
	}
	void ClearSessions() {
		map<WORD, CAudioSession*>::iterator it = pVecSessions.begin();
		while (it != pVecSessions.end()) {
			CAudioSession* sess = it->second;
			delete sess;
			it++;
		}
		pVecSessions.clear();
	}
	void DoTask(int count, char* data, WAVEFORMATEX* wf) {
		ZeroMemory(data, count * wf->nBlockAlign);
		if (fVol == 0.0f) return;
		float* flBuffer = (float*)data;
		map<WORD, CAudioSession*>::iterator it = pVecSessions.begin();
		while (it != pVecSessions.end()) {
			CAudioSession* sess = it->second;
			float* pBuf = (float*)sess->pBuffer.c_str();
			int len = sess->pBuffer.length() / wf->nBlockAlign;
			if (len > count) len = count;
			if (len > 0) {
				for (int i = 0; i < len * wf->nChannels; i++) {
					if (it == pVecSessions.begin())
						flBuffer[i] = pBuf[i];
					else
						flBuffer[i] = mix_pcm_sample_float(flBuffer[i], pBuf[i]);
				}
				sess->pBuffer.erase(sess->pBuffer.begin(), sess->pBuffer.begin() + (len * wf->nBlockAlign));
			}
			it++;
			float* pData = (float*)data;
			if (fVol != 1.0f) 
				for (int i = 0; i < count * OPUS_CHANNELS; i++)
					pData[i] = pData[0] * fVol;
		}
	}
private:
	map<WORD, CAudioSession*> pVecSessions;
	CAudioDevice* pDeviceOut;
	float fVol;
};
