#define MAX_FONT_ATLAS_WIDTH 250
#define MAX_FONT_ATLAS_HEIGHT 250
#define MAX_GLYPHS_ON_SCREEN 200

// STB Truetype requires x*y times width and height when oversampling
//
struct Glyph {
	float x0, y0, x1, y1;
	float s0, t0, s1, t1;
	Vec3 color;
};

struct FontData {
	FontData* next;

	void* pixels;
	int font_size;
	int font_line_width;
	stbtt_packedchar packed_chars[0x7E-0x20];

	Glyph glyphs[MAX_GLYPHS_ON_SCREEN];
	u32 glyph_counter;
};

struct TextUI {
	StructuredBuffer* structured_buffer;
	VertexShader* text_shader;
	PixelShader* monochrome_ps;

	void* ttf;
	FontData* font_data;

	float y_cursor;
	Vec2 screen_res;

	MemoryArena* frame_arena;
	Renderer* renderer;
};

static FontData*
GenerateFontData(float size, TextUI* text_ui) {
	FontData* result = PushStruct(text_ui->frame_arena, FontData);

	if(text_ui->font_data) {
		result->next = text_ui->font_data;
		text_ui->font_data = result;
	}
	else text_ui->font_data = result;

	result->pixels = PushSize(text_ui->frame_arena, MAX_FONT_ATLAS_WIDTH * MAX_FONT_ATLAS_HEIGHT);

	stbtt_fontinfo font_info;
	
	unsigned char* ttf = (unsigned char*)text_ui->ttf;

	stbtt_InitFont(&font_info, ttf, stbtt_GetFontOffsetForIndex(ttf, 0));
	stbtt_pack_context context;

	int ascent, descent, line_gap, baseline;
	float scale_factor = stbtt_ScaleForPixelHeight(&font_info, size);
	stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap); 
	ascent = (ascent * scale_factor);
	descent = (descent * scale_factor);
	line_gap = (line_gap * line_gap);
	baseline = (ascent*scale_factor);

	stbtt_PackBegin(&context, (unsigned char*)result->pixels, MAX_FONT_ATLAS_WIDTH, MAX_FONT_ATLAS_HEIGHT, 0, 1, 0);
	stbtt_PackSetOversampling(&context, 1, 1);
	stbtt_PackFontRange(&context, ttf, 0, ascent-descent, 0x20, 0x7E-0x20, result->packed_chars);
	stbtt_PackEnd(&context);

	result->font_size = size;
	result->font_line_width = ascent - descent + line_gap * size;

	return result;
}

static FontData*
GetFontData(float size, TextUI* text_ui) {
	FontData* font_data = 0;
	FontData* node = text_ui->font_data;
	while(node) {
		if(node->font_size == size) {
			font_data = node; 
			break;
		}
		node = node->next;
	}
	return font_data;
}

static TextUI*
InitFont(void* data, WindowDimensions wd, Renderer* renderer, MemoryArena* arena, MemoryArena* frame) {
	TextUI* result = PushStruct(arena, TextUI);
	result->ttf = data;
	result->frame_arena = frame;
	result->renderer = renderer;

	GenerateFontData(40.0f, result);

	result->text_shader = UploadVertexShader(TextShader, sizeof(TextShader), 
			"vsf", 0, 0, renderer);
	result->monochrome_ps = UploadPixelShader(MonochromeShader, sizeof(MonochromeShader), 
			"psf", renderer);
	result->structured_buffer = UploadStructuredBuffer(sizeof(Glyph), MAX_GLYPHS_ON_SCREEN, renderer);

	result->screen_res.x = (float)wd.width;
	result->screen_res.y = (float)wd.height;
	result->y_cursor += result->font_data->font_line_width;
	
	return result;
}

static void
PushGlyphs(char* text, FontData* font_data, float* x, float* y, Vec2 screen_res) {
	u32 len = StringLength(text);

	for(u32 i=0; i<len; i++) {
		// TODO: check
		int char_index = text[i] - 0x20;
		stbtt_aligned_quad quad; 

	stbtt_GetPackedQuad(font_data->packed_chars, MAX_FONT_ATLAS_WIDTH,
				MAX_FONT_ATLAS_HEIGHT, char_index, x, y, &quad, 1);
	
		u32 index = font_data->glyph_counter;
		font_data->glyphs[index].x0 = quad.x0/screen_res.x;
		font_data->glyphs[index].y0 = quad.y0/screen_res.y;
		font_data->glyphs[index].x1 = quad.x1/screen_res.x;
		font_data->glyphs[index].y1 = quad.y1/screen_res.y;
		font_data->glyphs[index].s0 = quad.s0;
		font_data->glyphs[index].t0 = quad.t0;
		font_data->glyphs[index].s1 = quad.s1;
		font_data->glyphs[index].t1 = quad.t1;

		font_data->glyphs[index].color = WHITE;

		font_data->glyph_counter++;
	}
}

