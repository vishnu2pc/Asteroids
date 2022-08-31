enum KEYBOARD_KEY {
	KBK_W,
	KBK_A,
	KBK_S,
	KBK_D,
	KBK_SHIFT,
	KBK_CTRL,
	KBK_ALT,
	KBK_SPACE,
	KBK_UP,
	KBK_DOWN,
	KBK_LEFT,
	KBK_RIGHT,

	KBK_F1,
	KBK_F2,
	KBK_F3,
	KBK_F4,
	KBK_F5,
	KBK_F6,
	KBK_F7,
	KBK_F8,
	KBK_F9,
	KBK_F10,
	KBK_F11,
	KBK_F12,

	KBK_TOTAL
};

enum MOUSE_KEY {
	MK_LEFT,
	MK_MIDDLE,
	MK_RIGHT,
	MK_TOTAL
};

enum MOUSE_MOVE {
	MM_HORIZONTAL,
	MM_VERTICAL,
	MM_TOTAL
};

struct KeyboardKey {
	bool held;
	bool pressed;
};

struct MouseKey {
	bool pressed;
};

struct MouseMove {
	Vec2 coord;
	Vec2 del;
};

struct Input {
	KeyboardKey kb[KBK_TOTAL];
	MouseKey mk[MK_TOTAL];
	MouseMove mm;
};

static void ProcessKeyboardKey(KeyboardKey* key, SDL_KeyboardEvent event) {
	if(!event.repeat) {
		if(event.type == SDL_KEYDOWN) {
			key->pressed = true;
			key->held = true;
		}
		if(event.type == SDL_KEYUP) {
			key->held = false;
		}
	}
}

static void PreProcessKeyboardKey(KeyboardKey* key) {
	if(key->held) key->pressed = false;
}

static void PreProcessInput(Input* input) {
	for(u8 i=0; i<KBK_TOTAL; i++) PreProcessKeyboardKey(&input->kb[i]);
}


static void HandleKeyboardEvents(SDL_KeyboardEvent event, Input* input) {
	switch(event.keysym.sym) {
		case SDLK_w: { ProcessKeyboardKey(&input->kb[KBK_W], event); } break;
		case SDLK_a: { ProcessKeyboardKey(&input->kb[KBK_A], event); } break;
		case SDLK_s: { ProcessKeyboardKey(&input->kb[KBK_S], event); } break;
		case SDLK_d: { ProcessKeyboardKey(&input->kb[KBK_D], event); } break;

		case SDLK_LSHIFT: { ProcessKeyboardKey(&input->kb[KBK_SHIFT], event); } break;
		case SDLK_LCTRL: { ProcessKeyboardKey(&input->kb[KBK_CTRL], event); } break;
		case SDLK_SPACE: { ProcessKeyboardKey(&input->kb[KBK_SPACE], event); } break;

		case SDLK_UP: { ProcessKeyboardKey(&input->kb[KBK_UP], event); } break;
		case SDLK_DOWN: { ProcessKeyboardKey(&input->kb[KBK_DOWN], event); } break;
		case SDLK_LEFT: { ProcessKeyboardKey(&input->kb[KBK_LEFT], event); } break;
		case SDLK_RIGHT: { ProcessKeyboardKey(&input->kb[KBK_RIGHT], event); } break;

		case SDLK_F1: { ProcessKeyboardKey(&input->kb[KBK_F1], event); } break;
		case SDLK_F2: { ProcessKeyboardKey(&input->kb[KBK_F2], event); } break;
		case SDLK_F3: { ProcessKeyboardKey(&input->kb[KBK_F3], event); } break;
		case SDLK_F4: { ProcessKeyboardKey(&input->kb[KBK_F4], event); } break;
		case SDLK_F5: { ProcessKeyboardKey(&input->kb[KBK_F5], event); } break;
		case SDLK_F6: { ProcessKeyboardKey(&input->kb[KBK_F6], event); } break;
		case SDLK_F7: { ProcessKeyboardKey(&input->kb[KBK_F7], event); } break;
		case SDLK_F8: { ProcessKeyboardKey(&input->kb[KBK_F8], event); } break;
		case SDLK_F9: { ProcessKeyboardKey(&input->kb[KBK_F9], event); } break;
		case SDLK_F10: { ProcessKeyboardKey(&input->kb[KBK_F10], event); } break;
		case SDLK_F11: { ProcessKeyboardKey(&input->kb[KBK_F11], event); } break;
		case SDLK_F12: { ProcessKeyboardKey(&input->kb[KBK_F12], event); } break;

	};
};

static void HandleMouseEvents(SDL_Event event, Input* input) {
	switch(event.type) {
		case SDL_MOUSEBUTTONUP: 
		case SDL_MOUSEBUTTONDOWN: {
			bool pressed;
			event.button.type == SDL_MOUSEBUTTONDOWN ? pressed = true : pressed = false;
			if(event.button.button == SDL_BUTTON_LEFT) pressed ? input->mk[MK_LEFT].pressed = true : input->mk[MK_LEFT].pressed = false;
			if(event.button.button == SDL_BUTTON_MIDDLE) pressed ? input->mk[MK_MIDDLE].pressed = true : input->mk[MK_MIDDLE].pressed = false;
			if(event.button.button == SDL_BUTTON_RIGHT) pressed ? input->mk[MK_RIGHT].pressed = true : input->mk[MK_RIGHT].pressed = false;
		}; break;

		case SDL_MOUSEMOTION: {
			input->mm.coord = V2(event.motion.x, event.motion.y);
			input->mm.del = V2(event.motion.xrel, event.motion.yrel);
		} break;
	};
}
