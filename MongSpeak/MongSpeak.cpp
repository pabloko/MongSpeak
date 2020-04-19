// MongSpeak.cpp
#include "stdafx.h"
#include "MongSpeak.h"
#define MAPWIDTH  600
#define MAPHEIGHT 450
using namespace std;
using namespace winreg;
using namespace Gdiplus;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	//Setup GDI+ for clipboard-upload process
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	HRESULT hr = S_OK;
	//Uncomment this to display debug window
	///ConsoleDebugger dbg;	
	//Setup this thread as DPI aware. this will maintain same aspect when using windows scaling
	SetProcessDPIAware();
	//Obtain unique ID associated with this computer
	if (GetCurrentHwProfileA(&hwProfileInfo) != NULL)
		hwProfileInfo.szHwProfileGuid[strlen(&hwProfileInfo.szHwProfileGuid[1])] = '\0';
	else return -666;
	//Setup COM on this thread as we will enumerate audio devices
	hr = CoInitialize(nullptr);
	if (FAILED(hr)) return hr;
	CoUninitializeCleanup release_CoInitialize;
	//Setup OLE ActiveX controls for MSHTML host
	hr = OleInitialize(0);
	if (FAILED(hr)) return hr;
	OleInitializeCleanup release_OleInitialize;
	//Create the network global instance
	g_network = new CNetwork();
	ReleaseDelete release_g_network(g_network);
	//Create preferences global instance
	g_preferences = new CPreferences();
	//Create jsobjet IDispatch that bind to our JS Interface on "mong" global
	g_jsObject = new JSObject();
	ReleaseIUnknown release_g_jsObject(g_jsObject);
	//Bind JS methods and refcount
	BindJSMethods();
	g_jsObject->AddRef();
	//Setup MSHTML control
	WebformDispatchImpl* webformDispatchImpl = new WebformDispatchImpl(g_jsObject);
	ReleaseDelete release_webformDispatchImpl(webformDispatchImpl);
	g_webWindow = new WebWindow(webformDispatchImpl);
	//Setup handler for WM_DROPFILES event
	g_webWindow->SetDropFileHandler(mm_drop_handler);
	ReleaseDelete release_g_webWindow(g_webWindow);
	const int ScreenX = (GetSystemMetrics(SM_CXSCREEN) - MAPWIDTH) / 2;
	const int ScreenY = (GetSystemMetrics(SM_CYSCREEN) - MAPHEIGHT) / 2;
	//Display IE Window
	g_webWindow->Create(hInstance, ScreenX, ScreenY, MAPWIDTH, MAPHEIGHT, FALSE);
	//Setup handler for ctrl+v
	g_webWindow->webForm->SetPasteAcceleratorHandler(mm_getclipboardimage);
	//Display empty page now
	g_webWindow->webForm->Go("about:blank;");
	//Load HTML from resources
	HMODULE hmodule = GetModuleHandle(NULL);
	HRSRC hrsrc = FindResource(hmodule, MAKEINTRESOURCE(IDR_HTML1), RT_RCDATA);
	if (!hrsrc) return FALSE;
	HGLOBAL hglobal = LoadResource(hmodule, hrsrc);
	void *data = LockResource(hglobal);
	//Put HTML to MSHTML Document, todo: check if CP_ACP in ConvertStringToBSTR produces unicode bug, may switch to CP_UTF8
	g_webWindow->webForm->GoMem(_com_util::ConvertStringToBSTR((char*)data));
	MSG msg;
	//Get default input/output audio devices, todo: probably will remove this and handle from UI
	hr = get_default_device(&g_dev_in, EDataFlow::eCapture);
	if (FAILED(hr)) return hr;
	hr = get_default_device(&g_dev_out, EDataFlow::eRender);
	if (FAILED(hr)) return hr;
	//Create instances to CAudioDevice for input and output
	g_audio_in = new CAudioDevice(g_dev_in, EDataFlow::eCapture);
	ReleaseDelete release_g_audio_in(g_audio_in);
	g_audio_out = new CAudioDevice(g_dev_out, EDataFlow::eRender);
	ReleaseDelete release_g_audio_out(g_audio_out);
	//Create global instance for CAudioSessionMixer and CAudioStream
	g_mix = new CAudioSessionMixer();
	ReleaseDelete release_g_mix(g_mix);
	g_stream = new CAudioStream();
	ReleaseDelete release_g_stream(g_stream);
	//Link devices with its handlers
	g_mix->SetDeviceOut(g_audio_out);
	g_stream->SetDeviceIn(g_audio_in);
	//Trick: As were processing js on event loop, we need events so its frequency of 
	//call is constant and it not stalls, this will produce a flood of useless WM_TIMER
	SetTimer(g_webWindow->hWndWebWindow, 1, 10, 0);
	while (GetMessage(&msg, nullptr, 0, 0)) {
		//Execute all javascript strings queued to buffer
		while (g_webWindow->webForm != nullptr && g_jsStack.size() > 0) {
			g_webWindow->webForm->RunJSFunctionW(g_jsStack.at(0).c_str());
			g_jsStack.erase(g_jsStack.begin(), g_jsStack.begin() + 1);
		}
		//Execute all queued events to javascript
		g_webWindow->webForm->DequeueCallToEvent();
		//Usual window stuff
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	//Liberate links and shutdown
	g_mix->SetDeviceOut(NULL);
	g_stream->SetDeviceIn(NULL);
	Sleep(20);
	GdiplusShutdown(gdiplusToken);
	return hr;
}
