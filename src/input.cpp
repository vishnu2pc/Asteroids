struct KeyboardKey {
	bool tapped;
	bool pressed;
};

// TODO: Tap aint working
struct MouseKey {
	bool tapped;
	bool pressed;
};

struct MouseMove {
	int val;
	int del;
};

struct Input {
	KeyboardKey up;
	KeyboardKey down;
	KeyboardKey left;
	KeyboardKey right;

	KeyboardKey shift;
	KeyboardKey alt;
	KeyboardKey ctrl;

	KeyboardKey w;
	KeyboardKey a;
	KeyboardKey s;
	KeyboardKey d;

	MouseKey lm;
	MouseKey rm;
	MouseKey mm;

	MouseMove mx;
	MouseMove my;

	int mw;
};

static Input HINPUT = {};

static inline void ProcessKeyboadKey(KeyboardKey* key, SDL_Event event) {
	switch (event.key.state) {
		case SDL_PRESSED: {
			key->pressed = true;
		} break;
		case SDL_RELEASED: {
			key->pressed = false;
		} break;
	}
}

static inline void ProcessMouseKey(MouseKey* key, SDL_Event event) {
	switch (event.button.state) {
		case SDL_PRESSED: {
			key->pressed = true;
		} break;
		case SDL_RELEASED: {
			key->pressed = false;
		} break;
	};
}

static inline void HandleKeyboardEvents(SDL_Event event) {
	switch (event.key.keysym.sym) {
		case SDLK_UP: { ProcessKeyboadKey(&HINPUT.up, event); } break;
		case SDLK_DOWN: { ProcessKeyboadKey(&HINPUT.down, event); } break;
		case SDLK_LEFT: { ProcessKeyboadKey(&HINPUT.left, event); } break;
		case SDLK_RIGHT: { ProcessKeyboadKey(&HINPUT.right, event); } break;
		case SDLK_LSHIFT: { ProcessKeyboadKey(&HINPUT.shift, event); } break;
		case SDLK_LALT: { ProcessKeyboadKey(&HINPUT.alt, event); } break;
		case SDLK_LCTRL: { ProcessKeyboadKey(&HINPUT.ctrl, event); } break;
		case SDLK_w: { ProcessKeyboadKey(&HINPUT.w, event); } break;
		case SDLK_a: { ProcessKeyboadKey(&HINPUT.a, event); } break;
		case SDLK_s: { ProcessKeyboadKey(&HINPUT.s, event); } break;
		case SDLK_d: { ProcessKeyboadKey(&HINPUT.d, event); } break;

	}
}

static inline void HandleMouseEvents(SDL_Event event) {
	switch (event.type) {
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN: {
			switch (event.button.which) {
				case SDL_BUTTON_LEFT: { ProcessMouseKey(&HINPUT.lm, event); } break;
				case SDL_BUTTON_RIGHT: { ProcessMouseKey(&HINPUT.rm, event); } break;
				case SDL_BUTTON_MIDDLE: { ProcessMouseKey(&HINPUT.mm, event); } break;
			};
		} break;

		case SDL_MOUSEWHEEL: {
			HINPUT.mw = event.wheel.y;
		} break;
		case SDL_MOUSEMOTION: {
			HINPUT.mx.val =  event.motion.x;
			HINPUT.mx.del =  event.motion.xrel;
			HINPUT.my.val =  event.motion.y;
			HINPUT.my.del =  event.motion.yrel;
		}
	}
}