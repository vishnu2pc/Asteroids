#define MAX_QUADS 100
#define MAX_TEXTURED_QUADS 100

struct RenderQuad {
	Quad quad;
	Vec4 color;
};

struct QuadRenderer {
	RenderQuad quads[MAX_QUADS]; // batches to issue one draw call
	u32 quad_count;

	VertexShader* quad_vs;
	PixelShader* quad_ps;

	Vec3 positions[MAX_TEXTURED_QUADS*4*3];
	Vec2 texcoords[MAX_TEXTURED_QUADS*4*2];
	RenderBuffer* textures[MAX_TEXTURED_QUADS];
	u32 textured_quad_count;

	VertexShader* textured_quad_vs;
	PixelShader* textured_quad_ps;
	RenderBufferGroup* vertex_buffers;
};

static void
CompileTexturedQuadShader(QuadRenderer* quad_renderer, Renderer* renderer) {
	char VertexShaderCode[] = R"FOO(

struct vs {
	float3 position : POSITION;
	float2 texcoord : TEXCOORD;
};

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

cbuffer camera : register(b1) {
	float4x4 view_proj;
};

ps vsf(vs input) {
	ps output;

	output.pixel_pos = mul(view_proj, float4(input.position, 1.0));
	output.texcoord = input.texcoord;

	return output;
}

	)FOO";

	char PixelShaderCode[] = R"FOO(
struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
};

Texture2D diffuse_texture : register(t0);
SamplerState default_sampler : register(s0);

float4 psf(ps input) : SV_TARGET {
	float3 diffuse_color = diffuse_texture.Sample(default_sampler, input.texcoord).xyz;
	return float4(diffuse_color, 1.0f);
}
	)FOO";
	{
		ShaderDesc sd = { VertexShaderCode, sizeof(VertexShaderCode), "vsf" };
		ConstantsBufferDesc cbd[] = { { CONSTANTS_BINDING_SLOT_CAMERA, sizeof(Mat4) } };
		VERTEX_BUFFER vbd[] = { VERTEX_BUFFER_POSITION, VERTEX_BUFFER_TEXCOORD };

		VertexShaderDesc vsd = {};
		vsd.shader = sd;
		vsd.cb_desc = cbd;
		vsd.vb_type = vbd;
		vsd.vb_count = ArrayCount(vbd);
		vsd.cb_count = ArrayCount(cbd);

		quad_renderer->textured_quad_vs = UploadVertexShader(vsd, renderer);
	}
	{
		PixelShaderDesc psd = {};
		ShaderDesc sd = { PixelShaderCode, sizeof(PixelShaderCode), "psf" };
		TEXTURE_SLOT slots[] = { TEXTURE_SLOT_DIFFUSE };

		psd.shader = sd;
		psd.texture_slot = slots;
		psd.texture_count = ArrayCount(slots);
		
		quad_renderer->textured_quad_ps = UploadPixelShader(psd, renderer);
	}
	{
		VertexBufferData vbd[] = { { (float*)quad_renderer->positions, VERTEX_BUFFER_POSITION },
																{ (float*)quad_renderer->texcoords, VERTEX_BUFFER_TEXCOORD } };

		MeshData mesh_data = {};
		mesh_data.vertices_count = MAX_TEXTURED_QUADS*4;
		mesh_data.vb_data = vbd;
		mesh_data.vb_data_count = ArrayCount(vbd);

		VertexBufferDesc vb_desc[] = { { VERTEX_BUFFER_POSITION, true }, { VERTEX_BUFFER_TEXCOORD, true } };

		MeshDesc mesh_desc = {};
		mesh_desc.vb_desc = vb_desc;
		mesh_desc.vb_count = ArrayCount(vb_desc);

		quad_renderer->vertex_buffers = UploadMesh(&mesh_data, &mesh_desc, renderer); 
	}
}

