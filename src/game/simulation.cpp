enum ENTITIY_PROPERTY {
	ENTITY_PROPERTY_HasMesh      = 1,
	ENTITY_PROPERTY_Rotate       = 1<<1,
	ENTITY_PROPERTY_TOTAL
};

struct Entity {
	u64 properties;
	Transform transform;
	MeshPipeline mesh_pipeline;
};

static Mesh
MakeTetrahedronMesh(Renderer* renderer) {
	Mesh result = {};

	Vec3* vertices = PushArray(renderer->frame_arena, Vec3, 12); 
	Vec3* normals = PushArray(renderer->frame_arena, Vec3, 12);

	GenerateTetrahedron(vertices);
	GenerateFlatShadedNormals(vertices, 12, normals);

	result.vertex_buffers[0] = UploadVertexBuffer(vertices, 12, 3, false, renderer);
	result.vertex_buffers[1] = UploadVertexBuffer(normals, 12, 3, false, renderer);
	result.topology = PRIMITIVE_TOPOLOGY_TriangleList;
	result.vertices_count = 12;

	return result;
}

static Entity*
SpawnSimpleEntity(Mesh mesh, Transform transform, Renderer* renderer, MemoryArena* arena) {
	Entity* result = PushStruct(arena, Entity);

	result->properties |= ENTITY_PROPERTY_HasMesh;
	result->properties |= ENTITY_PROPERTY_Rotate;

	result->transform = transform;
	result->mesh_pipeline.mesh = mesh;

	return result;
}

static void
UpdateEntity(Entity* entity, MeshRenderer* mesh_renderer, MemoryArena* frame_arena) {

	if(entity->properties & ENTITY_PROPERTY_Rotate) {
		entity->transform.rotation = QuatMul(QuatFromEuler((3.0f)*0.01f, 0.0f, 0.0f), entity->transform.rotation);
	}

	if(entity->properties & ENTITY_PROPERTY_HasMesh) {
		entity->mesh_pipeline.info = PushStruct(frame_arena, MeshInfo);
		entity->mesh_pipeline.info->model = MakeTransformMatrix(entity->transform);
		entity->mesh_pipeline.info->color = V4FromV3(WHITE, 1.0f);
		PushMeshPipeline(entity->mesh_pipeline, mesh_renderer);
	}

}












