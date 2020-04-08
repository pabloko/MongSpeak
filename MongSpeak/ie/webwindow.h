#ifndef _WEB_WINDOW_H__
#define _WEB_WINDOW_H__

#include <windows.h>
#include "webform.h"

class WebWindow {
private:
	
	HINSTANCE hInstWebWindow;
	bool showScrollbars;
	static LRESULT CALLBACK WebWindowWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	LRESULT InstanceWndProc(UINT msg, WPARAM wParam, LPARAM lParam);
	WebformDispatchHandler *webformDispatchHandler;

public:
	HWND hWndWebWindow;
	WebWindow(WebformDispatchHandler *wdh);
	WebForm *webForm;
	void Create(HINSTANCE hInstance, UINT x, UINT y, UINT width, UINT height, bool showScrollbars);
};

#endif
