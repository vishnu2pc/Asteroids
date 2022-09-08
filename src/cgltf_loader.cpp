enum TEXTURE_SLOT {
	TEXTURE_SLOT_ALBEDO,
	TEXTURE_SLOT_NORMAL,
};

enum VERTEX_BUFFER_TYPE {	
	VERTEX_BUFFER_TYPE_NOT_SET,
	VERTEX_BUFFER_TYPE_POSITION, 
	VERTEX_BUFFER_TYPE_NORMAL, 
	VERTEX_BUFFER_TYPE_TANGENT,
	VERTEX_BUFFER_TYPE_COLOR, 
	VERTEX_BUFFER_TYPE_TEXCOORD, 
};

//TODO: Move index buffers in here

struct VertexBufferData {
	float* data;
	VERTEX_BUFFER_TYPE type;
};

struct MeshData {
	VertexBufferData* vb_data; 
	u32 vb_data_count;
		
	u32* indices;
	u32 vertices_count;
	u32 indices_count;

	char* name;
};

struct TextureData {
	TEXTURE_SLOT type;
	void* data;
	u32 width;
	u32 height;
	u8 num_components;
};

struct MaterialData {
	TextureData* texture_data;
	u8 count;
	char* name;
};

struct ModelData {
	MeshData* mesh_data;
	MaterialData* material_data;
	u8 mesh_count;
	u8 mat_count;

	u8* mat_id;
	u8 id_count; 

	char* name;
};

// TODO: Make vertex values contiguous
static MeshData GetMeshDataFromCGLTF(cgltf_primitive primitive) {
	MeshData mesh_data = {};

	u32 vb_data_count = 0;
	for (u8 a = 0; a < primitive.attributes_count; a++) {
		if((primitive.attributes[a].type == cgltf_attribute_type_normal) ||
			 (primitive.attributes[a].type == cgltf_attribute_type_position) ||
			 (primitive.attributes[a].type == cgltf_attribute_type_color) ||
			 (primitive.attributes[a].type == cgltf_attribute_type_tangent) ||
			 (primitive.attributes[a].type == cgltf_attribute_type_texcoord))
			vb_data_count++;
	}
	mesh_data.vb_data = PushMaster(VertexBufferData, vb_data_count);
	mesh_data.vb_data_count = vb_data_count;

	u32 vb_counter = 0;
	for (int a = 0; a < primitive.attributes_count; a++) {
		assert(primitive.type == cgltf_primitive_type_triangles);

		cgltf_attribute* att = &primitive.attributes[a];
		cgltf_accessor* acc = att->data;
		cgltf_size acc_count = acc->count;
		mesh_data.vertices_count = acc_count;
			
		cgltf_size float_count = cgltf_accessor_unpack_floats(acc, NULL, 0);

		switch (att->type) {
			case cgltf_attribute_type_position: {
				assert(acc->component_type == cgltf_component_type_r_32f);
				assert(acc->type == cgltf_type_vec3);

				mesh_data.vb_data[vb_counter].type = VERTEX_BUFFER_TYPE_POSITION;
				float* data = PushMaster(float, float_count);
				cgltf_accessor_unpack_floats(acc, data, float_count);
				mesh_data.vb_data[vb_counter].data = data;
				vb_counter++;
			} break;

			case cgltf_attribute_type_normal: {
				assert(acc->component_type == cgltf_component_type_r_32f);
				assert(acc->type == cgltf_type_vec3);

				mesh_data.vb_data[vb_counter].type = VERTEX_BUFFER_TYPE_NORMAL;
				float* data = PushMaster(float, float_count);
				cgltf_accessor_unpack_floats(acc, data, float_count);
				mesh_data.vb_data[vb_counter].data = data;
				vb_counter++;
			} break;

			case cgltf_attribute_type_texcoord: {
				assert(acc->component_type == cgltf_component_type_r_32f);
				assert(acc->type == cgltf_type_vec2);

				mesh_data.vb_data[vb_counter].type = VERTEX_BUFFER_TYPE_TEXCOORD;
				float* data = PushMaster(float, float_count);
				cgltf_accessor_unpack_floats(acc, data, float_count);
				mesh_data.vb_data[vb_counter].data = data;
				vb_counter++;
			} break;
			
			case cgltf_attribute_type_color: {
				assert(acc->component_type == cgltf_component_type_r_32f);
				assert(acc->type == cgltf_type_vec3);

				mesh_data.vb_data[vb_counter].type = VERTEX_BUFFER_TYPE_COLOR;
				float* data = PushMaster(float, float_count);
				data = PushMaster(float, float_count);
				cgltf_accessor_unpack_floats(acc, data, float_count);
				mesh_data.vb_data[vb_counter].data = data;
				vb_counter++;
			} break;

			case cgltf_attribute_type_tangent: {
				assert(acc->component_type == cgltf_component_type_r_32f);
				assert(acc->type == cgltf_type_vec4);
				
				mesh_data.vb_data[vb_counter].type = VERTEX_BUFFER_TYPE_TANGENT;
				float* data = PushMaster(float, float_count);
				data = PushMaster(float, float_count);
				cgltf_accessor_unpack_floats(acc, data, float_count);
				mesh_data.vb_data[vb_counter].data = data;
				vb_counter++;
			} break;
		}
	}
	if (primitive.indices) {
		u32 count = primitive.indices->count;
		mesh_data.indices_count = count;
		mesh_data.indices = PushMaster(u32, count);
		for (int i = 0; i < count; i++) {
			mesh_data.indices[i] = (u32)cgltf_accessor_read_index(primitive.indices, i);
		}
	}
	return mesh_data;
}

