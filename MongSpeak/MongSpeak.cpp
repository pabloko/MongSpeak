// MongSpeak.cpp: define el punto de entrada de la aplicación.
//

#include "stdafx.h"
#include "MongSpeak.h"

using namespace std;
using namespace winreg;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
#define MAPWIDTH  1000
#define MAPHEIGHT 600
	ConsoleDebugger dbg;	
	if (GetCurrentHwProfileA(&hwProfileInfo) != NULL) {
		hwProfileInfo.szHwProfileGuid[strlen(&hwProfileInfo.szHwProfileGuid[1])] = '\0';
		//printf("Hardware GUID: %s\n", &hwProfileInfo.szHwProfileGuid[1]);
	}
	else return -666;

	HRESULT hr = S_OK;
	hr = CoInitialize(nullptr);
	if (FAILED(hr)) return hr;
	CoUninitializeCleanup release_CoInitialize;
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	hr = OleInitialize(0);
	if (FAILED(hr)) return hr;
	OleInitializeCleanup release_OleInitialize;

	RegKey key{ HKEY_CURRENT_USER, L"SOFTWARE\\MongSpeak" };
	g_reg = &key;

	/*vector<pair<wstring, DWORD>> values = key.EnumValues();
	wprintf(L"Values:\n");
	for (const pair<wstring,DWORD>& v : values)
		wprintf(L"  [%s] %s\n", v.first.c_str(), key.GetStringValue(v.first).c_str());*/

	g_jsObject = new JSObject();
	ReleaseIUnknown release_g_jsObject(g_jsObject);
	g_jsObject->AddMethod(L"alert", mm_alert);
	g_jsObject->AddMethod(L"log", mm_log);
	g_jsObject->AddMethod(L"send_message", mm_send_message);
	g_jsObject->AddMethod(L"list_devices", mm_list_devices);
	g_jsObject->AddMethod(L"connect", mm_connect);
	g_jsObject->AddMethod(L"disconnect", mm_disconnect);
	g_jsObject->AddMethod(L"change_room", mm_change_room);
	g_jsObject->AddMethod(L"set_device", mm_set_device);
	g_jsObject->AddMethod(L"set_preferences", mm_set_preferences);
	g_jsObject->AddMethod(L"get_preferences", mm_get_preferences);
	g_jsObject->AddRef();
	WebformDispatchImpl* webformDispatchImpl = new WebformDispatchImpl(g_jsObject);
	ReleaseDelete release_webformDispatchImpl(webformDispatchImpl);
	g_webWindow = new WebWindow(webformDispatchImpl);
	ReleaseDelete release_g_webWindow(g_webWindow);

	const int ScreenX = (GetSystemMetrics(SM_CXSCREEN) - MAPWIDTH) / 2;
	const int ScreenY = (GetSystemMetrics(SM_CYSCREEN) - MAPHEIGHT) / 2;

	g_webWindow->Create(hInstance, ScreenX, ScreenY, MAPWIDTH, MAPHEIGHT, FALSE);

	char path[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, (LPSTR)path);
	sprintf(path, "%s\\%s", path, "test.html");
	g_webWindow->webForm->Go((char*)path);
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

	//83.45.3.15
	//localhost
	//mong.connect("ws://localhost:8083");
	//g_network->ConnectServer("ws://localhost:8083");
	SetTimer(g_webWindow->hWndWebWindow, 1, 1, 0);
	while (GetMessage(&msg, nullptr, 0, 0)) {
		while (g_jsStack.size() > 0) {
			//gWS->sendBinary((const std::string&)g_jsStack.at(0));
			wprintf(L"%s\n", g_jsStack.at(0).c_str());
			g_webWindow->webForm->RunJSFunctionW(g_jsStack.at(0).c_str());
			g_jsStack.erase(g_jsStack.begin(), g_jsStack.begin() + 1);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	g_mix->SetDeviceOut(NULL);
	g_stream->SetDeviceIn(NULL);
	key.Close();
	Sleep(20);
	return hr;
}