enum POST_PROCESS_TYPE {
	POST_PROCESS_TYPE_Copy,
	POST_PROCESS_TYPE_Edge,
	POST_PROCESS_TYPE_TOTAL,
};

struct PostProcessPipeline {
	u8 type;
	ID3D11ShaderResourceView* in;
	ID3D11RenderTargetView* out;
};

struct PostProcessRenderer {
	VertexShader* full_screen_quad_shader;
	PixelShader* ps[POST_PROCESS_TYPE_TOTAL];

	ConstantsBuffer* resolution_constants;
	PostProcessPipeline pipeline;
};

static void
InitPostProcessShaders(PostProcessRenderer* pp_renderer, Renderer* renderer) {
	pp_renderer->full_screen_quad_shader = UploadVertexShader(FullScreenQuadShader, sizeof(FullScreenQuadShader), "vsf",
			0, 0, renderer);
	pp_renderer->ps[POST_PROCESS_TYPE_Copy] = UploadPixelShader(PostProcessShader, sizeof(PostProcessShader)
			, "ps_copy", renderer);
	pp_renderer->ps[POST_PROCESS_TYPE_Edge] = UploadPixelShader(PostProcessShader, sizeof(PostProcessShader), 
			"ps_edge", renderer);
};

static PostProcessRenderer*
InitPostProcessRenderer(Renderer* renderer, MemoryArena* arena) {
	PostProcessRenderer* result = PushStruct(arena, PostProcessRenderer);

	InitPostProcessShaders(result, renderer);

	result->resolution_constants = UploadConstantsBuffer(sizeof(Vec2), renderer);
	return result;
};

static void
PushPostProcessPipeline(PostProcessPipeline* pipeline, PostProcessRenderer* pp_renderer, Renderer* renderer) {
	pp_renderer->pipeline = *pipeline;
}

static void
PostProcessRendererFrame(PostProcessRenderer* pp_renderer, Renderer* renderer) {
#if INTERNAL
	if(executable_reloaded) InitPostProcessShaders(pp_renderer, renderer);
#endif
	PostProcessPipeline pipeline = pp_renderer->pipeline;

	SetRasterizerState* srs = PushRenderCommand(renderer, SetRasterizerState);
	srs->type = RASTERIZER_STATE_DoubleSided;

	SetVertexShader* svs = PushRenderCommand(renderer, SetVertexShader);
	svs->vertex = pp_renderer->full_screen_quad_shader;

	SetPixelShader* sps = PushRenderCommand(renderer, SetPixelShader);
	sps->pixel = pp_renderer->ps[pipeline.type];

	if(pipeline.type == POST_PROCESS_TYPE_Edge) {
		PushRenderBufferData* push_resolution = PushRenderCommand(renderer, PushRenderBufferData);
		Vec2* resolution = PushStruct(renderer->frame_arena, Vec2);
		resolution->x = (float)renderer->window_dim.width;
		resolution->y = (float)renderer->window_dim.height;
		push_resolution->buffer = pp_renderer->resolution_constants->buffer;
		push_resolution->data = resolution;
		push_resolution->size = sizeof(Vec2);

		SetBlendState* set_blend_state = PushRenderCommand(renderer, SetBlendState);
		set_blend_state->type = BLEND_STATE_NoBlend;

		SetConstantsBuffer* scb = PushRenderCommand(renderer, SetConstantsBuffer);
		scb->vertex_shader = false;
		scb->constants = pp_renderer->resolution_constants;
		scb->slot = 0;
	}
	else {
		SetBlendState* set_blend_state = PushRenderCommand(renderer, SetBlendState);
		set_blend_state->type = BLEND_STATE_Regular;
	}

	RenderTarget* rt = PushStruct(renderer->frame_arena, RenderTarget);
	rt->view = pipeline.out;
	SetRenderTarget* srt = PushRenderCommand(renderer, SetRenderTarget);
	srt->render_target = rt;

	ClearRenderTarget* crt = PushRenderCommand(renderer, ClearRenderTarget);
	crt->render_target = rt;
	*(Vec4*)crt->color = V4FromV3(RED, 1.0f);

	ClearDepth* cd = PushRenderCommand(renderer, ClearDepth);
	cd->value = 1.0f;

	SetSamplerState* set_sampler_state = PushRenderCommand(renderer, SetSamplerState);
	set_sampler_state->type = SAMPLER_STATE_Default;
	set_sampler_state->slot = 0;

	TextureBuffer* tb = PushStruct(renderer->frame_arena, TextureBuffer);
	tb->view = pipeline.in;
	SetTextureBuffer* stb = PushRenderCommand(renderer, SetTextureBuffer);
	stb->texture = tb;
	stb->slot = 0;

	SetPrimitiveTopology* set_topology = PushRenderCommand(renderer, SetPrimitiveTopology);
	set_topology->type = PRIMITIVE_TOPOLOGY_TriangleList;

	DrawVertices* dv = PushRenderCommand(renderer, DrawVertices);
	dv->vertices_count = 3;

	SetTextureBuffer* stb2 = PushRenderCommand(renderer, SetTextureBuffer);
	stb2->texture = 0;
	stb2->slot = 0;

}
