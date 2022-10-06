#define MAX_MESH_PIPELINES 100

struct LightInfo {
	Vec3 position;
	float ambience;
};

struct Mesh {
	VertexBuffer* vertex_buffers[2];
	u8 topology;
	u32 vertices_count;
};

struct MeshInfo {
	Mat4 model;
	Vec4 color;
};

struct MeshPipeline {
	Mesh mesh;
	MeshInfo* info;
};

struct MeshRenderer {
	MeshPipeline pipelines[MAX_MESH_PIPELINES];
	u32 count;

	ConstantsBuffer* camera_constants;
	ConstantsBuffer* mesh_constants;
	ConstantsBuffer* light_constants;
	VertexShader* vs;
	PixelShader* ps;
};

static void
PushMeshPipeline(MeshPipeline pipeline, MeshRenderer* mesh_renderer) {
	Assert(mesh_renderer->count < MAX_MESH_PIPELINES);
	mesh_renderer->pipelines[mesh_renderer->count++] = pipeline;
}

static void
InitMeshShader(MeshRenderer* mesh_renderer, Renderer* renderer) {
	VERTEX_BUFFER vb[] = { VERTEX_BUFFER_POSITION, VERTEX_BUFFER_NORMAL };
	mesh_renderer->vs = UploadVertexShader(MeshShader, sizeof(MeshShader), "vsf", vb, ArrayCount(vb), renderer);

	mesh_renderer->ps = UploadPixelShader(MeshShader, sizeof(MeshShader), "psf", renderer);
	mesh_renderer->camera_constants = UploadConstantsBuffer(sizeof(Mat4), renderer);
	mesh_renderer->light_constants = UploadConstantsBuffer(sizeof(LightInfo), renderer);
	mesh_renderer->mesh_constants = UploadConstantsBuffer(sizeof(MeshInfo), renderer);
}

MeshRenderer* 
InitMeshRenderer(Renderer* renderer, MemoryArena* arena) {
	MeshRenderer* result = PushStruct(arena, MeshRenderer);

	InitMeshShader(result, renderer);

	return result;
}

static void
MeshRendererFrame(MeshRenderer* mesh_renderer, Camera* camera, LightInfo* light, Renderer* renderer) {

#ifdef INTERNAL
	if(executable_reloaded) InitMeshShader(mesh_renderer, renderer);
#endif INTERNAL

	if(mesh_renderer->count == 0) return;

	Mat4* vp = PushStruct(renderer->frame_arena, Mat4);
	*vp = MakeViewPerspective(camera);

	SetBlendState* set_blend_state = PushRenderCommand(renderer, SetBlendState);
	set_blend_state->type = BLEND_STATE_Regular;

	SetVertexShader* set_vertex_shader = PushRenderCommand(renderer, SetVertexShader);
	set_vertex_shader->vertex = mesh_renderer->vs;

	SetPixelShader* set_pixel_shader = PushRenderCommand(renderer, SetPixelShader);
	set_pixel_shader->pixel = mesh_renderer->ps;

	SetConstantsBuffer* set_camera_constants = PushRenderCommand(renderer, SetConstantsBuffer);
	set_camera_constants->vertex_shader = true;
	set_camera_constants->constants = mesh_renderer->camera_constants;
	set_camera_constants->slot = 1;

	SetConstantsBuffer* set_light_constants = PushRenderCommand(renderer, SetConstantsBuffer);
	set_light_constants->vertex_shader = false;
	set_light_constants->constants = mesh_renderer->light_constants;
	set_light_constants->slot = 1;

	SetConstantsBuffer* set_mesh_constants = PushRenderCommand(renderer, SetConstantsBuffer);
	set_mesh_constants->vertex_shader = true;
	set_mesh_constants->constants = mesh_renderer->mesh_constants;
	set_mesh_constants->slot = 2;

	PushRenderBufferData* push_camera_constants = PushRenderCommand(renderer, PushRenderBufferData);
	push_camera_constants->buffer = mesh_renderer->camera_constants->buffer;
	push_camera_constants->size = sizeof(Mat4);
	push_camera_constants->data = vp;

	PushRenderBufferData* push_light_constants = PushRenderCommand(renderer, PushRenderBufferData);
	push_light_constants->buffer = mesh_renderer->light_constants->buffer;
	push_light_constants->size = sizeof(LightInfo);
	push_light_constants->data = light;

	for(u32 i=0; i<mesh_renderer->count; i++) {
		MeshPipeline* mesh_pipeline = mesh_renderer->pipelines + i;

		SetPrimitiveTopology* set_topology = PushRenderCommand(renderer, SetPrimitiveTopology);
		set_topology->type = mesh_pipeline->mesh.topology;

		{
			SetVertexBuffer* set_vertex_buffer = PushRenderCommand(renderer, SetVertexBuffer);
			set_vertex_buffer->vertex= mesh_pipeline->mesh.vertex_buffers[0];
			set_vertex_buffer->vertices_count = mesh_pipeline->mesh.vertices_count;
			set_vertex_buffer->offset = 0;
			set_vertex_buffer->stride = sizeof(float)*3;
			set_vertex_buffer->slot = 0;
		} {
			SetVertexBuffer* set_vertex_buffer = PushRenderCommand(renderer, SetVertexBuffer);
			set_vertex_buffer->vertex= mesh_pipeline->mesh.vertex_buffers[1];
			set_vertex_buffer->vertices_count = mesh_pipeline->mesh.vertices_count;
			set_vertex_buffer->offset = 0;
			set_vertex_buffer->stride = sizeof(float)*3;
			set_vertex_buffer->slot = 1;
		}

		PushRenderBufferData* push_mesh_info = PushRenderCommand(renderer, PushRenderBufferData);
		push_mesh_info->buffer = mesh_renderer->mesh_constants->buffer;
		push_mesh_info->size = sizeof(MeshInfo);
		push_mesh_info->data = mesh_pipeline->info;

		DrawVertices* draw_vertices = PushRenderCommand(renderer, DrawVertices);
		draw_vertices->vertices_count = mesh_pipeline->mesh.vertices_count;
	}

	mesh_renderer->count = 0;
}




