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
	RegisterExceptionHandler();
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	HRESULT hr = S_OK;
	///ConsoleDebugger dbg;	
	SetProcessDPIAware();
	GenerateHWID();
	hr = CoInitialize(nullptr);
	if (FAILED(hr)) return hr;
	CoUninitializeCleanup release_CoInitialize;
	hr = OleInitialize(0);
	if (FAILED(hr)) return hr;
	OleInitializeCleanup release_OleInitialize;
	g_network = new CNetwork();
	ReleaseDelete release_g_network(g_network);
	g_preferences = new CPreferences();
	const int ScreenX = (GetSystemMetrics(SM_CXSCREEN) - MAPWIDTH) / 2;
	const int ScreenY = (GetSystemMetrics(SM_CYSCREEN) - MAPHEIGHT) / 2;
	
#ifdef USING_MSHTML
	g_jsObject = new JSObject();
	ReleaseIUnknown release_g_jsObject(g_jsObject);
	g_jsObject->AddRef();
	WebformDispatchImpl* webformDispatchImpl = new WebformDispatchImpl(g_jsObject);
	ReleaseDelete release_webformDispatchImpl(webformDispatchImpl);
	g_webWindow = new WebWindow(webformDispatchImpl);
	ReleaseDelete release_g_webWindow(g_webWindow);
	g_webWindow->Create(hInstance, ScreenX, ScreenY, MAPWIDTH, MAPHEIGHT, FALSE);
	g_webWindow->webForm->Go("about:blank;");
#endif

#ifdef USING_MINIBLINK
	g_webWindow = new WebWindow(ScreenX, ScreenY, MAPWIDTH, MAPHEIGHT);
	g_jsObject = g_webWindow;
	ReleaseDelete release_g_webWindow(g_webWindow);
#endif

	g_webWindow->webForm->SetPasteAcceleratorHandler(mm_getclipboardimage);
	g_webWindow->SetDropFileHandler(mm_drop_handler);
	g_webWindow->SetMessageFilter(mm_messageFilter);

	HMODULE hmodule = GetModuleHandle(NULL);
	HRSRC hrsrc = FindResource(hmodule, MAKEINTRESOURCE(IDR_HTML1), RT_RCDATA);
	if (!hrsrc) return FALSE;
	HGLOBAL hglobal = LoadResource(hmodule, hrsrc);
	void *data = LockResource(hglobal);
	g_webWindow->webForm->GoMem(_com_util::ConvertStringToBSTR((char*)data));
	BindJSMethods();
	MSG msg;
	hr = get_default_device(&g_dev_in, EDataFlow::eCapture);
	if (FAILED(hr)) return hr;
	//ReleaseIUnknown g_dev_in_release(g_dev_in);
	hr = get_default_device(&g_dev_out, EDataFlow::eRender);
	if (FAILED(hr)) return hr;
	//ReleaseIUnknown g_dev_out_release(g_dev_out);

	g_mix = new CAudioSessionMixer();
	ReleaseDelete release_g_mix(g_mix);
	g_stream = new CAudioStream();
	ReleaseDelete release_g_stream(g_stream);

	g_audio_in = new CAudioDevice(g_dev_in, EDataFlow::eCapture);
	//ReleaseDelete release_g_audio_in(g_audio_in);
	g_audio_out = new CAudioDevice(g_dev_out, EDataFlow::eRender);
	//ReleaseDelete release_g_audio_out(g_audio_out);
	
	g_mix->SetDeviceOut(g_audio_out);
	g_stream->SetDeviceIn(g_audio_in);

	UINT taskbarButtonCreatedMessageId = RegisterWindowMessage(L"TaskbarButtonCreated");
	ChangeWindowMessageFilterEx(g_webWindow->hWndWebWindow, taskbarButtonCreatedMessageId, MSGFLT_ALLOW, NULL);
	g_Taskbar = NULL;
	ReleaseIUnknown g_Taskbar_release(g_Taskbar);
	SetTimer(g_webWindow->hWndWebWindow, 1, 10, 0);

	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == taskbarButtonCreatedMessageId && g_Taskbar == NULL)
			CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, (void**)&g_Taskbar);
		g_webWindow->webForm->DequeueCallToEvent();
		if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
			CancelUploadIfAny();
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	g_mix->SetDeviceOut(NULL);
	g_stream->SetDeviceIn(NULL);
	Sleep(20);
	GdiplusShutdown(gdiplusToken);
	g_dev_in->Release();
	g_dev_out->Release();
	delete g_audio_in;
	delete g_audio_out;

	return hr;
}
