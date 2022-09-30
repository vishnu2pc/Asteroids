struct GameLayer {
	struct Game* game;
	PlatformAPI* platform_api;
	bool quit_request;
	bool debug_cursor_request;
};

// app window states
// running game state -> hide cursor, capture cursor, teleport cursor to center of screen every frame
// debug game state with overlays -> show cursor, capture cursor

#define GAME_INIT(name) void name(GameLayer* game_layer, Win32Window* window)
typedef GAME_INIT(GameInit);

#define GAME_LOOP(name) void name(GameLayer* game_layer, Win32Window* window, Input* input) 
typedef GAME_LOOP(GameLoop);
