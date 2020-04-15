// MongSpeak.cpp: define el punto de entrada de la aplicación.
//

#include "stdafx.h"
#include "MongSpeak.h"
#define MAPWIDTH  600
#define MAPHEIGHT 450
using namespace std;
using namespace winreg;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	HRESULT hr = S_OK;
	///ConsoleDebugger dbg;	
	SetProcessDPIAware();
	if (GetCurrentHwProfileA(&hwProfileInfo) != NULL)
		hwProfileInfo.szHwProfileGuid[strlen(&hwProfileInfo.szHwProfileGuid[1])] = '\0';
	else return -666;
	hr = CoInitialize(nullptr);
	if (FAILED(hr)) return hr;
	CoUninitializeCleanup release_CoInitialize;
	hr = OleInitialize(0);
	if (FAILED(hr)) return hr;
	OleInitializeCleanup release_OleInitialize;
	g_preferences = new CPreferences();
	g_jsObject = new JSObject();
	ReleaseIUnknown release_g_jsObject(g_jsObject);
	BindJSMethods();
	g_jsObject->AddRef();
	WebformDispatchImpl* webformDispatchImpl = new WebformDispatchImpl(g_jsObject);
	ReleaseDelete release_webformDispatchImpl(webformDispatchImpl);
	g_webWindow = new WebWindow(webformDispatchImpl);
	g_webWindow->SetDropFileHandler(mm_drop_handler);
	ReleaseDelete release_g_webWindow(g_webWindow);
	const int ScreenX = (GetSystemMetrics(SM_CXSCREEN) - MAPWIDTH) / 2;
	const int ScreenY = (GetSystemMetrics(SM_CYSCREEN) - MAPHEIGHT) / 2;
	g_webWindow->Create(hInstance, ScreenX, ScreenY, MAPWIDTH, MAPHEIGHT, FALSE);
	g_webWindow->webForm->Go("about:blank;");
	HMODULE hmodule = GetModuleHandle(NULL);
	HRSRC hrsrc = FindResource(hmodule, MAKEINTRESOURCE(IDR_HTML1), RT_RCDATA);
	if (!hrsrc) return FALSE;
	HGLOBAL hglobal = LoadResource(hmodule, hrsrc);
	void *data = LockResource(hglobal);
	g_webWindow->webForm->GoMem(_com_util::ConvertStringToBSTR((char*)data));
	MSG msg;
	g_network = new CNetwork();
	ReleaseDelete release_g_network(g_network);
	hr = get_default_device(&g_dev_in, EDataFlow::eCapture);
	if (FAILED(hr)) return hr;
	hr = get_default_device(&g_dev_out, EDataFlow::eRender);
	if (FAILED(hr)) return hr;
	g_audio_in = new CAudioDevice(g_dev_in, EDataFlow::eCapture);
	ReleaseDelete release_g_audio_in(g_audio_in);
	g_audio_out = new CAudioDevice(g_dev_out, EDataFlow::eRender);
	ReleaseDelete release_g_audio_out(g_audio_out);
	g_mix = new CAudioSessionMixer();
	ReleaseDelete release_g_mix(g_mix);
	g_stream = new CAudioStream();
	ReleaseDelete release_g_stream(g_stream);
	g_mix->SetDeviceOut(g_audio_out);
	g_stream->SetDeviceIn(g_audio_in);
	SetTimer(g_webWindow->hWndWebWindow, 1, 10, 0);
	while (GetMessage(&msg, nullptr, 0, 0)) {
			while (g_webWindow->webForm != nullptr && g_jsStack.size() > 0) {
				g_webWindow->webForm->RunJSFunctionW(g_jsStack.at(0).c_str());
				g_jsStack.erase(g_jsStack.begin(), g_jsStack.begin() + 1);
			}
			g_webWindow->webForm->DequeueCallToEvent();
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	g_mix->SetDeviceOut(NULL);
	g_stream->SetDeviceIn(NULL);
	Sleep(20);
	return hr;
}
