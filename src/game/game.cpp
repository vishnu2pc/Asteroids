#include <windows.h>
#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#define AssertHR(result) Assert((result) == 0)

#include "base_types.h"
#include "memory_management.h"
#include "math.h"
#include "shapes.cpp"

#define STB_SPRINTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "include/stb_sprintf.h"
#include "include/stb_image.h"

#include "platform_api.h"
#include "game_layer.h"

#include "asset_loading.cpp"
#include "renderer.cpp"

#include "asset_info.cpp"

#include "font_handling.cpp"
#include "simulation.h"
#include "simulation.cpp"
#include "camera.cpp"
#include "quad_renderer.cpp"
#include "mesh_renderer.cpp"

#include "tetra.cpp"
#include "game.h"

extern "C" GAME_INIT(game_init) {

	PlatformAPI* platform_api = game_layer->platform_api;
	PlatformMemoryBlock* memblock = platform_api->allocate_memory(sizeof(Game) + Megabytes(30));

	game_layer->game = (Game*)memblock->bp;
	Game* game = game_layer->game;

	game->permanent.memory.bp   = memblock->bp;
	game->permanent.memory.size = Megabytes(25);
	game->permanent.used = sizeof(Game);

	game->transient.memory.bp   = memblock->bp + Megabytes(25); 
	game->transient.memory.size = Megabytes(5);

	AssetInfo* asset_info = InitAssetInfo(&game->permanent);
	game->asset_info = asset_info;

	GameAssets* assets = InitGameAssets(platform_api, &game->permanent);
	game->assets = assets;

	LoadAllTextureAssets(assets, asset_info);

	Renderer* renderer = InitRenderer(window, assets, &game->permanent);
	game->renderer = renderer;

	UploadAllTextureAssets(asset_info, renderer);

  game->debug_text = InitDebugText(platform_api, renderer, &game->permanent);

	game->camera = DefaultCamera(window->dim, &game->permanent);
	//game->renderer_shapes = InitRendererShapes(&game->permanent);
	game->quad_renderer = InitQuadRenderer(renderer, &game->permanent);

	game->mesh_renderer = InitMeshRenderer(renderer, &game->permanent);

	game->tetra = InitTetra(renderer, &game->permanent);
	Transform transform = TransformI();
	transform.position = V3(5.0f, 0.0f, 0.0f);
	SpawnTetra(game->tetra, TransformI(), V4FromV3(RED, 0.5f));
	SpawnTetra(game->tetra, transform, V4FromV3(GREEN, 0.5f));
}

static void
GameBegin(Game* game, Win32Window* window) {
	//BeginRendererShapes(game->renderer_shapes);
	BeginDebugText(game->debug_text, window->dim);
	RendererBeginFrame(game->renderer, window->dim);
}

static void
GameEnd(Game* game) {
	//SubmitRendererShapesDrawCall(game->renderer_shapes, game->camera, game->renderer);

	RendererFrame(game->renderer);
	RendererEndFrame(game->renderer);

	game->transient.used = 0;
}

