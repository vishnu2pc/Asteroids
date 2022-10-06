#include <windows.h>
#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#define AssertHR(result) Assert((result) == 0)

#include "base_types.h"
#include "platform_api.h"
#include "memory_management.h"
#include "math.h"
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
#include "asset_loading.cpp"
#include "asset_info.cpp"
#include "font_handling.cpp"
#include "simulation.h"
#include "simulation.cpp"

#include "game.h"

PlatformAPI platform_api;

#ifdef INTERNAL
bool executable_reloaded = false;
#endif

extern "C" GAME_LOOP(game_loop) {

#ifdef INTERNAL
	executable_reloaded = game_layer->executable_reloaded;
#endif INTERNAL

	platform_api = game_layer->platform_api;
	GameState* game_state = game_layer->game_state;

	Mesh tetra;

	if(!game_layer->game_state) {
		game_state = game_layer->game_state = BootstrapPushStruct(GameState, total_arena, Megabytes(20));

		game_state->frame_arena = (MemoryArena*)BootstrapPushSize_(sizeof(MemoryArena), 0, 0);
		game_state->assets = LoadGameAssets(&game_state->total_arena);
		LoadAllTextureAssets(game_state->assets);

		game_state->renderer = InitRenderer(window, &game_state->total_arena, game_state->frame_arena);

		UploadAllTextureAssets(game_state->assets, game_state->renderer);

		game_state->quad_renderer = InitQuadRenderer(game_state->renderer, &game_state->total_arena);
		game_state->mesh_renderer = InitMeshRenderer(game_state->renderer, &game_state->total_arena);

		game_state->camera = DefaultCamera(window->dim, &game_state->total_arena);

		void* font = GetFont("DejaVuSans", game_state->assets);

		game_state->text_ui = InitFont(font, window->dim, game_state->renderer, &game_state->total_arena);

		tetra = MakeTetrahedronMesh(game_state->renderer);

		Transform transform = TransformI();
		game_state->entity = SpawnSimpleEntity(tetra, transform, game_state->renderer, &game_state->total_arena);
	}

	game_state->frame_arena_temp = BeginTemporaryMemory(game_state->frame_arena);
	RendererBeginFrame(game_state->renderer, window->dim, game_state->frame_arena_temp.arena);

	if(input->buttons[WIN32_BUTTON_F1].pressed) {
		game_layer->debug_cursor_request = !game_layer->debug_cursor_request;
	}

	if(input->buttons[WIN32_BUTTON_LEFT_MOUSE].held) {
		FPControlInfo info = DefaultFPControlInfo();
		info.base_sens = 0.5f; 
		info.trans_sens = 1.0f;
		info.rot_sens = 10.0f;
		FirstPersonCamera(game_state->camera, &info, input);
	}

	LightInfo light = { V3(10.0f, 10.0f, 10.0f), 0.2f };

	UpdateEntity(game_state->entity, game_state->mesh_renderer, game_state->frame_arena);

	Quad quad = { V3(5.0f, 5.0f, -8.5f), V3(5.0f, -5.0f, -8.5f),
		V3(-5.0f, 5.0f, -8.5f), V3(-5.0f, -5.0f, -8.5f) };
	Quad quad1 = { V3(5.0f, 5.0f, -5.5f), V3(5.0f, -5.0f, -5.5f),
		V3(-5.0f, 5.0f, -5.5f),V3(-5.0f, -5.0f, -5.5f) };

	Line line = { V3(2.0f, 0.0f, 0.0f), V3(10.0f, 0.0f, 0.0f) };

	PushRenderQuad(&quad, V4FromV3(TEAL, 1.0f), game_state->quad_renderer);

	TextureAssetInfo* texture_asset = GetTextureAssetInfo("Blue Nebula", game_state->assets);
	TextureAssetInfo* texture_asset1 = GetTextureAssetInfo("Green Nebul", game_state->assets);

	PushTexturedQuad(&quad1, texture_asset1->buffer, game_state->quad_renderer);

	char* a = (char*)PushSize(game_state->frame_arena, 100);
	char* b = (char*)PushSize(game_state->frame_arena, 100);
	stbsp_sprintf(a, "Mouse x: %f", input->axes[WIN32_AXIS_MOUSE_DEL].x);
	stbsp_sprintf(b, "Mouse y: %f", input->axes[WIN32_AXIS_MOUSE_DEL].y);

	PushText(a, game_state->text_ui);
	PushText(b, game_state->text_ui);

	QuadRendererFrame(game_state->quad_renderer, game_state->camera, game_state->renderer);
	MeshRendererFrame(game_state->mesh_renderer, game_state->camera, &light, game_state->renderer);
	UIFrame(game_state->text_ui, window->dim, game_state->renderer);
	RendererEndFrame(game_state->renderer);

	EndTemporaryMemory(&game_state->frame_arena_temp);
	CheckArena(game_state->frame_arena);
}

