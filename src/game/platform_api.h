struct PlatformMemoryBlock {
	u64 size;
	u8* bp;
};

struct PlatformFileHandle {
	bool failed;
	void* handle;
};

struct PlatformFileInfo {
	u64 size;
	void* name;
};

enum WIN32_BUTTON {
	WIN32_BUTTON_A,
	WIN32_BUTTON_B,
	WIN32_BUTTON_C,
	WIN32_BUTTON_D,
	WIN32_BUTTON_E,
	WIN32_BUTTON_F,
	WIN32_BUTTON_G,
	WIN32_BUTTON_H,
	WIN32_BUTTON_I,
	WIN32_BUTTON_J,
	WIN32_BUTTON_L,
	WIN32_BUTTON_M,
	WIN32_BUTTON_N,
	WIN32_BUTTON_O,
	WIN32_BUTTON_P,
	WIN32_BUTTON_Q,
	WIN32_BUTTON_R,
	WIN32_BUTTON_S,
	WIN32_BUTTON_T,
	WIN32_BUTTON_U,
	WIN32_BUTTON_V,
	WIN32_BUTTON_W,
	WIN32_BUTTON_X,
	WIN32_BUTTON_Y,
	WIN32_BUTTON_Z,

	WIN32_BUTTON_TAB,
	WIN32_BUTTON_SHIFT,
	WIN32_BUTTON_CTRL,
	WIN32_BUTTON_SPACE,
	WIN32_BUTTON_ALT,

	WIN32_BUTTON_UP,
	WIN32_BUTTON_DOWN,
	WIN32_BUTTON_LEFT,
	WIN32_BUTTON_RIGHT,

	WIN32_BUTTON_0,
	WIN32_BUTTON_1,
	WIN32_BUTTON_2,
	WIN32_BUTTON_3,
	WIN32_BUTTON_4,
	WIN32_BUTTON_5,
	WIN32_BUTTON_6,
	WIN32_BUTTON_7,
	WIN32_BUTTON_8,
	WIN32_BUTTON_9,

	WIN32_BUTTON_F0,
	WIN32_BUTTON_F1,
	WIN32_BUTTON_F2,
	WIN32_BUTTON_F3,
	WIN32_BUTTON_F4,
	WIN32_BUTTON_F5,
	WIN32_BUTTON_F6,
	WIN32_BUTTON_F7,
	WIN32_BUTTON_F8,
	WIN32_BUTTON_F9,
	WIN32_BUTTON_F10,
	WIN32_BUTTON_F11,
	WIN32_BUTTON_F12,

	WIN32_BUTTON_ENTER,
	WIN32_BUTTON_ESC,

	WIN32_BUTTON_LEFT_MOUSE,
	WIN32_BUTTON_RIGHT_MOUSE,
	WIN32_BUTTON_MIDDLE_MOUSE,

	WIN32_BUTTON_TOTAL
};

enum WIN32_AXIS {
	WIN32_AXIS_MOUSE,
	WIN32_AXIS_MOUSE_DEL,
	WIN32_AXIS_TOTAL
};

struct WindowDimensions { u32 width; u32 height; };
struct Button { bool pressed, held; };
struct Axis { int x, y; };

struct Input {
	Button buttons[WIN32_BUTTON_TOTAL];
	Axis axes[WIN32_AXIS_TOTAL];
};

struct Win32Window {
	WindowDimensions dim;
	HWND handle;
};

#define PLATFORM_OPEN_FILE(name) PlatformFileHandle name(PlatformFileInfo* info)
typedef PLATFORM_OPEN_FILE(PlatformOpenFile);    

#define PLATFORM_CLOSE_FILE(name) void name(PlatformFileHandle* file_handle)
typedef PLATFORM_CLOSE_FILE(PlatformCloseFile);
     
#define PLATFORM_READ_FILE(name) void name(PlatformFileHandle* win32_handle, u32 size, void *dst)
typedef PLATFORM_READ_FILE(PlatformReadFile);
     
#define PLATFORM_ALLOCATE_MEMORY(name) PlatformMemoryBlock* name(u64 size)
typedef PLATFORM_ALLOCATE_MEMORY(PlatformAllocateMemory);

struct PlatformAPI {
	PlatformOpenFile* open_file;
	PlatformCloseFile* close_file;
	PlatformReadFile* read_file;
	PlatformAllocateMemory* allocate_memory;
};

