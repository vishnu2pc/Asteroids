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

typedef u32 SemanticNameIndex;

static char* SemanticName[] = {
"NOT SET",
"POSITION",
"NORMAL",
"TANGENT",
"COLOR",
"TEXCOORD",
};

enum DEPTH_STENCIL_STATE {
	DSS_NONE,
	DSS_DEFAULT,
	DSS_TOTAL
};

enum BLEND_STATE {
	BS_NONE,
	BS_DEFAULT,
	BS_TOTAL
};

enum RASTERIZER_STATE {
	RS_NONE,
	RS_DEFAULT,
	RS_WIREFRAME,
	RS_DOUBLE_SIDED,
	RS_TOTAL
};

enum VIEWPORT {
	VP_NONE,
	VP_DEFAULT,
	VP_TOTAL
};

enum VERTEX_SHADER {
	VS_NONE,
	VS_POS_NOR,
	VS_POS_TEX,
	VS_TOTAL
};

enum PIXEL_SHADER {
	PS_NONE,
	PS_UNLIT,
	PS_UNLIT_TEXTURED,
	PS_DIFFUSE,
	PS_TOTAL
};

enum SAMPLER_STATE {
	SS_NONE,
	SS_DEFAULT,
	SS_TILE,
	SS_TOTAL
};

enum MATERIAL {
	MAT_NONE,
	MAT_UNLIT,
	MAT_DIFFUSE,
	MAT_GRID,
	MAT_END,
	MAT_TOTAL = 255
};

enum MATERIAL_TYPE {
	MT_NOT_SET,
	MT_UNLIT,
	MT_DIFFUSE,
	MT_TEXTURED
};

enum CONSTANTS_SLOT {
	CS_PER_FRAME,
	CS_PER_CAMERA,  // per viewport?
	CS_PER_MESH_MATERIAL,
	CS_PER_OBJECT,
	CS_TOTAL
};

enum MESH {
	MESH_NONE,
	MESH_CUBE,
	MESH_SPHERE,
	MESH_CONE,
	MESH_TORUS,
	MESH_PLANE,
	MESH_END,
	MESH_TOTAL = 255
};

struct ConstantsBufferDesc {
	CONSTANTS_SLOT slot;
	u32 size;
};

struct ShaderDesc {
	wchar_t* path;
	char* entry;
};

struct BufferLayout {
	VERTEX_BUFFER_TYPE vb_type;
	BUFFER_FORMAT buffer_format;
	COMPONENT_TYPE component_type;
};

struct VertexShaderDesc {
	ShaderDesc shader;
	BufferLayout* bl;
	ConstantsBufferDesc* cb_desc;

	u8 bl_count;
	u8 cb_desc_count;
};

struct TextureLayout {
	TEXTURE_TYPE type;
	u8 index;
};

struct PixelShaderDesc {
	ShaderDesc shader;
	TEXTURE_TYPE* texture_type;
	ConstantsBufferDesc* cb_desc;
	//SamplerDesc* sampler_desc;

	u8 texture_count;
	u8 cb_desc_count;
	u8 ss_count;
};

struct MatDesc {
	char* mat_name;
	PIXEL_SHADER ps;
};

struct ModelDesc {
	MatDesc* mat_desc;
	u8 count;

	VERTEX_SHADER vs_default;
	PIXEL_SHADER ps_default;
};

struct ConstantsData {
	CONSTANTS_SLOT slot;
	void* data;
};

struct ConstantsBuffer {
	CONSTANTS_SLOT slot;
	ID3D11Buffer* buffer;
	u32 size;
};

struct VertexShader {
	ID3D11VertexShader* shader;
	ID3D11InputLayout* il;
	VERTEX_BUFFER_TYPE* vb_type;	
	u8 vb_count;

	ConstantsBuffer* cb;
	u8 cb_count;
};

struct PixelShader {
	ID3D11PixelShader* shader;

	TEXTURE_TYPE* texture_type;
	u8 texture_count;

	ConstantsBuffer* cb;
	u8 cb_count;

	u8 ss_count;
};

struct Texture {
	TEXTURE_TYPE type;
	ID3D11ShaderResourceView* srv;
};

struct Material {
	MATERIAL_TYPE type;
	Texture* texture;
	u8 texture_count;
	u8* sampler_id;

	union {
		struct {
			Vec3 color;
		} params_unlit;

		struct {
			Vec3 color;
			float diffuse_factor;
		} params_diffuse;

	} constants_data;
};

struct VertexBuffer {
	VERTEX_BUFFER_TYPE type;
	ID3D11Buffer* buffer;
	u32 stride;
};

struct Mesh {
	VertexBuffer* vb;
	u8 vb_count;
	ID3D11Buffer* ib;
	u32 indices_count;
};

struct RenderState {
	DEPTH_STENCIL_STATE dss;
	BLEND_STATE bs;
	RASTERIZER_STATE rs;
	VIEWPORT vp;
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

	ID3D11DepthStencilState* dss[DSS_TOTAL];
	ID3D11BlendState* bs[BS_TOTAL];

	ID3D11RasterizerState* rs[RS_TOTAL];
	D3D11_VIEWPORT vp[VP_TOTAL];	// Scissor rect

	VertexShader vs[VS_TOTAL];
	PixelShader ps[PS_TOTAL];

	ID3D11SamplerState* ss[SS_TOTAL];
	
	Material mat[MAT_TOTAL];
	Mesh mesh[MESH_TOTAL];

	RenderState state_overrides;

	u8 mat_id;
	u8 mesh_id;
};

struct RenderPipeline {
	// Render target
	RenderState rs;
	VERTEX_SHADER vs;
	PIXEL_SHADER ps;
	u8 mesh_id;
	u8 mat_id;
	ConstantsData cd_vertex;
	ConstantsData cd_pixel;
};


static u8 PushMesh(Mesh mesh, Renderer* renderer) {
	u8 id = renderer->mesh_id + (u8)MESH_END;
	assert(id  < MESH_TOTAL);
	renderer->mesh[id] = mesh;
	renderer->mesh_id++;
	return id;
};

static u8 PushMaterial(Material material, Renderer* renderer) {
	u8 id = renderer->mat_id + (u8)MAT_END;
	assert(id  < MAT_TOTAL);
	renderer->mat[id] = material;
	renderer->mat_id++;
	return id;
};

static RenderState RenderStateDefaults() {
	return RenderState { DSS_DEFAULT, BS_DEFAULT, RS_DEFAULT, VP_DEFAULT };
}

static Material MakeDiffuseMaterial(Vec3 color, float diffuse_factor) {
	Material mat = {};
	mat.type = MT_DIFFUSE;
	mat.constants_data.params_diffuse.color = color;
	mat.constants_data.params_diffuse.diffuse_factor = diffuse_factor;

	return mat;
}