static void
CompileQuadShader(QuadRenderer* quad_renderer, Renderer* renderer) {
	char VertexShaderCode[] = R"FOO(

struct Quad {
	float3 tl;
	float3 tr;
	float3 bl;
	float3 br;
	float4 color;
};

struct ps {
	float4 pixel_pos : SV_POSITION;
	float4 color : COLOR;
};

cbuffer camera : register(b1) {
	float4x4 view_proj;
};

StructuredBuffer<Quad> quad_list : register(t0);

ps vsf(in uint vert_id : SV_VertexID) {
	ps output;
	int index = vert_id / 6;
	int vert_count = vert_id % 6;

	Quad quad = quad_list[index];

	float3 corner;
	if(vert_count == 0)
		corner = quad.tl;
	if(vert_count == 1)
		corner = quad.bl;
	if(vert_count == 2)
		corner = quad.br;

	if(vert_count == 3)
		corner = quad.br;
	if(vert_count == 4)
		corner = quad.tr;
	if(vert_count == 5)
		corner = quad.tl;

	float4 pos = float4(corner, 1.0);
	output.pixel_pos = mul(view_proj, pos);

	output.color = quad.color;
	return output;
}

	)FOO";

	char PixelShaderCode[] = R"FOO(

struct ps {
	float4 pixel_pos : SV_POSITION;
	float4 color : COLOR;
};

float4 psf(ps input) : SV_TARGET {
	return input.color;
}
	)FOO";

	{
		ShaderDesc sd = { VertexShaderCode, sizeof(VertexShaderCode), "vsf" };
		ConstantsBufferDesc cbd[] = { { CONSTANTS_BINDING_SLOT_CAMERA, sizeof(Mat4) } };
		StructuredBufferDesc sbd[] = { { STRUCTURED_BINDING_SLOT_FRAME, sizeof(RenderQuad), MAX_QUADS } };

		VertexShaderDesc vsd = {};
		vsd.shader = sd;
		vsd.cb_desc = cbd;
		vsd.sb_desc = sbd;
		vsd.cb_count = ArrayCount(cbd);
		vsd.sb_count = ArrayCount(sbd);

		quad_renderer->quad_vs = UploadVertexShader(vsd, renderer);
	}
	{
		PixelShaderDesc psd = {};
		ShaderDesc sd = { PixelShaderCode, sizeof(PixelShaderCode), "psf" };
		psd.shader = sd;

		quad_renderer->quad_ps = UploadPixelShader(psd, renderer);
	}
}

static QuadRenderer*
InitQuadRenderer(Renderer* renderer, MemoryArena* arena) {
	QuadRenderer* result = PushStruct(arena, QuadRenderer);

	CompileQuadShader(result, renderer);
	CompileTexturedQuadShader(result, renderer);
	return result;
}

static void
PushRenderQuad(Quad* quad, Vec4 color, QuadRenderer* quad_renderer) {
	Assert(quad_renderer->quad_count <= MAX_QUADS);
	RenderQuad render_quad = { *quad, color };
	quad_renderer->quads[quad_renderer->quad_count++] = render_quad;
}

static void
PushTexturedQuad(Quad* quad, RenderBuffer* texture_buffer, QuadRenderer* quad_renderer) {
	Assert(quad_renderer->textured_quad_count <= MAX_TEXTURED_QUADS);
	Vec2* texcoords = quad_renderer->texcoords;
	Vec3* positions = quad_renderer->positions;
	u32 textured_quad_count = quad_renderer->textured_quad_count;

	positions[textured_quad_count*4*3] = quad->bl;
	texcoords[textured_quad_count*4*2] = V2(0.0f, 1.0f);

	positions[textured_quad_count*4+1] = quad->br;
	texcoords[textured_quad_count*4+1] = V2(1.0f, 1.0f);

	positions[textured_quad_count*4+2] = quad->tl;
	texcoords[textured_quad_count*4+2] = V2(0.0f, 0.0f);

	positions[textured_quad_count*4+3] = quad->tr;
	texcoords[textured_quad_count*4+3] = V2(1.0f, 0.0f);

	quad_renderer->textures[quad_renderer->textured_quad_count++] = texture_buffer;
}

