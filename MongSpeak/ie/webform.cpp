#include <windows.h>
#include <mshtmhst.h>
#include <mshtmdid.h>
#include <exdispid.h>
#include <DispEx.h>
#include <tchar.h>
#include <string>
#include "webform.h"
#include "TOleClientSite.h"
#include "toleinplaceframe.h"
#include "webformdispatchhandler.h"

WebForm::WebForm(WebformDispatchHandler *wdh) :
	ref(0), ibrowser(NULL), cookie(0), isnaving(0), url(NULL), kurl(NULL),
	hasScrollbars(false), hWnd(NULL), dispatchHandler(wdh)
{
}

void WebForm::setupOle()
{
	RECT rc;
	GetClientRect(hWnd, &rc);

	HRESULT hr;
	IOleObject* iole = 0;
	hr = CoCreateInstance(CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER, IID_IOleObject, (void**)&iole);
	if (iole == 0) {
		return;
	}

	hr = iole->SetClientSite(this);
	if (hr != S_OK) {
		iole->Release();
		return;
	}

	hr = iole->SetHostNames(L"MyHost", L"MyDoc");
	if (hr != S_OK) {
		iole->Release();
		return;
	}

	hr = OleSetContainedObject(iole, TRUE);
	if (hr != S_OK) {
		iole->Release();
		return;
	}

	hr = iole->DoVerb(OLEIVERB_SHOW, 0, this, 0, hWnd, &rc);
	if (hr != S_OK) {
		iole->Release();
		return;
	}

	bool connected = false;
	IConnectionPointContainer *cpc = 0;
	iole->QueryInterface(IID_IConnectionPointContainer, (void**)&cpc);
	if (cpc != 0) {
		IConnectionPoint *cp = 0;
		cpc->FindConnectionPoint(DIID_DWebBrowserEvents2, &cp);
		if (cp != 0) {
			cp->Advise(this, &cookie);
			cp->Release();
			connected = true;
		}
		cpc->Release();
	}

	if (!connected) {
		iole->Release();
		return;
	}
	iole->QueryInterface(IID_IWebBrowser2, (void**)&ibrowser);
	iole->Release();
}

void WebForm::Close()
{
	if (ibrowser != 0) {
		IConnectionPointContainer *cpc = 0;
		ibrowser->QueryInterface(IID_IConnectionPointContainer, (void**)&cpc);

		if (cpc != 0) {
			IConnectionPoint *cp = 0;
			cpc->FindConnectionPoint(DIID_DWebBrowserEvents2, &cp);

			if (cp != 0) {
				cp->Unadvise(cookie);
				cp->Release();
			}

			cpc->Release();
		}

		IOleObject *iole = 0;
		ibrowser->QueryInterface(IID_IOleObject, (void**)&iole);
		UINT refCount = ibrowser->Release();
		ibrowser = 0;

		if (iole != 0) {
			iole->Close(OLECLOSE_NOSAVE);
			iole->Release();
		}
	}
}

WebForm::~WebForm()
{
	if (url != 0) {
		delete[] url;
	}

	if (kurl != 0) {
		delete[] kurl;
	}
}


void TCharToWide(const char *src, wchar_t *dst, int dst_size_in_wchars)
{
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, src, -1, dst, dst_size_in_wchars);
}

void TCharToWide(const wchar_t *src, wchar_t *dst, int dst_size_in_wchars)
{
	#pragma warning(suppress:4996)
	wcscpy(dst, src);
}

char *BSTRToLPSTR(BSTR bStr, LPSTR lpstr)
{
	int lenW = SysStringLen(bStr);
	int lenA = WideCharToMultiByte(CP_ACP, 0, bStr, lenW, 0, 0, NULL, NULL);

	if (lenA > 0) {
		lpstr = new char[lenA + 1]; // allocate a final null terminator as well
		WideCharToMultiByte(CP_ACP, 0, bStr, lenW, lpstr, lenA, NULL, NULL);
		lpstr[lenA] = '\0'; // Set the null terminator yourself
	} else {
		lpstr = NULL;
	}

	return lpstr;
}

