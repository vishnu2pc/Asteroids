struct AppState {
	bool running;
	WindowDimensions wd;
	Input input;
	u32 frame_time_begin;
	u32 frame_time_end;
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
	app_state->frame_time_begin = SDL_GetTicks();
	PreProcessInput(&app_state->input);
	HandleSDLevents(app_state);
}
static void EndAppState(AppState* app_state) {
	app_state->frame_time_end = SDL_GetTicks();

}
