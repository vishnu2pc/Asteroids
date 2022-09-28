struct TextureAssetInfo {
	TextureData* data;
	RenderBuffer* render_buffer;
	char* name;
};

struct AssetInfo {
	TextureAssetInfo textures[MAX_TEXTURES];
	u32 texture_counter;
	MemoryArena permanent;
};

static TextureAssetInfo*
PushTextureAssetInfo(AssetInfo* info) {
	Assert(info->texture_counter < MAX_TEXTURES);
	TextureAssetInfo* result = info->textures + info->texture_counter++;
	return result;
}