void WebForm::GoMem(wchar_t* data) {
	if (data == NULL || ibrowser == NULL)
		return;
	/*IHTMLDocument2* doc = GetDoc();
	SAFEARRAY *psaStrings = SafeArrayCreateVector(VT_VARIANT, 0, 1);
	VARIANT *param;
	HRESULT hr = SafeArrayAccessData(psaStrings, (LPVOID*)&param);
	param->vt = VT_BSTR;
	param->bstrVal = SysAllocString(data);
	hr = SafeArrayUnaccessData(psaStrings);
	hr = doc->write(psaStrings);
	//doc->close();
	SafeArrayDestroy(psaStrings);
	doc->Release();*/
	IDispatch *dispatch = 0;
	ibrowser->get_Document(&dispatch);
	if (dispatch == NULL)
		return;
	IPersistStreamInit *psi;
	dispatch->QueryInterface(IID_IPersistStreamInit, (void**)&psi);
	dispatch->Release();
	HGLOBAL hMem = ::GlobalAlloc(GPTR, wcslen(data) * sizeof(wchar_t));
	::GlobalLock(hMem);
	::CopyMemory(hMem, (LPCTSTR)data, wcslen(data) * sizeof(wchar_t));
	IStream* stream = NULL;
	HRESULT hr = ::CreateStreamOnHGlobal(hMem, FALSE, &stream);
	if (SUCCEEDED(hr))
	{
		psi->Load(stream);
		stream->Release();
	}
	::GlobalUnlock(hMem);
	::GlobalFree(hMem);
	SysFreeString(data);
	psi->Release();
}

void WebForm::Go(const char *url)
{
	if (url == NULL || ibrowser == NULL) {
		return;
	}

	wchar_t ws[MAX_PATH];
	TCharToWide(url, ws, MAX_PATH);
	isnaving = 7;
	VARIANT v;
	v.vt = VT_I4;
	v.lVal = 0; // v.lVal = navNoHistory;
	ibrowser->Navigate(ws, &v, NULL, NULL, NULL);
	// (Special case: maybe it's already loaded by the time we get here!)
	if ((isnaving & 2) == 0) {
		WPARAM w = (GetWindowLong(hWnd, GWL_ID) & 0xFFFF) | ((WEBFN_LOADED & 0xFFFF) << 16);
		PostMessage(GetParent(hWnd), WM_COMMAND, w, (LPARAM)hWnd);
	}

	isnaving &= ~4;
}

void WebForm::Forward()
{
	ibrowser->GoForward();
}

void WebForm::Back()
{
	ibrowser->GoBack();
}

void WebForm::Refresh(bool clearCache)
{
	if (clearCache) {
		VARIANT v;
		v.vt = VT_I4;
		v.lVal = REFRESH_COMPLETELY;
		ibrowser->Refresh2(&v);
	} else {
		ibrowser->Refresh();
	}
}

IHTMLDocument2 *WebForm::GetDoc()
{
	IDispatch *dispatch = 0;
	if (ibrowser == nullptr) return NULL;
	ibrowser->get_Document(&dispatch);
	
	if (dispatch == NULL) {
		return NULL;
	}
	
	IHTMLDocument2 *doc;
	dispatch->QueryInterface(IID_IHTMLDocument2, (void**)&doc);
	dispatch->Release();
	return doc;
}

IWebBrowser2* WebForm::GetBrowser() {
	if (ibrowser == nullptr) return NULL;
	return ibrowser;
}

void WebForm::RunJSFunction(std::string cmd)
{
	IHTMLDocument2 *doc = GetDoc();
	if (doc != NULL) {
		IHTMLWindow2 *win = NULL;
		doc->get_parentWindow(&win);

		if (win != NULL) {
			int lenW = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cmd.c_str(), -1, NULL, 0);
			BSTR bstrCmd = SysAllocStringLen(0, lenW);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cmd.c_str(), -1, bstrCmd, lenW);

			VARIANT v;
			VariantInit(&v);
			win->execScript(bstrCmd, NULL, &v);
			VariantClear(&v);
			SysFreeString(bstrCmd);
			win->Release();
		}

		doc->Release();
	}
}

