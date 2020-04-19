#pragma once
#include <sapi.h>
//Logs a string to the debug console
void mm_log(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs > 0) {
		if (pDispParams->rgvarg[0].vt == VT_BSTR) {
			wprintf(L"[JS] %s\n", pDispParams->rgvarg[0].bstrVal);
		}
	}
}
//List audio devices input/output to a stringified json array
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
//Connect to a server
void mm_connect(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs > 0) {
		//if type string
		if (pDispParams->rgvarg[0].vt == VT_BSTR) {
			g_network->ConnectServer(_com_util::ConvertBSTRToString(pDispParams->rgvarg[0].bstrVal));
		}
	}
}
//Disconnect from server
void mm_disconnect(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	g_network->Disconnect();
}
//Send RPCID::CHANGE_ROOM
void mm_change_room(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs > 0) {
		if (pDispParams->rgvarg[0].vt == VT_I4) {
			vector<uint8_t> pv;
			rpc_write_short(&pv, pDispParams->rgvarg[0].intVal);
			g_network->Send(RPCID::CHANGE_ROOM, &pv);
		}
	}
}
//Send RPCID::USER_CHAT
void mm_send_message(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs > 0) {
		if (pDispParams->rgvarg[0].vt == VT_BSTR) {
			g_network->Send(RPCID::USER_CHAT, (char*)pDispParams->rgvarg[0].bstrVal, wcslen(pDispParams->rgvarg[0].bstrVal) * 2);
		}
	}
}
//Change audio device, input/output, device id (0=default, 1...n devices from list)
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
//Read key,value from regedit
void mm_set_preferences(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 2 && pDispParams->rgvarg[0].vt == VT_BSTR && pDispParams->rgvarg[1].vt == VT_BSTR) {
		g_preferences->SetPreferences(pDispParams->rgvarg[1].bstrVal, pDispParams->rgvarg[0].bstrVal);
	}
}
//Write key,value from regedit
void mm_get_preferences(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_BSTR) {
		wstring data = g_preferences->GetPreferences(pDispParams->rgvarg[0].bstrVal);
		if (data.length() > 0) {
			pVarResult->vt = VT_BSTR;
			pVarResult->bstrVal = SysAllocString(data.c_str());
		} else pVarResult->vt = VT_NULL;
	}
}
//Set volume multiplicator of -1=Output -2:Input >=0: id of CAudioSession in CAudioSessionMixer
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
//Set CAudioStream input method
void mm_set_inputmethod(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_I4) {
		g_stream->SetInputMethod(pDispParams->rgvarg[0].intVal);
	}
}
//Next key pressed after this call will send a UI_COMMAND id -5
void mm_findkeybind(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	g_webWindow->webForm->bKeyLookup = TRUE;
}
//virtual keycode to string representation
void mm_findkeyname(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	pVarResult->vt = VT_NULL;
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_I4) {
		WCHAR name[50];
		UINT scanCode = MapVirtualKeyW(pDispParams->rgvarg[0].intVal, MAPVK_VK_TO_VSC);
		LONG lParamValue = (scanCode << 16);
		int result = GetKeyNameTextW(lParamValue, name, 50);
		if (result > 0)
		{
			pVarResult->vt = VT_BSTR;
			pVarResult->bstrVal = SysAllocString(name);
		}
	}
}
//Text-to-speech thread
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
//Spawns a tts thread to say a string
void mm_say(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_BSTR)
		CreateThread(NULL, NULL, tts, pDispParams->rgvarg[0].bstrVal, NULL, NULL);
}
//Broadcast UI_COMMAND over the network
void mm_send_uicommand(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_I4) {
		vector<uint8_t> pv;
		rpc_write_short(&pv, pDispParams->rgvarg[0].intVal);
		g_network->Send(RPCID::UI_COMMAND, &pv);
	}
}
//Check if window is minimized to play notification sound alerts
//Todo: we need to move this to WM_ACTIVATE, notification should play if not visible, also should be in settings
void mm_is_iconic(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	pVarResult->vt = VT_I4;
	if (IsIconic(g_webWindow->hWndWebWindow))
		pVarResult->intVal = 1;
	else
		pVarResult->intVal = 0;
}
//Set the username and returns it, if no param provided, returns default PC name or previously saved
//note: ie fails when pVarResult is filled but you dont put the returned to a var
void mm_set_username(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_BSTR) {
		g_network->SetUserName(pDispParams->rgvarg[0].bstrVal);
	}
	pVarResult->vt = VT_BSTR;
	pVarResult->bstrVal = SysAllocString(g_network->GetUserName());
}
//Set CAudioStream fast vu updates
void mm_send_vu(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 1 && pDispParams->rgvarg[0].vt == VT_I4) {
		g_stream->SendVU(pDispParams->rgvarg[0].intVal?TRUE:FALSE);
	}
}
//Bind JS Interface methods to IE "mong" object
void BindJSMethods() {
	g_jsObject->AddMethod(L"log", mm_log);
	g_jsObject->AddMethod(L"send_message", mm_send_message);
	g_jsObject->AddMethod(L"list_devices", mm_list_devices);
	g_jsObject->AddMethod(L"connect", mm_connect);
	g_jsObject->AddMethod(L"disconnect", mm_disconnect);
	g_jsObject->AddMethod(L"change_room", mm_change_room);
	g_jsObject->AddMethod(L"set_device", mm_set_device);
	g_jsObject->AddMethod(L"set_preferences", mm_set_preferences);
	g_jsObject->AddMethod(L"get_preferences", mm_get_preferences);
	g_jsObject->AddMethod(L"set_volume", mm_set_vol);
	g_jsObject->AddMethod(L"set_inputmethod", mm_set_inputmethod);
	g_jsObject->AddMethod(L"findkeybind", mm_findkeybind);
	g_jsObject->AddMethod(L"findkeyname", mm_findkeyname);
	g_jsObject->AddMethod(L"say", mm_say);
	g_jsObject->AddMethod(L"send_uicommand", mm_send_uicommand);
	g_jsObject->AddMethod(L"is_iconic", mm_is_iconic);
	g_jsObject->AddMethod(L"set_username", mm_set_username);
	g_jsObject->AddMethod(L"send_vu", mm_send_vu);
}
