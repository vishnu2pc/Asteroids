#define MAX_MESH_PIPELINES 10
#define MAX_SHADER_LENGTH 5000

struct LightInfo {
	Vec3 position;
	float ambience;
};

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

	char shader_code[MAX_SHADER_LENGTH];
	VertexShader* vs;
	PixelShader* ps;
};

static void
PushMeshPipeline(MeshPipeline pipeline, MeshRenderer* mesh_renderer) {
	Assert(mesh_renderer->count < MAX_MESH_PIPELINES);
	mesh_renderer->pipelines[mesh_renderer->count++] = pipeline;
}
char MeshShaderCode[] = R"FOO(

struct MeshInfo {
	float4x4 model;
	float4 color;
};

struct vs {
	float3 position : POSITION;
	float3 normal : NORMAL;
};

cbuffer camera : register(b1) {
	float4x4 view_proj;
};

struct ps {
	float4 pixel_pos : SV_POSITION;
	float3 vertex_pos : POSITION;
	float3 normal : NORMAL;
	float4 color : COLOR;
};

StructuredBuffer<MeshInfo> mesh_info_array : register(t0);

cbuffer light : register(b1) {
	float3 light_position;
	float light_ambience;
};

ps vsf(vs input, in uint instance_id : SV_InstanceID) {
	ps output;
	
	MeshInfo mesh_info = mesh_info_array[instance_id];

	float4 world_pos = mul(mesh_info.model, float4(input.position, 1.0f));
	output.pixel_pos = mul(view_proj, world_pos);
	output.vertex_pos = world_pos.xyz;
	output.normal = mul(mesh_info.model, float4(input.normal, 0.0)).xyz;
	output.color = mesh_info.color;

	return output;
}

float4 psf(ps input) : SV_Target {
	float3 light_dir = normalize(input.vertex_pos - light_position);

	float cos = max(0.0, dot(input.normal, light_dir));

	float3 diffuse_color = cos * input.color.rgb;
	float3 ambient_color = input.color.rbg * light_ambience;

	float3 final_color = ambient_color + diffuse_color;

	return float4(final_color, input.color.a);
};
	)FOO";

static void
CompileMeshShader(MeshRenderer* mesh_renderer, Renderer* renderer) {

	u32 shader_length = sizeof(MeshShaderCode);
	CopyMem(mesh_renderer->shader_code, MeshShaderCode, shader_length+1);
	{
		ShaderDesc sd = { MeshShaderCode, sizeof(MeshShaderCode), "vsf" };
		ConstantsBufferDesc cbd[] = { { CONSTANTS_BINDING_SLOT_CAMERA, sizeof(Mat4) } };
		VERTEX_BUFFER vbd[] = { VERTEX_BUFFER_POSITION, VERTEX_BUFFER_NORMAL };
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
		ShaderDesc sd = { MeshShaderCode, sizeof(MeshShaderCode), "psf" };
		ConstantsBufferDesc cbd[] = { { CONSTANTS_BINDING_SLOT_CAMERA, sizeof(LightInfo) } };

		psd.shader = sd;
		psd.cb_desc = cbd;
		psd.cb_count = ArrayCount(cbd);
		
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
MeshRendererFrame(MeshRenderer* mesh_renderer, Camera* camera, LightInfo* light, Renderer* renderer) {
	if(!StringCompare(mesh_renderer->shader_code, MeshShaderCode)) {
		ID3D11VertexShader* vs;
		ID3D11PixelShader* ps;
		ID3DBlob* blob;
		if(CompileShader(MeshShaderCode, sizeof(MeshShaderCode), "vsf", (void**)&vs, &blob, true, renderer)) {
			if(CompileShader(MeshShaderCode, sizeof(MeshShaderCode), "psf", (void**)&ps, &blob, false, renderer)) {
				mesh_renderer->vs->shader = vs;
				mesh_renderer->ps->shader = ps;
				CopyMem(mesh_renderer->shader_code, MeshShaderCode, sizeof(MeshShaderCode)+1);
			}
		}
}

	if(mesh_renderer->count == 0) return;

	RenderPipeline rp = {};
	MeshPipeline mesh_pipeline = {};

	Mat4* vp = PushStruct(&renderer->transient, Mat4);
	*vp = MakeViewPerspective(camera);

	rp.vs = mesh_renderer->vs;
	rp.ps = mesh_renderer->ps;

	rp.vrbd_count = 1;
	rp.vrbd = PushArray(&renderer->transient, PushRenderBufferData, rp.vrbd_count);
	rp.vrbd->type = RENDER_BUFFER_TYPE_CONSTANTS;
	rp.vrbd->constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
	rp.vrbd->constants.data = vp;

	rp.prbd_count = 1;
	rp.prbd = PushArray(&renderer->transient, PushRenderBufferData, rp.prbd_count);
	rp.prbd->type = RENDER_BUFFER_TYPE_CONSTANTS;
	rp.prbd->constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
	rp.prbd->constants.data = light;

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
























