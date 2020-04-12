#ifndef _WEBFORM_DISPATCH_IMPL_H__
#define _WEBFORM_DISPATCH_IMPL_H__

#include "webformdispatchhandler.h"

class WebForm;
class JSObject;

class WebformDispatchImpl : public WebformDispatchHandler {
private:
	JSObject *jsobj;
public:
	WebformDispatchImpl(JSObject *jsobj);
	virtual void BeforeNavigate(std::wstring url, bool *cancel);
	virtual void NavigateComplete(std::wstring url, WebForm *webForm);
};

#endif