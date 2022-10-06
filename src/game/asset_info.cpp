static TextureData*
LoadTextureData(TextureFormat* tf, GameAssets* assets) {
	TextureData* result = PushStruct(assets->permanent_arena, TextureData);

	result->pixels = assets->data + tf->offset_to_data;
	result->width = tf->width;
	result->height = tf->height;
	result->num_components = tf->num_components;

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
		texture_info->name = tf_array[i].name;
		texture_info->data = LoadTextureData(tf_array+i, assets);
	}
}

static void
UploadAllTextureAssets(GameAssets* assets, Renderer* renderer) {
	TextureAssetInfo* info = assets->texture_assets;
	if(info) {
		while(info) {
			info->buffer = UploadTexture(info->data->pixels, info->data->width,
					info->data->height, info->data->num_components, renderer);
			info = info->next;
		}
	}
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

