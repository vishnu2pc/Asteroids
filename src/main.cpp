#define DEBUG
#define DEBUG_RENDERER

#ifndef DEBUG
#define NDEBUG 							// disables assert 
#endif

#define WIN32_LEAN_AND_MEAN

#include "SDL/SDL.h"
#include "SDL/SDL_syswm.h"
#include "base_types.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define assertHR(result) assert(SUCCEEDED(result))

struct WindowDimensions {	u32 width; u32 height; };

#include "math.cpp"
#include "memory_management.cpp"
#include "input.cpp"
#include "app_state.cpp"

#include "asset_loading.cpp"
#include "font_handling_structs.cpp"
#include "rendering/renderer.cpp"
#include "font_handling.cpp"
#include "camera.cpp"
#include "rendering/renderer_shapes.cpp"
#include "simulation.cpp"

struct Entity {
	Transform transform;
	RenderPipeline rp;
};

int main(int argc, char* argv[]) {
	MemoryArena global_arena = {};
	global_arena.mem = AllocateMemory(Megabytes(200));
	MemoryArena scratch_arena = ExtractMemoryArena(&global_arena, Kilobytes(20));

	stbi_set_flip_vertically_on_load(false);
	GameAssets game_assets = LoadAssetFile(&global_arena);

	AppState* app_state = PushStruct(&global_arena, AppState);
	app_state->running = true;
	app_state->wd = { 1600, 900 };

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window = SDL_CreateWindow("Asteroids", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		app_state->wd.width, app_state->wd.height, 0);
	HWND handle = GetWindowHandleSDL(window);

	SDL_WarpMouseInWindow(window, app_state->wd.width/2, app_state->wd.height/2);
	SDL_FlushEvents(SDL_MOUSEMOTION, SDL_MOUSEWHEEL);

	Renderer* renderer = PushStruct(&global_arena, Renderer);
	InitRendering(renderer, handle, app_state->wd, &game_assets, &global_arena);

	RendererShapes* renderer_shapes = PushStruct(&global_arena, RendererShapes);
	DebugText* dt = PushStruct(&global_arena, DebugText);

	LoadFont("../assets/fonts/JetBrainsMono/jetbrains_mono_light.fi",
		"../assets/fonts/JetBrainsMono/jetbrains_mono_light.png", dt, renderer);

	CameraInfo camera = {};
	camera.position = V3(0.0f, 0.0f, 70.0f);
	camera.rotation = QuatI();
	camera.fov = 75.0f;
	camera.near_clip = 0.1f;
	camera.far_clip = 1000.0f;
	camera.aspect_ratio = (float)app_state->wd.width/(float)app_state->wd.height;
	
	FPControlInfo fpci = { 1.0f, 1.0f, 1.0f, };

	while (app_state->running) {
		BeginAppState(app_state);
		BeginDebugText(dt, app_state->wd);
		BeginRendererShapes(renderer_shapes);
		BeginRendering(renderer);
		if(app_state->input.kb[KB_F1].pressed) renderer->state_overrides.rs = renderer->state_overrides.rs ? RASTERIZER_STATE_NONE : RASTERIZER_STATE_WIREFRAME; 

		if(app_state->input.mk[MK_RIGHT].held) {
			if(app_state->input.kb[KB_SHIFT].held) fpci.block_pitch = true; else fpci.block_pitch = false;
			if(app_state->input.kb[KB_CTRL].held) fpci.block_yaw = true; else fpci.block_yaw = false;

			FirstPersonControl(&camera.position, &camera.rotation, true, fpci, app_state->input);
		}
		Mat4 vp = MakeViewPerspective(camera);
		SubmitRendererShapesDrawCall(renderer_shapes, camera, renderer);

		char* ms = PushArray(&scratch_arena, char, 100);
		sprintf(ms, "ms - %u", SDL_GetTicks());
		DrawDebugText(ms, GREEN, QUADRANT_TOP_LEFT, dt, &scratch_arena);
		CameraDrawDebugText(&camera, dt, &scratch_arena);
		SubmitDebugTextDrawCall(dt, renderer);

		Render(renderer);
		EndRendering(renderer);

		scratch_arena.filled = 0;
	}
	return 0;
}

/* Rendering context implementation
	Two viewports
	Two separate worlds
	Two separate simulations
	Two separate camera constant buffers
	Two separate camera
	Two separate debug contexts
	A "context" struct that carries details about vertex and pixel shaders in play
	Entity sim list that conatins all renderable entities
	Has camera
	camera reads all vertex shaders in play and submits data?
	auto add debug stuff
	common data formats? 
	*/