struct Win32DLL { // InOut struct
	bool valid;
	HMODULE dll;
	FILETIME last_write_time; 
	u32 transient_dll_counter;
	// These need to be passed in
	char* absfilepath;
	char* lock_absfilepath;
	char* transient_dll_name;
	void** functions;
	char** function_names;
	u8 function_count;
};

char* win32_game_function_names[] = {
	"game_init",
	"game_loop",
};

struct Win32GameFunctionTable {
	GameInit* game_init;
	GameLoop* game_loop;
};

struct Win32State {
	char exe_absfilepath[MAX_PATH];
	char exe_absfolderpath[MAX_PATH];
	HWND default_window_handle;
};

