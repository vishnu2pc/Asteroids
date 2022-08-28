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
#include "input.cpp"
#include "math.cpp"

#include "cgltf_loader.cpp"
#include "rendering/renderer.cpp"

#define Kilobytes(value) (1024LL*(value))
#define Megabytes(value) (Kilobytes(1024)*(value))
#define Gigabytes(value) (Megabytes(1024)*(value))


static void HandleSDLevents(bool* RUNNING) {
	SDL_Event event;
	SDL_PollEvent(&event);
	HINPUT = {};
	switch (event.type) {
		case SDL_QUIT: {
			*RUNNING = false;
		} break;
		case SDL_KEYUP: 
		case SDL_KEYDOWN: { 
			HandleKeyboardEvents(event);
		} break;
		case SDL_MOUSEMOTION:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEWHEEL: {
			HandleMouseEvents(event);
		} break;
	}
}

static HWND GetWindowHandleSDL(SDL_Window* window) {
	SDL_SysWMinfo wmInfo;
	SDL_VERSION(&wmInfo.version);
	SDL_GetWindowWMInfo(window, &wmInfo);
	HWND hwnd = wmInfo.info.win.window;
	return hwnd;
}

struct Camera {
	Vec3 position;
	Vec3 target;
	float fov;
	float near_clip;
	float far_clip;
	float aspect_ratio;
};

static Mat4 MakeViewPerspective(Camera camera) {
	Mat4 result = M4I();
	Mat4 view = M4LookAt(camera.position, camera.target, V3Up());
	Mat4 perspective = M4Perspective(camera.fov, camera.aspect_ratio, camera.near_clip, camera.far_clip);
	result = M4Mul(perspective, view);
	return result;
}

int main(int argc, char* argv[]) {
	AllocateMasterMemory(Megabytes(200));
	AllocateScratchMemory(Kilobytes(200));
	stbi_set_flip_vertically_on_load(false);

	bool RUNNING = true;
	WindowDimensions wd = { 1600, 900 };

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window = SDL_CreateWindow("Asteroids", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		wd.width, wd.height, 0);
	HWND handle = GetWindowHandleSDL(window);

	SDL_WarpMouseInWindow(window, wd.width/2, wd.height/2);
	SDL_FlushEvents(SDL_MOUSEMOTION, SDL_MOUSEWHEEL);

	Renderer renderer = {};
	Camera camera = {};
	camera.position = V3(0.0f, 0.0f, 0.0f);
	camera.target = V3(0.0f, 0.0f, -10.0f);
	camera.fov = 45.0f;
	camera.near_clip = 0.1f;
	camera.far_clip = 100.0f;
	camera.aspect_ratio = (float)wd.width/(float)wd.height;

	renderer = InitRendering(handle, wd);
	RenderPipeline rp = {};
	rp.mat_id = MAT_CAMERA_LIT_DIFFUSE;
	rp.mesh_id = MESH_CUBE;
	Mat4 vp = MakeViewPerspective(camera);
	renderer.vs[VS_DIFFUSE].cb_data[CS_PER_CAMERA] = &vp;

	Vec3 color = V3(0.8f, 0.3f, 0.1f);
	rp.psc_per_mesh_data = &color;
	Vec3 ambience = V3(0.4f, 0.4f, 0.4f);
	DiffusePC dpc = { camera.position, ambience };
	renderer.ps[PS_CAMERA_LIT_DIFFUSE].cb_data[CS_PER_CAMERA] = &dpc;
	Mat4 cube_matrix = M4I();

	while (RUNNING) {
		HandleSDLevents(&RUNNING);
		BeginRendering(renderer);

		Mat4 translation = M4Translate(V3(0.0f, 0.0f, -10.0f));
		Mat4 rotation = M4Rotate(V3(1.0f, 2.0f, 3.0f), 100 * sin((float)SDL_GetTicks() * 0.001));
		cube_matrix = M4Mul(translation, rotation);

		rp.vsc_per_mesh_data = &cube_matrix;

		ExecuteRenderPipeline(rp, renderer);
		EndRendering(renderer);
	}
	return 0;
}
