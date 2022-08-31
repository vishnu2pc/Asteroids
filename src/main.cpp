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

	Renderer renderer = {};
	renderer = InitRendering(handle, app_state.wd);

	CameraInfo camera = {};
	camera.position = V3(-3.0f, -3.0f, -15.0f);
	//camera.rotation = QuatFromAxisAngle(V3(0.1f, -0.1f, 0.0f), 0.3f);
	camera.rotation = QuatI();
	camera.fov = 75.0f;
	camera.near_clip = 0.1f;
	camera.far_clip = 100.0f;
	camera.aspect_ratio = (float)app_state.wd.width/(float)app_state.wd.height;

	
	Entity plane = {};
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.rs.rs = RS_DOUBLE_SIDED;
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(PURPLE, 1.0f), &renderer);
		rp.mesh_id = MESH_PLANE;
		plane = { V3(0.0f, 0.0f, 0.0f), QuatI(), V3(5.0f, 5.0f, 5.0f), rp };
	}

	Entity cube = {};
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(WHITE, 1.0f), &renderer);
		rp.mesh_id = MESH_CONE;
		cube = { V3(0.0f, 0.0f, 0.0f), QuatI(), V3(1.0f, 1.0f, 1.0f), rp };
	}

	Entity right = {} ;
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(RED, 1.0f), &renderer);
		rp.mesh_id = MESH_CUBE;
		right = { V3(5.0f, 0.0f, 0.0f), QuatI(), V3(5.0f, 0.2f, 0.2f), rp };
	}

	Entity up = {} ;
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(YELLOW, 1.0f), &renderer);
		rp.mesh_id = MESH_CUBE;
		up = { V3(0.0f, 5.0f, 0.0f), QuatI(), V3(0.2f, 5.0f, 0.2f), rp };
	}

	Entity forward = {} ;
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.mat_id = PushMaterial(MakeDiffuseMaterial(BLUE, 1.0f), &renderer);
		rp.mesh_id = MESH_CUBE;
		forward = { V3(0.0f, 0.0f, 5.0f), QuatI(), V3(0.2f, 0.2f, 5.0f), rp };
	}

	Entity light = {};
	{
		RenderPipeline rp = {};
		rp.rs = RenderStateDefaults();
		rp.mat_id = MAT_UNLIT;
		rp.mesh_id = MESH_CUBE;
		light = { V3Z(), QuatI(), V3MulF(V3I(), 0.1f), rp };
	}

	Mat4 vp = MakeViewPerspective(camera);
	renderer.vs[VS_DIFFUSE].cb_data[CS_PER_CAMERA] = &vp;

	float ambience = 0.5f;

	while (app_state.running) {
		PreProcessInput(&app_state.input);
		HandleSDLevents(&app_state);
		BeginRendering(renderer);
		if(app_state.input.kb[KBK_F1].pressed) renderer.state_overrides.rs = renderer.state_overrides.rs ? RS_NONE : RS_WIREFRAME; 

		Mat4 plane_matrix = MakeTransformMatrix(plane.transform);
		plane.rp.vsc_per_object_data = &plane_matrix;

		Mat4 right_matrix = MakeTransformMatrix(right.transform);
		right.rp.vsc_per_object_data = &right_matrix;

		Mat4 up_matrix = MakeTransformMatrix(up.transform);
		up.rp.vsc_per_object_data = &up_matrix;
		
		Mat4 forward_matrix = MakeTransformMatrix(forward.transform);
		forward.rp.vsc_per_object_data = &forward_matrix;

		Mat4 cube_matrix = MakeTransformMatrix(cube.transform);
		cube.rp.vsc_per_object_data = &cube_matrix;

		float x = 3.0f * cosf(SDL_GetTicks() * 0.001);
		float y = 0.0f;
		float z = 3.0f * sinf(SDL_GetTicks() * 0.001);
		x += cube.transform.position.x;
		y += cube.transform.position.y;
		z += cube.transform.position.z;

		light.transform.position = V3(x, y, z);
		Mat4 light_matrix = MakeTransformMatrix(light.transform);
		light.rp.vsc_per_object_data = &light_matrix;

		DiffusePC dpc = { light.transform.position, ambience };
		renderer.ps[PS_DIFFUSE].cb_data[CS_PER_CAMERA] = &dpc;

		ExecuteRenderPipeline(right.rp, renderer);
		ExecuteRenderPipeline(up.rp, renderer);
		ExecuteRenderPipeline(forward.rp, renderer);
		ExecuteRenderPipeline(cube.rp, renderer);
		ExecuteRenderPipeline(plane.rp, renderer);

		ExecuteRenderPipeline(light.rp, renderer);

		EndRendering(renderer);
	}
	return 0;
}