void WebForm::RunJSFunctionW(std::wstring cmd)
{
	IHTMLDocument2 *doc = GetDoc();
	if (doc != NULL) {
		IHTMLWindow2 *win = NULL;
		doc->get_parentWindow(&win);

		if (win != NULL) {
			/*int lenW = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cmd.c_str(), -1, NULL, 0);
			BSTR bstrCmd = SysAllocStringLen(0, lenW);
			MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, cmd.c_str(), -1, bstrCmd, lenW);
			*/
			VARIANT v;
			VariantInit(&v);
			win->execScript((BSTR)cmd.c_str(), NULL, &v);

			VariantClear(&v);
			//SysFreeString(bstrCmd);
			win->Release();
		}

		doc->Release();
	}
}

class TIHTMLEditDesigner : public IHTMLEditDesigner
{
public:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void __RPC_FAR *__RPC_FAR *ppvObject) {
		HRESULT hrRet = S_OK;
		*ppvObject = NULL;
		if (IsEqualIID(riid, IID_IUnknown))
			*ppvObject = (IUnknown *)this;
		else if (IsEqualIID(riid, IID_IHTMLEditDesigner))
			*ppvObject = (IHTMLEditDesigner *)this;
		else
			hrRet = E_NOINTERFACE;
		if (S_OK == hrRet)
			((IUnknown *)*ppvObject)->AddRef();
		return hrRet;
	}
	virtual ULONG   STDMETHODCALLTYPE AddRef(void) {
		return ++m_uRefCount;
	};
	virtual ULONG   STDMETHODCALLTYPE Release(void) {
		--m_uRefCount;
		if (m_uRefCount == 0) delete this;
		return m_uRefCount;
	};

	virtual HRESULT STDMETHODCALLTYPE PreHandleEvent(DISPID inEvtDispId, IHTMLEventObj *pIEventObj) {
		if (m_webform->bKeyLookup && inEvtDispId == DISPID_KEYDOWN) {
			long keycode = NULL;
			if (FAILED(pIEventObj->get_keyCode(&keycode))) return S_FALSE;
			m_webform->bKeyLookup = FALSE;
			wchar_t kc[5];
			swprintf_s(kc, 5, L"%d", keycode);
			m_webform->QueueCallToEvent(7, -5, kc);
		}
		if (inEvtDispId == DISPID_KEYPRESS) {
			IHTMLElement* ele = NULL;
			if (FAILED(pIEventObj->get_srcElement(&ele))) return S_FALSE;
			VARIANT_BOOL bIsEdit = 0;
			if (FAILED(ele->get_isTextEdit(&bIsEdit))) return S_FALSE;
			ele->Release();
			VARIANT_BOOL bControl = 0;
			if (FAILED(pIEventObj->get_ctrlKey(&bControl))) return S_FALSE;
			if (bControl) {
				long keycode = 0;
				if (FAILED(pIEventObj->get_keyCode(&keycode))) return S_FALSE;
				if (keycode == 3)
					m_webform->GetBrowser()->ExecWB(OLECMDID_COPY, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
				if (keycode == 22 && bIsEdit)
					m_webform->GetBrowser()->ExecWB(OLECMDID_PASTE, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
				if (keycode == 24 && bIsEdit)
					m_webform->GetBrowser()->ExecWB(OLECMDID_CUT, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
				if (keycode == 1 && bIsEdit)
					m_webform->GetBrowser()->ExecWB(OLECMDID_SELECTALL, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
			}
		}
		return S_FALSE;
	};
	virtual HRESULT STDMETHODCALLTYPE PostHandleEvent(DISPID inEvtDispId, IHTMLEventObj *pIEventObj) {
		return S_FALSE;
	};
	virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(DISPID inEvtDispId, IHTMLEventObj *pIEventObj) {
		return S_FALSE;
	};
	virtual HRESULT STDMETHODCALLTYPE PostEditorEventNotify(DISPID inEvtDispId, IHTMLEventObj *pIEventObj) {
		return S_FALSE;
	};

	TIHTMLEditDesigner(WebForm* wf) : m_webform(wf) {};
	~TIHTMLEditDesigner() {};

private:
	ULONG m_uRefCount = 0;
	WebForm* m_webform;
};


void WebForm::AddCustomObject(IDispatch *custObj, std::string name)
{
	IHTMLDocument2 *doc = GetDoc();

	if (doc == NULL) {
		return;
	}

	IHTMLWindow2 *win = NULL;
	doc->get_parentWindow(&win);
	

	if (win == NULL) {
		return;
	}

	IDispatchEx *winEx;
	win->QueryInterface(&winEx);
	win->Release();

	if (winEx == NULL) {
		return;
	}

	int lenW = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name.c_str(), -1, NULL, 0);
	BSTR objName = SysAllocStringLen(0, lenW);
	MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, name.c_str(), -1, objName, lenW);

	DISPID dispid; 
	HRESULT hr = winEx->GetDispID(objName, fdexNameEnsure, &dispid);

	SysFreeString(objName);

	if (FAILED(hr)) {
		return;
	}

	DISPID namedArgs[] = {DISPID_PROPERTYPUT};
	DISPPARAMS params;
	params.rgvarg = new VARIANT[1];
	params.rgvarg[0].pdispVal = custObj;
	params.rgvarg[0].vt = VT_DISPATCH;
	params.rgdispidNamedArgs = namedArgs;
	params.cArgs = 1;
	params.cNamedArgs = 1;

	hr = winEx->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &params, NULL, NULL, NULL); 
	winEx->Release();

	if (FAILED(hr)) {
		return;
	}

	IHTMLEditServices* m_pServices = NULL;;
	IServiceProvider *pTemp;
	TIHTMLEditDesigner* cdes = new TIHTMLEditDesigner(this);
	if (doc == (IHTMLDocument2 *)NULL)
		return;
	doc->QueryInterface(IID_IServiceProvider, (void **)&pTemp);
	if (pTemp != (IServiceProvider *)NULL)
	{
		pTemp->QueryService(SID_SHTMLEditServices, IID_IHTMLEditServices, (void **)&m_pServices);
		if (m_pServices != (IHTMLEditServices *)NULL)
		{
			m_pServices->AddDesigner(cdes);
		}
	}
	doc->Release();
}

