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

void WebformDispatchImpl::BeforeNavigate(std::string url, bool *cancel)
{
	if (!url.compare("about:blank;"))
		*cancel = false;
	else
		*cancel = true;
}

void WebformDispatchImpl::NavigateComplete(std::string url, WebForm *webForm)
{
	webForm->AddCustomObject(jsobj, "mong");
	webForm->SinkScriptErrorEvents(jsobj);
}
