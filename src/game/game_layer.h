struct GameLayer {
	struct GameState* game_state;
	PlatformAPI platform_api;
	bool quit_request;

	bool debug_cursor_request;

#ifdef INTERNAL
	bool executable_reloaded;
#endif
};

#ifdef INTERNAL
extern bool executable_reloaded;
#endif

#define GAME_LOOP(name) void name(GameLayer* game_layer, Win32Window* window, Input* input) 
typedef GAME_LOOP(GameLoop);
