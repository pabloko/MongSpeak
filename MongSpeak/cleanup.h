#pragma once

class PropVariantClearCleanup {
public:
	PropVariantClearCleanup(PROPVARIANT *p) : m_p(p) {}
	~PropVariantClearCleanup() {
		PropVariantClear(m_p);
	}
private:
	PROPVARIANT * m_p;
};

class BoolToggleCleanup {
public:
	BoolToggleCleanup(BOOL *p) : m_p(p) {}
	~BoolToggleCleanup() {
		*m_p=FALSE;
	}
private:
	BOOL* m_p;
};

class ReleaseSpeexResampler {
public:
	ReleaseSpeexResampler(SpeexResamplerState *p) : m_p(p) {}
	~ReleaseSpeexResampler() {
		if (m_p != NULL)
			speex_resampler_destroy(m_p);
	}
private:
	SpeexResamplerState * m_p;
};

class ReleaseIUnknown {
public:
	ReleaseIUnknown(IUnknown *p) : m_p(p) {}
	~ReleaseIUnknown() {
		if (m_p != NULL)
		m_p->Release();
	}
private:
	IUnknown * m_p;
};

class ConsoleDebugger {
public:
	ConsoleDebugger() {
		AllocConsole();
		freopen("CONOUT$", "w", stdout);
		freopen("CONIN$", "r", stdin);
	}
	~ConsoleDebugger() {
		FreeConsole();
	}
};

class ReleaseDelete {
public:
	ReleaseDelete(void *p) : m_p(p) {}
	~ReleaseDelete() {
		if(m_p != NULL)
			delete m_p;
	}
private:
	void * m_p;
};

class ReleaseAndStopIUnknown {
public:
	ReleaseAndStopIUnknown(IAudioClient *p) : m_p(p) {}
	~ReleaseAndStopIUnknown() {
		m_p->Stop();
		m_p->Release();
	}
private:
	IAudioClient * m_p;
};

class CoUninitializeCleanup {
public:
	~CoUninitializeCleanup() {
		CoUninitialize();
	}
};

class OleInitializeCleanup {
public:
	~OleInitializeCleanup() {
		OleUninitialize();
	}
};

class AvRevertMmThreadCharacteristicsRelease {
public:
	AvRevertMmThreadCharacteristicsRelease(HANDLE hTask) : m_hTask(hTask) {}
	~AvRevertMmThreadCharacteristicsRelease() {
		if (m_hTask != NULL)
			AvRevertMmThreadCharacteristics(m_hTask);
	}
private:
	HANDLE m_hTask;
};

class CoTaskMemFreeRelease {
public:
	CoTaskMemFreeRelease(PVOID p) : m_p(p) {}
	~CoTaskMemFreeRelease() {
		CoTaskMemFree(m_p);
	}
private:
	PVOID m_p;
};

class HandleRelease {
public:
	HandleRelease(HANDLE p) : m_p(p) {}
	~HandleRelease() {
		CloseHandle(m_p);
	}
private:
	HANDLE m_p;
};

class ReleaseClipboard {
public:
	ReleaseClipboard() {}
	~ReleaseClipboard() {
		CloseClipboard();
	}
};
