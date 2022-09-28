#define MAX_TETRA 10

struct Tetra {
	Transform transform[MAX_TETRA];
	Vec4 color[MAX_TETRA];
	u32 count;

	MeshInfo info[MAX_TETRA];
	RenderBufferGroup* vertex_buffers;
};

static Tetra*
InitTetra(Renderer* renderer, MemoryArena* arena) {
	Tetra* result = PushStruct(arena, Tetra);

	Vec3* vertices = PushArray(&renderer->transient, Vec3, 12); 

	GenerateTetrahedron(vertices);

	VertexBufferData vbd[] = { vertices, VERTEX_BUFFER_POSITION };

	MeshData mesh_data = {};
	mesh_data.vb_data = vbd;
	mesh_data.vb_data_count = ArrayCount(vbd);
	mesh_data.vertices_count = 12;

	result->vertex_buffers = UploadMesh(&mesh_data, nullptr, renderer);
	return result;
}

static void
SpawnTetra(Tetra* tetra, Transform transform, Vec4 color) {
	tetra->transform[tetra->count] = transform;
	tetra->color[tetra->count] = color;
	tetra->count++;
}

static void
GenerateTetraInfo(Tetra* tetra) {
	for(u32 i=0; i<tetra->count; i++) {
		tetra->info[i].model = MakeTransformMatrix(tetra->transform[i]);
		tetra->info[i].color = tetra->color[i];
	}
}

static void
RenderTetra(Tetra* tetra, MeshRenderer* mesh_renderer) {
	MeshPipeline mesh_pipeline = {};
	mesh_pipeline.vertex_buffers = tetra->vertex_buffers;;
	mesh_pipeline.instance_data = (void*)tetra->info;
	mesh_pipeline.instance_count = tetra->count;
	PushMeshPipeline(mesh_pipeline, mesh_renderer);
}

