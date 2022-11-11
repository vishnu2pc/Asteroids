struct GameLayer {
	struct GameState* game_state;
	PlatformAPI platform_api;
	float timer;
	bool quit_request;

	bool debug_cursor_request;

#ifdef INTERNAL
	bool executable_reloaded;
	double ms_per_frame;
#endif
};

#ifdef INTERNAL
extern bool executable_reloaded;
#endif

#define GAME_LOOP(name) void name(GameLayer* game_layer, Win32Window* window, Input* input) 
typedef GAME_LOOP(GameLoop);
