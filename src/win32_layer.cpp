#define WIN32_LEAN_AND_MEAN
#include "base_types.h"
#include "windows.h"

#define assertHR(result) assert(SUCCEEDED(result))

LRESULT Win32WindowProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
	LRESULT result = {};
	switch(msg) {
		case WM_SIZE: {
		} break;
		case WM_DESTROY: {
		} break;
		case WM_CLOSE: {
		} break;
		case WM_ACTIVATEAPP: {
		} break;
		default: {
			result = DefWindowProc(window, msg, wparam, lparam);
		} break;
	}
	return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdline, int show_code) {
	HRESULT hr = {};
	WNDCLASS window_class = {};

	char window_name[] = "Game";
	char class_name[] = "Game";

	window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	window_class.lpfnWndProc = Win32WindowProc;
	window_class.hInstance = instance;
	window_class.lpszMenuName = window_name;
	window_class.lpszClassName = class_name;

	//NOTE: GetLastError for error codes
	assert(RegisterClassA(&window_class));

	DWORD extended_styles = WS_EX_OVERLAPPEDWINDOW;
	DWORD styles = WS_OVERLAPPEDWINDOW;
	int pos_x = CW_USEDEFUALT;
	int pos_y = CW_USEDEFUALT;
	int width = CW_USEDEFAULT;
	int height = CW_USEDEFUALT;
	// NOTE: Start here for multiple windows
	HWND window = CreateWindowExA(styles, class_name, window_name, styles, x, y, width, height, NULL,
	                     NULL, instance, NULL);
	assert(window);

	bool running = true;
	while(running) {
		MSG msg = {};
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		MSG msg
	}

	return 1;
}