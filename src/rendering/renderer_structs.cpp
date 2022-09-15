#define BLACK V3(0.0f, 0.0f, 0.0f)
#define WHITE V3(1.0f, 1.0f, 1.0f)
#define GREY V3(0.5f, 0.5f, 0.5f)
#define SILVER V3(0.75f, 0.75f, 0.75f)

#define BLUE V3(0.0f, 0.0f, 1.0f)
#define CYAN V3(0.0f, 1.0f, 1.0f)
#define TEAL V3(0.0f, 0.5f, 0.5f)

#define RED V3(1.0f, 0.0f, 0.0f)
#define MAROON V3(0.5f, 0.0f, 0.0f)
#define CRIMSON V3(0.8f, 0.1f, 0.2f)

#define LIME V3(0.0f, 1.0f, 0.0f)
#define GREEN V3(0.0f, 0.5f, 0.0f)

#define OLIVE V3(0.5f, 0.5f, 0.0f)
#define YELLOW V3(1.0f, 1.0f, 0.0f)
#define ORANGE V3(1.0f, 0.6f, 0.0f)

#define MAGENTA V3(1.0f, 0.0f, 1.0f)
#define PURPLE V3(0.5f, 0.0f, 0.5f)

enum RENDERER_MISC_FLAGS {
	RENDER_BUFFER_MAX = 255,
	RENDER_QUEUE_MAX = 255
};

enum DEPTH_STENCIL_STATE {
	DEPTH_STENCIL_STATE_NONE,
	DEPTH_STENCIL_STATE_DEFAULT,
	DEPTH_STENCIL_STATE_TOTAL
};

enum BLEND_STATE {
	BLEND_STATE_NONE,
	BLEND_STATE_NO_BLEND,
	BLEND_STATE_ENABLED,
	BLEND_STATE_TOTAL
};

enum RASTERIZER_STATE {
	RASTERIZER_STATE_NONE,
	RASTERIZER_STATE_DEFAULT,
	RASTERIZER_STATE_WIREFRAME,
	RASTERIZER_STATE_DOUBLE_SIDED,
	RASTERIZER_STATE_TOTAL
};

enum VIEWPORT {
	VIEWPORT_NONE,
	VIEWPORT_DEFAULT,
	VIEWPORT_TOTAL
};

enum VERTEX_SHADER {
	VERTEX_SHADER_NONE,
	VERTEX_SHADER_POS_NOR,
	VERTEX_SHADER_POS_NOR_TEX,
	VERTEX_SHADER_TEXT,
	VERTEX_SHADER_LINE,
	VERTEX_SHADER_TOTAL
};

enum PIXEL_SHADER {
	PIXEL_SHADER_NONE,
	PIXEL_SHADER_UNLIT,
	PIXEL_SHADER_DIFFUSE,
	PIXEL_SHADER_DIFFUSE_TEXTURED,
	PIXEL_SHADER_TEXT,
	PIXEL_SHADER_LINE,
	PIXEL_SHADER_TOTAL
};

enum SAMPLER_STATE {
	SAMPLER_STATE_NONE,
	SAMPLER_STATE_DEFAULT,
	SAMPLER_STATE_TILE,
	SAMPLER_STATE_TOTAL
};

enum RENDER_BUFFER_GROUP {
	RENDER_BUFFER_GROUP_NONE,
	RENDER_BUFFER_GROUP_CUBE,
	RENDER_BUFFER_GROUP_SPHERE,
	RENDER_BUFFER_GROUP_CONE,
	RENDER_BUFFER_GROUP_TORUS,
	RENDER_BUFFER_GROUP_PLANE,
	RENDER_BUFFER_GROUP_FONT,
	RENDER_BUFFER_GROUP_SPACE_BACKGROUND,
	RENDER_BUFFER_GROUP_SPACE_RED,
	RENDER_BUFFER_GROUP_END,
	RENDER_BUFFER_GROUP_TOTAL = 255
};

enum STRUCTURED_BINDING_SLOT {
	STRUCTURED_BINDING_SLOT_FRAME
};

enum CONSTANTS_BINDING_SLOT {
	CONSTANTS_BINDING_SLOT_FRAME,
	CONSTANTS_BINDING_SLOT_CAMERA,  // per viewport?
	CONSTANTS_BINDING_SLOT_OBJECT,
	CONSTANTS_BINDING_SLOT_INSTANCE,
};

enum TEXTURE_BINDING_SLOT {
	TEXTURE_BINDING_SLOT_ALBEDO,
	TEXTURE_BINDING_SLOT_NORMAL
};

enum RENDER_BUFFER_TYPE {
	RENDER_BUFFER_TYPE_VERTEX,
	RENDER_BUFFER_TYPE_INDEX,
	RENDER_BUFFER_TYPE_CONSTANTS,
	RENDER_BUFFER_TYPE_STRUCTURED,
	RENDER_BUFFER_TYPE_TEXTURE,
};

enum DRAW_CALL {
	DRAW_CALL_NONE,
	DRAW_CALL_DEFAULT,
	DRAW_CALL_INDEXED,
	DRAW_CALL_VERTICES,
};

struct ShaderDesc {
	wchar_t* path;
	char* entry;
};

struct ConstantsBufferDesc {
	CONSTANTS_BINDING_SLOT slot;
	u32 size;
};

struct StructuredBufferDesc {
	STRUCTURED_BINDING_SLOT slot;
	u32 struct_size_in_bytes;
	u32 count;
};

struct VertexShaderDesc {
	ShaderDesc shader;
	StructuredBufferDesc* sb_desc;
	ConstantsBufferDesc* cb_desc;
	VERTEX_BUFFER* vb_type;

	u8 cb_count, vb_count, sb_count;
};

