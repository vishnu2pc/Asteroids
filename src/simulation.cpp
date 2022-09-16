struct Ship {
	Transform transform;
	RenderPipeline rp;
};

static Ship MakeShip() {
	Ship result = {};
	result.transform = TransformI();
	result.transform.scale = V3(3.0f, 3.0f, 3.0f);
	result.rp.rs = RenderStateDefaults();
	result.rp.vs = VERTEX_SHADER_POS_NOR_TEX;
	result.rp.ps = PIXEL_SHADER_DIFFUSE_TEXTURED;
	result.rp.vrbg = RENDER_BUFFER_GROUP_SPHERE;
	result.rp.prbg = RENDER_BUFFER_GROUP_SPACE_RED;
	result.rp.dc.type = DRAW_CALL_DEFAULT;
	return result;
}

static void FirstPersonControl(Vec3* position, Quat* rotation, bool camera, FPControlInfo ci, Input input) {
	float base_sens = ci.base_sens;
	float trans_sens = ci.trans_sens;
	float rot_sens = ci.rot_sens;

	float tdel = base_sens * trans_sens * 0.00001f;
	float rdel = base_sens * rot_sens * 0.005f;

	bool f = input.kb[KB_W].held;
	bool b = input.kb[KB_S].held;
	bool l = input.kb[KB_A].held;
	bool r = input.kb[KB_D].held;
	bool u = input.kb[KB_Q].held;
	bool d = input.kb[KB_E].held;

	float y = input.mouse.del.x * !ci.block_yaw;
	float p = input.mouse.del.y * !ci.block_pitch;

	Vec3 pos = *position;
	Quat rot = *rotation;

	// We move in the negative of the forward vector because camera
	Vec3 fv = camera ? V3Neg(GetForwardVector(rot)) : GetForwardVector(rot);
	Vec3 fdisp = V3MulF(fv, tdel);

	Vec3 rv = GetRightVector(rot);
	Vec3 rdisp = V3MulF(rv, tdel);

	Vec3 uv = V3Up();
	Vec3 udisp = V3MulF(uv, tdel);

	pos = V3Add(pos, V3MulF(V3Add(fv, fdisp), f));
	pos = V3Sub(pos, V3MulF(V3Add(fv, fdisp), b));

	pos = V3Add(pos, V3MulF(V3Add(rv, rdisp), r));
	pos = V3Sub(pos, V3MulF(V3Add(rv, rdisp), l));

	pos = V3Add(pos, V3MulF(V3Add(uv, rdisp), u));
	pos = V3Sub(pos, V3MulF(V3Add(uv, rdisp), d));

	Quat yrot = y ? QuatFromAxisAngle(V3Up(), -y * rdel) : QuatI();
	Quat xrot = p ? QuatFromAxisAngle(rv, -p * rdel) : QuatI();

	rot = QuatMul(QuatMul(yrot, xrot), rot);

	*position = pos;
	*rotation = rot;
}

static void SubmitDrawCallShip(Ship ship, Renderer* renderer) {
	ship.rp.vrbd_count = 1;
	ship.rp.vrbd = PushStruct(&renderer->sm, RenderBufferData);
	ship.rp.vrbd->type = RENDER_BUFFER_TYPE_CONSTANTS;
	ship.rp.vrbd->constants.slot = CONSTANTS_BINDING_SLOT_INSTANCE;
	Mat4* model = PushStruct(&renderer->sm, Mat4);
	*model = MakeTransformMatrix(ship.transform);
	ship.rp.vrbd->constants.data = model;
	ship.rp.vrbd->type = RENDER_BUFFER_TYPE_CONSTANTS;
	AddToRenderQueue(ship.rp, renderer);
}

static void DrawDebugTransform(Transform transform, RendererShapes* rs) {
	float length = 10.0f;
	Vec3 forward = GetForwardVector(transform.rotation);
	Vec3 right = GetRightVector(transform.rotation);
	Vec3 up = GetUpVector(transform.rotation);
	Vec3 scaled_pos = V3Mul(transform.position, transform.scale);
	DrawLine(transform.position, V3Add(transform.position, V3MulF(forward, length)), BLUE, rs);
	DrawLine(transform.position, V3Add(transform.position, V3MulF(right, length)), RED, rs);
	DrawLine(transform.position, V3Add(transform.position, V3MulF(up, length)), YELLOW, rs);
}