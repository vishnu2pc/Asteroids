// TODO LIST:
// Toggle wireframe mode
// Switch asserts to debugbreak

#define DEBUG
#define DEBUG_RENDERER

#ifndef DEBUG
#define NDEBUG 							// disables assert 
#endif

#define WIN32_LEAN_AND_MEAN

#include "SDL/SDL.h"
#include "SDL/SDL_syswm.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define assertHR(result) assert(SUCCEEDED(result))

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(*(array)))

struct WindowDimensions {	u32 width; u32 height; };

#include "memory_management.cpp"
#include "math.cpp"
#include "input.cpp"
#include "app_state.cpp"

#include "cgltf_loader.cpp"
#include "font_handling_structs.cpp"
#include "rendering/renderer.cpp"
#include "font_handling.cpp"
#include "camera.cpp"
#include "editor.cpp"

#define Kilobytes(value) (1024LL*(value))
#define Megabytes(value) (Kilobytes(1024)*(value))
#define Gigabytes(value) (Megabytes(1024)*(value))

struct Entity {
	Transform transform;
	RenderPipeline rp;
};

int main(int argc, char* argv[]) {
	AllocateMasterMemory(Megabytes(200));
	AllocateScratchMemory(Kilobytes(200));
	stbi_set_flip_vertically_on_load(false);

	AppState app_state = {};
	app_state.running = true;
	app_state.wd = { 1600, 900 };

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window = SDL_CreateWindow("Asteroids", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		app_state.wd.width, app_state.wd.height, 0);
	HWND handle = GetWindowHandleSDL(window);

	SDL_WarpMouseInWindow(window, app_state.wd.width/2, app_state.wd.height/2);
	SDL_FlushEvents(SDL_MOUSEMOTION, SDL_MOUSEWHEEL);

	Renderer* renderer = PushMaster(Renderer, 1);
	InitRendering(renderer, handle, app_state.wd);

	DebugText* dt = PushMaster(DebugText, 1);
	LoadFont("../assets/fonts/JetBrainsMono/jetbrains_mono_light.fi",
		"../assets/fonts/JetBrainsMono/jetbrains_mono_light.png", dt, renderer);

	CameraInfo camera = {};
	camera.position = V3(0.0f, 0.0f, -5.0f);
	camera.rotation = QuatI();
	camera.fov = 75.0f;
	camera.near_clip = 0.1f;
	camera.far_clip = 10000.0f;
	camera.aspect_ratio = (float)app_state.wd.width/(float)app_state.wd.height;
	
	Entity cube = {};
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VERTEX_SHADER_POS_NOR;
		rp.ps = PIXEL_SHADER_DIFFUSE;
		rp.vrbg = RENDER_BUFFER_GROUP_CUBE;
		rp.dc.type = DRAW_CALL_DEFAULT;
		cube = { V3(0.0f, 0.0f, 0.0f), QuatI(), V3(2.0f, 2.0f, 2.0f), rp };
	}

	Entity plane = {};
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VERTEX_SHADER_POS_NOR_TEX;
		rp.ps = PIXEL_SHADER_DIFFUSE_TEXTURED;
		rp.vrbg = RENDER_BUFFER_GROUP_PLANE;
		rp.prbg = RENDER_BUFFER_GROUP_FONT;
		rp.dc.type = DRAW_CALL_DEFAULT;
		plane = { V3(0.0f, 0.0f, 0.0f), QuatI(), V3(1000.0f, 1000.0f, 1000.0f), rp };
	}

	Entity right = {} ;
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VERTEX_SHADER_POS_NOR;
		rp.ps = PIXEL_SHADER_DIFFUSE;
		rp.vrbg = RENDER_BUFFER_GROUP_CUBE;
		rp.dc.type = DRAW_CALL_DEFAULT;
		right = { V3(1.0f, 0.0f, 0.0f), QuatI(), V3(5.0f, 0.2f, 0.2f), rp };
	}

	Entity up = {} ;
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VERTEX_SHADER_POS_NOR;
		rp.ps = PIXEL_SHADER_DIFFUSE;
		rp.vrbg = RENDER_BUFFER_GROUP_CUBE;
		rp.dc.type = DRAW_CALL_DEFAULT;
		up = { V3(0.0f, 1.0f, 0.0f), QuatI(), V3(0.2f, 5.0f, 0.2f), rp };
	}

	Entity forward = {} ;
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VERTEX_SHADER_POS_NOR;
		rp.ps = PIXEL_SHADER_DIFFUSE;
		rp.vrbg = RENDER_BUFFER_GROUP_CUBE;
		rp.dc.type = DRAW_CALL_DEFAULT;
		forward = { V3(0.0f, 0.0f, 1.0f), QuatI(), V3(0.2f, 0.2f, 5.0f), rp };
	}

	Entity light = {};
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VERTEX_SHADER_POS_NOR;
		rp.ps = PIXEL_SHADER_UNLIT;
		rp.vrbg = RENDER_BUFFER_GROUP_CUBE;
		rp.dc.type = DRAW_CALL_DEFAULT;
		light = { V3Z(), QuatI(), V3MulF(V3I(), 0.1f), rp };
	}

	float ambience = 0.5f;

	FPControlInfo fpci = { 1.0f, 1.0f, 1.0f };

	while (app_state.running) {
		PreProcessInput(&app_state.input);
		HandleSDLevents(&app_state);
		BeginDebugText(dt, app_state.wd);
		BeginRendering(renderer);
		if(app_state.input.kb[KB_F1].pressed) renderer->state_overrides.rs = renderer->state_overrides.rs ? RASTERIZER_STATE_NONE : RASTERIZER_STATE_WIREFRAME; 

		Mat4 plane_matrix = MakeTransformMatrix(plane.transform);
		DiffusePM plane_dpm = { PURPLE, 0.5f };
		plane.rp.vrbd = PushScratch(RenderBufferData, 1);
		plane.rp.prbd = PushScratch(RenderBufferData, 1);
		plane.rp.vrbd_count = 1;
		plane.rp.prbd_count = 1;
		plane.rp.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		plane.rp.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_INSTANCE;
		plane.rp.vrbd[0].constants.data = &plane_matrix;
		plane.rp.prbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		plane.rp.prbd[0].constants.slot = CONSTANTS_BINDING_SLOT_OBJECT;
		plane.rp.prbd[0].constants.data = &plane_dpm;

		Mat4 right_matrix = MakeTransformMatrix(right.transform);
		DiffusePM right_dpm = { RED, 1.0f };
		right.rp.vrbd = PushScratch(RenderBufferData, 1);
		right.rp.prbd = PushScratch(RenderBufferData, 1);
		right.rp.vrbd_count = 1;
		right.rp.prbd_count = 1;
		right.rp.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		right.rp.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_INSTANCE;
		right.rp.vrbd[0].constants.data = &right_matrix;
		right.rp.prbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		right.rp.prbd[0].constants.slot = CONSTANTS_BINDING_SLOT_OBJECT;
		right.rp.prbd[0].constants.data = &right_dpm;

		Mat4 up_matrix = MakeTransformMatrix(up.transform);
		DiffusePM up_dpm = { YELLOW, 1.0f };
		up.rp.vrbd = PushScratch(RenderBufferData, 1);
		up.rp.prbd = PushScratch(RenderBufferData, 1);
		up.rp.vrbd_count = 1;
		up.rp.prbd_count = 1;
		up.rp.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		up.rp.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_INSTANCE;
		up.rp.vrbd[0].constants.data = &up_matrix;
		up.rp.prbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		up.rp.prbd[0].constants.slot = CONSTANTS_BINDING_SLOT_OBJECT;
		up.rp.prbd[0].constants.data = &up_dpm;

		Mat4 forward_matrix = MakeTransformMatrix(forward.transform);
		DiffusePM forward_dpm = { BLUE, 1.0f };
		forward.rp.vrbd = PushScratch(RenderBufferData, 1);
		forward.rp.prbd = PushScratch(RenderBufferData, 1);
		forward.rp.vrbd_count = 1;
		forward.rp.prbd_count = 1;
		forward.rp.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		forward.rp.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_INSTANCE;
		forward.rp.vrbd[0].constants.data = &forward_matrix;
		forward.rp.prbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		forward.rp.prbd[0].constants.slot = CONSTANTS_BINDING_SLOT_OBJECT;
		forward.rp.prbd[0].constants.data = &forward_dpm;

		Mat4 cube_matrix = MakeTransformMatrix(cube.transform);
		DiffusePM cube_dpm = { WHITE, 1.0f };
		cube.rp.vrbd = PushScratch(RenderBufferData, 1);
		cube.rp.prbd = PushScratch(RenderBufferData, 1);
		cube.rp.vrbd_count = 1;
		cube.rp.prbd_count = 1;
		cube.rp.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		cube.rp.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_INSTANCE;
		cube.rp.vrbd[0].constants.data = &cube_matrix;
		cube.rp.prbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		cube.rp.prbd[0].constants.slot = CONSTANTS_BINDING_SLOT_OBJECT;
		cube.rp.prbd[0].constants.data = &cube_dpm;

		//FirstPersonControl(&cube.transform.position, &cube.transform.rotation, fpci, app_state.input);
		//if(app_state.input.mk[MK_RIGHT].held) 
		FirstPersonControl(&camera.position, &camera.rotation, fpci, app_state.input);

		SDL_Log("cam pos:%f, %f, %f", camera.position.x, camera.position.y, camera.position.z);
		Mat4 vp = MakeViewPerspective(camera);

		RenderPipeline vs_rp = {};
		vs_rp.vs = VERTEX_SHADER_POS_NOR;
		vs_rp.vrbd_count = 1;
		vs_rp.vrbd = PushScratch(RenderBufferData, 1);
		vs_rp.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		vs_rp.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
		vs_rp.vrbd[0].constants.data = &vp;

		RenderPipeline vs_rp_2 = {};
		vs_rp_2.vs = VERTEX_SHADER_POS_NOR_TEX;
		vs_rp_2.vrbd_count = 1;
		vs_rp_2.vrbd = PushScratch(RenderBufferData, 1);
		vs_rp_2.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		vs_rp_2.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
		vs_rp_2.vrbd[0].constants.data = &vp;

		float x = 3.0f * cosf(SDL_GetTicks() * 0.001);
		float y = 0.0f;
		float z = 3.0f * sinf(SDL_GetTicks() * 0.001);
		x += cube.transform.position.x;
		y += cube.transform.position.y;
		z += cube.transform.position.z;

		light.transform.position = V3(x, y, z);
		Mat4 light_matrix = MakeTransformMatrix(light.transform);
		light.rp.vrbd = PushScratch(RenderBufferData, 1);
		light.rp.vrbd_count = 1;
		light.rp.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		light.rp.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_INSTANCE;
		light.rp.vrbd[0].constants.data = &light_matrix;

		DiffusePC dpc = { light.transform.position, ambience };
		RenderPipeline ps_rp = {};
		ps_rp.ps = PIXEL_SHADER_DIFFUSE;
		ps_rp.prbd = PushScratch(RenderBufferData, 1);
		ps_rp.prbd_count = 1;
		ps_rp.prbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		ps_rp.prbd[0].constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
		ps_rp.prbd[0].constants.data = &dpc;

		RenderPipeline ps_rp_2 = {};
		ps_rp_2.ps = PIXEL_SHADER_DIFFUSE_TEXTURED;
		ps_rp_2.prbd = PushScratch(RenderBufferData, 1);
		ps_rp_2.prbd_count = 1;
		ps_rp_2.prbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		ps_rp_2.prbd[0].constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
		ps_rp_2.prbd[0].constants.data = &dpc;

		CameraDrawDebugText(&camera, dt);
		RenderPipeline debug_text = {};
		debug_text.rs = RenderStateDefaults();
		debug_text.rs.rs = RASTERIZER_STATE_DOUBLE_SIDED;
		debug_text.vs = VERTEX_SHADER_TEXT;
		debug_text.ps = PIXEL_SHADER_TEXT;
		debug_text.dc.type = DRAW_CALL_VERTICES;
		debug_text.dc.vertices_count = 6 * dt->glyph_counter;
		debug_text.vrbd = PushMaster(RenderBufferData, 1);
		debug_text.vrbd_count = 1;
		debug_text.vrbd[0].type = RENDER_BUFFER_TYPE_STRUCTURED;
		debug_text.vrbd[0].structured.slot = STRUCTURED_BINDING_SLOT_FRAME;
		debug_text.vrbd[0].structured.data = dt->quads;
		debug_text.vrbd[0].structured.count = dt->glyph_counter;

		renderer->context->PSSetSamplers(0, 1, &renderer->ss[SAMPLER_STATE_DEFAULT]);

		ExecuteRenderPipeline(vs_rp, renderer);
		ExecuteRenderPipeline(vs_rp_2, renderer);
		ExecuteRenderPipeline(ps_rp, renderer);
		ExecuteRenderPipeline(ps_rp_2, renderer);

		ExecuteRenderPipeline(right.rp, renderer);
		ExecuteRenderPipeline(up.rp, renderer);
		ExecuteRenderPipeline(forward.rp, renderer);
		ExecuteRenderPipeline(plane.rp, renderer);
		ExecuteRenderPipeline(cube.rp, renderer);

		ExecuteRenderPipeline(light.rp, renderer);

		// TODO: RenderPipeline to clear states
		renderer->context->ClearDepthStencilView(renderer->dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
		ExecuteRenderPipeline(debug_text, renderer);
		EndRendering(renderer);

		PopScratch(RenderBufferData, 15);
	}
	return 0;
}