static char* GetImagePathFromTexture(char* uri, char* path) {
	u32 size = strlen(uri) + strlen(path) + 1;
	char* new_path = PushScratch(char, size);
	cgltf_combine_paths(new_path, path, uri);
	cgltf_decode_uri(new_path + strlen(path) - strlen(uri));

	PopScratch(char, size);
	return new_path;
};

static TextureData GetTextureFromCGLTF(cgltf_texture texture_data, char* path) {
	TextureData result = {};
	assert(texture_data.image->uri);
	char* imagepath = GetImagePathFromTexture(texture_data.image->uri, path);
	int x, y, n;
	unsigned char* data = stbi_load(imagepath, &x, &y, &n, 4);
	assert(data);

	result.data = data;
	result.width = x;
	result.height = y;
	// RESEARCH: We are forcing 4 channel images as d3d11 is fucky with 3 channel images
	result.num_components = 4;
	return result;
}

static MaterialData GetMaterialFromCGLTF(cgltf_material cmat, char* path) {
	MaterialData mat = {};
	mat.name = PushMaster(char, strlen(cmat.name) + 1);
	strcpy(mat.name, cmat.name);

	/*
	// TODO: Handle specular glossiness
	if (cmat.normal_texture.texture_data) mat.count++;
	if (cmat.pbr_metallic_roughness.base_color_texture.texture_data) mat.count++;
	if (cmat.pbr_metallic_roughness.metallic_roughness_texture.texture_data) mat.count++;
	if (cmat.pbr_specular_glossiness.diffuse_texture.texture_data) mat.count++;
	if (cmat.pbr_specular_glossiness.specular_glossiness_texture.texture_data) mat.count++;

	mat.textures = PushMaster(TextureData, mat.count);
	u8 texture_counter = 0;
	// This might be the wrong null check
	// TODO: Read up on sampler importing
	if (cmat.normal_texture.texture_data) {
		mat.textures[texture_counter] = GetTextureFromCGLTF(*cmat.normal_texture.texture_data, path);
		mat.textures[texture_counter].type = TEXTURE_SLOT_NORMAL;
		texture_counter++;
	}

	if (cmat.has_pbr_metallic_roughness) {
		mat.type = MT_PBR_MR;
		cgltf_pbr_metallic_roughness pbr = cmat.pbr_metallic_roughness;
		memcpy(mat.mr_constants.base_color_factor, pbr.base_color_factor, sizeof(float) * 4);
		mat.mr_constants.metallic_factor = pbr.metallic_factor;
		mat.mr_constants.roughness_factor = pbr.roughness_factor;

		if (pbr.base_color_texture.texture_data) {
			mat.textures[texture_counter] = GetTextureFromCGLTF(*pbr.base_color_texture.texture_data, path);
			mat.textures[texture_counter].type = TEXTURE_BASE_COLOR;
			texture_counter++;
		}
		if (pbr.metallic_roughness_texture.texture_data) {
			// Will the num_components need to be manually set?
			mat.textures[texture_counter] = GetTextureFromCGLTF(*pbr.metallic_roughness_texture.texture_data, path);
			mat.textures[texture_counter].type = TEXTURE_METALLIC_ROUGHNESS;
			texture_counter++;
		}
	}

	else if (cmat.has_pbr_specular_glossiness) {
		mat.type = MT_PBR_SG;
		cgltf_pbr_specular_glossiness pbr = cmat.pbr_specular_glossiness;
		memcpy(mat.sg_constants.diffuse_factor, pbr.diffuse_factor, sizeof(float) * 4);
		memcpy(mat.sg_constants.specular_factor, pbr.specular_factor, sizeof(float) * 3);
		mat.sg_constants.glossiness_factor = pbr.glossiness_factor;

		if (cmat.pbr_specular_glossiness.diffuse_texture.texture_data) {
		
		}
	}

	assert(texture_counter == mat.count);
		*/
	return mat;
}

