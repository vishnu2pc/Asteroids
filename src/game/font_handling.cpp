#include "font_handling.h"

static void
CompileDebugTextShader(DebugText* debug_text, Renderer* renderer) {
	char VertexShaderCode[] = R"FOO(
// NDC TopLeft(-1, 1), BottomRight(1, -1)

struct Glyph {
	float2 pos0;			// Normalized top left
	float2 pos1;  		// normalized bot right
	float2 uv0;
	float2 uv1;
	float3 color; 
};

StructuredBuffer<Glyph> glyph : register(t0);

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float3 color : COLOR;
};

float map(float value, float min1, float max1, float min2, float max2) {
	// Convert the current value to a percentage
	// 0% - min1, 100% - max1
	float perc = (value - min1) / (max1 - min1);

	// Do the same operation backwards with min2 and max2
	float final = perc * (max2 - min2) + min2;
	return final;
}

ps vsf(in uint vert_id : SV_VertexID) {
	ps output;
	// Vertices are drawn ccw
	// 0, 1, 2, 3, 4, 5
	// 2, 0, 1, 2, 1, 3
	int index = vert_id/6;

	// TODO: we have actual coords instead of bb size, redo this, possibly never
	float pos0x = glyph[index].pos0.x;
	float pos0y = glyph[index].pos0.y;
	float xoff = glyph[index].pos1.x - pos0x;
	float yoff = glyph[index].pos1.y - pos0y;
	float2 uv0 = glyph[index].uv0;
	float2 uv1 = glyph[index].uv1;
	float zval = 1.0f;

	float2 top_left = float2(pos0x, pos0y);
	float2 top_right = float2(pos0x+xoff, pos0y);
	float2 bottom_right = float2(pos0x+xoff, pos0y+yoff);
	float2 bottom_left = float2(pos0x, pos0y+yoff);

	top_left = float2(map(top_left.x, 0, 1, -1, 1), map(top_left.y, 0, 1, 1, -1));
	top_right = float2(map(top_right.x, 0, 1, -1, 1), map(top_right.y, 0, 1, 1, -1));
	bottom_right = float2(map(bottom_right.x, 0, 1, -1, 1), map(bottom_right.y, 0, 1, 1, -1));
	bottom_left = float2(map(bottom_left.x, 0, 1, -1, 1), map(bottom_left.y, 0, 1, 1, -1));

	int vert_count = vert_id % 6;

	if(vert_count == 0) {
		output.pixel_pos = float4(top_left, zval, zval);
		output.texcoord = uv0;
	}
	if(vert_count == 1) {
		output.pixel_pos = float4(bottom_left, zval, zval);
		output.texcoord = float2(uv0.x, uv1.y);
	}
	if(vert_count == 2) {
		output.pixel_pos = float4(bottom_right, zval, zval);
		output.texcoord = uv1;
	}
	if(vert_count == 3) {
		output.pixel_pos = float4(bottom_right, zval, zval);
		output.texcoord = uv1;
	}
	if(vert_count == 4) {
		output.pixel_pos = float4(top_right, zval, zval);
		output.texcoord = float2(uv1.x, uv0.y);
	}
	if(vert_count == 5) {
		output.pixel_pos = float4(top_left, zval, zval);
		output.texcoord = uv0;
	}

	output.color = glyph[index].color;
	return output;
}
)FOO";

char PixelShaderCode[] = R"FOO(

struct ps {
	float4 pixel_pos : SV_POSITION;
	float2 texcoord : TEXCOORD;
	float3 color : COLOR;
};

Texture2D atlas : register(t0);
SamplerState texture_sampler : register(s0);

float4 psf(ps input) : SV_TARGET {
	float3 tex = atlas.Sample(texture_sampler, input.texcoord).xyz;
	if(tex.r == 0.0) discard;
	tex = input.color;
	return float4(tex, 1.0f);
}

)FOO";

	VertexShaderDesc vs_desc = {};
	vs_desc.shader.code = VertexShaderCode;
	vs_desc.shader.size = sizeof(VertexShaderCode);
	vs_desc.shader.entry = "vsf";

	StructuredBufferDesc sb_desc[] = { { STRUCTURED_BINDING_SLOT_FRAME, sizeof(GlyphQuad), MAX_DEBUG_TEXT_GLYPHS } };
	vs_desc.sb_desc = sb_desc;
	vs_desc.sb_count = 1;

	debug_text->vertex_shader = UploadVertexShader(vs_desc, renderer);

	PixelShaderDesc ps_desc = {};
	ps_desc.shader.code = PixelShaderCode;
	ps_desc.shader.size = sizeof(PixelShaderCode);
	ps_desc.shader.entry = "psf";

	TEXTURE_SLOT texture_slot[] = { TEXTURE_SLOT_DIFFUSE };
	ps_desc.texture_slot = texture_slot;
	ps_desc.texture_count = ArrayCount(texture_slot);;

	debug_text->pixel_shader = UploadPixelShader(ps_desc, renderer);

}

static DebugText* 
InitDebugText(PlatformAPI* platform_api, Renderer* renderer, MemoryArena* arena) { 
	DebugText* dt = PushStruct(arena, DebugText);

	PlatformFileInfo info = {};
	char* info_path = "../assets/fonts/JetBrainsMono/jetbrains_mono_light.fi";
	char* image_path = "../assets/fonts/JetBrainsMono/jetbrains_mono_light.png";
	info.name = info_path;
	PlatformFileHandle handle = platform_api->open_file(&info);
	Assert(!handle.failed);

	platform_api->read_file(&handle, sizeof(FontInfo), &dt->info);

	dt->texture_atlas = UploadTextureFromFile(image_path, TEXTURE_SLOT_DIFFUSE, renderer);

	CompileDebugTextShader(dt, renderer);

	return dt;
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
	dt->glyph_counter = 0;
	dt->w = wd.width;
	dt->h = wd.height;
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

		gq[i].x0 = (gq[i].x0 * 0.35f);
		gq[i].y0 = (gq[i].y0 * 0.35f);
		gq[i].x1 = (gq[i].x1 * 0.35f);
		gq[i].y1 = (gq[i].y1 * 0.35f);

		gq[i].color = color;
	}
}

static void PushGlyph(GlyphQuad gq, DebugText* dt) {
	dt->glyphs[dt->glyph_counter++] = gq;
};

// Extremely hacky but it will do
static void DrawDebugText(char* text, Vec3 color, QUADRANT quad, DebugText* dt, MemoryArena* arena) {
	assert(text);
	int len = StringLength(text);
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
	RenderPipeline rp = {};

	rp.rs = RenderStateDefaults();
	rp.rs.command = RENDER_COMMAND_CLEAR_DEPTH_STENCIL;
	rp.vs = dt->vertex_shader;
	rp.ps = dt->pixel_shader;
	rp.dc.type = DRAW_CALL_VERTICES;
	rp.dc.vertices_count = 6 * dt->glyph_counter;
	rp.prbg = dt->texture_atlas;
	rp.vrbd_count = 1;
	rp.vrbd = PushStruct(&renderer->transient, PushRenderBufferData);
	rp.vrbd->type = RENDER_BUFFER_TYPE_STRUCTURED;
	rp.vrbd->structured.slot = STRUCTURED_BINDING_SLOT_FRAME;
	rp.vrbd->structured.data = dt->glyphs;
	rp.vrbd->structured.count = dt->glyph_counter;

	AddToRenderQueue(rp, renderer);
}
