struct RendererShapes {
	LineInfo lines[MAX_DEBUG_LINES];
	u32 lines_count;
};

static void BeginRendererShapes(RendererShapes* rs) {
	rs->lines_count = 0;
}

static void DrawLine(Vec3 start, Vec3 end, Vec3 color, RendererShapes* rs) {
	LineInfo line = { start, end, color };
	if(rs->lines_count >= MAX_DEBUG_LINES) SDL_Log("Max debug lines exceeded");
	else rs->lines[rs->lines_count++] = line;
}

static void SubmitRendererShapesDrawCall(RendererShapes* rs, CameraInfo cam, Renderer* renderer) {
	RenderPipeline rp = {};
	rp.rs = RenderStateDefaults();
	rp.rs.topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	rp.vs = VERTEX_SHADER_LINE;
	rp.ps = PIXEL_SHADER_LINE;
	rp.dc.type = DRAW_CALL_VERTICES;
	rp.dc.vertices_count = 2 * rs->lines_count;
	rp.vrbd_count = 2;
	rp.vrbd = PushArray(&renderer->sm, RenderBufferData, 2);
	rp.vrbd[0].type = RENDER_BUFFER_TYPE_STRUCTURED;
	rp.vrbd[0].structured.slot = STRUCTURED_BINDING_SLOT_FRAME;
	rp.vrbd[0].structured.count = rs->lines_count;
	rp.vrbd[0].structured.data = rs->lines;
	rp.vrbd[1].type = RENDER_BUFFER_TYPE_CONSTANTS;
	rp.vrbd[1].constants.slot = CONSTANTS_BINDING_SLOT_CAMERA;
	Mat4* vp = PushStruct(&renderer->sm, Mat4);
	*vp = MakeViewPerspective(cam);
	rp.vrbd[1].constants.data = vp;
	AddToRenderQueue(rp, renderer);
}