struct TextureDesc {
	TEXTURE_SLOT slot;
	u8 num_channels;
};

struct PixelShaderDesc {
	ShaderDesc shader;
	ConstantsBufferDesc* cb_desc;
	//StructuredBufferDesc* sb_desc;
	TEXTURE_SLOT* texture_slot;

	u8 texture_count, cb_count;
};

struct IndexBuffer {
	ID3D11Buffer* buffer;
	u32 num_indices;
};

struct VertexBuffer {
	VERTEX_BUFFER type;
	ID3D11Buffer* buffer;
	u32 stride;
};

struct StructuredBuffer {
	STRUCTURED_BINDING_SLOT slot;
	ID3D11Buffer* buffer;
	ID3D11ShaderResourceView* view;
	u32 struct_size;
	u32 count;
};

struct TextureBuffer {
	TEXTURE_SLOT slot;
	ID3D11Texture2D* buffer;
	ID3D11ShaderResourceView* view;
};

struct ConstantsBuffer {
	CONSTANTS_BINDING_SLOT slot;
	ID3D11Buffer* buffer;
	u32 size;
};

struct RenderBuffer {
	RENDER_BUFFER_TYPE type;
	union {
		VertexBuffer vertex;
		IndexBuffer index;
		ConstantsBuffer constants;
		StructuredBuffer structured;
		TextureBuffer texture;
	};
};

struct RenderBufferGroup {
	RenderBuffer* rb;
	u8 count;
};

struct VertexShader {
	ID3D11VertexShader* shader;
	ID3D11InputLayout* il;
	VERTEX_BUFFER* vb_type;
	RenderBuffer* rb;
	u8 vb_count, rb_count;
};

struct PixelShader {
	ID3D11PixelShader* shader;
	TEXTURE_SLOT* texture_slot;
	RenderBuffer* rb;

	u8 texture_count, rb_count;
};

struct ConstantsBufferData {
	CONSTANTS_BINDING_SLOT slot;
	void* data;
};

struct StructuredBufferData {
	STRUCTURED_BINDING_SLOT slot;
	u32 count;
	void* data;
};

struct TextureBufferData {
	TEXTURE_SLOT slot;
	void* data;
};

struct RenderBufferData {
	RENDER_BUFFER_TYPE type;
	union {
		ConstantsBufferData constants;
		StructuredBufferData structured;
		TextureBufferData texture;
	};
};

struct DrawCall {
	DRAW_CALL type;
	union {
		u32 indices_count;
		u32 vertices_count;
	};
};

struct RenderState {
	DEPTH_STENCIL_STATE dss;
	BLEND_STATE bs;
	RASTERIZER_STATE rs;
	VIEWPORT vp;
	D3D11_PRIMITIVE_TOPOLOGY topology;
};

struct RenderPipeline {
	// Render target
	RenderState rs;
	VERTEX_SHADER vs;
	PIXEL_SHADER ps;
	RENDER_BUFFER_GROUP vrbg, prbg;
	DrawCall dc;
	RenderBufferData* vrbd;
	RenderBufferData* prbd;

	u8 vrbd_count, prbd_count;
};

// Push this on the heap
struct Renderer {
	ID3D11Device* device;
	ID3D11DeviceContext* context; // This changes when we start using deferred contexts
	IDXGISwapChain1* swapchain;

#ifdef DEBUG_RENDERER
	ID3D11Debug* d3d_debug;
	IDXGIDebug* dxgi_debug;
#endif

	ID3D11RenderTargetView* rtv;
	ID3D11DepthStencilView* dsv;

	ID3D11DepthStencilState* dss[DEPTH_STENCIL_STATE_TOTAL];
	ID3D11BlendState* bs[BLEND_STATE_TOTAL];

	ID3D11RasterizerState* rs[RASTERIZER_STATE_TOTAL];
	D3D11_VIEWPORT vp[VIEWPORT_TOTAL];	// Scissor rect

	VertexShader vs[VERTEX_SHADER_TOTAL];
	PixelShader ps[PIXEL_SHADER_TOTAL];

	ID3D11SamplerState* ss[SAMPLER_STATE_TOTAL];
	
	RenderBuffer rb[RENDER_BUFFER_MAX];
	RenderBufferGroup rbg[RENDER_BUFFER_GROUP_TOTAL];
	u8 rbg_id;
	u8 rb_id;

	RenderPipeline rq[RENDER_QUEUE_MAX];
	u8 rq_id;

	RenderState state_overrides;
};

static void PushRenderPipeline(RenderPipeline rp , Renderer* renderer) {
	assert(renderer->rq_id < RENDER_QUEUE_MAX);
	renderer->rq[renderer->rq_id++] = rp;
}

static RenderBuffer* PushRenderBuffer(u8 count, Renderer* renderer) {
	RenderBuffer* ptr;
	u8 id = renderer->rb_id;
	ptr = &renderer->rb[id];

	assert(id + count < RENDER_BUFFER_MAX);
	renderer->rb_id += count;

	return ptr;
}

static u8 PushRenderBufferGroup(RENDER_BUFFER_GROUP index, RenderBufferGroup rbg, Renderer* renderer) {
	if(index == RENDER_BUFFER_GROUP_NONE) {
		u8 id = renderer->rbg_id + (u8)RENDER_BUFFER_GROUP_END;
		assert(id  < RENDER_BUFFER_GROUP_TOTAL);
		renderer->rbg[id] = rbg;
		renderer->rbg_id++;
		return id;
	}
	renderer->rbg[index] = rbg;
	return index;
};

static RenderState RenderStateDefaults() {
	return RenderState { DEPTH_STENCIL_STATE_DEFAULT, BLEND_STATE_NO_BLEND, RASTERIZER_STATE_DEFAULT, VIEWPORT_DEFAULT, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST };
}