// use this function while loading models
static TextureData LoadTexture(char* path) {
	TextureData texture_data = {};
	int x, y, n;
	void* data = stbi_load(path, &x, &y, &n, 4);
	assert(data);
	texture_data.data = data;
	texture_data.width = x;
	texture_data.height = y;
	texture_data.num_components = 4;
	return texture_data;
}
// TODO: Handle unnamed mesh_data and material_data
static ModelData LoadModelDataGLTF(char* path, char* name) {
	assert(path);
	assert(name);

	ModelData model_data = {};

	cgltf_data* data;
	cgltf_options opt = {};
	assert(cgltf_parse_file(&opt, path, &data) == cgltf_result_success);
	assert(cgltf_validate(data) == cgltf_result_success);
	assert(cgltf_load_buffers(&opt, data, path) == cgltf_result_success);

	// TODO: do assert checks for counts
	//model_data.name = PushMaster(char, strlen(name) + 1);
	//strcpy(model_data.name, name);

	model_data.mesh_data = PushMaster(MeshData, data->meshes_count);
	model_data.mesh_count = data->meshes_count;
	model_data.material_data = PushMaster(MaterialData, data->materials_count); 
	model_data.mat_count = data->materials_count;
	model_data.id_count = data->meshes_count;
	model_data.mat_id = PushMaster(u8, data->meshes_count);

	for (int i = 0; i < data->materials_count; i++) {
		cgltf_material cmat = data->materials[i];
		MaterialData new_mat = GetMaterialFromCGLTF(cmat, path);
		model_data.material_data[i] = new_mat;
	}

	for (int i = 0; i < data->meshes_count; i++) {
		cgltf_mesh cmesh = data->meshes[i];

		for (int j = 0; j < cmesh.primitives_count; j++) {
			MeshData mesh_data = GetMeshDataFromCGLTF(cmesh.primitives[j]);

			mesh_data.name = PushMaster(char, strlen(cmesh.name) + 1);
			strcpy(mesh_data.name, cmesh.name);
			model_data.mesh_data[j] = mesh_data;

			if(cmesh.primitives[j].material) {
				char* mat_name = cmesh.primitives[j].material->name;
				for (int k = 0; k < data->materials_count; k++) 
					if (strcmp(mat_name, model_data.material_data[k].name)) 
						model_data.mat_id[j] = k;
			}
		}
	}
	cgltf_free(data);
	return model_data;
}
