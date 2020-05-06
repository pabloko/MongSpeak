#pragma once
#include <shellapi.h>

typedef void t_invoke_t(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);
typedef BOOL t_pastefile_t();
typedef void t_dropfn_t(HDROP);

typedef struct {
	void* _this;
	void* _cb;
	//wchar_t* _name;
} st_jsmethod;

class WebForm {
public:
	WebForm() {
		webForm = this;
		//crear ventana iniciar wke etc...
	}

	~WebForm() {
		//a tomar por culo
	}

	BOOL GetWindowIsActive() {
		//obtener de WM_ONACTIVATE see webwindow.cpp
		return false;
	}

	static void JSToVARIANT(jsExecState es, jsValue val, VARIANT* vt) {
		switch (jsTypeOf(val)) {
		case jsType::JSTYPE_STRING:
			vt->vt = VT_BSTR;
			vt->bstrVal = (BSTR)jsToStringW(es, val);
			break;
		case jsType::JSTYPE_BOOLEAN:
			vt->vt = VT_BOOL;
			vt->boolVal = jsToBoolean(es, val);
			break;
			//TODO: Continue handling used VT_xxx
		default:
			vt->vt = VT_NULL;
			break;
		}
	}
	static jsValue VARIANTToJS(jsExecState es, VARIANT* vt) {
		jsValue ret = jsNull();
		switch (vt->vt) {
		case VT_BSTR:
			ret = jsStringW(es, vt->bstrVal);
			break;
		case VT_BOOL:
			ret = jsBoolean(vt->boolVal);
			break;
			//TODO: Continue handling used VT_xxx
		default:
			break;
		}
		return ret;
	}
	
	static jsValue MethodHandler(jsExecState es, void* param) {
		st_jsmethod* info = (st_jsmethod*)param;
		DISPPARAMS pdisp;
		pdisp.cArgs = jsArgCount(es);
		pdisp.rgvarg = new VARIANT[pdisp.cArgs];
		for (int i = pdisp.cArgs - 1, idx = 0; i >= 0; i--,idx++)
			JSToVARIANT(es, jsArg(es, idx), &pdisp.rgvarg[i]);
		VARIANT pret;
		((t_invoke_t*)(info->_cb))(&pdisp, &pret, NULL, NULL);
		delete[] pdisp.rgvarg;
		return VARIANTToJS(es, &pret);
	}

	void AddMethod(const wchar_t* a, /*t_invoke_t*/void* b) {
		if (m_webview == NULL) return;
		//m_pScriptCommands.insert(std::make_pair(std::wstring(a), b));
		st_jsmethod* method = (st_jsmethod*)malloc(sizeof(st_jsmethod));
		method->_this = this;
		method->_cb = b;
		//method->_name = _wcsdup(a);
		wkeJsBindFunction(_com_util::ConvertBSTRToString((BSTR)a), MethodHandler, method, 0);
	}
	
	void RunJSFunctionW(const wchar_t* c) {
		//eval code
	}
	void SetDropFileHandler(/*t_dropfn_t*/void* f) {
		m_dropfile_handler_ = f;
	}

	void SetPasteAcceleratorHandler(/*t_pastefile_t*/void* p) {
		m_paste_handler_ = p;
	}
	
	void GoMem(const wchar_t* c) {
		//Load html to web
	} 

	int DequeueCallToEvent() {
		if (m_webview == NULL) return E_ABORT;
		while (vec_rpc_id.size() > 0) {
			jsExecState es = wkeGlobalExec(m_webview);
			jsValue* args = new jsValue[3];
			jsCall(es, jsGetGlobal(es, "onEvent"), NULL, args, 3);
			delete[] args;
			vec_rpc_id.erase(vec_rpc_id.begin());
			vec_pkt_id.erase(vec_pkt_id.begin());
			vec_message.erase(vec_message.begin());
		}
		return S_OK;
	} 
	
	int QueueCallToEvent(short rpcid, short id, wchar_t* str) {
		vec_rpc_id.push_back(rpcid);
		vec_pkt_id.push_back(id);
		vec_message.push_back(std::wstring(str));
		return S_OK;
	}

	HRESULT Release() {
		delete this;
	}

	BOOL bKeyLookup = FALSE;
	WebForm* webForm;
	HWND hWndWebWindow;
private:
	void* m_paste_handler_;
	void* m_dropfile_handler_;
	std::vector<short> vec_rpc_id;
	std::vector<short> vec_pkt_id;
	std::vector<std::wstring> vec_message;
	//std::map<std::wstring, void*> m_pScriptCommands;
	wkeWebView m_webview;
};

typedef WebForm JSObject;
typedef WebForm WebWindow;