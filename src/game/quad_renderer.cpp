#define MAX_QUADS 100
#define MAX_TEXTURED_QUADS 100

struct RenderQuad {
	Quad quad;
	Vec4 color;
};

struct QuadRenderer {
	RenderQuad quads[MAX_QUADS];
	u32 quad_counter;
	ConstantsBuffer* camera_constants;

	VertexShader* quad_vs;
	PixelShader* quad_ps;
	StructuredBuffer* quad_buffer;

	Vec3 positions[MAX_TEXTURED_QUADS*4];
	Vec2 texcoords[MAX_TEXTURED_QUADS*4];
	TextureBuffer* textures[MAX_TEXTURED_QUADS];
	u32 textured_quad_counter;

	VertexShader* textured_quad_vs;
	PixelShader* textured_quad_ps;
	VertexBuffer* textured_quad_vertex_buffers[2];
};

static QuadRenderer*
InitQuadRenderer(Renderer* renderer, MemoryArena* arena) {
	QuadRenderer* result = PushStruct(arena, QuadRenderer);

	VERTEX_BUFFER vertex_buffers[] = { VERTEX_BUFFER_POSITION, VERTEX_BUFFER_TEXCOORD };

	result->textured_quad_vs = UploadVertexShader(TexturedQuadShader, sizeof(TexturedQuadShader),
			"vsf", vertex_buffers, ArrayCount(vertex_buffers), renderer);

	result->textured_quad_ps = UploadPixelShader(TexturedQuadShader, sizeof(TexturedQuadShader), 
			"psf", renderer);

	result->quad_vs = UploadVertexShader(QuadShader, sizeof(QuadShader), "vsf", 0, 0, renderer);
	result->quad_ps = UploadPixelShader(QuadShader, sizeof(QuadShader), "psf", renderer);

	result->quad_buffer = UploadStructuredBuffer(sizeof(RenderQuad), MAX_QUADS, renderer);

	result->camera_constants = UploadConstantsBuffer(sizeof(Mat4), renderer);

	result->textured_quad_vertex_buffers[0] = UploadVertexBuffer(0, MAX_TEXTURED_QUADS*4, 3, true, renderer);
	result->textured_quad_vertex_buffers[1] = UploadVertexBuffer(0, MAX_TEXTURED_QUADS*4, 2, true, renderer);

	return result;
}

static void
PushRenderQuad(Quad* quad, Vec4 color, QuadRenderer* quad_renderer) {
	Assert(quad_renderer->quad_counter <= MAX_QUADS);
	RenderQuad render_quad = { *quad, color };
	quad_renderer->quads[quad_renderer->quad_counter++] = render_quad;
}

static void
PushTexturedQuad(Quad* quad, TextureBuffer* texture_buffer, QuadRenderer* quad_renderer) {
	Assert(quad_renderer->textured_quad_counter <= MAX_TEXTURED_QUADS);
	Vec2* texcoords = quad_renderer->texcoords;
	Vec3* positions = quad_renderer->positions;
	u32 textured_quad_counter = quad_renderer->textured_quad_counter;

	positions[textured_quad_counter*4] = quad->bl;
	texcoords[textured_quad_counter*4] = V2(0.0f, 1.0f);

	positions[textured_quad_counter*4 + 1] = quad->br;
	texcoords[textured_quad_counter*4 + 1] = V2(1.0f, 1.0f);

	positions[textured_quad_counter*4 + 2] = quad->tl;
	texcoords[textured_quad_counter*4 + 2] = V2(0.0f, 0.0f);

	positions[textured_quad_counter*4 + 3] = quad->tr;
	texcoords[textured_quad_counter*4 + 3] = V2(1.0f, 0.0f);

	quad_renderer->textures[quad_renderer->textured_quad_counter++] = texture_buffer;
}

static void
PushRenderLine(Line* line, Vec4 color, float thickness, Camera* camera, QuadRenderer* quad_renderer) {
	Vec3 line_unit_vector = V3Norm(V3Sub(line->end, line->start));
	Vec3 camera_forward = GetForwardVector(camera->rotation);
	Vec3 perp = V3Cross(camera_forward, line_unit_vector);

	Quad quad = MakeQuadFromLine(line, thickness, perp);

	PushRenderQuad(&quad, color, quad_renderer);
}

