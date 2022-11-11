#include <windows.h>
#include <stdlib.h>
#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#define AssertHR(result) Assert((result) == 0)

#include "base_types.h"
#include "platform_api.h"
#include "memory_management.h"
#include "math.h"
#include "easings.h"
#include "shapes.cpp"

#define STB_SPRINTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION

#include "include/stb_sprintf.h"
#include "include/stb_image.h"
#include "include/stb_rect_pack.h"
#include "include/stb_truetype.h"

#include "game_layer.h"
#include "asset_formats.h"
#include "file_formats.h"
#include "shader_code.h"
#include "camera.cpp"
#include "renderer.cpp"
#include "quad_renderer.cpp"
#include "mesh_renderer.cpp"
#include "post_process_renderer.cpp"
#include "asset_loading.cpp"
#include "asset_info.cpp"
#include "font_handling.cpp"
#include "ui_renderer.cpp"
#include "simulation.h"

#include "timer.h"
#include "game_mode.h"
#include "game.h"

#include "simulation.cpp"
#include "game_mode.cpp"

PlatformAPI platform_api;
bool pressed = false;

#ifdef INTERNAL
bool executable_reloaded = false;
#endif

extern "C" GAME_LOOP(game_loop) {

#ifdef INTERNAL
	executable_reloaded = game_layer->executable_reloaded;
#endif INTERNAL

	platform_api = game_layer->platform_api;
	GameState* game_state = game_layer->game_state;

	if(!game_layer->game_state) {
		game_state = game_layer->game_state = BootstrapPushStruct(GameState, total_arena, Megabytes(20));

		game_state->frame_arena = (MemoryArena*)BootstrapPushSize_(sizeof(MemoryArena), 0, 0);
		game_state->assets = LoadGameAssets(&game_state->total_arena);
		LoadAllTextureAssets(game_state->assets);
		LoadAllMeshAssets(game_state->assets);

		game_state->renderer = InitRenderer(window, &game_state->total_arena, game_state->frame_arena);

		UploadAllTextureAssets(game_state->assets, game_state->renderer);
		UploadAllMeshAssets(game_state->assets, game_state->renderer);

		game_state->quad_renderer = InitQuadRenderer(game_state->renderer, &game_state->total_arena);
		game_state->mesh_renderer = InitMeshRenderer(game_state->renderer, &game_state->total_arena);
		game_state->post_process_renderer = InitPostProcessRenderer(game_state->renderer, &game_state->total_arena);

		//void* font = GetFont("FiraSans-Li", game_state->assets);
		void* font = GetFont("JetBrainsMo", game_state->assets);

		game_state->text_ui = InitFont(font, window->dim, game_state->renderer, &game_state->total_arena, game_state->frame_arena);
		game_state->ui_renderer = InitUIRenderer(game_state->renderer, window->dim, game_state->text_ui, &game_state->total_arena, game_state->frame_arena);

		game_state->entity_blob.permanent_arena = &game_state->total_arena;
		game_state->entity_blob.frame_arena = game_state->frame_arena;

		game_state->game_mode = GAME_MODE_TEST;

		game_state->frame_arena_temp = BeginTemporaryMemory(game_state->frame_arena);
	}

	EndTemporaryMemory(&game_state->frame_arena_temp);
	game_state->frame_arena_temp = BeginTemporaryMemory(game_state->frame_arena);

	game_state->timer.frame_time = game_layer->timer - game_state->timer.real_time;
	game_state->timer.real_time = game_layer->timer;

	srand(game_state->timer.real_time);

	game_state->camera = DefaultPerspectiveCamera(window->dim, &game_state->total_arena);

	RendererBeginFrame(game_state->renderer, window->dim, game_state->frame_arena_temp.arena);

	if(input->buttons[WIN32_BUTTON_F1].pressed) {
		game_layer->debug_cursor_request = !game_layer->debug_cursor_request;
	}
	if(input->buttons[WIN32_BUTTON_F2].pressed) {
		game_state->dev_mode =(DEV_MODE)(game_state->dev_mode ^ DEV_MODE_PAUSED);
	}

	if(game_state->dev_mode & DEV_MODE_PAUSED) {
		FPControlInfo info = DefaultFPControlInfo();
		info.base_sens = 0.5f; 
		info.trans_sens = 2500.0f;
		info.rot_sens = 10.0f;
		if(input->buttons[WIN32_BUTTON_LEFT_MOUSE].held)
			FirstPersonCamera(game_state->camera, &info, input);
	}

#if 0
	if(game_state->game_mode == GAME_MODE_TEST) {
		UpdateTestMode(game_state, input, window->dim);
	}
#endif

	game_state->camera->position = V3(0.0f, 0.0f, 300.0f);

	TextureAssetInfo* texture_asset = GetTextureAssetInfo("Blue Nebula", game_state->assets);
	Quad quad = {};
	quad.tl = V3(-500.0f, 500.0f, 50.0f);
	quad.tr = V3( 500.0f, 500.0f, 50.0f);
	quad.bl = V3(-500.0f,-500.0f, 50.0f);
	quad.br = V3( 500.0f,-500.0f, 50.0f);

	PushTexturedQuad(&quad, texture_asset->buffer, game_state->quad_renderer);
	char* text[] = { "Play" };
	char buffer[100];

	char text1[100];
	char text2[100];

	stbsp_sprintf(text1, "%.01f: Frame Time", game_state->timer.frame_time);
	stbsp_sprintf(text2, "%0.01f: Game Time ms", game_state->timer.real_time);

	char* info_text[] = { text1, text2 };
	PushUIOverlay(info_text, ArrayCount(info_text), V2Z(), game_state->ui_renderer);

	if(pressed) UpdateTestMode(game_state, input, window->dim);
	else pressed = PushUIButton(text, V2(0.5f, 0.5f), game_state->ui_renderer); 

	QuadRendererFrame(game_state->quad_renderer, game_state->camera, game_state->renderer);
	MeshRendererFrame(game_state->mesh_renderer, game_state->camera, game_state->renderer);

	PostProcessPipeline copy;
	copy.type = POST_PROCESS_TYPE_Copy;
	copy.in = game_state->renderer->readable_render_target.shader_resource;
	copy.out = game_state->renderer->backbuffer.view;

	Vec2 mouse_pos = *(Vec2*)&input->axes[WIN32_AXIS_MOUSE];
	stbsp_sprintf(buffer, "%.02f: Mouse x", mouse_pos.x);
	//PushTextScreenSpace(buffer, 40.0f, V2Z(), game_state->text_ui);

	stbsp_sprintf(buffer, "%0.02f: Mouse y", mouse_pos.y);
	//PushTextScreenSpace(buffer, 60.0f, V2(0.0f, 0.5f), game_state->text_ui);

	PushPostProcessPipeline(&copy, game_state->post_process_renderer, game_state->renderer);
	PostProcessRendererFrame(game_state->post_process_renderer, game_state->renderer);
	UIRendererFrame(input, game_state->ui_renderer, game_state->renderer);
	UITextFrame(game_state->text_ui, window->dim, game_state->renderer);
	RendererEndFrame(game_state->renderer);

	//CheckArena(game_state->frame_arena);
}


