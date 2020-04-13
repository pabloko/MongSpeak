//#include "../stdafx.h"
#include "../resource.h"
#include "webwindow.h"
#include <vector>

WebWindow::WebWindow(WebformDispatchHandler *wdh) : webformDispatchHandler(wdh) {}
HINSTANCE gh;

void WebWindow::Create(HINSTANCE hInstance, UINT x, UINT y, UINT width, UINT height, bool showScrollbars)
{
	gh = hInstance;
	WNDCLASSEX wcex;
	static const wchar_t *className = L"WebWindowClass";
	this->hInstWebWindow = hInstance;
	this->showScrollbars = showScrollbars;
	if (!GetClassInfoEx(hInstance, className, &wcex)) {
		ZeroMemory(&wcex, sizeof(wcex));
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = (WNDPROC)WebWindowWndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(this);
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground = NULL;
		wcex.lpszMenuName = NULL;
		wcex.lpszClassName = className;
		wcex.hIconSm = NULL;
		if (!RegisterClassEx(&wcex)) {
			MessageBoxA(NULL, "Error registering WebWindow window class", "WebWindow::Create", MB_OK);
			return;
		}
	}
	webForm = new WebForm(webformDispatchHandler);
	hWndWebWindow = CreateWindowEx(0, className, L"MongSpeak", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS/*WS_CHILD*//*WS_POPUP | WS_CLIPCHILDREN* /WS_POPUPWINDOW | WS_CAPTION*/,
		x, y, width, height, NULL, NULL, hInstance, (LPVOID)this);
	if (hWndWebWindow == NULL) {
		MessageBoxA(NULL, "Error creating WebWindow window", "WebWindow::Create", MB_OK);
		UnregisterClass(className, hInstance);
		return;
	}

	ShowWindow(hWndWebWindow, SW_SHOW);
}

LRESULT CALLBACK WebWindow::WebWindowWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_NCCREATE) {
		HICON hIcon = LoadIcon(gh, MAKEINTRESOURCE(IDC_MYICON));
		SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

		WebWindow *webw = (WebWindow*)((LPCREATESTRUCT(lParam))->lpCreateParams);
		webw->hWndWebWindow = hWnd;

#pragma warning(suppress:4244)
		SetWindowLongPtr(hWnd, 0, (LONG_PTR)webw);

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

#pragma warning(suppress:4312)
	WebWindow *webw = (WebWindow*)GetWindowLongPtr(hWnd, 0);

	if (webw == NULL) {
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return webw->InstanceWndProc(msg, wParam, lParam);
}

LRESULT WebWindow::InstanceWndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_CREATE: {
		webForm->create(hWndWebWindow, hInstWebWindow, 103, showScrollbars);
		HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
		HICON hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MONGSPEAK));
		if (hIcon != NULL) 
			SendMessage(hWndWebWindow, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
		break;
	}
	case WM_SIZE:
		if (webForm->hWnd) {
			MoveWindow(webForm->hWnd, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		}
		break;
	case WM_DESTROY:
	{
		webForm->Close();
		PostQuitMessage(0);
	}
	break;
	}
	return DefWindowProc(hWndWebWindow, msg, wParam, lParam);
}