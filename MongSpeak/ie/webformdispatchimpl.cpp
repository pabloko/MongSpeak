//#include "../stdafx.h"
#include "webformdispatchimpl.h"
#include <locale>
#include "windows.h"
#include "urlcode.h"
#include "webform.h"
#include "jsobject.h"

WebformDispatchImpl::WebformDispatchImpl(JSObject *jsobj)
{
	this->jsobj = jsobj;
}

void WebformDispatchImpl::BeforeNavigate(std::wstring url, bool *cancel)
{
	if (!url.compare(L"about:blank;"))
		*cancel = false;
	else {
		*cancel = true;
		ShellExecute(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
	}
	
}

void WebformDispatchImpl::NavigateComplete(std::wstring url, WebForm *webForm)
{
	webForm->AddCustomObject(jsobj, "mong");
	webForm->SinkScriptErrorEvents(jsobj);
}