static void
QuadRendererFrame(QuadRenderer* quad_renderer, Camera* cam, Renderer* renderer) {
	if(!quad_renderer->quad_counter && !quad_renderer->textured_quad_counter) return;

	Mat4* vp = PushStruct(renderer->frame_arena, Mat4);
	*vp = MakeViewPerspective(cam);

	PushRenderBufferData* push_camera_constants = PushRenderCommand(renderer, PushRenderBufferData);
	push_camera_constants->buffer = quad_renderer->camera_constants->buffer;
	push_camera_constants->data = vp;
	push_camera_constants->size = sizeof(Mat4);

	SetConstantsBuffer* set_camera_constants = PushRenderCommand(renderer, SetConstantsBuffer);
	set_camera_constants->constants = quad_renderer->camera_constants;
	set_camera_constants->slot = (u8)1;
	set_camera_constants->vertex_shader = true;

	if(quad_renderer->quad_counter) {
		PushRenderBufferData* push_quad_buffer = PushRenderCommand(renderer, PushRenderBufferData);
		push_quad_buffer->buffer = quad_renderer->quad_buffer->buffer;
		push_quad_buffer->data = quad_renderer->quads;
		push_quad_buffer->size = sizeof(RenderQuad) * quad_renderer->quad_counter;

		SetStructuredBuffer* set_quad_buffer = PushRenderCommand(renderer, SetStructuredBuffer);
		set_quad_buffer->vertex_shader = true;
		set_quad_buffer->structured = quad_renderer->quad_buffer;
		set_quad_buffer->slot = 0;

		SetVertexShader* set_vertex_shader = PushRenderCommand(renderer, SetVertexShader);
		set_vertex_shader->vertex = quad_renderer->quad_vs;

		SetPixelShader* set_pixel_shader = PushRenderCommand(renderer, SetPixelShader);
		set_pixel_shader->pixel = quad_renderer->quad_ps;

		DrawVertices* draw_verts = PushRenderCommand(renderer, DrawVertices);
		draw_verts->vertices_count = 6 * quad_renderer->quad_counter;
	}

	if(quad_renderer->textured_quad_counter) {
		SetPrimitiveTopology* set_topology = PushRenderCommand(renderer, SetPrimitiveTopology);
		set_topology->type = (u8)PRIMITIVE_TOPOLOGY_TriangleStrip;

		SetVertexShader* set_vertex_shader = PushRenderCommand(renderer, SetVertexShader);
		set_vertex_shader->vertex = quad_renderer->textured_quad_vs;

		SetPixelShader* set_pixel_shader = PushRenderCommand(renderer, SetPixelShader);
		set_pixel_shader->pixel = quad_renderer->textured_quad_ps;

		PushRenderBufferData* push_pos_verts = PushRenderCommand(renderer, PushRenderBufferData);
		push_pos_verts->buffer = quad_renderer->textured_quad_vertex_buffers[0]->buffer;
		push_pos_verts->data = quad_renderer->positions;
		push_pos_verts->size = sizeof(float)*3 * quad_renderer->textured_quad_counter*4;

		PushRenderBufferData* push_tex_verts = PushRenderCommand(renderer, PushRenderBufferData);
		push_tex_verts->buffer = quad_renderer->textured_quad_vertex_buffers[1]->buffer;
		push_tex_verts->data = quad_renderer->texcoords;
		push_tex_verts->size = sizeof(float)*2 * quad_renderer->textured_quad_counter*4;

		SetVertexBuffer* set_position_buffer = PushRenderCommand(renderer, SetVertexBuffer);
		set_position_buffer->vertex = quad_renderer->textured_quad_vertex_buffers[0];
		set_position_buffer->slot = 0;
		set_position_buffer->stride = sizeof(float)*3;
		set_position_buffer->offset = 0;

		SetVertexBuffer* set_texcoord_buffer = PushRenderCommand(renderer, SetVertexBuffer);
		set_texcoord_buffer->vertex = quad_renderer->textured_quad_vertex_buffers[1];
		set_texcoord_buffer->slot = 1;
		set_texcoord_buffer->stride = sizeof(float)*2;
		set_texcoord_buffer->offset = 0;

		for(u32 i=0; i<quad_renderer->textured_quad_counter; i++) {
			SetTextureBuffer* set_texture_buffer = PushRenderCommand(renderer, SetTextureBuffer);
			set_texture_buffer->texture = quad_renderer->textures[i];
			set_texture_buffer->slot = 0;

			DrawVertices* draw_verts = PushRenderCommand(renderer, DrawVertices);
			draw_verts->vertices_count = 4 * quad_renderer->textured_quad_counter;
		}
	}
	quad_renderer->quad_counter = 0;
	quad_renderer->textured_quad_counter = 0;
}





