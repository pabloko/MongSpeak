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
		short* flBuffer = (short*)data;
		map<WORD, CAudioSession*>::iterator it = pVecSessions.begin();
		while (it != pVecSessions.end()) {
			CAudioSession* sess = it->second;
			short* pBuf = (short*)sess->pBuffer.c_str();
			int sm = 0;
			int len = sess->pBuffer.length() / sizeof(short);
			//len /= wf->nChannels;
			if (len > count) len = count;
			if (len > 0) {
				if (fVol != 0.0f)
					for (int i = 0; i < len * wf->nChannels; i+= wf->nChannels) {
						if (it == pVecSessions.begin()) {
							flBuffer[i] = pBuf[sm];
							if (wf->nChannels > 1) flBuffer[i + 1] = pBuf[sm];
						} else {
							flBuffer[i] = mix_pcm_sample_short(flBuffer[i], pBuf[sm]);
							if (wf->nChannels > 1) flBuffer[i + 1] = flBuffer[i];
						}
						sm++;
					}
				sess->pBuffer.erase(sess->pBuffer.begin(), sess->pBuffer.begin() + (len * sizeof(short)));
			}
			it++;
		}
		if (fVol != 0.0f)
			if (fVol != 1.0f)
				for (int i = 0; i < count * wf->nChannels; i += wf->nChannels) {
					flBuffer[i] = flBuffer[i] * fVol;
					if (wf->nChannels > 1) flBuffer[i + 1] = flBuffer[i];
				}
	}
private:
	map<WORD, CAudioSession*> pVecSessions;
	CAudioDevice* pDeviceOut;
	float fVol;
};
