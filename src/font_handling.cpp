
//Font info allocated here
static void LoadFont(char* info_path, char* image_path, DebugText* dt, Renderer* renderer) { 
	FILE* file = fopen(info_path, "rb");

	fread(&dt->info, sizeof(FontInfo), 1, file); 

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

	fclose(file);
	STBI_FREE(png);
}

// from stb_tt
GlyphQuad MakeGlyphQuad(PackedChar *pc, int pw, int ph, float *xpos, float *ypos)
{
	GlyphQuad q = {};
	float ipw = 1.0f / pw, iph = 1.0f / ph;
	PackedChar* b = pc;

	/*
	if (align_to_integer) {
		float x = (float) STBTT_ifloor((*xpos + b->xoff) + 0.5f);
		float y = (float) STBTT_ifloor((*ypos + b->yoff) + 0.5f);
		q->x0 = x;
		q->y0 = y;
		q->x1 = x + b->xoff2 - b->xoff;
		q->y1 = y + b->yoff2 - b->yoff;
		*/
	q.x0 = *xpos + b->xoff;
	q.y0 = *ypos + b->yoff;
	q.x1 = *xpos + b->xoff2;
	q.y1 = *ypos + b->yoff2;

	q.u0 = b->x0 * ipw;
	q.v0 = b->y0 * iph;
	q.u1 = b->x1 * ipw;
	q.v1 = b->y1 * iph;

	*xpos += b->xadvance;

	return q;
}

static void BeginDebugText(DebugText* dt, WindowDimensions wd) {
	dt->w = wd.width;
	dt->h = wd.height;
	dt->glyph_counter = 0;
	dt->line_count[QUADRANT_TOP_LEFT] = 0;
	dt->line_count[QUADRANT_TOP_RIGHT] = 0;
	dt->line_count[QUADRANT_BOTTOM_LEFT] = 0;
	dt->line_count[QUADRANT_BOTTOM_RIGHT] = 0;
};

static void DrawDebugText(char* text, Vec3 color, float scale, QUADRANT quad, DebugText* dt) {
	assert(text);
	float x, y;
	x = 0.0f;
	int line_width = dt->info.ascent - dt->info.descent + dt->info.line_gap;
	if(quad == QUADRANT_TOP_LEFT) y = (dt->line_count[QUADRANT_TOP_LEFT] + 1) * line_width;  
	y *= 0.8f*scale;

	for(u8 i=0; text[i] != 0; i++) {
		u8 glyph_index = text[i] - 0x20;
		PackedChar pc = dt->info.pc[glyph_index];

		GlyphQuad gq = MakeGlyphQuad(&pc, dt->info.bitmap_w, dt->info.bitmap_h, &x, &y);

		gq.x0 = (gq.x0 * 0.25f * scale)/dt->w;
		gq.y0 = (gq.y0 * 0.25f * scale)/dt->h;
		gq.x1 = (gq.x1 * 0.25f * scale)/dt->w;
		gq.y1 = (gq.y1 * 0.25f * scale)/dt->h;

		gq.color = color;

		PushGlyph(gq, dt);
	}
	dt->line_count[quad]++;
};