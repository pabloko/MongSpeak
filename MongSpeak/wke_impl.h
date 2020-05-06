#pragma once
#include <shellapi.h>
#include "resource.h"

typedef void t_invoke_t(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);
typedef BOOL t_pastefile_t();
typedef void t_dropfn_t(HDROP);

class WebForm {
public:
	
	static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		extern WebForm* g_webWindow;
		switch (msg) {
		/*case WM_NCCREATE: {
			HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDC_MYICON));
			SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			
		} break;*/
		case WM_CREATE: {
			DragAcceptFiles(hwnd, TRUE);
			HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
			HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MONGSPEAK));
			if (hIcon != NULL)
				SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		} break;
		case WM_SIZE: {
			if (g_webWindow->m_webview)
				wkeResize(g_webWindow->m_webview, LOWORD(lParam), HIWORD(lParam));
		} break;
		case WM_DROPFILES: {
			if (g_webWindow->m_dropfile_handler_ != NULL)
				((t_dropfn_t*)g_webWindow->m_dropfile_handler_)((HDROP)wParam);
		} break;
		case WM_ACTIVATE: {
			g_webWindow->bActiveWindow = LOWORD(wParam) == 0 ? FALSE : TRUE;
		} break;
		case WM_DESTROY: {
			//g_webWindow->Release();
			PostQuitMessage(0);
		} break;
		}
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	WebForm() {
		extern WebForm* g_webWindow;
		bActiveWindow = FALSE;
		webForm = this;
		g_webWindow = this;
		wkeInitialize();
		WNDCLASSEX wc;
		HINSTANCE hInstance = GetModuleHandle(NULL);
		const wchar_t* g_szClassName = { L"WKEWEBWIN" };
		MSG Msg;
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = (WNDPROC)WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = sizeof(this);
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = g_szClassName;
		wc.hIconSm = NULL;
		RegisterClassEx(&wc);
		hWndWebWindow = CreateWindowEx(0, g_szClassName, L"MongSpeak", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, CW_USEDEFAULT, CW_USEDEFAULT, 600, 400, NULL, NULL, hInstance, NULL);
		m_webview = wkeCreateWebWindow(wkeWindowType::WKE_WINDOW_TYPE_CONTROL, hWndWebWindow, 0, 0, 600, 400);
		//wkeLoadURL(m_webview, "http://google.es");
		ShowWindow(hWndWebWindow, SW_SHOW);
		wkeShowWindow(m_webview, TRUE);
		//jsExecState es = wkeGlobalExec(m_webview);
		//jsSetGlobal(es, "mong", jsGetGlobal(es,"window"));
	}

	~WebForm() {
		wkeShutdown();
	}

	BOOL GetWindowIsActive() {
		return bActiveWindow;
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
		case jsType::JSTYPE_NUMBER:
			vt->vt = VT_I4;
			vt->boolVal = jsToInt(es, val);
			break;
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
		case VT_I4:
			ret = jsInt(vt->intVal);
			break;
		default:
			break;
		}
		return ret;
	}
	
	static jsValue MethodHandler(jsExecState es, void* param) {
		DISPPARAMS pdisp;
		pdisp.cArgs = jsArgCount(es);
		pdisp.rgvarg = new VARIANT[pdisp.cArgs];
		for (int i = pdisp.cArgs - 1, idx = 0; i >= 0; i--,idx++)
			JSToVARIANT(es, jsArg(es, idx), &pdisp.rgvarg[i]);
		VARIANT pret;
		((t_invoke_t*)param)(&pdisp, &pret, NULL, NULL);
		delete[] pdisp.rgvarg;
		return VARIANTToJS(es, &pret);
	}

	void AddMethod(const wchar_t* a, /*t_invoke_t*/void* b) {
		if (m_webview == NULL) return;
		//m_pScriptCommands.insert(std::make_pair(std::wstring(a), b));
		//st_jsmethod* method = (st_jsmethod*)malloc(sizeof(st_jsmethod));
		//method->_this = this;
		//method->_cb = b;
		//method->_name = _wcsdup(a);
		wkeJsBindFunction(_com_util::ConvertBSTRToString((BSTR)a), MethodHandler, b, 10);
	}
	
	void RunJSFunctionW(const wchar_t* c) {
		if (m_webview)
			wkeRunJSW(m_webview, c);
	}
	void SetDropFileHandler(/*t_dropfn_t*/void* f) {
		m_dropfile_handler_ = f;
	}

	void SetPasteAcceleratorHandler(/*t_pastefile_t*/void* p) {
		m_paste_handler_ = p;
	}
	
	void GoMem(const wchar_t* c) {
		if (m_webview)
			wkeLoadHTMLW(m_webview, c);
	} 

	int DequeueCallToEvent() {
		if (m_webview == NULL) return E_ABORT;
		while (vec_rpc_id.size() > 0) {
			jsExecState es = wkeGlobalExec(m_webview);
			jsValue* args = new jsValue[3];
			args[0] = jsInt(vec_rpc_id.at(0));
			args[1] = jsInt(vec_pkt_id.at(0));
			args[2] = jsStringW(es, vec_message.at(0).c_str());
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
	void* m_paste_handler_;
	void* m_dropfile_handler_;
	BOOL bActiveWindow;
	wkeWebView m_webview;
private:
	std::vector<short> vec_rpc_id;
	std::vector<short> vec_pkt_id;
	std::vector<std::wstring> vec_message;
	//std::map<std::wstring, void*> m_pScriptCommands;
};

typedef WebForm JSObject;
typedef WebForm WebWindow;