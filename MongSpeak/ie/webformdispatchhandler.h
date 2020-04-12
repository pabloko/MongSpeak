#ifndef _WEBFORM_DISPATCH_HANDLER_H__
#define _WEBFORM_DISPATCH_HANDLER_H__

#include <string>

class WebForm;

class WebformDispatchHandler {
public:
	virtual void BeforeNavigate(std::wstring url, bool *cancel);
	virtual void DocumentComplete(std::wstring url);
	virtual void NavigateComplete(std::wstring url, WebForm *webForm);
};

#endif