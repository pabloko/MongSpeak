//#include "../stdafx.h"
#include <utility>
#include <fstream>
#include "jsobject.h"
#include "atlbase.h"
#include <cstring>

JSObject::JSObject(): ref(0) {}

void JSObject::AddMethod(std::wstring a, void* b) {
	idMap.insert(std::make_pair(a, b));
}

HRESULT STDMETHODCALLTYPE JSObject::QueryInterface(REFIID riid, void** ppv)
{
	*ppv = NULL;

	if (riid == IID_IUnknown || riid == IID_IDispatch) {
		*ppv = static_cast<IDispatch*>(this);
	}

	if (*ppv != NULL) {
		AddRef();
		return S_OK;
	}

	return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE JSObject::AddRef()
{
	return InterlockedIncrement(&ref);
}

ULONG STDMETHODCALLTYPE JSObject::Release()
{
	int tmp = InterlockedDecrement(&ref);

	if (tmp == 0) {
		delete this;
	}
	return tmp;
}

HRESULT STDMETHODCALLTYPE JSObject::GetTypeInfoCount(UINT* pctinfo)
{
	*pctinfo = 0;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE JSObject::GetTypeInfo(UINT iTInfo, LCID lcid,
	ITypeInfo** ppTInfo)
{
	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE JSObject::GetIDsOfNames(REFIID riid,
	LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId)
{
	HRESULT hr = S_OK;
	for (UINT i = 0; i < cNames; i++) {
		std::map<std::wstring, void*>::iterator iter = idMap.find(rgszNames[i]);
		if (iter != idMap.end()) {
			rgDispId[i] = (DISPID)iter->second;
		}
		else {
			rgDispId[i] = DISPID_UNKNOWN;
			hr = DISP_E_UNKNOWNNAME;
		}
	}
	return hr;
}

typedef void invoke_t(DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr);


HRESULT STDMETHODCALLTYPE JSObject::Invoke(DISPID dispIdMember, REFIID riid,
	LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult,
	EXCEPINFO* pExcepInfo, UINT* puArgErr)
{
		if (wFlags & DISPATCH_METHOD) {
			HRESULT hr = S_OK;
			if (dispIdMember > 0) {
				invoke_t* pInvokeMethod = (invoke_t*)dispIdMember;
				pInvokeMethod(pDispParams, pVarResult, pExcepInfo, puArgErr);
			}
			else
				if (pDispParams->cArgs == 3 && pDispParams->rgvarg[0].vt == VT_I4 && pDispParams->rgvarg[1].vt == VT_BSTR && pDispParams->rgvarg[2].vt == VT_BSTR)
					wprintf(L"[JS EXCEPTION] [%s] Line: %d\n%s\n", pDispParams->rgvarg[1].bstrVal, pDispParams->rgvarg[0].intVal, pDispParams->rgvarg[2].bstrVal);
			return hr;
			//hr = DISP_E_MEMBERNOTFOUND;
			//return hr;
		}
	return E_FAIL;
}

