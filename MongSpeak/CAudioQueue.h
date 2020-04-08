#pragma once

class CAudioQueue {
public:
	virtual void DoTask(int len, char* data, WAVEFORMATEX* wf) {};
};