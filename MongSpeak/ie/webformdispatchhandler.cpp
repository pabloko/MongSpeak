#include "webformdispatchhandler.h"

void WebformDispatchHandler::BeforeNavigate(std::wstring url, bool *cancel)
{
	*cancel = false;
}

void WebformDispatchHandler::DocumentComplete(std::wstring url)
{
}

void WebformDispatchHandler::NavigateComplete(std::wstring url, WebForm *webForm)
{
}
