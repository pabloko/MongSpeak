//#include "../stdafx.h"
#include <utility>
#include <fstream>
#include "jsobject.h"
#include "atlbase.h"
#include <cstring>

JSObject::JSObject()
	: ref(0)
{
	//sprintf(szGenericMehod, "");
}

void JSObject::AddMethod(std::wstring a, /*DISPID*/ void* b) {
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
		//OutputDebugStringA("JSObject::Release(): delete this");
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
		//wprintf(L"ID of method asked: %s iter %d\n", rgszNames[i], i);
		std::map<std::wstring, void*>::iterator iter = idMap.find(rgszNames[i]);
		if (iter != idMap.end()) {
			rgDispId[i] = (DISPID)iter->second;
		}
		else {
			/*if (cNames == 1) {
				sprintf(szGenericMehod, "ui_%s", (const char*)_com_util::ConvertBSTRToString(rgszNames[i]));
				lua_getglobal(L, szGenericMehod);
				if (lua_isfunction(L, -1)) {
					rgDispId[i] = DISPID_USER_LUACALL;
					lua_remove(L, -1);
					return hr;
				}
				lua_remove(L, -1);
			}*/
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
			/*std::string *args = new std::string[pDispParams->cArgs];
			for (size_t i = 0; i < pDispParams->cArgs; ++i) {
			BSTR bstrArg = pDispParams->rgvarg[i].bstrVal;
			LPSTR arg = NULL;
			arg = BSTRToLPSTR(bstrArg, arg);
			args[pDispParams->cArgs - 1 - i] = arg; // also re-reverse order of arguments
			delete [] arg;
			}*/

			switch (dispIdMember) {
			/*case DISPID_USER_EXECUTE: {
				break;
			}*/
			/*case DISPID_USER_EVAL: {
				if (luaL_dostring(L, (const char*)_com_util::ConvertBSTRToString(pDispParams->rgvarg[0].bstrVal)) != 0)
					lua_err(L);
				break;
			}

			case DISPID_USER_LUACALL: {
				//LUASTART;
				lua_getglobal(L, szGenericMehod);
				//OutputDebugString(szGenericMehod);
				int arg_count = pDispParams->cArgs - 1;
				int arguments = pDispParams->cArgs;
				if (arg_count >= 0) {
					//push arguments
					while (arg_count >= 0) {
						switch (pDispParams->rgvarg[arg_count].vt) {
						case VT_BSTR: {
							lua_pushstring(L, (const char*)_com_util::ConvertBSTRToString(pDispParams->rgvarg[arg_count].bstrVal));
							//OutputDebugString((const char*)_com_util::ConvertBSTRToString(pDispParams->rgvarg[arg_count].bstrVal));
						} break;
						case VT_I4: {
							lua_pushinteger(L, pDispParams->rgvarg[arg_count].intVal);
						} break;
						case VT_R8: {
							lua_pushnumber(L, pDispParams->rgvarg[arg_count].dblVal);
						} break;
						case VT_BOOL: {
							lua_pushboolean(L, pDispParams->rgvarg[arg_count].boolVal);
						} break;
						default: {
							lua_pushnil(L);
						} break;
						}
						arg_count--;
					}
				}

				if (lua_pcall(L, arguments, 0, 0) != 0)
					lua_err(L);
				//LUAEND;
				/*else {
				//OutputDebugString(lua_tostring(L, -1));
				switch (lua_type(L, -1))
				{
				/*
				@@TODO: fix stack corruption... no mola
				case LUA_TSTRING:
				{
				pVarResult->vt = VT_BSTR;
				//pVarResult->bstrVal = _com_util::ConvertStringToBSTR(lua_tostring(L, -1));
				}
				break;
				case LUA_TNUMBER:
				{
				double inum = lua_tonumber(L, -1);
				if (fmod(inum, 1) != 0)
				pVarResult->vt = VT_R8;
				else
				pVarResult->vt = VT_I4;
				pVarResult->intVal = inum;
				pVarResult->dblVal = inum;
				}
				break;
				case LUA_TBOOLEAN:
				{
				pVarResult->vt = VT_BOOL;
				pVarResult->boolVal = lua_toboolean(L, -1);
				}
				break;
				default:
				{
				//pVarResult->vt = VT_NULL;
				}
				break;
				* /
				}
				lua_remove(L, -1)
				}* /
				//lua_remove(L, -1);
			} break;*/
			default: {
				hr = DISP_E_MEMBERNOTFOUND;
			} break;
			}
			return hr;
		}
	return E_FAIL;
}

