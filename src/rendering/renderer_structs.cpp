typedef u32 SemanticNameIndex;

static char* SemanticName[] = {
"NOT SET",
"POSITION",
"NORMAL",
"TANGENT",
"TEXCOORD",
"COLOR",
"TANGENT_BASIS"
};

#define BLACK V3(0.0f, 0.0f, 0.0f)
#define WHITE V3(1.0f, 1.0f, 1.0f)


enum DEPTH_STENCIL_STATE {
	DSS_DEFAULT,
	DSS_TOTAL
};

enum BLEND_STATE {
	BS_DEFAULT,
	BS_TOTAL
};

enum RASTERIZER_STATE {
	RS_DEFAULT,
	RS_TOTAL
};

enum VIEWPORT {
	VP_DEFAULT,
	VP_TOTAL
};

enum VERTEX_SHADER {
	VS_DIFFUSE,
	VS_TOTAL
};

enum PIXEL_SHADER {
	PS_CAMERA_LIT_DIFFUSE,
	PS_DIFFUSE,
	PS_TOTAL
};

enum MATERIAL {
	MAT_CAMERA_LIT_DIFFUSE,
	MAT_DIFFUSE,
	MAT_TOTAL
};

enum CONSTANTS_SLOT {
	CS_PER_FRAME,
	CS_PER_CAMERA,  // per viewport?
	CS_PER_MESH,
	CS_TOTAL
};

enum MESH {
	MESH_CUBE,
	MESH_TOTAL
};

struct ConstantsBufferDesc {
	CONSTANTS_SLOT slot;
	u32 size_in_bytes;
	void* default_data;
};

struct ShaderDesc {
	char* version;
	wchar_t* path;
	char* entry;
};

struct BufferLayout {
	VERTEX_BUFFER_TYPE vertex_buffer_type;
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
	TextureLayout* tl;
	ConstantsBufferDesc* cb_desc;
	//SamplerDesc* sampler_desc;

	u8 tl_count;
	u8 cb_desc_count;
	//u8 sampler_count;
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

struct ConstantsBuffer {
	ID3D11Buffer* buffer;
	u32 size_in_bytes;
	void* default_data;
};

struct VertexShader {
	ID3D11VertexShader* vs;
	ID3D11InputLayout* il;
	BufferLayout* bl;
	ConstantsBuffer cb[CS_TOTAL];
	u8 bl_count;

	void* cb_data[CS_TOTAL];
};

struct PixelShader {
	ID3D11PixelShader* ps;
	TextureLayout* tl;
	ConstantsBuffer cb[CS_TOTAL];
	u8 ss_count;
	u8 tl_count;

	void* cb_data[CS_TOTAL];
};

struct Material {
	PIXEL_SHADER ps;
};

struct Mesh {
	VERTEX_SHADER vs;
	ID3D11Buffer** vertex_buffers;
	ID3D11Buffer* index_buffer;
	u32* strides;
	u32 vertices_count;
	u32 indices_count;
};

struct Renderer {
	ID3D11Device* device;
	ID3D11DeviceContext* context; // This changes when we start using deferred contexts
	IDXGISwapChain1* swapchain;

	// TODO: go on a renaming spree
#ifdef DEBUG_RENDERER
	ID3D11Debug* d3d_debug;
	IDXGIDebug* dxgi_debug;
#endif

	ID3D11RenderTargetView* rtv;
	ID3D11DepthStencilView* dsv;

	ID3D11DepthStencilState* dss[DSS_TOTAL];
	ID3D11BlendState* bs[BS_TOTAL];

	ID3D11RasterizerState* rs[RS_TOTAL];
	D3D11_VIEWPORT vp[VP_TOTAL];		// Scissor rect

	VertexShader vs[VS_TOTAL];
	PixelShader ps[PS_TOTAL];
	
	Material mat[MAT_TOTAL];	
	Mesh mesh[MESH_TOTAL];
};

struct RenderState {
	DEPTH_STENCIL_STATE dss;
	BLEND_STATE bs;
	RASTERIZER_STATE rs;
	VIEWPORT vp;
};

struct RenderPipeline {
	RenderState rs;
	u8 mat_id;
	u8 mesh_id;
	void* vsc_per_mesh_data;
	void* psc_per_mesh_data;
};
