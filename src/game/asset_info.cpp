static TextureData*
LoadTextureData(TextureFormat* tf, GameAssets* assets) {
	TextureData* result = PushStruct(assets->permanent_arena, TextureData);

	result->pixels = assets->data + tf->offset_to_data;
	result->width = tf->width;
	result->height = tf->height;
	result->num_components = tf->num_components;

	return result;
}

static MeshAssetInfo*
PushMeshAssetInfo(GameAssets* ga) {
	MeshAssetInfo* result = 0;

	if(!ga->mesh_assets) {
		result = ga->mesh_assets = PushStruct(ga->permanent_arena, MeshAssetInfo);
	}
	else {
		result = ga->mesh_assets;
		while(result->next) result = result->next;
		result = result->next = PushStruct(ga->permanent_arena, MeshAssetInfo);
	}

	return result;
}

static TextureAssetInfo*
PushTextureAssetInfo(GameAssets* ga) {
	TextureAssetInfo* result = 0;

	if(!ga->texture_assets) {
		result = ga->texture_assets = PushStruct(ga->permanent_arena, TextureAssetInfo);
	}
	else {
		result = ga->texture_assets;
		while(result->next) result = result->next;
		result = result->next = PushStruct(ga->permanent_arena, TextureAssetInfo);
	}

	return result;
}

static MeshAssetInfo*
LoadMeshAsset(char* name, GameAssets* assets) {
	MeshAssetInfo* mesh_info = PushMeshAssetInfo(assets);

	MeshFormat* mf = GetMeshFormat(name, assets); 
	MeshData* md = LoadMeshData(mf, assets);

	mesh_info->name = mf->name;
	mesh_info->data = md;

	return mesh_info;
}

static TextureAssetInfo*
LoadTextureAsset(char* name, GameAssets* assets) {
	TextureAssetInfo* texture_info = PushTextureAssetInfo(assets);

	TextureFormat* tf = GetTextureFormat(name, assets); 
	TextureData* td = LoadTextureData(tf, assets);

	texture_info->name = tf->name;
	texture_info->data = td;

	return texture_info;
}

static void
LoadAllTextureAssets(GameAssets* assets) {
	u32 count = 0;
	TextureFormat* tf_array = GetAllTextureFormats(assets, &count);

	TextureAssetInfo* texture_info;
	TextureFormat* texture_format;
	for(u32 i=0; i<count; i++) {
		texture_info = PushTextureAssetInfo(assets);
		texture_format = tf_array+i;
		texture_info->name = texture_format->name;
		texture_info->data = LoadTextureData(texture_format, assets);
	}
}

static void
LoadAllMeshAssets(GameAssets* assets) {
	u32 count = 0;
	MeshFormat* mf_array = GetAllMeshFormats(assets, &count);

	MeshAssetInfo* mesh_info;
	MeshFormat* mesh_format;
	for(u32 i=0; i<count; i++) {
		mesh_info = PushMeshAssetInfo(assets);
		mesh_format = mf_array+i;
		mesh_info->name = mesh_format->name;
		mesh_info->data = LoadMeshData(mesh_format, assets);
	}
}

static void
UploadAllMeshAssets(GameAssets* assets, Renderer* renderer) {
	MeshAssetInfo* info = assets->mesh_assets;
	if(info) {
		while(info) {
			MeshData* mesh_data = info->data;
			Mesh* mesh = PushStruct(renderer->permanent_arena, Mesh);

			if(mesh_data->indices)
				mesh->index_buffer = UploadIndexBuffer(mesh_data->indices, mesh_data->indices_count, renderer);
			mesh->indices_count = mesh_data->indices_count;

			for(u8 i=0; i<info->data->vb_data_count; i++) {
				u8 num_components = 0;
				if(mesh_data->vb_data[i].type == VERTEX_BUFFER_POSITION) num_components = 3;
				else if(mesh_data->vb_data[i].type == VERTEX_BUFFER_NORMAL) num_components = 3;
				else continue; 
				mesh->vertex_buffers[i] = UploadVertexBuffer(mesh_data->vb_data[i].data, mesh_data->vertices_count,
						num_components, false, renderer);
				mesh->vertices_count = mesh_data->vertices_count;
				info->mesh = mesh;
			}
			info = info->next;
		}
	}
}

static void
UploadAllTextureAssets(GameAssets* assets, Renderer* renderer) {
	TextureAssetInfo* info = assets->texture_assets;
	if(info) {
		while(info) {
			info->buffer = UploadTexture(info->data->pixels, info->data->width,
					info->data->height, info->data->num_components, false, false, renderer);
			info = info->next;
		}
	}
}


static MeshAssetInfo*
GetMeshAssetInfo(char* name, GameAssets* assets) {
	MeshAssetInfo* result = 0;

	bool found = false;
	
	Assert(assets->mesh_assets);
	MeshAssetInfo* info = assets->mesh_assets;
	while(info) {
		if(StringCompare(info->name, name)) {
			result = info;
			found = true;
			break;
		}
		info = info->next;
	}

	Assert(found);
	return result;
}

static TextureAssetInfo*
GetTextureAssetInfo(char* name, GameAssets* assets) {
	TextureAssetInfo* result = 0;

	bool found = false;
	
	Assert(assets->texture_assets);
	TextureAssetInfo* info = assets->texture_assets;
	while(info) {
		if(StringCompare(info->name, name)) {
			result = info;
			found = true;
			break;
		}
		info = info->next;
	}

	Assert(found);
	return result;
}

