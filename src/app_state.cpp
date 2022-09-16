struct AppState {
	bool running;
	WindowDimensions wd;
	Input input;
};

static HWND GetWindowHandleSDL(SDL_Window* window) {
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	return hwnd;
}
//------------------------------------------------------------------------
static void HandleSDLevents(AppState* app_state) {
	SDL_Event event;
	SDL_PollEvent(&event);
	switch (event.type) {
		case SDL_QUIT: {
			app_state->running = false;
		} break;
		case SDL_KEYUP: 
		case SDL_KEYDOWN: { 
			HandleKeyboardEvents(event.key, &app_state->input);
		} break;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEWHEEL: {
			HandleMouseEvents(event, &app_state->input);
		} break;
	}
}
//------------------------------------------------------------------------
static void BeginAppState(AppState* app_state) {
	PreProcessInput(&app_state->input);
	HandleSDLevents(app_state);
}