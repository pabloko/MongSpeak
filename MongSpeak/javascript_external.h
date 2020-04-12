#pragma once
#include <sapi.h>
extern CNetwork* g_network;

void mm_log(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs > 0) {
		if (pDispParams->rgvarg[0].vt == VT_BSTR) {
			wprintf(L"[JS] %s\n", pDispParams->rgvarg[0].bstrVal);
		}
	}
}

void mm_list_devices(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1) {
		if (pDispParams->rgvarg[0].vt == VT_I4) {
			wstring res;
			HRESULT hr = list_devices((EDataFlow)(pDispParams->rgvarg[0].intVal), &res);
			pVarResult->vt = VT_BSTR;
			
			pVarResult->bstrVal = SysAllocString(res.c_str());
		}
	}
}

void mm_connect(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs > 0) {
		//if type string
		if (pDispParams->rgvarg[0].vt == VT_BSTR) {
			g_network->ConnectServer(_com_util::ConvertBSTRToString(pDispParams->rgvarg[0].bstrVal));
		}
	}
}

void mm_disconnect(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	g_network->Disconnect();
}

void mm_change_room(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs > 0) {
		if (pDispParams->rgvarg[0].vt == VT_I4) {
			vector<uint8_t> pv;
			rpc_write_short(&pv, pDispParams->rgvarg[0].intVal);
			g_network->Send(RPCID::CHANGE_ROOM, &pv);
		}
	}
}

void mm_send_message(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs > 0) {
		if (pDispParams->rgvarg[0].vt == VT_BSTR) {
			g_network->Send(RPCID::USER_CHAT, (char*)pDispParams->rgvarg[0].bstrVal, wcslen(pDispParams->rgvarg[0].bstrVal) * 2);
		}
	}
}

void mm_set_device(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 2) {
		HRESULT hr = S_OK;
		if (pDispParams->rgvarg[0].vt == VT_I4 && pDispParams->rgvarg[1].vt == VT_I4) {
			EDataFlow flow = (EDataFlow)pDispParams->rgvarg[1].intVal;
			UINT32 deviceIndex = pDispParams->rgvarg[0].intVal;
			//wprintf(L"change audio device %d->%d\n", flow, deviceIndex);
			if (flow == EDataFlow::eCapture) {
				g_stream->SetDeviceIn(NULL);
				delete g_audio_in;
				g_dev_in->Release();
				if (deviceIndex == 0)
					hr = get_default_device(&g_dev_in, flow);
				else
					hr = get_specific_device(deviceIndex - 1, &g_dev_in, flow);
				if (FAILED(hr))
					exit(1); //TODO
				g_audio_in = new CAudioDevice(g_dev_in, flow);
				g_stream->SetDeviceIn(g_audio_in);
			}
			if (flow == EDataFlow::eRender) {
				g_mix->SetDeviceOut(NULL);
				delete g_audio_out;
				g_dev_out->Release();
				if (deviceIndex == 0)
					hr = get_default_device(&g_dev_out, flow);
				else
					hr = get_specific_device(deviceIndex - 1, &g_dev_out, flow);
				if (FAILED(hr))
					exit(1); //TODO
				g_audio_out = new CAudioDevice(g_dev_out, flow);
				g_mix->SetDeviceOut(g_audio_out);
			}
		}
	}
}

void mm_set_preferences(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 2 && pDispParams->rgvarg[0].vt == VT_BSTR && pDispParams->rgvarg[1].vt == VT_BSTR) {
		g_preferences->SetPreferences(pDispParams->rgvarg[1].bstrVal, pDispParams->rgvarg[0].bstrVal);
	}
}

void mm_get_preferences(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_BSTR) {
		wstring data = g_preferences->GetPreferences(pDispParams->rgvarg[0].bstrVal);
		if (data.length() > 0) {
			pVarResult->vt = VT_BSTR;
			pVarResult->bstrVal = SysAllocString(data.c_str());
		} else pVarResult->vt = VT_NULL;
	}
}

void mm_set_vol(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 2 && pDispParams->rgvarg[1].vt == VT_I4 && pDispParams->rgvarg[0].vt == VT_I4) {
		short client = pDispParams->rgvarg[1].intVal;
		float vol = pDispParams->rgvarg[0].intVal / 100.0f;
		//wprintf(L"new vol on %d %f\n", client, vol);
		switch (client) {
		case -1: {
			g_mix->SetVol(vol);
		} break;
		case -2: {
			g_stream->SetVol(vol);
		} break;
		default: {
			g_mix->GetSession(client)->SetVol(vol);
		} break;
		}
	}
}

