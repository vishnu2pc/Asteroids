struct Entity {
	Transform transform;
	RenderPipeline rp;
};

struct Shooter {
	Transform transform;
	RenderPipeline rp;
};

static Shooter MakeShooter() {
	Shooter result = {};
	result.transform = TransformI();
	result.rp.rs = MakeRenderStateDefaults();
	result.rp.vs = VERTEX_SHADER_POS_NOR_TEX;
	result.rp.ps = PIXEL_SHADER_DIFFUSE_TEXTURED;
	result.rp.vrbg = RENDER_BUFFER_GROUP_CONE;
	result.rp.prbg = RENDER_BUFFER_GROUP_SPACE_RED;
	result.rp.dc.type = DRAW_CALL_DEFAULT;
	return result;
}