void WebForm::DocumentComplete(const wchar_t *)
{
	isnaving &= ~2;

	if (isnaving & 4) {
		return; // "4" means that we're in the middle of Go(), so the notification will be handled there
	}

	WPARAM w = (GetWindowLong(hWnd, GWL_ID) & 0xFFFF) | ((WEBFN_LOADED & 0xFFFF) << 16);
	PostMessage(GetParent(hWnd), WM_COMMAND, w, (LPARAM)hWnd);
}

HRESULT STDMETHODCALLTYPE WebForm::QueryInterface(REFIID riid, void **ppv)
{
	*ppv = NULL;

	if (riid == IID_IUnknown || riid == IID_IOleClientSite) {
		*ppv = static_cast<IOleClientSite*>(this);
	} else if (riid == IID_IOleWindow || riid == IID_IOleInPlaceSite) {
		*ppv = static_cast<IOleInPlaceSite*>(this);
	} else if (riid == IID_IOleInPlaceUIWindow) {
		*ppv = static_cast<IOleInPlaceUIWindow*>(this);
	} else if (riid == IID_IOleInPlaceFrame) {
		*ppv = static_cast<IOleInPlaceFrame*>(this);
	} else if (riid == IID_IDispatch) {
		*ppv = static_cast<IDispatch*>(this);
	} else if (riid == IID_IDocHostUIHandler) {
		*ppv = static_cast<IDocHostUIHandler*>(this);
	} else if (riid == IID_IDocHostShowUI) {
		*ppv = static_cast<IDocHostShowUI*>(this);
	}

	if (*ppv != NULL) {
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE WebForm::AddRef()
{
	return InterlockedIncrement(&ref);
}

ULONG STDMETHODCALLTYPE WebForm::Release()
{
	int tmp = InterlockedDecrement(&ref);
	
	if (tmp == 0) {
		OutputDebugStringA("WebForm::Release(): delete this");
		delete this;
	}
	
	return tmp;
}

LRESULT CALLBACK WebForm::WebformWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE) {
		WebForm *webf = (WebForm*)((LPCREATESTRUCT(lParam))->lpCreateParams);
		webf->hWnd = hwnd;
		webf->setupOle();
		if (webf->ibrowser == 0) {
			MessageBoxA(NULL, "web->ibrowser is NULL", "WM_CREATE", MB_OK);
			delete webf;
			webf = NULL;
		} else {
			webf->AddRef();
		}

		#pragma warning(suppress:4244)
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)webf);

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	#pragma warning(suppress:4312)
	WebForm *webf = (WebForm*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (webf == NULL) {
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	return webf->InstanceWndProc(msg, wParam, lParam);
}

LRESULT WebForm::InstanceWndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_CREATE: {
			CREATESTRUCT *cs = (CREATESTRUCT*)lParam;

			if (cs->style & (WS_HSCROLL | WS_VSCROLL)) {
				SetWindowLongPtr(hWnd, GWL_STYLE, cs->style & ~(WS_HSCROLL | WS_VSCROLL));
			}

			break;
		}
		case WM_DESTROY:
			Close();
			//Release();
			SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
			break;
		/*case WM_SETTEXT:
			Go((char*)lParam);
			break;*/
		case WM_SIZE:
			if (ibrowser != NULL) {
				ibrowser->put_Width(LOWORD(lParam));
				ibrowser->put_Height(HIWORD(lParam));
			}
			break;
		/*case WM_PAINT: {
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			HBRUSH brush = CreateSolidBrush(RGB(255, 255, 255));

			FillRect(ps.hdc, &ps.rcPaint, brush);

			DeleteObject(brush);
			EndPaint(hWnd, &ps);

			return 0;
		}*/
	}

	return DefWindowProc(hWnd, msg, wParam, lParam);
}

