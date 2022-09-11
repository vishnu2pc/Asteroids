struct FPControlInfo {
	float base_sens;
	float trans_sens;
	float rot_sens;

	bool block_yaw;
	bool block_pitch;
};

static Mat4 MakeViewPerspective(CameraInfo camera) {
	Mat4 result = M4I();
	Mat4 translation = M4Translate(V3Neg(camera.position));
	/*
	Vec3 direction = V3(0.0f, 0.0f, -1.0f);
	Vec3 rotated_dir = RotateVecByQuat(direction, camera.rotation);
	Vec3 target_disp = V3MulF(rotated_dir, 2.0f);

	Vec3 target = V3Add(target_disp, camera.position);
	Mat4 view = M4LookAt(camera.position, target, V3Up());
		*/

	Mat4 rot = M4FromQuat(camera.rotation);
	Mat4 view = M4Mul(M4Transpose(rot), translation);
	Mat4 perspective = M4Perspective(camera.fov, camera.aspect_ratio, camera.near_clip, camera.far_clip);
	result = M4Mul(perspective, view);
	return result;
}

static void FirstPersonCameraControl(CameraInfo* cam, FPControlInfo ci, Input input) {
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

	Vec3 pos = cam->position;
	Quat rot = cam->rotation;

	// We move in the negative of the forward vector because camera
	Vec3 fv = V3Neg(GetForwardVector(rot));
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

	cam->position = pos;
	cam->rotation = rot;
}