void mm_set_inputmethod(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_I4) {
		g_stream->SetInputMethod(pDispParams->rgvarg[0].intVal);
	}
}

const wchar_t g_szClassName[] = L"KEYEXTUI";
int vk = NULL;
int vk2 = NULL;
HWND staticwnd;
char windowText[355];

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE:
		
		vk = -666;
		break;
	case WM_DESTROY:
		
		break;
	case WM_CTLCOLORSTATIC:
	{
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(10, 10, 10));
		SetBkColor(hdcStatic, RGB(255, 255, 255));
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}
	break;
	case WM_KEYDOWN:
	{
		char keyext[100];
		GetKeyNameTextA(lParam, keyext, sizeof(keyext));
		sprintf(windowText, "Found key: %d [%s]", wParam, keyext);
		SetWindowTextA(staticwnd, windowText);
		Sleep(1000);
		vk = lParam;
		vk2 = wParam;
	}
	break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI FakeWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASSEX wc;
	MSG Msg;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = NULL; // LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = NULL; // LoadIcon(NULL, IDI_APPLICATION);
	RegisterClassEx(&wc);
	HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, g_szClassName, L"Capturing a key...", (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU ), CW_USEDEFAULT, CW_USEDEFAULT, 200, 80, NULL, NULL, hInstance, NULL);
	if (hwnd == NULL) return 0;
	sprintf(windowText, "Press any key...");
	HWND statichwnd = CreateWindow(L"static", L"", (WS_CHILD | WS_VISIBLE), 10, 10, 190, 55, hwnd, NULL, hInstance, NULL);
	HFONT hFont = CreateFont(15, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
	SendMessage(statichwnd, WM_SETFONT, WPARAM(hFont), TRUE);
	ShowWindow(hwnd, nCmdShow);
	SetWindowTextA(statichwnd, windowText);
	staticwnd = statichwnd;
	ShowWindow(statichwnd, nCmdShow);
	UpdateWindow(hwnd);
	while (vk == 0) {
		if (GetMessage(&Msg, NULL, 0, 0) > 0) {
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	}
	CloseWindow(hwnd);
	DestroyWindow(hwnd);
	return 0;// Msg.wParam;
}

void mm_findkeybind(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	pVarResult->vt = VT_NULL;
	vk = 0;
	vk2 = 0;
	FakeWinMain(GetModuleHandle(0), 0, 0, 1);
	if (vk == -666) return;
	g_stream->SetInputMethod(vk2);
	char keyext[100];
	GetKeyNameTextA(vk, keyext, sizeof(keyext));
	pVarResult->vt = VT_BSTR;
	pVarResult->bstrVal = _com_util::ConvertStringToBSTR(keyext);
}

DWORD WINAPI tts(void* pv) {
	const wchar_t* str = (const wchar_t*)pv;
	ISpVoice* pVoice = NULL;
	HRESULT hr = S_OK;
	if (FAILED(::CoInitialize(NULL)))
		return FALSE;
	hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
	if (SUCCEEDED(hr)) {
		hr = pVoice->SetRate(3);
		hr = pVoice->Speak(str, 0, NULL);
		pVoice->Release();
		pVoice = NULL;
	}
	CoUninitialize();
	return hr;
}

void mm_say(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_BSTR)
		CreateThread(NULL, NULL, tts, pDispParams->rgvarg[0].bstrVal, NULL, NULL);
}

void mm_send_uicommand(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_I4) {
		vector<uint8_t> pv;
		rpc_write_short(&pv, pDispParams->rgvarg[0].intVal);
		g_network->Send(RPCID::UI_COMMAND, &pv);
	}
}

void mm_is_iconic(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	pVarResult->vt = VT_I4;
	if (IsIconic(g_webWindow->hWndWebWindow))
		pVarResult->intVal = 1;
	else
		pVarResult->intVal = 0;
}

void mm_set_username(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_BSTR) {
		g_network->SetUserName(pDispParams->rgvarg[0].bstrVal);
	}
	pVarResult->vt = VT_BSTR;
	pVarResult->bstrVal = SysAllocString(g_network->GetUserName());
}
