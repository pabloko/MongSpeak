#pragma once
extern CNetwork* g_network;
void mm_alert(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	pVarResult->vt = VT_BSTR;
	pVarResult->bstrVal = _com_util::ConvertStringToBSTR("AYYY LMAO");
}

void mm_log(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	//num args > 0
	if (pDispParams->cArgs > 0) {
		//if type string
		if (pDispParams->rgvarg[0].vt == VT_BSTR) {
			wprintf(L"[JS] %s\n", pDispParams->rgvarg[0].bstrVal);
			//OutputDebugString(pDispParams->rgvarg[0].bstrVal);
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
		//if type integer
		if (pDispParams->rgvarg[0].vt == VT_I4 /*&& pDispParams->rgvarg[1].vt == VT_I4*/) {
			vector<uint8_t> pv;
			rpc_write_short(&pv, pDispParams->rgvarg[0].intVal);
			g_network->Send(RPCID::CHANGE_ROOM, &pv);
			//wprintf(L"USER ID: %d - ROOM ID: %d\n", pDispParams->rgvarg[1].bstrVal, pDispParams->rgvarg[0].bstrVal);
		}
	}
}
/* Chat 

set_audio_device
get_preference
set_preference
change_room

*/
void mm_send_message(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs > 0) {
		//if type string
		if (pDispParams->rgvarg[0].vt == VT_BSTR) {
			g_network->Send(RPCID::USER_CHAT, (char*)pDispParams->rgvarg[0].bstrVal, wcslen(pDispParams->rgvarg[0].bstrVal) * 2);
		}
	}
}

void mm_set_device(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
	if (pDispParams->cArgs == 2) {
		if (pDispParams->rgvarg[0].vt == VT_I4 && pDispParams->rgvarg[1].vt == VT_I4) {
			EDataFlow flow = (EDataFlow)pDispParams->rgvarg[1].intVal;
			UINT32 deviceIndex = pDispParams->rgvarg[0].intVal;
			wprintf(L"change audio device %d->%d\n", flow, deviceIndex);
			if (flow == EDataFlow::eCapture) {
				g_stream->SetDeviceIn(NULL);
				delete g_audio_in;
				g_dev_in->Release();
				//delete g_dev_in;
				if (deviceIndex == 0)
					get_default_device(&g_dev_in, flow);
				else
					get_specific_device(deviceIndex - 1, &g_dev_in, flow);
				g_audio_in = new CAudioDevice(g_dev_in, flow);
				g_stream->SetDeviceIn(g_audio_in);
			}
			if (flow == EDataFlow::eRender) {
				g_mix->SetDeviceOut(NULL);
				delete g_audio_out;
				g_dev_out->Release();
				//delete g_dev_out;
				if (deviceIndex == 0)
					get_default_device(&g_dev_in, flow);
				else
					get_specific_device(deviceIndex - 1, &g_dev_out, flow);
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
		}
	}
}