extern "C" GAME_LOOP(game_loop) {
	Game* game = game_layer->game;
	GameBegin(game, window);

	if(input->buttons[WIN32_BUTTON_F2].pressed) {
		game->renderer->state_overrides.rs ? game->renderer->state_overrides.rs = RASTERIZER_STATE_NONE : 
			game->renderer->state_overrides.rs = RASTERIZER_STATE_WIREFRAME;
	}
	if(input->buttons[WIN32_BUTTON_F1].pressed) {
		game_layer->debug_cursor_request = !game_layer->debug_cursor_request;
	}

	FPControlInfo info = DefaultFPControlInfo();
	ingo.base_sens = 0.5f; 
	info.trans_sens = 10.0f;
	info.rot_sens = 0.6f;
	FirstPersonCamera(game->camera, &info, input);

	CameraDrawDebugText(game->camera, game->debug_text, &game->transient);
	Quad quad = { V3(5.0f, 5.0f, -8.5f), V3(5.0f, -5.0f, -8.5f),
		V3(-5.0f, 5.0f, -8.5f), V3(-5.0f, -5.0f, -8.5f) };
	Quad quad1 = { V3(5.0f, 5.0f, -5.5f), V3(5.0f, -5.0f, -5.5f),
		V3(-5.0f, 5.0f, -5.5f),V3(-5.0f, -5.0f, -5.5f) };

	Line line = { V3(2.0f, 0.0f, 0.0f), V3(10.0f, 0.0f, 0.0f) };

	TextureAssetInfo* texture_asset = GetTextureAssetInfo("Blue Nebula", game->asset_info);
	TextureAssetInfo* texture_asset1 = GetTextureAssetInfo("Green Nebul", game->asset_info);

	PushTexturedQuad(&quad, texture_asset->render_buffer, game->quad_renderer);
	//PushTexturedQuad(&quad, texture_asset1->render_buffer, game->quad_renderer);
	PushRenderQuad(&quad1, V4FromV3(BLUE, 0.8f), game->quad_renderer);
	//PushRenderQuad(quad1, V4FromV3(RED, 0.5f), game->quad_renderer);
	//PushRenderLine(&line, V4FromV3(MAGENTA, 1.0f), 0.05f, game->camera, game->quad_renderer);

	//PushDebugLine(&line, game->quad_renderer);
	//
	GenerateTetraInfo(game->tetra);

	if(input->buttons[WIN32_BUTTON_A].pressed)
		DrawDebugText("Pressed", WHITE, QUADRANT_TOP_RIGHT, game->debug_text, &game->transient);
	if(input->buttons[WIN32_BUTTON_A].held)
		DrawDebugText("Held", WHITE, QUADRANT_TOP_RIGHT, game->debug_text, &game->transient);

	int x, y, xdel, ydel;
	x = input->axes[WIN32_AXIS_MOUSE].x;
	y = input->axes[WIN32_AXIS_MOUSE].y;
	xdel = input->axes[WIN32_AXIS_MOUSE_DEL].x;
	ydel = input->axes[WIN32_AXIS_MOUSE_DEL].y;

	char *a, *b, *c, *d;
	a = (char*)GetMemory(&game->transient, 50);
	b = (char*)GetMemory(&game->transient, 50);
	c = (char*)GetMemory(&game->transient, 50);
	d = (char*)GetMemory(&game->transient, 50);
	stbsp_sprintf(a, "Mouse x: %i", x);
	stbsp_sprintf(b, "Mouse y: %i", y);
	stbsp_sprintf(c, "Mouse xdel: %i", xdel);
	stbsp_sprintf(d, "Mouse ydel: %i", ydel);

	DrawDebugText(a, WHITE, QUADRANT_TOP_RIGHT, game->debug_text, &game->transient);
	DrawDebugText(b, WHITE, QUADRANT_TOP_RIGHT, game->debug_text, &game->transient);
	DrawDebugText(c, WHITE, QUADRANT_TOP_RIGHT, game->debug_text, &game->transient);
	DrawDebugText(d, WHITE, QUADRANT_TOP_RIGHT, game->debug_text, &game->transient);
	DrawDebugText("working", RED, QUADRANT_BOTTOM_LEFT, game->debug_text, &game->transient);
	//DrawDebugTransform(TransformI(), game->renderer_shapes);
	//

	LightInfo light = {};
	light.position = V3(0.2f, 3.0f, -4.0f);
	light.ambience = 0.2f;

	RenderTetra(game->tetra, game->mesh_renderer);
	UpdateTetra(game->tetra);

	MeshRendererFrame(game->mesh_renderer, game->camera, &light, game->renderer);
	QuadRendererFrame(game->quad_renderer, game->camera, game->renderer);
	SubmitDebugTextDrawCall(game->debug_text, game->renderer);

	GameEnd(game);
}