static void
PushRenderLine(Line* line, Vec4 color, float thickness, Camera* camera, QuadRenderer* quad_renderer) {
	Vec3 line_unit_vector = V3Norm(V3Sub(line->end, line->start));
	Vec3 camera_forward = GetForwardVector(camera->rotation);
	Vec3 perp = V3Cross(camera_forward, line_unit_vector);

	Quad quad = MakeQuadFromLine(line, thickness, perp);

	PushRenderQuad(&quad, color, quad_renderer);
}

static void
QuadRendererFrame(QuadRenderer* quad_renderer, Camera* cam, Renderer* renderer) {
	Mat4* vp = PushStruct(&renderer->transient, Mat4);
	*vp = MakeViewPerspective(cam);

	RenderPipeline rp = {};
	rp.rs = RenderStateDefaults();
	rp.rs.rs = RASTERIZER_STATE_DOUBLE_SIDED;
	rp.rs.bs = BLEND_STATE_ENABLED;
	rp.dc.type = DRAW_CALL_VERTICES;
	rp.dc.vertices_count = quad_renderer->quad_count * 6;
	rp.vs = quad_renderer->quad_vs;
	rp.ps = quad_renderer->quad_ps;

	rp.vrbd_count = 2;
	rp.vrbd = PushArray(&renderer->transient, PushRenderBufferData, rp.vrbd_count);

	rp.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
	rp.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
	rp.vrbd[0].constants.data = vp;

	rp.vrbd[1].type = RENDER_BUFFER_TYPE_STRUCTURED;
	rp.vrbd[1].structured.slot = STRUCTURED_BINDING_SLOT_FRAME;
	rp.vrbd[1].structured.count = quad_renderer->quad_count;
	rp.vrbd[1].structured.data = quad_renderer->quads;

	if(quad_renderer->quad_count) AddToRenderQueue(rp, renderer);

	if(quad_renderer->textured_quad_count) {
		rp.rs.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		rp.dc.type = DRAW_CALL_NONE;
		rp.vs = quad_renderer->textured_quad_vs;
		rp.ps = quad_renderer->textured_quad_ps;

		rp.vrbg = quad_renderer->vertex_buffers;

		rp.vrbd_count = 3;
		rp.vrbd = PushArray(&renderer->transient, PushRenderBufferData, rp.vrbd_count);

		rp.vrbd[0].type = RENDER_BUFFER_TYPE_CONSTANTS;
		rp.vrbd[0].constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
		rp.vrbd[0].constants.data = vp;

		rp.vrbd[1].type = RENDER_BUFFER_TYPE_VERTEX;
		rp.vrbd[1].vertex.type = VERTEX_BUFFER_POSITION;
		rp.vrbd[1].vertex.data = quad_renderer->positions;
		rp.vrbd[1].vertex.vertices_count = quad_renderer->textured_quad_count*4;

		rp.vrbd[2].type = RENDER_BUFFER_TYPE_VERTEX;
		rp.vrbd[2].vertex.type = VERTEX_BUFFER_TEXCOORD;
		rp.vrbd[2].vertex.data = quad_renderer->texcoords;
		rp.vrbd[2].vertex.vertices_count = quad_renderer->textured_quad_count*4;

		AddToRenderQueue(rp, renderer);

		rp = {};
		rp.rs.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		rp.dc.type = DRAW_CALL_VERTICES;
		rp.vs = quad_renderer->textured_quad_vs;
		rp.ps = quad_renderer->textured_quad_ps;

		rp.dc.vertices_count = 4;
		for(u32 i=0; i<quad_renderer->textured_quad_count; i++) {

			RenderBufferGroup* buffer_group = PushStruct(&renderer->transient, RenderBufferGroup);
			buffer_group->count = 1;
			buffer_group->rb = quad_renderer->textures[i];

			rp.dc.offset = i*4;
			rp.prbg = buffer_group;
			AddToRenderQueue(rp, renderer);
		}
	}

	quad_renderer->quad_count = 0;
	quad_renderer->textured_quad_count = 0;
}





