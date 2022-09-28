#define MAX_MESH_PIPELINES 10

struct MeshInfo {
	Mat4 model;
	Vec4 color;
};

struct MeshPipeline {
	RenderBufferGroup* vertex_buffers;
	void* instance_data;
	u32 instance_count;
};

struct MeshRenderer {
	MeshPipeline pipelines[MAX_MESH_PIPELINES];
	u32 count;

	VertexShader* vs;
	PixelShader* ps;
};

static void
PushMeshPipeline(MeshPipeline pipeline, MeshRenderer* mesh_renderer) {
	Assert(mesh_renderer->count < MAX_MESH_PIPELINES);
	mesh_renderer->pipelines[mesh_renderer->count++] = pipeline;
}

static void
CompileMeshShader(MeshRenderer* mesh_renderer, Renderer* renderer) {
	char ShaderCode[] = R"FOO(

struct MeshInfo {
	float4x4 model;
	float4 color;
};

struct vs {
	float3 position : POSITION;
};

cbuffer camera : register(b1) {
	float4x4 view_proj;
};

struct ps {
	float4 pixel_pos : SV_POSITION;
	float4 color : COLOR;
};

StructuredBuffer<MeshInfo> mesh_info_array : register(t0);

ps vsf(vs input, in uint instance_id : SV_InstanceID) {
	ps output;
	
	MeshInfo mesh_info = mesh_info_array[instance_id];

	float4 world_pos = mul(mesh_info.model, float4(input.position, 1.0f));
	output.pixel_pos = mul(view_proj, world_pos);
	output.color = mesh_info.color;

	return output;
}

float4 psf(ps input) : SV_Target {
	return input.color;
};
	)FOO";

	{
		ShaderDesc sd = { ShaderCode, sizeof(ShaderCode), "vsf" };
		ConstantsBufferDesc cbd[] = { { CONSTANTS_BINDING_SLOT_CAMERA, sizeof(Mat4) } };
		VERTEX_BUFFER vbd[] = { VERTEX_BUFFER_POSITION };
		StructuredBufferDesc sbd[] = { STRUCTURED_BINDING_SLOT_FRAME, sizeof(MeshInfo), MAX_MESH_PIPELINES };

		VertexShaderDesc vsd = {};
		vsd.shader = sd;
		vsd.cb_desc = cbd;
		vsd.vb_type = vbd;
		vsd.sb_desc = sbd;
		vsd.vb_count = ArrayCount(vbd);
		vsd.cb_count = ArrayCount(cbd);
		vsd.sb_count = ArrayCount(sbd);

		mesh_renderer->vs = UploadVertexShader(vsd, renderer);
	}
	{
		PixelShaderDesc psd = {};
		ShaderDesc sd = { ShaderCode, sizeof(ShaderCode), "psf" };
		psd.shader = sd;
		
		mesh_renderer->ps = UploadPixelShader(psd, renderer);
	}
}

MeshRenderer* 
InitMeshRenderer(Renderer* renderer, MemoryArena* arena) {
	MeshRenderer* result = PushStruct(arena, MeshRenderer);

	CompileMeshShader(result, renderer);

	return result;
}

static void
MeshRendererFrame(MeshRenderer* mesh_renderer, Camera* camera, Renderer* renderer) {
	if(mesh_renderer->count == 0) return;

	RenderPipeline rp = {};
	MeshPipeline mesh_pipeline = {};

	Mat4* vp = PushStruct(&renderer->transient, Mat4);
	*vp = MakeViewPerspective(camera);

	rp.vs = mesh_renderer->vs;

	rp.vrbd_count = 1;
	rp.vrbd = PushArray(&renderer->transient, PushRenderBufferData, rp.vrbd_count);
	rp.vrbd->type = RENDER_BUFFER_TYPE_CONSTANTS;
	rp.vrbd->constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
	rp.vrbd->constants.data = vp;

	AddToRenderQueue(rp, renderer);

	for(u32 i=0; i<mesh_renderer->count; i++) {
		rp = {};
		mesh_pipeline = mesh_renderer->pipelines[i];

		u32 vertices_count;
		for(u8 j=0; j<mesh_pipeline.vertex_buffers->count; j++) {
			if(mesh_pipeline.vertex_buffers->rb[j].type == RENDER_BUFFER_TYPE_VERTEX) {
				vertices_count = mesh_pipeline.vertex_buffers->rb[j].vertex.vertices_count;
				break;
			}
		}
		rp.rs = RenderStateDefaults();
		rp.dc.type = DRAW_CALL_INSTANCED;
		rp.dc.vertices_count = vertices_count;
		rp.dc.instance_count = mesh_pipeline.instance_count;

		rp.vs = mesh_renderer->vs;
		rp.ps = mesh_renderer->ps;

		rp.vrbg = mesh_pipeline.vertex_buffers;

		rp.vrbd_count = 1;
		rp.vrbd = PushArray(&renderer->transient, PushRenderBufferData, rp.vrbd_count);
		rp.vrbd->type = RENDER_BUFFER_TYPE_STRUCTURED;
		rp.vrbd->structured.slot = STRUCTURED_BINDING_SLOT_FRAME;
		rp.vrbd->structured.count = mesh_pipeline.instance_count;
		rp.vrbd->structured.data = mesh_pipeline.instance_data;

		AddToRenderQueue(rp, renderer);
	}

	mesh_renderer->count = 0;
}
























