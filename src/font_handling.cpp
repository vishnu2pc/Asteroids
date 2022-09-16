// Stack no alloc
struct DebugText {
	FontInfo info;
	GlyphQuad quads[MAX_DEBUG_TEXT_GLYPHS];
	u32 glyph_counter;
	u32 w, h; // Screen res
	u8 line_count[QUADRANT_TOTAL];
	//float font_size;
};

static void PushGlyph(GlyphQuad gq, DebugText* dt) {
	assert(dt->glyph_counter < MAX_DEBUG_TEXT_GLYPHS);
	dt->quads[dt->glyph_counter] = gq;
	dt->glyph_counter++;
};

//Font info allocated here
static void LoadFont(char* info_path, char* image_path, DebugText* dt, Renderer* renderer) { 
	FILE* file = fopen(info_path, "rb");

	fread(&dt->info, sizeof(FontInfo), 1, file); 
	UploadTextureFromFile(image_path, TEXTURE_SLOT_ALBEDO, RENDER_BUFFER_GROUP_FONT, renderer);
	fclose(file);
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

static void GenerateGlyphsTopLeft(char* text, int len, Vec3 color, GlyphQuad* gq, float* x, float* y, DebugText* dt) {
	for(int i=0; i<=len; i++) {
		u8 glyph_index = text[i] - 0x20;
		PackedChar pc = dt->info.pc[glyph_index];

		gq[i] = MakeGlyphQuad(&pc, dt->info.bitmap_w, dt->info.bitmap_h, x, y);

		gq[i].x0 = (gq[i].x0 * 0.20f);
		gq[i].y0 = (gq[i].y0 * 0.20f);
		gq[i].x1 = (gq[i].x1 * 0.20f);
		gq[i].y1 = (gq[i].y1 * 0.20f);

		gq[i].color = color;
	}
}

// Extremely hacky but it will do
static void DrawDebugText(char* text, Vec3 color, QUADRANT quad, DebugText* dt, MemoryArena* arena) {
	assert(text);
	int len = strlen(text);
	float x, y;
	x = 0.0f;
	int font_line_width = dt->info.ascent - dt->info.descent + dt->info.line_gap;
	GlyphQuad* gq = PushArray(arena, GlyphQuad, len);
	float line_width = (dt->line_count[quad] + 1) * font_line_width * 0.8f;

	switch(quad) {
		case QUADRANT_TOP_LEFT: {
			y = line_width;
			GenerateGlyphsTopLeft(text, len, color, gq, &x, &y, dt);
			for(u8 i=0; i<=len; i++) {
				gq[i].x0 /= dt->w;
				gq[i].y0 /= dt->h;
				gq[i].x1 /= dt->w;
				gq[i].y1 /= dt->h;

				PushGlyph(gq[i], dt);
			}
		} break;

		case QUADRANT_TOP_RIGHT: {
			y = line_width;
			GenerateGlyphsTopLeft(text, len, color, gq, &x, &y, dt);
			float xpos = dt->w - gq[len].x1;

			for(int i=0; i<=len; i++) {
				gq[i].x0 = (gq[i].x0 + xpos)/dt->w;
				gq[i].x1 = (gq[i].x1 + xpos)/dt->w;
				gq[i].y0 = gq[i].y0/dt->h;
				gq[i].y1 = gq[i].y1/dt->h;

				PushGlyph(gq[i], dt);
			}
		} break;

		case QUADRANT_BOTTOM_LEFT: {
			y = line_width;
			GenerateGlyphsTopLeft(text, len, color, gq, &x, &y, dt);
			float ypos = dt->h - gq[len].y1*(dt->line_count[quad]+1);
			for(u8 i=0; i<=len; i++) {
				gq[i].x0 /= dt->w;
				gq[i].x1 /= dt->w;
				gq[i].y0 = (gq[i].y0 + ypos)/dt->h;
				gq[i].y1 = (gq[i].y1 + ypos)/dt->h;

				PushGlyph(gq[i], dt);
			}
		} break;

		case QUADRANT_BOTTOM_RIGHT: {
			y = line_width;
			GenerateGlyphsTopLeft(text, len, color, gq, &x, &y, dt);
			float xpos = dt->w - gq[len].x1;
			float ypos = dt->h - gq[len].y1*(dt->line_count[quad]+1);
			for(u8 i=0; i<=len; i++) {
				gq[i].x0 = (gq[i].x0 + xpos)/dt->w;
				gq[i].x1 = (gq[i].x1 + xpos)/dt->w;
				gq[i].y0 = (gq[i].y0 + ypos)/dt->h;
				gq[i].y1 = (gq[i].y1 + ypos)/dt->h;

				PushGlyph(gq[i], dt);
			}
		} break;
	}
	
	dt->line_count[quad]++;
	PopArray(arena, GlyphQuad, len);
};

void SubmitDebugTextDrawCall(DebugText* dt, Renderer* renderer) {
	RenderPipeline debug_text = {};

	debug_text.rs = RenderStateDefaults();
	debug_text.rs.command = RENDER_COMMAND_CLEAR_DEPTH_STENCIL;
	debug_text.rs.rs = RASTERIZER_STATE_DOUBLE_SIDED;
	debug_text.vs = VERTEX_SHADER_TEXT;
	debug_text.ps = PIXEL_SHADER_TEXT;
	debug_text.dc.type = DRAW_CALL_VERTICES;
	debug_text.dc.vertices_count = 6 * dt->glyph_counter;
	debug_text.prbg = RENDER_BUFFER_GROUP_FONT;
	debug_text.vrbd_count = 1;
	debug_text.vrbd = PushStruct(&renderer->transient, RenderBufferData);
	debug_text.vrbd->type = RENDER_BUFFER_TYPE_STRUCTURED;
	debug_text.vrbd->structured.slot = STRUCTURED_BINDING_SLOT_FRAME;
	debug_text.vrbd->structured.data = &dt->quads;
	debug_text.vrbd->structured.count = dt->glyph_counter;

	AddToRenderQueue(debug_text, renderer);
}