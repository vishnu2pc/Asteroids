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
#include "rendering/renderer.cpp"
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

	CameraInfo camera = {};
	camera.position = V3(0.0f, 0.0f, -5.0f);
	camera.rotation = QuatI();
	camera.fov = 75.0f;
	camera.near_clip = 0.1f;
	camera.far_clip = 1000.0f;
	camera.aspect_ratio = (float)app_state.wd.width/(float)app_state.wd.height;
	
	Entity cube = {};
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(WHITE, 1.0f), renderer);
		rp.mesh_id = MESH_CUBE;
		cube = { V3(0.0f, 0.0f, 0.0f), QuatI(), V3(2.0f, 2.0f, 2.0f), rp };
	}

	Entity plane = {};
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.rs.rs = RS_DOUBLE_SIDED;
		rp.vs = VS_POS_NOR;
		rp.ps = PS_DIFFUSE;
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(PURPLE, 1.0f), renderer);
		rp.mesh_id = MESH_PLANE;
		plane = { V3(0.0f, 0.0f, 0.0f), QuatI(), V3(1000.0f, 1000.0f, 1000.0f), rp };
	}

	Entity right = {} ;
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VS_POS_NOR;
		rp.ps = PS_DIFFUSE;
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(RED, 1.0f), renderer);
		rp.mesh_id = MESH_CUBE;
		right = { V3(1.0f, 0.0f, 0.0f), QuatI(), V3(5.0f, 0.2f, 0.2f), rp };
	}

	Entity up = {} ;
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VS_POS_NOR;
		rp.ps = PS_DIFFUSE;
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(YELLOW, 2.0f), renderer);
		rp.mesh_id = MESH_CUBE;
		up = { V3(0.0f, 1.0f, 0.0f), QuatI(), V3(0.2f, 5.0f, 0.2f), rp };
	}

	Entity forward = {} ;
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VS_POS_NOR;
		rp.ps = PS_DIFFUSE;
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(BLUE, 2.0f), renderer);
		rp.mesh_id = MESH_CUBE;
		forward = { V3(0.0f, 0.0f, 1.0f), QuatI(), V3(0.2f, 0.2f, 5.0f), rp };
	}

	Entity light = {};
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.vs = VS_POS_NOR;
		rp.ps = PS_UNLIT;
		rp.mat_id = MAT_UNLIT;
		rp.mesh_id = MESH_CUBE;
		light = { V3Z(), QuatI(), V3MulF(V3I(), 0.1f), rp };
	}
	
	float ambience = 0.5f;

	FPControlInfo fpci = { 1.0f, 1.0f, 1.0f };

	while (app_state.running) {
		PreProcessInput(&app_state.input);
		HandleSDLevents(&app_state);
		BeginRendering(renderer);
		if(app_state.input.kb[KB_F1].pressed) renderer->state_overrides.rs = renderer->state_overrides.rs ? RS_NONE : RS_WIREFRAME; 

		Mat4 plane_matrix = MakeTransformMatrix(plane.transform);
		plane.rp.cd_vertex.slot = CS_PER_OBJECT;
		plane.rp.cd_vertex.data = &plane_matrix;

		Mat4 right_matrix = MakeTransformMatrix(right.transform);
		right.rp.cd_vertex.slot = CS_PER_OBJECT;
		right.rp.cd_vertex.data = &right_matrix;

		Mat4 up_matrix = MakeTransformMatrix(up.transform);
		up.rp.cd_vertex.slot = CS_PER_OBJECT;
		up.rp.cd_vertex.data = &up_matrix;

		Mat4 forward_matrix = MakeTransformMatrix(forward.transform);
		forward.rp.cd_vertex.slot = CS_PER_OBJECT;
		forward.rp.cd_vertex.data = &forward_matrix;

		Mat4 cube_matrix = MakeTransformMatrix(cube.transform);
		cube.rp.cd_vertex.slot = CS_PER_OBJECT;
		cube.rp.cd_vertex.data = &cube_matrix;

		//FirstPersonControl(&cube.transform.position, &cube.transform.rotation, fpci, app_state.input);
		FirstPersonControl(&camera.position, &camera.rotation, fpci, app_state.input);

		SDL_Log("cam pos:%f, %f, %f", camera.position.x, camera.position.y, camera.position.z);
		Mat4 vp = MakeViewPerspective(camera);

		RenderPipeline vs_rp = {};
		vs_rp.vs = VS_POS_NOR;
		vs_rp.cd_vertex.slot = CS_PER_CAMERA;
		vs_rp.cd_vertex.data = &vp;

		float x = 3.0f * cosf(SDL_GetTicks() * 0.001);
		float y = 0.0f;
		float z = 3.0f * sinf(SDL_GetTicks() * 0.001);
		x += cube.transform.position.x;
		y += cube.transform.position.y;
		z += cube.transform.position.z;

		light.transform.position = V3(x, y, z);
		Mat4 light_matrix = MakeTransformMatrix(light.transform);
		light.rp.cd_vertex.slot = CS_PER_OBJECT;
		light.rp.cd_vertex.data = &light_matrix;

		DiffusePC dpc = { light.transform.position, ambience };
		RenderPipeline ps_rp = {};
		ps_rp.ps = PS_DIFFUSE;
		ps_rp.cd_pixel.slot = CS_PER_CAMERA;
		ps_rp.cd_pixel.data = &dpc;

		ExecuteRenderPipeline(vs_rp, renderer);
		ExecuteRenderPipeline(ps_rp, renderer);

		ExecuteRenderPipeline(right.rp, renderer);
		ExecuteRenderPipeline(up.rp, renderer);
		ExecuteRenderPipeline(forward.rp, renderer);
		ExecuteRenderPipeline(plane.rp, renderer);
		ExecuteRenderPipeline(cube.rp, renderer);

		ExecuteRenderPipeline(light.rp, renderer);

		EndRendering(renderer);
	}
	return 0;
}
