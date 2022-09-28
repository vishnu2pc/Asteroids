#include <windows.h>
#include <windowsx.h>

#include "base_types.h"
#include "platform_api.h"
#include "memory_management.h"

#include "game_layer.h"
#include "win32.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "include/stb_sprintf.h"

PlatformAPI win32_api;
global bool g_running;
global Win32State g_win32_state;
global Win32Window g_win32_window;
global Win32GameFunctionTable g_game_functions;

//------------------------------------------------------------------------------------------------------------------
static FILETIME
Win32GetLastWriteTime(char* absfilepath) {
	FILETIME result = {};
	WIN32_FILE_ATTRIBUTE_DATA file_attribute = {};
	Assert(GetFileAttributesExA(absfilepath, GetFileExInfoStandard, &file_attribute)); 
	result = file_attribute.ftLastWriteTime;
	return result;
}
//------------------------------------------------------------------------------------------------------------------
static WindowDimensions 
Win32GetWindowDimensions(HWND window) {
	WindowDimensions wd = {};
	RECT rect;
	GetClientRect(window, &rect);
	wd.width = (u32)(rect.right  - rect.left);
	wd.height = (u32)(rect.bottom - rect.top);
	return wd;
}
//------------------------------------------------------------------------------------------------------------------
static void
Win32MakeTempDLLAbsFilePath(Win32State* state, Win32DLL* code, char* dst) {
	stbsp_sprintf(dst, "%s%d_%s", state->exe_absfolderpath, code->transient_dll_counter++, code->transient_dll_name);
	if(code->transient_dll_counter >= 1024) code->transient_dll_counter = 0;
}
//------------------------------------------------------------------------------------------------------------------
static void 
Win32UnloadDLL(Win32DLL* code) {
	FreeLibrary(code->dll);
	code->valid = false;
	code->dll = 0;
	ZeroArray(code->functions, code->function_count);
}
//-------------------------------------------------------------------------------------------------------------------
static void 
Win32LoadDLL(Win32State* state, Win32DLL* code) {
	char temp_dll_absfilepath[MAX_PATH] = {};
	WIN32_FILE_ATTRIBUTE_DATA data;
	if(!GetFileAttributesExA(code->lock_absfilepath, GetFileExInfoStandard, &data)) {
		code->last_write_time = Win32GetLastWriteTime(code->absfilepath);
		for(u32 tries=0; tries<128; tries++) {
			Win32MakeTempDLLAbsFilePath(state, code, temp_dll_absfilepath);
			if(CopyFile(code->absfilepath, temp_dll_absfilepath, FALSE)) break;
		}

		code->dll = LoadLibraryA(temp_dll_absfilepath);
		if(code->dll) {
			code->valid = true;
			for(u8 i=0; i<code->function_count; i++) {
				void* func = GetProcAddress(code->dll, code->function_names[i]);
				if(func) code->functions[i] = func;
				else code->valid = false;
			}
		}

	}
	if(!code->valid) Win32UnloadDLL(code);
}
//-------------------------------------------------------------------------------------------------------------------
static void
Win32ReloadDLL(Win32State* state, Win32DLL* code) {
	Win32UnloadDLL(code);
	for(u32 tries=0; tries<100; tries++) {
		if(!code->valid) {
			Win32LoadDLL(state, code);
			Sleep(100);
		}
	}
}
//-------------------------------------------------------------------------------------------------------------------
static bool 
Win32HasDLLChanged(Win32DLL* code) {
	FILETIME ft = Win32GetLastWriteTime(code->absfilepath);
	return CompareFileTime(&ft, &code->last_write_time);
}
//-------------------------------------------------------------------------------------------------------------------
static LRESULT CALLBACK
Win32MainWindowCallback(HWND window, UINT msg, WPARAM wparam, LPARAM lparam) {
	LRESULT result = 0;
	switch(msg) {
		case WM_CLOSE: { g_running = false; } break;
		case WM_DESTROY: { g_running = false; } break;
		case WM_SIZE: { 
			g_win32_window.dim.width = LOWORD(lparam);
			g_win32_window.dim.height = HIWORD(lparam);
		} break;
		default: { result = DefWindowProcA(window, msg, wparam, lparam); } break;
	}
	return result;
}
//-------------------------------------------------------------------------------------------------------------------
static void
Win32PreProcessButton(Button* button) {
	if(button->pressed) button->held = true;
}
//-------------------------------------------------------------------------------------------------------------------
static void
Win32ProcessButton(Button* button, bool is_down) {
	if(is_down) button->pressed = true;
	else {
		button->pressed = false;
		button->held = false;
	}
}
//-------------------------------------------------------------------------------------------------------------------
static PLATFORM_ALLOCATE_MEMORY(win32_allocate_memory) {
	PlatformMemoryBlock* block;
	u64 page_size = 4096; 
	u64 total_size = sizeof(PlatformMemoryBlock) + size;
	u64 offset = sizeof(PlatformMemoryBlock);

	block = (PlatformMemoryBlock*)VirtualAlloc(0, total_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE); 
	Assert(block);
	block->bp = (u8*)block + offset;
	block->size = total_size;
	return block;
}
//-------------------------------------------------------------------------------------------------------------------
static PLATFORM_OPEN_FILE(win32_open_file) {
	PlatformFileHandle result = {};
	char* filename = (char*)info->name;

	HANDLE handle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	LARGE_INTEGER li = {};
	GetFileSizeEx(handle, &li);
	info->size = li.QuadPart;

	result.failed = handle == INVALID_HANDLE_VALUE;
	*(HANDLE*)&result.handle = handle;

	return result;
}
//-------------------------------------------------------------------------------------------------------------------
static PLATFORM_CLOSE_FILE(win32_close_file) {
	Assert(!file_handle->failed);
	HANDLE handle = *(HANDLE*)&file_handle->handle;
	CloseHandle(handle);
}
//-------------------------------------------------------------------------------------------------------------------
static PLATFORM_READ_FILE(win32_read_file) {
	Assert(!win32_handle->failed);
	HANDLE handle = *(HANDLE*)&win32_handle->handle;
	Assert(ReadFile(handle, dst, size, 0, 0)); 
}
//-------------------------------------------------------------------------------------------------------------------
static void
Win32ProcessButtonInput(MSG msg, Input* input) {
	bool is_key_down = msg.message == WM_KEYDOWN ? true : false; 
	u32 VKCode = (u32)msg.wParam;
	bool is_down = ((msg.lParam & (1UL << 31)) == 0);

	if(VKCode == 'W') Win32ProcessButton(&input->buttons[WIN32_BUTTON_W], is_down); 
	if(VKCode == 'A') Win32ProcessButton(&input->buttons[WIN32_BUTTON_A], is_down); 
	if(VKCode == 'S') Win32ProcessButton(&input->buttons[WIN32_BUTTON_S], is_down); 
	if(VKCode == 'D') Win32ProcessButton(&input->buttons[WIN32_BUTTON_D], is_down); 
	if(VKCode == 'Q') Win32ProcessButton(&input->buttons[WIN32_BUTTON_Q], is_down); 
	if(VKCode == 'E') Win32ProcessButton(&input->buttons[WIN32_BUTTON_E], is_down); 

	if(VKCode == VK_SHIFT) Win32ProcessButton(&input->buttons[WIN32_BUTTON_SHIFT], is_down);
	if(VKCode == VK_LMENU) Win32ProcessButton(&input->buttons[WIN32_BUTTON_ALT], is_down);
	if(VKCode == VK_SPACE) Win32ProcessButton(&input->buttons[WIN32_BUTTON_SPACE], is_down);
	if(VKCode == VK_CONTROL) Win32ProcessButton(&input->buttons[WIN32_BUTTON_B], is_down);

	if(VKCode == VK_F1) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F1], is_down);
	if(VKCode == VK_F2) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F2], is_down);
	if(VKCode == VK_F3) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F3], is_down);
	if(VKCode == VK_F4) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F4], is_down);
	if(VKCode == VK_F5) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F5], is_down);
	if(VKCode == VK_F6) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F6], is_down);
	if(VKCode == VK_F7) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F7], is_down);
	if(VKCode == VK_F8) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F8], is_down);
	if(VKCode == VK_F9) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F9], is_down);
	if(VKCode == VK_F10) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F10], is_down);
	if(VKCode == VK_F11) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F11], is_down);
	if(VKCode == VK_F12) Win32ProcessButton(&input->buttons[WIN32_BUTTON_F12], is_down);

	if(msg.message == WM_LBUTTONDOWN) Win32ProcessButton(&input->buttons[WIN32_BUTTON_LEFT_MOUSE], true);
	if(msg.message == WM_LBUTTONUP) Win32ProcessButton(&input->buttons[WIN32_BUTTON_RIGHT_MOUSE], false);
	if(msg.message == WM_RBUTTONDOWN) Win32ProcessButton(&input->buttons[WIN32_BUTTON_RIGHT_MOUSE], true);
	if(msg.message == WM_RBUTTONUP) Win32ProcessButton(&input->buttons[WIN32_BUTTON_RIGHT_MOUSE], false);
}
//-------------------------------------------------------------------------------------------------------------------
static void
Win32PreProcessMouseMove(Input* input) {
	input->axes[WIN32_AXIS_MOUSE_DEL].x = 0;
	input->axes[WIN32_AXIS_MOUSE_DEL].y = 0;
}
//-------------------------------------------------------------------------------------------------------------------
static void
Win32ProcessMouseMove(MSG msg, Input* input) {
	int x_pos = GET_X_LPARAM(msg.lParam);
	int y_pos = GET_Y_LPARAM(msg.lParam);
	int old_x_pos = input->axes[WIN32_AXIS_MOUSE].x;
	int old_y_pos = input->axes[WIN32_AXIS_MOUSE].y;
	input->axes[WIN32_AXIS_MOUSE_DEL].x = x_pos - old_x_pos;
	input->axes[WIN32_AXIS_MOUSE_DEL].y = y_pos - old_y_pos;
	input->axes[WIN32_AXIS_MOUSE].x = x_pos;
	input->axes[WIN32_AXIS_MOUSE].y = y_pos;
}
//-------------------------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------------
int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmdline, int show_code) {
	HRESULT hr = {};

	WNDCLASS window_class = {};

	window_class.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	window_class.lpfnWndProc = Win32MainWindowCallback;
	window_class.hInstance = instance;
	window_class.lpszMenuName = "Game_menu";
	window_class.lpszClassName = "Game_class";
	window_class.hCursor = LoadCursor(0, IDC_ARROW);

	Assert(RegisterClassA(&window_class));

	HWND window = CreateWindowExA(0, window_class.lpszClassName, "Game", 
			WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 
			CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, instance, 0);
	Assert(window);

	g_win32_state.default_window_handle = window;
	GetModuleFileNameA(0, g_win32_state.exe_absfilepath, sizeof(g_win32_state.exe_absfilepath));

	u32 exe_folder_path_len = 0;
	for(u32 i=0; g_win32_state.exe_absfilepath[i] != 0; i++)
		if(g_win32_state.exe_absfilepath[i] == '\\') exe_folder_path_len = i+1;
	CopyMem(g_win32_state.exe_absfolderpath, g_win32_state.exe_absfilepath, exe_folder_path_len);

	char temp_exe_absfilepath[MAX_PATH]     = {};
	char old_exe_absfilepath[MAX_PATH]      = {};
	char game_dll_absfilepath[MAX_PATH]     = {};
	char lock_absfilepath[MAX_PATH]         = {};
	char renderer_dll_absfilepath[MAX_PATH] = {};
	{
		char* temp_exe_filename     = "win32_entry_temp.exe";
		char* old_exe_filename      = "win32_entry_old.exe";
		char* game_dll_filename     = "game.dll";
		char* renderer_dll_filename = "win32_d3d11.dll";
		char* lock_filename         = "lock.tmp";

		stbsp_sprintf(temp_exe_absfilepath     , "%s%s" , g_win32_state.exe_absfolderpath , temp_exe_filename);
		stbsp_sprintf(old_exe_absfilepath      , "%s%s" , g_win32_state.exe_absfolderpath , old_exe_filename);
		stbsp_sprintf(game_dll_absfilepath     , "%s%s" , g_win32_state.exe_absfolderpath , game_dll_filename);
		stbsp_sprintf(renderer_dll_absfilepath , "%s%s" , g_win32_state.exe_absfolderpath , renderer_dll_filename);
		stbsp_sprintf(lock_absfilepath         , "%s%s" , g_win32_state.exe_absfolderpath , lock_filename);
	}

	g_running = true;
	Input input = {};


	Win32DLL game_code           = {};
	game_code.transient_dll_name = "game_temp.dll";
	game_code.absfilepath        = game_dll_absfilepath;
	game_code.lock_absfilepath   = lock_absfilepath;
	game_code.function_names     = win32_game_function_names;
	game_code.functions          = (void**)&g_game_functions;
	game_code.function_count     = ArrayCount(win32_game_function_names);

	Win32LoadDLL(&g_win32_state, &game_code);

	win32_api.open_file       = win32_open_file;
	win32_api.close_file      = win32_close_file;
	win32_api.read_file       = win32_read_file;
	win32_api.allocate_memory = win32_allocate_memory;

	GameLayer game_layer = {};
	game_layer.platform_api = &win32_api;

	g_win32_window.handle = window;
	g_win32_window.dim = Win32GetWindowDimensions(window);

	g_game_functions.game_init(&game_layer, &g_win32_window);

	//--------------------------------------------------------------------------------//
	while(g_running) {
		for(u8 i=0; i<WIN32_BUTTON_TOTAL; i++) Win32PreProcessButton(&input.buttons[i]);
		Win32PreProcessMouseMove(&input);

		MSG msg = {};
		while(PeekMessageA(&msg, 0, 0, 0, PM_REMOVE)) {
			switch(msg.message) {
				case WM_QUIT: { g_running = false; } break;
				case WM_SYSKEYDOWN:
				case WM_SYSKEYUP:
				case WM_KEYUP:
				case WM_KEYDOWN:
				case WM_LBUTTONDOWN:
				case WM_RBUTTONDOWN: {
					Win32ProcessButtonInput(msg, &input);
				} break;
				case WM_MOUSEMOVE: {
					Win32ProcessMouseMove(msg, &input);
				} break;

				default: {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				} break;
			}
		}
		g_game_functions.game_loop(&game_layer, &g_win32_window, &input);

		if(Win32HasDLLChanged(&game_code)) Win32ReloadDLL(&g_win32_state, &game_code); 

		if(game_layer.quit_requested) g_running = false;
	}
	return 1;
}

