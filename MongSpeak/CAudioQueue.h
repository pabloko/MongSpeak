#pragma once
/*CAudioQueue must be parent of any class reading/writing audio to a CAudioDevice
DoTask is called by CAudioDevice's thread regularly providing its buffer*/

class CAudioQueue {
public:
	virtual void DoTask(int len, char* data, WAVEFORMATEX* wf) {};
};