void WebForm::create(HWND hWndParent, HINSTANCE hInstance, UINT id, bool showScrollbars)
{
	hasScrollbars = showScrollbars;
	this->id = id;

	WNDCLASSEX wcex = {0};
	if (!GetClassInfoEx(hInstance, WEBFORM_CLASS, &wcex)) {
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = (WNDPROC)WebForm::WebformWndProc;
		wcex.hInstance = hInstance;
		wcex.lpszClassName = WEBFORM_CLASS;
		wcex.cbWndExtra = sizeof(WebForm*);

		if(!RegisterClassEx(&wcex)) {
			MessageBoxA(NULL, "Could not auto register the webform", "WebForm::create", MB_OK);

			return;
		}
	}

	hWnd = CreateWindow(
		WEBFORM_CLASS,
		_T("IE_PABLOKO"),
		WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
		0, 0, 100, 100, hWndParent, (HMENU)(LONG_PTR)id, hInstance, (LPVOID)this);
}

HRESULT STDMETHODCALLTYPE WebForm::Invoke(DISPID dispIdMember, REFIID riid,
	LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
	EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
	//wprintf(L"DISPID %d\n", dispIdMember);
	switch (dispIdMember) { // DWebBrowserEvents2
		case DISPID_BEFORENAVIGATE2: {
			BSTR bstrUrl = pDispParams->rgvarg[5].pvarVal->bstrVal;
			bool cancel = false;
			dispatchHandler->BeforeNavigate(bstrUrl, &cancel);
			// Set Cancel parameter to TRUE to cancel the current event
			*(((*pDispParams).rgvarg)[0].pboolVal) = cancel ? TRUE : FALSE;
			break;
		}
		case DISPID_DOCUMENTCOMPLETE:
			DocumentComplete(pDispParams->rgvarg[0].pvarVal->bstrVal);
			break;
		case DISPID_NAVIGATECOMPLETE2: {
			BSTR bstrUrl = pDispParams->rgvarg[0].pvarVal->bstrVal;
			dispatchHandler->NavigateComplete(bstrUrl, this);
			break;
		}
		case DISPID_AMBIENT_DLCONTROL:
			pVarResult->vt = VT_I4;
			pVarResult->lVal = DLCTL_DLIMAGES | DLCTL_VIDEOS | DLCTL_BGSOUNDS | DLCTL_SILENT;
			break;
		default:
			return DISP_E_MEMBERNOTFOUND;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE WebForm::GetHostInfo(DOCHOSTUIINFO *pInfo)
{
	pInfo->dwFlags = (hasScrollbars ? 0 : DOCHOSTUIFLAG_SCROLL_NO) | DOCHOSTUIFLAG_NO3DOUTERBORDER;
	
	return S_OK;
}

HRESULT STDMETHODCALLTYPE WebForm::GetExternal(IDispatch **ppDispatch)
{
	*ppDispatch = static_cast<IDispatch*>(this);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE WebForm::GetWindow(HWND *phwnd)
{
	*phwnd = hWnd;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE WebForm::GetWindowContext(IOleInPlaceFrame **ppFrame,
	IOleInPlaceUIWindow **ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect,
	LPOLEINPLACEFRAMEINFO info)
{
	*ppFrame = static_cast<IOleInPlaceFrame*>(this);
	//*ppDoc = static_cast<IOleInPlaceUIWindow*>(this);
	AddRef();
	*ppDoc = NULL;
	info->fMDIApp = FALSE;
	info->hwndFrame = hWnd;
	info->haccel = 0;
	info->cAccelEntries = 0;
	GetClientRect(hWnd, lprcPosRect);
	GetClientRect(hWnd, lprcClipRect);

	return S_OK;
}

HRESULT STDMETHODCALLTYPE WebForm::OnPosRectChange(LPCRECT lprcPosRect)
{
	IOleInPlaceObject *iole = NULL;
	ibrowser->QueryInterface(IID_IOleInPlaceObject, (void**)&iole);

	if (iole != NULL) {
		iole->SetObjectRects(lprcPosRect, lprcPosRect);
		iole->Release();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE WebForm::ShowMessage(HWND hwnd, LPOLESTR lpstrText,
	LPOLESTR lpstrCaption, DWORD dwType, LPOLESTR lpstrHelpFile,
	DWORD dwHelpContext, LRESULT *plResult)
{
	return S_FALSE;
}

void WebForm::SinkScriptErrorEvents(IDispatch* pperr) {
	ibrowser->put_Silent(TRUE);
	IDispatch *dispatch = 0;
	ibrowser->get_Document(&dispatch);
	if (dispatch == NULL) return;
	IHTMLDocument2 *doc;
	dispatch->QueryInterface(IID_IHTMLDocument2, (void**)&doc);
	dispatch->Release();
	if (doc == NULL) return;
	IHTMLWindow2 *win = NULL;
	doc->get_parentWindow(&win);
	if (win == NULL) return;
	VARIANT pv;
	pv.vt = VT_DISPATCH;
	pv.pdispVal = pperr;
	win->put_onerror(pv);
	win->Release();
	doc->Release();
}

HRESULT WebForm::DequeueCallToEvent() {
	
	IHTMLDocument2 *doc = GetDoc();
	if (doc == NULL) 
		return NULL;
	IHTMLWindow2 *win = NULL;
	doc->get_parentWindow(&win);
	doc->Release();
	if (win == NULL) 
		return NULL;
	IDispatchEx *winEx;
	win->QueryInterface(&winEx);
	win->Release();
	if (winEx == NULL) 
		return NULL;
	while (vec_rpc_id.size() > 0) {
		DISPID dispid;
		BSTR idname = SysAllocString(L"onEvent");
		HRESULT hr = winEx->GetDispID(idname, fdexNameEnsure, &dispid);
		SysFreeString(idname);
		if (FAILED(hr))
			continue;
		DISPID namedArgs[] = { DISPID_PROPERTYPUT,DISPID_PROPERTYPUT,DISPID_PROPERTYPUT };
		DISPPARAMS params;
		BSTR custObj = SysAllocString(vec_message[0].c_str());
		params.rgvarg = new VARIANT[3];
		params.rgvarg[0].bstrVal = custObj;
		params.rgvarg[0].vt = VT_BSTR;
		params.rgvarg[1].intVal = vec_pkt_id[0];
		params.rgvarg[1].vt = VT_I4;
		params.rgvarg[2].intVal = vec_rpc_id[0];
		params.rgvarg[2].vt = VT_I4;
		params.rgdispidNamedArgs = namedArgs;
		params.cArgs = 3;
		params.cNamedArgs = 3;
		hr = winEx->InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, NULL, NULL, NULL);
		//SysFreeString(custObj);
		//delete[] params.rgvarg;

		vec_rpc_id.erase(vec_rpc_id.begin());
		vec_pkt_id.erase(vec_pkt_id.begin());
		vec_message.erase(vec_message.begin());
		if (FAILED(hr)) 
			continue;
	}
	winEx->Release();
	return S_OK;
}

HRESULT WebForm::QueueCallToEvent(short rpcid, short id, wchar_t* str) {
	vec_rpc_id.push_back(rpcid);
	vec_pkt_id.push_back(id);
	vec_message.push_back(std::wstring(str));
	return S_OK;
}
