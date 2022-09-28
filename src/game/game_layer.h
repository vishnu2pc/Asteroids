struct GameLayer {
	struct Game* game;
	PlatformAPI* platform_api;
	bool quit_requested;
};

#define GAME_INIT(name) void name(GameLayer* game_layer, Win32Window* window)
typedef GAME_INIT(GameInit);

#define GAME_LOOP(name) void name(GameLayer* game_layer, Win32Window* window, Input* input) 
typedef GAME_LOOP(GameLoop);
