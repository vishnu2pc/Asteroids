#define MAX_FONT_ATLAS_WIDTH 1000
#define MAX_FONT_ATLAS_HEIGHT 1000

#define MAX_TEXT_GLYPHS_ON_SCREEN 200
// STB Truetype requires x*y times width and height when oversampling
//
struct Glyph {
	float x0, y0, x1, y1;
	float s0, t0, s1, t1;
	Vec3 color;
};

struct TextUI {
	TextureBuffer* texture_buffer;
	VertexShader* text_shader;
	PixelShader* monochrome_ps;
	StructuredBuffer* glyph_buffer;

	void* pixels;
	stbtt_packedchar packed_chars[0x7E-0x20];
	Glyph glyphs[MAX_TEXT_GLYPHS_ON_SCREEN];

	float y_cursor;
	u32 font_line_width;
	Vec2 screen_resolution;
	u32 glyph_counter;
};

static TextUI*
InitFont(void* data, WindowDimensions wd, Renderer* renderer, MemoryArena* arena) {
	TextUI* result = PushStruct(arena, TextUI);
	result->pixels = PushSize(arena, MAX_FONT_ATLAS_WIDTH * MAX_FONT_ATLAS_HEIGHT);

	stbtt_fontinfo font_info;
	
	unsigned char* font_data = (unsigned char*)data;

	stbtt_InitFont(&font_info, font_data, stbtt_GetFontOffsetForIndex(font_data, 0));
	stbtt_pack_context context;

	int font_size = 25;
	int ascent, descent, line_gap, baseline;
	float scale_factor = stbtt_ScaleForPixelHeight(&font_info, font_size);
	stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap); 
	ascent = (ascent * scale_factor);
	descent = (descent * scale_factor);
	line_gap = (line_gap * line_gap);
	baseline = (ascent*scale_factor);

	stbtt_PackBegin(&context, (unsigned char*)result->pixels, MAX_FONT_ATLAS_WIDTH, MAX_FONT_ATLAS_HEIGHT, 0, 1, 0);
	stbtt_PackSetOversampling(&context, 1, 1);
	stbtt_PackFontRange(&context, font_data, 0, ascent-descent, 0x20, 0x7E-0x20, result->packed_chars);
	stbtt_PackEnd(&context);

	result->texture_buffer = UploadTexture(result->pixels, MAX_FONT_ATLAS_WIDTH, 
			MAX_FONT_ATLAS_HEIGHT, 1, renderer);

	result->text_shader = UploadVertexShader(TextShader, sizeof(TextShader), 
			"vsf", 0, 0, renderer);
	result->monochrome_ps = UploadPixelShader(MonochromeShader, sizeof(MonochromeShader), 
			"psf", renderer);
	result->glyph_buffer = UploadStructuredBuffer(sizeof(Glyph), MAX_TEXT_GLYPHS_ON_SCREEN, renderer);

	result->font_line_width = ascent - descent + line_gap * font_size;
	result->y_cursor = result->font_line_width;
	result->screen_resolution.x = (float)wd.width;
	result->screen_resolution.y = (float)wd.height;
	
	return result;
}

static void
PushText(char* text, TextUI* text_ui) {
	u32 len = StringLength(text);
	float x = 0;
	float y = text_ui->y_cursor;

	for(u32 i=0; i<=len; i++) {
		// TODO: check
		int char_index = text[i] - 0x20;
		stbtt_aligned_quad quad; 

	stbtt_GetPackedQuad(text_ui->packed_chars, MAX_FONT_ATLAS_WIDTH,
				MAX_FONT_ATLAS_HEIGHT, char_index, &x, &y, &quad, 1);
	
		u32 index = text_ui->glyph_counter;
		text_ui->glyphs[index].x0 = quad.x0/text_ui->screen_resolution.x;
		text_ui->glyphs[index].y0 = quad.y0/text_ui->screen_resolution.y;
		text_ui->glyphs[index].x1 = quad.x1/text_ui->screen_resolution.x;
		text_ui->glyphs[index].y1 = quad.y1/text_ui->screen_resolution.y;
		text_ui->glyphs[index].s0 = quad.s0;
		text_ui->glyphs[index].t0 = quad.t0;
		text_ui->glyphs[index].s1 = quad.s1;
		text_ui->glyphs[index].t1 = quad.t1;

		text_ui->glyphs[index].color = WHITE;

		text_ui->glyph_counter++;
	}
	text_ui->y_cursor += text_ui->font_line_width;
}

static void
UIFrame(TextUI* text_ui, WindowDimensions wd, Renderer* renderer) {
	SetBlendState* set_blend_state = PushRenderCommand(renderer, SetBlendState);
	set_blend_state->type = BLEND_STATE_Regular;

	SetPrimitiveTopology* set_topology = PushRenderCommand(renderer, SetPrimitiveTopology);
	set_topology->type = PRIMITIVE_TOPOLOGY_TriangleList;

	SetVertexShader* set_vertex_shader = PushRenderCommand(renderer, SetVertexShader);
	set_vertex_shader->vertex = text_ui->text_shader;

	SetPixelShader* set_pixel_shader = PushRenderCommand(renderer, SetPixelShader);
	set_pixel_shader->pixel = text_ui->monochrome_ps;

	SetTextureBuffer* set_texture_buffer = PushRenderCommand(renderer, SetTextureBuffer);
	set_texture_buffer->texture = text_ui->texture_buffer;
	set_texture_buffer->slot = 0;

	SetStructuredBuffer* set_glyph_buffer = PushRenderCommand(renderer, SetStructuredBuffer);
	set_glyph_buffer->vertex_shader = true;
	set_glyph_buffer->structured = text_ui->glyph_buffer;
	set_glyph_buffer->slot = 0;

	PushRenderBufferData* push_glyph_buffer = PushRenderCommand(renderer,PushRenderBufferData);
	push_glyph_buffer->buffer = text_ui->glyph_buffer->buffer;
	push_glyph_buffer->size = text_ui->glyph_counter * sizeof(Glyph);
	push_glyph_buffer->data = text_ui->glyphs;

	DrawVertices* draw_verts = PushRenderCommand(renderer, DrawVertices);
	draw_verts->vertices_count = 6*text_ui->glyph_counter;
	draw_verts->offset = 0;

	text_ui->y_cursor = text_ui->font_line_width;
	text_ui->glyph_counter = 0;
	text_ui->screen_resolution = V2((float)wd.width, (float)wd.height);
}











