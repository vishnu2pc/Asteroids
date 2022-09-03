enum KEYBOARD_BUTTON {
	KB_W,
	KB_A,
	KB_S,
	KB_D,
	KB_Q,
	KB_E,

	KB_SHIFT,
	KB_CTRL,
	KB_ALT,
	KB_SPACE,
	KB_UP,
	KB_DOWN,
	KB_LEFT,
	KB_RIGHT,

	KB_F1,
	KB_F2,
	KB_F3,
	KB_F4,
	KB_F5,
	KB_F6,
	KB_F7,
	KB_F8,
	KB_F9,
	KB_F10,
	KB_F11,
	KB_F12,
	KB_TOTAL
};

enum MOUSE_KEY {
	MK_LEFT,
	MK_MIDDLE,
	MK_RIGHT,
	MK_TOTAL
};

enum MOUSE {
	MOUSE_HORIZONTAL,
	MOUSE_VERTICAL,
	MOUSE_TOTAL
};

struct Button {
	bool held;
	bool pressed;
};

struct Mouse {
	Vec2 coord;
	Vec2 del;
};

struct Input {
	Button kb[KB_TOTAL];
	Button mk[MK_TOTAL];
	Mouse mouse;
};

static void ProcessKeyboardButton(Button* button, SDL_KeyboardEvent event) {
	if(!event.repeat) {
		if(event.type == SDL_KEYDOWN) {
			button->pressed = true;
			button->held = true;
		}
		if(event.type == SDL_KEYUP) {
			button->held = false;
		}
	}
}

static void ProcessMouseButton(Button* button, SDL_MouseButtonEvent event) {
	if(event.type == SDL_MOUSEBUTTONDOWN) {
		button->pressed = true;
		button->held = true;
	}
	if(event.type == SDL_MOUSEBUTTONUP) {
		button->held = false;
	}
}

static void PreProcessButton(Button* button) {
	if(button->held) button->pressed = false;
}

static void PreProcessInput(Input* input) {
	for(u8 i=0; i<KB_TOTAL; i++) PreProcessButton(&input->kb[i]);
	for(u8 i=0; i<MK_TOTAL; i++) PreProcessButton(&input->mk[i]);

	input->mouse.del = V2Z();
}


static void HandleKeyboardEvents(SDL_KeyboardEvent event, Input* input) {
	switch(event.keysym.sym) {
		case SDLK_w: { ProcessKeyboardButton(&input->kb[KB_W], event); } break;
		case SDLK_a: { ProcessKeyboardButton(&input->kb[KB_A], event); } break;
		case SDLK_s: { ProcessKeyboardButton(&input->kb[KB_S], event); } break;
		case SDLK_d: { ProcessKeyboardButton(&input->kb[KB_D], event); } break;
		case SDLK_q: { ProcessKeyboardButton(&input->kb[KB_E], event); } break;
		case SDLK_e: { ProcessKeyboardButton(&input->kb[KB_E], event); } break;

		case SDLK_LSHIFT: { ProcessKeyboardButton(&input->kb[KB_SHIFT], event); } break;
		case SDLK_LCTRL: { ProcessKeyboardButton(&input->kb[KB_CTRL], event); } break;
		case SDLK_SPACE: { ProcessKeyboardButton(&input->kb[KB_SPACE], event); } break;

		case SDLK_UP: { ProcessKeyboardButton(&input->kb[KB_UP], event); } break;
		case SDLK_DOWN: { ProcessKeyboardButton(&input->kb[KB_DOWN], event); } break;
		case SDLK_LEFT: { ProcessKeyboardButton(&input->kb[KB_LEFT], event); } break;
		case SDLK_RIGHT: { ProcessKeyboardButton(&input->kb[KB_RIGHT], event); } break;

		case SDLK_F1: { ProcessKeyboardButton(&input->kb[KB_F1], event); } break;
		case SDLK_F2: { ProcessKeyboardButton(&input->kb[KB_F2], event); } break;
		case SDLK_F3: { ProcessKeyboardButton(&input->kb[KB_F3], event); } break;
		case SDLK_F4: { ProcessKeyboardButton(&input->kb[KB_F4], event); } break;
		case SDLK_F5: { ProcessKeyboardButton(&input->kb[KB_F5], event); } break;
		case SDLK_F6: { ProcessKeyboardButton(&input->kb[KB_F6], event); } break;
		case SDLK_F7: { ProcessKeyboardButton(&input->kb[KB_F7], event); } break;
		case SDLK_F8: { ProcessKeyboardButton(&input->kb[KB_F8], event); } break;
		case SDLK_F9: { ProcessKeyboardButton(&input->kb[KB_F9], event); } break;
		case SDLK_F10: { ProcessKeyboardButton(&input->kb[KB_F10], event); } break;
		case SDLK_F11: { ProcessKeyboardButton(&input->kb[KB_F11], event); } break;
		case SDLK_F12: { ProcessKeyboardButton(&input->kb[KB_F12], event); } break;

	};
};

static void HandleMouseEvents(SDL_Event event, Input* input) {
	switch(event.type) {
		case SDL_MOUSEBUTTONUP: 
		case SDL_MOUSEBUTTONDOWN: {
			switch(event.button.button) {
				case SDL_BUTTON_LEFT: { ProcessMouseButton(&input->mk[MK_LEFT], event.button); } break;
				case SDL_BUTTON_MIDDLE: { ProcessMouseButton(&input->mk[MK_MIDDLE], event.button); } break;
				case SDL_BUTTON_RIGHT: { ProcessMouseButton(&input->mk[MK_RIGHT], event.button); } break;
			};
		}; break;

		case SDL_MOUSEMOTION: {
			input->mouse.coord = V2(event.motion.x, event.motion.y);
			input->mouse.del = V2(event.motion.xrel, event.motion.yrel);
		} break;
	};
}
