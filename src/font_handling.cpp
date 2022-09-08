
//Font info allocated here
static void LoadFont(char* info_path, char* image_path, FontInfo* font_info, Renderer* renderer) { 
	FILE* file = fopen(info_path, "rb");

	font_info = PushMaster(FontInfo, 1);
	fread(font_info, sizeof(FontInfo), 1, file); 

	int x, y, n;
	void* png = stbi_load(image_path, &x, &y, &n, 4);
	assert(png);

	TextureData texture_data = { TEXTURE_SLOT_ALBEDO, png, 
															 1000, 1000, 4 };

	RenderBuffer* rb = PushRenderBuffer(1, renderer);
	rb->type = RENDER_BUFFER_TYPE_TEXTURE;
	rb->texture = UploadTexture(texture_data, renderer->device);
	RenderBufferGroup rbg = { rb, 1 };
	PushRenderBufferGroup(RENDER_BUFFER_GROUP_FONT, rbg, renderer);

	STBI_FREE(png);
}

// TODO: Dump gltf to disk