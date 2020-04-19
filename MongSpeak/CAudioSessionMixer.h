#pragma once
/*CAudioSessionMixer will work with an output CAudioDevice and run in its thread
Provides a way to create/obtain/remove CAudioSession instances, mix it buffers, and output the audio*/

class CAudioSessionMixer : CAudioQueue {
public:
	CAudioSessionMixer() : fVol(1.0f) {}
	~CAudioSessionMixer() {
		ClearSessions();
		if (pDeviceOut != NULL)
			pDeviceOut->SetAudioQueue(NULL);
	}
	void SetVol(float vol) {
		//Set master volume multiplicator for output
		fVol = vol;
	}
	void SetDeviceOut(CAudioDevice* dev) {
		//Connect this instance to a CAudioDevice instance
		//If provided is NULL, were cleaning the house, so remove this instance of previously used CAudioDevice
		if (pDeviceOut != NULL && dev == NULL)
			pDeviceOut->SetAudioQueue(NULL);
		//Else, connect CAudioDevice and notify it we're accepting its data
		pDeviceOut = dev;
		if (dev != NULL)
			dev->SetAudioQueue(this);
	}
	CAudioSession* AddSession(WORD id) {
		//Creates a CAudioSession instance linked to its id
		CAudioSession* ret = new CAudioSession();
		pVecSessions.insert(make_pair(id, ret));
		return ret;
	}
	void RemoveSession(WORD id) {
		//Find a CAudioSession with provided id, and remove it
		map<WORD, CAudioSession*>::const_iterator iter = pVecSessions.find(id);
		if (iter != pVecSessions.end()) {
			delete iter->second;
			pVecSessions.erase(iter);
		}
	}
	CAudioSession* GetSession(WORD id) {
		//Obtain a CAudioSession from a id, for ease of use, calling this will create the session if not exists
		map<WORD, CAudioSession*>::const_iterator iter = pVecSessions.find(id);
		if (iter != pVecSessions.end())
			return iter->second;
		return AddSession(id);
	}
	void ClearSessions() {
		//Remove all CAudioSession instances
		map<WORD, CAudioSession*>::iterator it = pVecSessions.begin();
		while (it != pVecSessions.end()) {
			CAudioSession* sess = it->second;
			delete sess;
			it++;
		}
		pVecSessions.clear();
	}
	//CAudioQueue task called by CAudioDevice
	void DoTask(int count, char* data, WAVEFORMATEX* wf) {
		//Fill buffer with silence
		ZeroMemory(data, count * wf->nBlockAlign);
		short* flBuffer = (short*)data;
		//Iterate over all CAudioSession instances
		map<WORD, CAudioSession*>::iterator it = pVecSessions.begin();
		while (it != pVecSessions.end()) {
			//On every CAudioSession instance we have
			CAudioSession* sess = it->second;
			short* pBuf = (short*)sess->pBuffer.c_str();
			int sm = 0;
			//Calculate if we have enough buffer to mix
			int len = sess->pBuffer.length() / sizeof(short);
			if (len > count) len = count;
			//In case there are samples
			if (len > 0) {
				if (fVol != 0.0f) //Volume is 0, dont process
					//Write sample to audio devices buffer
					for (int i = 0; i < len * wf->nChannels; i+= wf->nChannels) {
						if (it == pVecSessions.begin()) { //First instance, no need to mix
							flBuffer[i] = pBuf[sm];
							if (wf->nChannels > 1) flBuffer[i + 1] = pBuf[sm]; //Todo: providing only 2 ch
						} else { //Rest of instances, mix the buffer
							flBuffer[i] = mix_pcm_sample_short(flBuffer[i], pBuf[sm]);
							if (wf->nChannels > 1) flBuffer[i + 1] = flBuffer[i]; //Todo: providing only 2 ch
						}
						sm++;
					}
				//Erase the used buffer from CAudioSession
				sess->pBuffer.erase(sess->pBuffer.begin(), sess->pBuffer.begin() + (len * sizeof(short)));
			}
			//Go to the next CAudioSession instance
			it++;
		}
		//We have now all sessions mixed, compute output volume
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
