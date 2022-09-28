static void 
FirstPersonControl(Vec3* position, Quat* rotation, bool camera, FPControlInfo* ci, Input* input) {
	float base_sens = ci->base_sens;
	float trans_sens = ci->trans_sens;
	float rot_sens = ci->rot_sens;

	float tdel = base_sens * trans_sens * 0.00001f;
	float rdel = base_sens * rot_sens * 0.005f;

	bool f = input->buttons[WIN32_BUTTON_W].held;
	bool b = input->buttons[WIN32_BUTTON_S].held;
	bool l = input->buttons[WIN32_BUTTON_A].held;
	bool r = input->buttons[WIN32_BUTTON_D].held;
	bool u = input->buttons[WIN32_BUTTON_Q].held;
	bool d = input->buttons[WIN32_BUTTON_E].held;

	float y = input->axes[WIN32_AXIS_MOUSE_DEL].x * !ci->block_yaw;
	float p = input->axes[WIN32_AXIS_MOUSE_DEL].y * !ci->block_pitch;

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

