#include "asset_info.h"

static AssetInfo*
InitAssetInfo(MemoryArena* arena) {
	AssetInfo* result = 0;
	u8* memblock = GetMemory(arena, Megabytes(1));
	result = (AssetInfo*)memblock;
	result->permanent.memory.bp = memblock;
	result->permanent.memory.size = Megabytes(1);
	result->permanent.used = sizeof(AssetInfo);

	return result;
}

static TextureData*
LoadTextureData(TextureFormat* tf, GameAssets* assets, MemoryArena* arena) {
	TextureData* result = PushStruct(arena, TextureData);

	result->pixels = assets->data + tf->offset_to_data;
	result->width = tf->width;
	result->height = tf->height;
	result->num_components = tf->num_components;

	return result;
}

static TextureAssetInfo*
LoadTextureAsset(char* name, GameAssets* assets, AssetInfo* asset_info) {
	TextureAssetInfo* texture_info = PushTextureAssetInfo(asset_info);

	TextureFormat* tf = GetTextureFormat(name, assets); 
	TextureData* td = LoadTextureData(tf, assets, &asset_info->permanent);

	texture_info->name = tf->name;
	texture_info->data = td;

	return texture_info;
}

static void
LoadAllTextureAssets(GameAssets* assets, AssetInfo* info) {
	u32 count = 0;
	TextureFormat* tf_array = GetAllTextureFormats(assets, &count);

	TextureAssetInfo* texture_info;
	TextureFormat* texture_format;
	for(u32 i=0; i<count; i++) {
		texture_info = PushTextureAssetInfo(info);
		texture_format = tf_array+i;
		texture_info->name = tf_array[i].name;
		texture_info->data = LoadTextureData(tf_array+i, assets, &info->permanent);
	}
}

static void
UploadAllTextureAssets(AssetInfo* info, Renderer* renderer) {
	TextureAssetInfo* texture_info;
	for(u32 i=0; i<info->texture_counter; i++) {
		texture_info = info->textures + i;
		if(texture_info->data->pixels) {
			texture_info->render_buffer = UploadTexture(info->textures[i].data, renderer);
		}
	}
}

static TextureAssetInfo*
GetTextureAssetInfo(char* name, AssetInfo* info) {
	TextureAssetInfo* result = 0;
	Assert(name);
	bool found = false;
	u32 i=0;
	for(i=0; i<info->texture_counter; i++) {
		if(StringCompare(info->textures[i].name, name)) {
			result = info->textures + i;
			found = true;
			break;
		}
	}
	Assert(found);
	return result;
}

