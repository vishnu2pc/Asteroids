enum BLEND_STATE {
	BLEND_STATE_NoBlend,
	BLEND_STATE_Regular,
	BLEND_STATE_PreMulAlpha,

	BLEND_STATE_TOTAL
};

enum RASTERIZER_STATE {
	RASTERIZER_STATE_Default,
	RASTERIZER_STATE_Wireframe,
	RASTERIZER_STATE_DoubleSided,

	RASTERIZER_STATE_TOTAL
};

enum SAMPLER_STATE {
	SAMPLER_STATE_Default,

	SAMPLER_STATE_TOTAL
};

enum STRUCTURED_BINDING_SLOT {
	STRUCTURED_BINDING_SLOT_Frame
};

enum CONSTANTS_BINDING_SLOT {
	CONSTANTS_BINDING_SLOT_Frame,
	CONSTANTS_BINDING_SLOT_Camera,
	CONSTANTS_BINDING_SLOT_Object
};

enum TEXTURE_BINDING_SLOT {
	TEXTURE_BINDING_SLOT_ALBEDO,
	TEXTURE_BINDING_SLOT_NORMAL
};

enum PRIMITITVE_TOPOLOGY {
	PRIMITIVE_TOPOLOGY_TriangleList,
	PRIMITIVE_TOPOLOGY_TriangleStrip
};

struct RenderTarget {
	ID3D11Texture2D* texture;
	ID3D11RenderTargetView* view;
};

struct ReadableRenderTarget {
	ID3D11Texture2D* texture;
	ID3D11RenderTargetView* render_target;
	ID3D11ShaderResourceView* shader_resource;
};

struct DepthStencil {
	ID3D11Texture2D* texture;
	ID3D11DepthStencilView* view;
};

struct IndexBuffer     { ID3D11Buffer* buffer; };
struct ConstantsBuffer { ID3D11Buffer* buffer; };
struct VertexBuffer    { ID3D11Buffer* buffer; };

struct StructuredBuffer {
	ID3D11Buffer* buffer;
	ID3D11ShaderResourceView* view;
};

struct TextureBuffer {
	ID3D11Texture2D* buffer;
	ID3D11ShaderResourceView* view;
};

struct VertexShader {
	ID3D11VertexShader* shader;
	ID3D11InputLayout* il;
};

struct PixelShader {
	ID3D11PixelShader* shader;
};

struct SetVertexBuffer { 
	VertexBuffer* vertex;         
	u32 vertices_count;
	u32 offset;
	u32 stride;
	u8 slot;
};

struct SetStructuredBuffer { 
	bool vertex_shader;
	StructuredBuffer* structured; 
	u8 slot;
};

struct SetTextureBuffer {
	TextureBuffer* texture; 
	u8 slot;
};

struct SetConstantsBuffer { 
	bool vertex_shader;
	ConstantsBuffer* constants;
	u8 slot;
};

struct SetIndexBuffer { 
	IndexBuffer* index;
	u32 offset;
};

struct SetSamplerState {
	u8 type;
	u8 slot;
};

struct FreeRenderResource {
	void* buffer;
};

struct SetBlendState        { u8 type;              } ;
struct SetRasterizerState   { u8 type;              } ;
struct SetPrimitiveTopology { u8 type;              } ;
struct SetVertexShader      { VertexShader* vertex; } ;
struct SetPixelShader       { PixelShader* pixel;   } ;

struct SetViewport { 
	Vec2 topleft;
	Vec2 dim;
};

struct SetRenderTarget{
	RenderTarget* render_target;   // 0 means set backbuffer as render target
};

struct SetDepthStencilState {
	u8 type;
};

struct PushRenderBufferData {
	void* buffer;
	void* data;
	u32 size;
	u32 pitch;
};

struct DrawIndexed {
	u32 indices_count;
	u32 offset;
};

struct DrawVertices {
	u32 vertices_count;
	u32 offset;
};

struct DrawInstanced {
	u32 vertices_count;
	u32 instance_count;
	u32 offset;
};

struct ClearRenderTarget {
	RenderTarget* render_target;
	float color[4];
};

struct ClearDepth {
	float value;
};

struct ClearStencil {
	DepthStencil* depth_stencil;
	u8 value;
};

enum RENDER_COMMAND {
	RENDER_COMMAND_ClearRenderTarget,
	RENDER_COMMAND_ClearDepth,
	RENDER_COMMAND_ClearStencil,

	RENDER_COMMAND_SetRenderTarget,
	RENDER_COMMAND_SetBlendState,
	RENDER_COMMAND_SetDepthStencilState,
	RENDER_COMMAND_SetRasterizerState,
	RENDER_COMMAND_SetPrimitiveTopology,
	RENDER_COMMAND_SetSamplerState,
	RENDER_COMMAND_SetViewport,
	RENDER_COMMAND_SetTopology,

	RENDER_COMMAND_SetVertexShader,
	RENDER_COMMAND_SetPixelShader,

	RENDER_COMMAND_SetVertexBuffer,
	RENDER_COMMAND_SetIndexBuffer,
	RENDER_COMMAND_SetStructuredBuffer,
	RENDER_COMMAND_SetConstantsBuffer,
	RENDER_COMMAND_SetTextureBuffer,

	RENDER_COMMAND_DrawVertices,
	RENDER_COMMAND_DrawIndexed,
	RENDER_COMMAND_DrawInstanced,

	RENDER_COMMAND_PushRenderBufferData,
	RENDER_COMMAND_FreeRenderResource
};

struct RenderCommandHeader { u8 type; };

// Push this on the heap
struct Renderer {
	MemoryArena* permanent_arena;
	MemoryArena* frame_arena;

	WindowDimensions window_dim;

	ID3D11Device* device;
	ID3D11DeviceContext* context; 
	IDXGISwapChain1* swapchain;
	u32 msaa_sample_count;
	u32 msaa_quality_level;

	RenderTarget backbuffer;	// d3d11 api swaps buffers behind your back and internally remaps it
	ReadableRenderTarget readable_render_target;

	DepthStencil depth_stencil;	
	ID3D11SamplerState* samplers[SAMPLER_STATE_TOTAL];

	ID3D11DepthStencilState* default_depth_stencil_state;

	ID3D11BlendState* blend_states[BLEND_STATE_TOTAL];
	ID3D11RasterizerState* rasterizer_states[RASTERIZER_STATE_TOTAL];

	u32 command_buffer_size;
	u8* command_buffer_base;
	u8* command_buffer_cursor;

};