static void
PushTextScreenSpace(char* text, float size, Vec2 ssc, TextUI* text_ui) {
	FontData* font_data = GetFontData(size, text_ui);
	if(!font_data) font_data = GenerateFontData(size, text_ui);

	float x = ssc.x * text_ui->screen_res.x;
	float y = ssc.y * text_ui->screen_res.y;
	y += font_data->font_line_width;

	PushGlyphs(text, font_data, &x, &y, text_ui->screen_res);
}

static float
GetPixelWidthForText(char* text, float size, TextUI* text_ui) {
	FontData* font_data = GetFontData(size, text_ui);
	if(!font_data) font_data = GenerateFontData(size, text_ui);

	u32 len = StringLength(text);
	float x, y;
	x=0;
	y=0;

	for(u32 i=0; i<len; i++) {
		int char_index = text[i] - 0x20;
		stbtt_aligned_quad quad; 

		stbtt_GetPackedQuad(font_data->packed_chars, MAX_FONT_ATLAS_WIDTH,
				MAX_FONT_ATLAS_HEIGHT, char_index, &x, &y, &quad, 1);
	}
	int char_index = ' ' - 0x20;
	stbtt_aligned_quad quad; 

	stbtt_GetPackedQuad(font_data->packed_chars, MAX_FONT_ATLAS_WIDTH,
			MAX_FONT_ATLAS_HEIGHT, char_index, &x, &y, &quad, 1);

	return x;
}

static void
PushTextPixelSpace(char* text, float font_size, Vec2 psc, TextUI* text_ui) {
	FontData* font_data = GetFontData(font_size, text_ui);
	if(!font_data) font_data = GenerateFontData(font_size, text_ui);

	float x = psc.x;
	float y = psc.y + font_data->font_line_width;

	PushGlyphs(text, font_data, &x, &y, text_ui->screen_res);
}

static void
UITextFrame(TextUI* text_ui, WindowDimensions wd, Renderer* renderer) {

	SetRenderTarget* set_render_target = PushRenderCommand(renderer, SetRenderTarget);
	set_render_target->render_target = &renderer->backbuffer;

	SetBlendState* set_blend_state = PushRenderCommand(renderer, SetBlendState);
	set_blend_state->type = BLEND_STATE_Regular;

	SetRasterizerState* srs = PushRenderCommand(renderer, SetRasterizerState);

	SetPrimitiveTopology* set_topology = PushRenderCommand(renderer, SetPrimitiveTopology);
	set_topology->type = PRIMITIVE_TOPOLOGY_TriangleList;

	SetVertexShader* set_vertex_shader = PushRenderCommand(renderer, SetVertexShader);
	set_vertex_shader->vertex = text_ui->text_shader;

	SetPixelShader* set_pixel_shader = PushRenderCommand(renderer, SetPixelShader);
	set_pixel_shader->pixel = text_ui->monochrome_ps;

	SetStructuredBuffer* set_glyph_buffer = PushRenderCommand(renderer, SetStructuredBuffer);
	set_glyph_buffer->vertex_shader = true;
	set_glyph_buffer->structured = text_ui->structured_buffer;
	set_glyph_buffer->slot = 0;

	FontData* font_data = text_ui->font_data;
	while(font_data) {

		TextureBuffer* texture_buffer = UploadTexture(font_data->pixels, MAX_FONT_ATLAS_WIDTH, MAX_FONT_ATLAS_HEIGHT, 
				1, false, true, renderer);

		SetTextureBuffer* set_texture_buffer = PushRenderCommand(renderer, SetTextureBuffer);
		set_texture_buffer->texture = texture_buffer;
		set_texture_buffer->slot = 0;

		PushRenderBufferData* push_glyph_buffer = PushRenderCommand(renderer,PushRenderBufferData);
		push_glyph_buffer->buffer = text_ui->structured_buffer->buffer;
		push_glyph_buffer->size = font_data->glyph_counter * sizeof(Glyph);
		push_glyph_buffer->data = font_data->glyphs;

		DrawVertices* draw_verts = PushRenderCommand(renderer, DrawVertices);
		draw_verts->vertices_count = 6*font_data->glyph_counter;
		draw_verts->offset = 0;

		FreeRenderResource* free_buffer = PushRenderCommand(renderer, FreeRenderResource);
		free_buffer->buffer = texture_buffer->buffer;

		FreeRenderResource* free_view = PushRenderCommand(renderer, FreeRenderResource);
		free_view->buffer = texture_buffer->view;

		font_data->glyph_counter = 0;

		//texture_buffer->buffer->Release();
		//texture_buffer->view->Release();
		font_data = font_data->next;
	}
	text_ui->font_data = 0;

	text_ui->screen_res = V2((float)wd.width, (float)wd.height);
}











