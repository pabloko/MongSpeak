#ifndef _JS_LITESTEP_H__
#define _JS_LITESTEP_H__

#include <windows.h>
#include <mshtmhst.h>
#include <string>
#include <map>

class JSObject : public IDispatch {
private:
	std::map<std::wstring, /*DISPID*/ void*> idMap;
	long ref;
	char szGenericMehod[124];
public:
	JSObject();
	void AddMethod(std::wstring, /*DISPID*/ void*);

	// IUnknown
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppv);
	virtual ULONG STDMETHODCALLTYPE AddRef();
	virtual ULONG STDMETHODCALLTYPE Release();

	// IDispatch
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT *pctinfo);
	virtual HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT iTInfo, LCID lcid,
		ITypeInfo **ppTInfo);
	virtual HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID riid,
		LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId);
	virtual HRESULT STDMETHODCALLTYPE Invoke(DISPID dispIdMember, REFIID riid,
		LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
		EXCEPINFO *pExcepInfo, UINT *puArgErr);
};

#endif